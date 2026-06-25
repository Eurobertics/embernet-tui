#pragma once

#include <iostream>
#include <string>

class TerminalModeGuard {
public:
    TerminalModeGuard(std::string enable, std::string disable)
        : disable_(std::move(disable))
    {
        std::cout << enable << std::flush;
    }

    ~TerminalModeGuard()
    {
        std::cout << disable_ << std::flush;
    }

    TerminalModeGuard(const TerminalModeGuard&) = delete;
    TerminalModeGuard& operator=(const TerminalModeGuard&) = delete;

private:
    std::string disable_;
};

