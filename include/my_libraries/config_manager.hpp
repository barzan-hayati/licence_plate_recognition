#pragma once
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

class ConfigManager {
   public:
    static ConfigManager& get_instance();

    const nlohmann::json& get_config() const;

   private:
    ConfigManager();  // constructor loads the JSON file
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    nlohmann::json config;
};