cmake_minimum_required(VERSION 3.13)

# Raspberry PI PICO2 Application

add_executable(app src/main.c src/spidrv.c src/lcddrv.c src/canvas.c src/font.c)
target_link_libraries(app pico_stdlib hardware_spi hardware_dma hardware_pwm)
target_include_directories(app PRIVATE inc)
pico_enable_stdio_usb(app 0)
pico_enable_stdio_uart(app 1)

pico_add_extra_outputs(app)
