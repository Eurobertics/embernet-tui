#include "../src/tool/ToolManager.hpp"
#include "../src/config/AppConfig.hpp"
#include <cassert>
#include <fstream>
#include <filesystem>
#include <ostream>
#include <iostream>
#include <string>

void testDeleteDirectory() {
    {
        std::filesystem::create_directory("./testdir");
    }
    AppConfig config;
    config.permission_mode = PermissionMode::Dangerous;

    ToolManager tools(config);

    ToolCall call{
        "delete_directory",
        {{"path", "./testdir"}}
    };

    ToolResult result = tools.Execute(call);
    assert(result.success == true);
    assert(result.name == "delete_directory");
    assert(std::filesystem::is_directory("./testdir") == false);
}

void testDeleteFile() {
    {
        std::ofstream file("test_delete_file.txt");
        file << "Hello Tooling!";
    }

    AppConfig config;
    config.permission_mode = PermissionMode::Dangerous;

    ToolManager tools(config);

    ToolCall call{
        "delete_file",
        {{"path", "./test_delete_file.txt"}}
    };

    ToolResult result = tools.Execute(call);
    assert(result.success == true);
    assert(result.name == "delete_file");
    assert(std::filesystem::exists("./test_delete_file.txt") == false);
}

void testModifyText() {
    {
        std::ofstream file("test_modify_file.txt");
        file << "Hello Tooling!";
    }
    
    AppConfig config;
    config.permission_mode = PermissionMode::Dangerous;

    ToolManager tools(config);

    ToolCall call{
        "replace_text",
        {{"path", "test_modify_file.txt"}, {"olt_text", "Tooling"}, {"new_text", "World"}}
    };

    ToolResult result = tools.Execute(call);
    assert(result.success == true);
    assert(result.name == "replace_text");

    std::ifstream resfile("test_modify_file.txt");
    std::string content;
    std::getline(resfile, content);
    assert(content == "Hello World!");

    std::filesystem::remove("test_modify_file.txt");
}

void testCreateDirectory() {
    AppConfig config;
    config.permission_mode = PermissionMode::Dangerous;

    ToolManager tools(config);

    ToolCall call{
        "create_directory",
        {{"path", "./testdir"}}
    };

    ToolResult result = tools.Execute(call);
    assert(result.success == true);
    assert(result.name == "create_directory");

    std::filesystem::remove("./testdir");
}

void testWriteFile() {
    AppConfig config;
    config.permission_mode = PermissionMode::Dangerous;

    ToolManager tools(config);

    ToolCall call{
        "write_file",
        {{"path", "./test_read_file.txt"}, {"content", "Hello World!"}}
    };

    ToolResult result = tools.Execute(call);
    assert(result.success == true);
    assert(result.name == "write_file");

    std::filesystem::remove("test_read_file.txt");
}

void testReadFile() {
    {
        std::ofstream file("test_read_file.txt");
        file << "Hello Tooling!";
    }
    
    AppConfig config;
    config.permission_mode = PermissionMode::Dangerous;

    ToolManager tools(config);

    ToolCall call{
        "read_file",
        {{"path", "test_read_file.txt"}}
    };

    ToolResult result = tools.Execute(call);
    assert(result.success == true);
    assert(result.name == "read_file");
    assert(result.content == "Hello Tooling!");

    ToolCall missing{
        "read_file",
        {{"path", "does_not_exist.txt"}}
    };

    ToolResult missing_result = tools.Execute(missing);
    assert(missing_result.success == false);

    std::filesystem::remove("test_read_file.txt");
}

void TestReadDirectory() {
    AppConfig config;
    config.permission_mode = PermissionMode::Dangerous;
    ToolManager tools(config);

    ToolCall call {
        "read_directory",
        {{"path", "./"}}
    };

    ToolResult result = tools.Execute(call);
    assert(result.success == true);

    ToolCall wrong_directory_call{
        "read_directory",
        {{"path", "/does_not_exist"}}
    };

    ToolResult wrong_result = tools.Execute(wrong_directory_call);
    assert(wrong_result.success == false);
}

int main() {
    testReadFile();
    TestReadDirectory();
    testWriteFile();
    testCreateDirectory();
    testWriteFile();
    testDeleteDirectory();
    testDeleteFile();
    std::cout << "ToolManager tests passed.\n" << std::endl;
    return 0;
}

