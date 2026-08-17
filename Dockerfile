# Не просто создаём образ, но даём ему имя build
FROM gcc:13.3 as build

RUN apt update && \
    apt install -y \
    python3-pip \
    cmake \
    && \
    rm -rf /var/lib/apt/lists/*

RUN pip3 install conan --upgrade --break-system-packages
RUN conan profile detect

# Запуск conan как раньше
COPY conanfile.txt /app/
RUN mkdir -p /app/build && cd /app/build && \
    conan install .. --build=missing \
    -s build_type=Release

# Папка data больше не нужна
COPY ./src /app/src
COPY ./tests /app/tests
COPY CMakeLists.txt /app/

RUN cd /app/build && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake && \
    cmake --build . -j $(nproc)

# Второй контейнер в том же докерфайле
FROM ubuntu:24.04 as run

# Создадим пользователя www
RUN groupadd -r www && useradd -r -g www www
USER www

# Скопируем приложение со сборочного контейнера в директорию /app.
# Не забываем также папку data, она пригодится.
COPY --from=build /app/build/game_server /app/
COPY --from=build /app/build/libGameModelLib.a /app/;
COPY ./data /app/data
COPY ./static /app/static

# Запускаем игровой сервер
ENTRYPOINT ["/app/game_server", "--config-file", "/app/data/config.json", "--www-root", "/app/static", "--tick-period", "50", "--randomize-spawn-points"]