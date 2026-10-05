set(MOD_TARGET test_console)

set(MOD_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
)

add_library(${MOD_TARGET} SHARED ${MOD_SOURCES})

