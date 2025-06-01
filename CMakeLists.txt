cmake_minimum_required(VERSION 3.30)
project(choco C)

set(CMAKE_C_STANDARD 23)

find_package(raylib REQUIRED)

add_executable(choco
        main.c
        nes.c
        core.c
        cartridge.c
        cpu.c
        ppu.c
        instructions.c
        nes-logging.c
        tests/tests.c
        tests/cpu-addressing.c
        palette.h)

target_link_libraries(choco raylib)