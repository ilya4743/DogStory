#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <iostream>
#include <thread>

#include "application.h"
#include "database_invariants.h"
#include "db_connection_settings.h"
#include "json_deserializer.h"
#include "logger.h"
#include "logging_request_handler.h"
#include "request_handler.h"
#include "sdk.h"
#include "ticker.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;

namespace {

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned num_threads, const Fn& fn) {
    num_threads = std::max(1u, num_threads);
    std::vector<std::jthread> workers;
    workers.reserve(num_threads - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--num_threads) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace

struct Args {
    fs::path config_json_path;
    fs::path static_files_root;
    std::optional<std::chrono::milliseconds> tick_period;
    bool randomize_spawn_points = false;
    std::optional<fs::path> state_file_path;
    std::optional<std::chrono::milliseconds> state_period = std::nullopt;
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* argv[]) {
    namespace po = boost::program_options;

    po::options_description desc{"Allowed options"};
    unsigned tick_period = 0;
    std::string config_json_path, static_files_root, state_file_path;
    unsigned state_period = 0;
    desc.add_options()("help,h", "produce help message")("tick-period,t", po::value<unsigned>(&tick_period)->value_name("milliseconds"s), "set tick period")("config-file,c", po::value<std::string>(&config_json_path)->value_name("file"s), "set config file path")("www-root,w", po::value<std::string>(&static_files_root)->value_name("dir"s), "set static files root")("randomize-spawn-points", "spawn dogs at random positions")("state-file", po::value<std::string>(&state_file_path)->value_name("file"s))("save-state-period", po::value<unsigned>(&state_period)->value_name("milliseconds"s));

    po::positional_options_description p;
    p.add("config-file", 1).add("www-root", 1);

    po::variables_map vm;
    po::store(po::command_line_parser{argc, argv}.options(desc).positional(p).run(), vm);
    po::notify(vm);

    Args args;
    if (vm.contains("help"s)) {
        std::cout << desc << std::endl;
        return std::nullopt;
    }
    if (vm.contains("tick-period"s)) {
        if (tick_period == 0) {
            throw std::runtime_error{"Invalid tick-period"s};
        }
        args.tick_period = std::chrono::milliseconds{tick_period};
    }
    if (vm.contains("www-root"s)) {
        args.static_files_root = static_files_root;
    } else {
        throw std::runtime_error{"--www-root option is not specified"s};
    }
    if (vm.contains("config-file"s)) {
        args.config_json_path = config_json_path;
    } else {
        throw std::runtime_error{"--config-file is not specified"s};
    }
    if (vm.contains("randomize-spawn-points"s)) {
        args.randomize_spawn_points = true;
    }
    if (vm.contains("state-file"s)) {
        args.state_file_path = state_file_path;
    }
    if (vm.contains("save-state-period"s)) {
        if (state_period == 0) {
            throw std::runtime_error{"Invalid tick-period"s};
        }
        args.state_period = std::chrono::milliseconds{state_period};
    }
    return args;
}

int main(int argc, const char* argv[]) {
    InitLogger();
    try {
        auto args = ParseCommandLine(argc, argv);
        // 1. Загружаем карту из файла и построить модель игры
        model::Game game = json_deserializer::LoadGame(args->config_json_path);

        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);
        const char* db_url = std::getenv(db_invariants::DB_URL.c_str());
        if (!db_url) {
            throw std::runtime_error("Empty database URL");
        }
        DbConnectrioSettings db_settings{num_threads, std::move(db_url)};
        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        // Подписываемся на сигналы и при их получении завершаем работу сервера
        auto app = std::make_shared<Application>(game, args->randomize_spawn_points, ioc, args->tick_period, args->state_file_path, args->state_period, std::move(db_settings));
        app->RestoreGame();
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc, app](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
                if (app->GetStateFilePath().has_value())
                    if (app->GetStateFilePath().value().has_filename())
                        app->SaveGame();
            }
        });

        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        std::filesystem::path root = args->static_files_root;
        root = std::filesystem::canonical(root);
        auto strand = net::make_strand(ioc);
        auto handler = std::make_shared<http_handler::RequestHandler>(app, root, strand);

        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0"sv);
        constexpr net::ip::port_type port = 8080;
        LoggingRequestHandler logging_request_handler{[handler](auto&& endpoint, auto&& req, auto&& send) {
            (*handler)(std::forward<decltype(endpoint)>(endpoint), std::forward<decltype(req)>(req),
                       std::forward<decltype(send)>(send));
        }};
        http_server::ServeHttp(ioc, {address, port}, logging_request_handler);

        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        json::value start_server_json{{"port", port}, {"address", address.to_string()}};
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, start_server_json)
                                << "server started";

        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        json::value start_server_json{{"code", EXIT_FAILURE}, {"exception", ex.what()}};
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, start_server_json)
                                << "server exited";

        return EXIT_FAILURE;
    }
    json::value start_server_json{{"code", 0}};
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, start_server_json)
                            << "server exited";
}
