set(MOD_TARGET developer_menu)

set(MOD_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui.cpp
)

add_library(${MOD_TARGET} SHARED ${MOD_SOURCES})
