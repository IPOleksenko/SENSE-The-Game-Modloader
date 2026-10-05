set(MOD_TARGET steam_achievements)

set(MOD_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
)

add_library(${MOD_TARGET} SHARED ${MOD_SOURCES})

