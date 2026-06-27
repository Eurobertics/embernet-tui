cmake_minimum_required(VERSION 3.20)

project(embernet_tui
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_EXTENSIONS off)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

# Fallback wenn kein Build-Type angegeben
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type specified, defaulting to Debug")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

include(FetchContent)

set(CPR_ENABLE_SSL ON)
set(CPR_CURL_USE_LIBPSL OFF)
FetchContent_Declare(
    markdown-ui
    GIT_REPOSITORY https://github.com/zvasilev/MarkdownFTXUI.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
)
FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG v6.1.9
)
FetchContent_Declare(
  cpr
  GIT_REPOSITORY https://github.com/libcpr/cpr.git
  GIT_TAG 1.14.2
)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)
FetchContent_MakeAvailable(json)
FetchContent_MakeAvailable(cpr)
FetchContent_MakeAvailable(ftxui)
FetchContent_MakeAvailable(markdown-ui)

enable_testing()

add_executable(test_tool_manager
    tests/test_tool_manager.cpp
    src/tool/ToolManager.cpp
    src/config/AppConfig.cpp
)

target_link_libraries(test_tool_manager
    PRIVATE nlohmann_json::nlohmann_json
)

add_test(NAME ToolManagerTest COMMAND test_tool_manager)

add_executable(embernet_tui
    src/main.cpp
    src/app/App.cpp
    src/store/MessageStore.cpp
    src/ollama/OllamaClient.cpp
    src/app/InputBuffer.cpp
    src/markdown/Markdown.cpp
    src/tool/ToolManager.cpp
    src/tool/ToolFormat.cpp
    src/config/PromptProvider.cpp
    src/app/SlashCommand.cpp
    src/session/SessionStore.cpp
    src/config/AppConfig.cpp
)

target_link_libraries(embernet_tui
    PRIVATE
        ftxui::screen
        ftxui::dom
        ftxui::component
        cpr::cpr
        nlohmann_json::nlohmann_json
        markdown-ui
        uuid
)

