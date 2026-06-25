#include "./app/App.hpp"
#include "config/AppConfig.hpp"
#include "session/SessionStore.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <utility>
#include <uuid/uuid.h>

std::string GenerateUuid() {
    uuid_t uuid;
    uuid_generate(uuid);

    char buffer[37];
    uuid_unparse_lower(uuid, buffer);

    return buffer;
}

AppConfig ParseArgs(int argc, char* argv[])
{
    bool session_name_set = false;
    bool session_disable_set = false;

    AppConfig config;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--prompt-file") {
            if (i + 1 >= argc) {
                std::cerr << "Error: --prompt-file requires a path\n";
                std::exit(1);
            }

            config.prompt_file = std::filesystem::path(argv[++i]);
        }
        else if (arg == "--list-sessions") {
            std::cout << std::endl;
            std::cout << "Liste der vergangenen Sessions:" << std::endl;
            std::cout << "===============================" << std::endl;
            std::cout << std::endl;
            SessionStore store;
            for (const auto& name : store.ListSessions()) {
                std::cout << name << std::endl;
            }
            std::cout << std::endl;
            std::exit(0);
        } else if (arg == "--session") {
            if (session_disable_set) {
                std::cerr << "Error: --session and --disable-session cannot be used together\n";
                std::exit(1);
            }

            if (i + 1 >= argc) {
                std::cerr << "Error: --session requires a name\n";
                std::exit(1);
            }

            config.session_name = argv[++i];
            session_name_set = true;
        } else if (arg == "--disable-session") {
            if (session_name_set) {
                std::cerr << "Error: --session and --disable-session cannot be used together\n";
                std::exit(1);
            }

            config.session_enabled = false;
            session_disable_set = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout
                << "Usage: embernet-tui [options]\n\n"
                << "Options:\n"
                << "  --prompt-file <path>  Use a custom system prompt file\n"
                << "  --list-sessions       List all past sessions.\n"
                << "  --disable-session     Disable session save.\n"
                << "  --session <name>      Load and save a named chat session\n"
                << "  -h, --help            Show this help\n";

            std::exit(0);
        }
        else {
            std::cerr << "Error: unknown argument: " << arg << "\n";
            std::exit(1);
        }
    }

    return config;
}

int main(int argc, char* argv[])
{
    AppConfig config = ParseArgs(argc, argv);
    
    if (config.session_enabled && config.session_name.empty()) {
        config.session_name = GenerateUuid();
    }

    App app(std::move(config));
    app.Run();

    return 0;
}

