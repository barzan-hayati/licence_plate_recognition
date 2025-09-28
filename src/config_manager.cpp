#include "../include/my_libraries/config_manager.hpp"

ConfigManager::ConfigManager() {
    std::ifstream file("../data/configuration.json");
    if (!file) {
        throw std::runtime_error("Could not open configuration.json");
    }
    file >> config;
}

ConfigManager& ConfigManager::get_instance() {
    static ConfigManager instance;
    return instance;
}

const nlohmann::json& ConfigManager::get_config() const { return config; }