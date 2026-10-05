set(MOD_TARGET test_track_modifiers)

set(MOD_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
)

add_library(${MOD_TARGET} SHARED ${MOD_SOURCES})

