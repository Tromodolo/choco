cmake_minimum_required(VERSION 3.30)
project(choco C)

set(CMAKE_C_STANDARD 23)

find_package(raylib REQUIRED)

if (TESTS)
    add_definitions(-DTESTS=1)
endif (TESTS)

add_executable(choco
        main.c
        blip_buf/blip_buf.c
        core.c

        nes/nes.c
        nes/cartridge.c
        nes/cpu/cpu.c
        nes/ppu/ppu.c
        nes/apu/apu.c
        nes/apu/pulse.c
        nes/apu/triangle.c
        nes/apu/noise.c
        nes/apu/dmc.c
        nes/cpu/instructions.c
        nes/nes-logging.c
        nes/mappers/mapper.c
        nes/mappers/impl/nrom.c
        nes/mappers/impl/uxrom.c
        nes/mappers/impl/mmc1.c
        nes/mappers/impl/mmc3.c

        tests/tests.c
        tests/json/cJSON.c

        dmg/dmg.c
        dmg/dmg.h
        dmg/cpu.c
        dmg/cpu.h
        dmg/cartridge.c
        dmg/cartridge.h
)

target_link_libraries(choco raylib m)

file(COPY tests/nes/tomharte DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
file(COPY tests/dmg/SingleStepTests DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")