# CMakeLists.txt for nofrendo-esp32 tests
idf_component_register(SRCS "test_joystick_gpio.c"
                           "test_sdcard_ops.c"
                       # Add other test_*.c files here if created
                       INCLUDE_DIRS .. # To access joystick.h and sdcard.h from parent component
                       REQUIRES unity esp_idf_lib_helpers
                               driver # For gpio.h, sdmmc_host.h, etc.
                               fatfs  # For VFS/FAT operations
                               # nofrendo-esp32 (component being tested - usually auto-linked)
)
