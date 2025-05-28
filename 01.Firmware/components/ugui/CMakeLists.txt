# CMakeLists.txt for ugui component

set(UGUI_SRCS
    ugui.c
)

idf_component_register(SRCS "${UGUI_SRCS}"
                       INCLUDE_DIRS "."
)
