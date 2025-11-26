cmake_minimum_required(VERSION 3.30)
project(choco C)

set(CMAKE_C_STANDARD 23)

find_package(raylib REQUIRED)

if (TESTS)
    add_definitions(-DTESTS=1)
endif (TESTS)

add_executable(choco
        main.c
        nes/nes.c
        nes/core.c
        nes/cartridge.c
        nes/cpu/cpu.c
        nes/ppu/ppu.c
        nes/apu/apu.c
        nes/cpu/instructions.c
        nes/nes-logging.c
        tests/tests.c
        tests/auto-tests.c
        tests/json/cJSON.c
        nes/apu/pulse.c
        nes/mappers/mapper.c
        nes/mappers/impl/nrom.c
        nes/mappers/impl/uxrom.c)

target_link_libraries(choco raylib)

file(COPY tests/tomharte DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")