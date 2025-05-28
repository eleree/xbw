# CMakeLists.txt for nofrendo component

# List of source directories
set(NOFRENDO_SRC_DIRS
    .
    cpu
    libsnss
    mappers
    nes
    sndhrdw
)

# List of include directories
set(NOFRENDO_INCLUDE_DIRS
    .
    cpu
    libsnss
    nes
    sndhrdw
)

# Register the component
idf_component_register(SRC_DIRS "${NOFRENDO_SRC_DIRS}"
                       INCLUDE_DIRS "${NOFRENDO_INCLUDE_DIRS}"
                       # REQUIRES esp_idf_lib_helpers # Example, if it had dependencies
)

# Add compile options and definitions
# CFLAGS += -Wno-error=char-subscripts -Wno-error=attributes -DNOFRENDO_DEBUG
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=char-subscripts -Wno-error=attributes)
target_compile_definitions(${COMPONENT_LIB} PRIVATE NOFRENDO_DEBUG)
