cmake_minimum_required(VERSION 3.30)
project(choco C)

set(CMAKE_C_STANDARD 23)

find_package(raylib REQUIRED)

add_executable(choco
        main.c
        nes.c
        core.c
        cartridge.c
        cartridge.h
        cpu.c
        cpu.h
        tests/tests.c
        tests/tests.h
        nestypes.h)

target_link_libraries(choco raylib)