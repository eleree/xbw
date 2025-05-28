# CMakeLists.txt for nofrendo-esp32 component

set(NOFRENDO_ESP32_SRCS
    joystick.c
    osd.c
    psxcontroller.c
    sdcard.c
    sound.cpp
    spi_lcd.c
    vga16.cpp
    video_audio.c
)

idf_component_register(SRCS "${NOFRENDO_ESP32_SRCS}"
                       INCLUDE_DIRS "."
                       REQUIRES nofrendo
)
