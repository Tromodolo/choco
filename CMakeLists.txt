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
        instructions.c
        tests/tests.c
        tests/cpu-addressing.c)

target_link_libraries(choco raylib)