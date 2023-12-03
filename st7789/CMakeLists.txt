add_executable(st7789
        st7789.c
        font.c
        main.c)

# Generate PIO headers
pico_generate_pio_header(st7789 ${CMAKE_CURRENT_LIST_DIR}/st7789.pio)

target_include_directories(st7789 PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        )

# pull in common dependencies
target_link_libraries(st7789
    pico_stdlib
    pico_cyw43_arch_none
    hardware_dma
    hardware_spi
    hardware_pio)

# create map/bin/hex file etc.
pico_add_extra_outputs(st7789)
