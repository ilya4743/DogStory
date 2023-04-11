#pragma once
#include <string>

struct DbConnectrioSettings {
    size_t number_of_connection{1};
    std::string db_url{};
};