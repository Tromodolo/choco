cmake_minimum_required(VERSION 3.30)
project(choco C)

set(CMAKE_C_STANDARD 23)

find_package(raylib REQUIRED)

if (TESTS)
    add_definitions(-DTESTS=1)
endif (TESTS)

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
        tests/auto-tests.c
        tests/json/cJSON.c)

target_link_libraries(choco raylib)

file(COPY tests/tomharte DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")