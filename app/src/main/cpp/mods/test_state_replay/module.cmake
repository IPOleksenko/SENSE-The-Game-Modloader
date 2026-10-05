set(MOD_TARGET test_state_replay)

set(MOD_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
)

add_library(${MOD_TARGET} SHARED ${MOD_SOURCES})

