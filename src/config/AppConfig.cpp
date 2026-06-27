#include "AppConfig.hpp"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <filesystem>
#include "nlohmann/json.hpp"

namespace fs = std::filesystem;

AppConfig::AppConfig() {
    std::string home = std::getenv("HOME");
    fs::path config_path = fs::path(home)
        / ".config"
        / "embernet-tui"
        / "config.json";

    if (!ConfigExists(config_path)) {
        CreateDefaultConfig(config_path);
    }

    config_file_ = config_path;
    LoadConfig(); 
}

void AppConfig::SetConfigFile(const std::string& path) {
    try {
        fs::path full_path(config_file_);
        
        if (ConfigExists(full_path)) {
            config_file_ = path;
            LoadConfig(); 
        } else {
            std::cerr << "Warnung: Der neue Konfigurationspfad existiert nicht und wurde nicht gesetzt." << std::endl;
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Fehler beim Setzen des Konfigurationspfads: " << e.what() << std::endl;
    }
}

std::string AppConfig::GetConfigFile() const {
    return config_file_;
}

bool AppConfig::ConfigExists(const fs::path& path) const {
    fs::path parent_dir = path.parent_path();
    if (!fs::exists(parent_dir)) {
        return false;
    }
    return fs::exists(path) && fs::is_regular_file(path);
}

void AppConfig::CreateDefaultConfig(const fs::path& path) {
    try {
        fs::create_directories(path.parent_path());

        std::string home = std::getenv("HOME");
        fs::path default_prompt_path = fs::path(home)
            / ".config"
            / "embernet-tui"
            / "system_prompt.md";
        std::string default_content = "{\n"
            "  \"permission_mode\": 2,\n"
            "  \"session_enabled\": true,\n"
            "  \"prompt_file\": \"" + default_prompt_path.string() + "\",\n"
            "  \"llm_backend\": \"ollama\",\n"
            "  \"api_base_url\": \"http://localhost:11434\",\n"
            "  \"default_model\": \"gemma4:e4b\"\n"
            "}";

        std::ofstream outfile(path);
        if (outfile.is_open()) {
            outfile << default_content;
            outfile.close();
            std::cout << "Info: Standard-Konfigurationsdatei erstellt unter: " << path << std::endl;
        } else {
            std::cerr << "Fehler: Konnte die Standard-Konfigurationsdatei nicht schreiben." << std::endl;
        }

    } catch (const fs::filesystem_error& e) {
        std::cerr << "Fehler beim Erstellen der Konfigurationsstruktur: " << e.what() << std::endl;
    }
}

void AppConfig::LoadConfig() {
    if (config_file_.empty()) {
        std::cerr << "Fehler beim Laden der Konfiguration: Der Konfigurationspfad ist nicht gesetzt." << std::endl;
        return;
    }

    fs::path config_path(config_file_);
    std::ifstream infile(config_path);
    if (!infile.is_open()) {
        std::cerr << "Warnung: Konfigurationsdatei konnte nicht geöffnet werden. Standardwerte bleiben erhalten." << std::endl;
        return;
    }

    try {
        nlohmann::json j;
        infile >> j;
        infile.close();

        //std::cout << "Info: Konfigurationsdatei erfolgreich gelesen und geparst." << std::endl;

        // 1. permission_mode (Integer)
        int perm_mode = 2; // Defaultwert
        if (j.contains("permission_mode") && j["permission_mode"].is_number()) {
            perm_mode = j["permission_mode"].get<int>();
        }

        if (perm_mode == 1) {
            this->permission_mode = PermissionMode::ReadOnly;
        } else if (perm_mode == 2) {
            this->permission_mode = PermissionMode::WorkspaceWrite;
        } else if (perm_mode == 3) {
            this->permission_mode = PermissionMode::Dangerous;
        } else {
            this->permission_mode = PermissionMode::WorkspaceWrite;
        }


        // 2. prompt_file (String)
        std::string home = std::getenv("HOME");
        fs::path default_prompt_path = fs::path(home)
            / ".config"
            / "embernet-tui"
            / "system_prompt.md";
        std::string prompt_file_str = default_prompt_path.string();
        if (j.contains("prompt_file") && j["prompt_file"].is_string()) {
            prompt_file_str = j["prompt_file"].get<std::string>();
        }
        if (!prompt_file_str.empty()) {
            this->prompt_file = fs::path(prompt_file_str);
        }


        // 3. session_enabled (Boolean)
        bool session_enabled_val = true; // Defaultwert
        if (j.contains("session_enabled") && j["session_enabled"].is_boolean()) {
            session_enabled_val = j["session_enabled"].get<bool>();
        }
        this->session_enabled = session_enabled_val; 

        if (j.contains("api_base_url") && j["api_base_url"].is_string()) {
            this->base_api_url = j["api_base_url"].get<std::string>();
        }

        if (j.contains("llm_backend") && j["llm_backend"].is_string()) {
            this->llm_backend = j["llm_backend"].get<std::string>();
        }

        if (j.contains("default_model") && j["default_model"].is_string()) {
            this->default_model = j["default_model"].get<std::string>();
        }

        //std::cout << "Info: Konfiguration erfolgreich geladen und Member-Variablen aktualisiert." << std::endl;

    } catch (const nlohmann::json::exception& e) {
        std::cerr << "Fehler beim Parsen der JSON-Konfigurationsdatei: " << e.what() << ". Standardwerte werden beibehalten." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Ein allgemeiner Fehler beim Laden der Konfiguration ist aufgetreten: " << e.what() << std::endl;
    }
}

