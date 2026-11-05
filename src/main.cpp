// main.cpp — CLI entry point. Kept thin on purpose: parse arguments, call the
// library, print results. All logic lives in rtos_core so tests can reach it.

#include <iostream>

#include "rtos/Version.hpp"

int main() {
    std::cout << "RTOS Scheduler Simulator v" << rtos::version() << '\n'
              << "Phase 1 skeleton: build system OK.\n";
    return 0;
}
