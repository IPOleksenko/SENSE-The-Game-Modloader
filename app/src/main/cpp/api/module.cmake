set(API_TARGET ${PROJECT_NAME}_api)

set(API_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/core.cpp
    ${CMAKE_CURRENT_LIST_DIR}/lowlevel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/plugin.cpp
    ${CMAKE_CURRENT_LIST_DIR}/net.cpp
    ${CMAKE_CURRENT_LIST_DIR}/camera.cpp
    ${CMAKE_CURRENT_LIST_DIR}/player.cpp
    ${CMAKE_CURRENT_LIST_DIR}/game.cpp
    ${CMAKE_CURRENT_LIST_DIR}/render.cpp
    ${CMAKE_CURRENT_LIST_DIR}/input.cpp
    ${CMAKE_CURRENT_LIST_DIR}/audio.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui.cpp
    ${CMAKE_CURRENT_LIST_DIR}/config.cpp
    ${CMAKE_CURRENT_LIST_DIR}/assets.cpp
    ${CMAKE_CURRENT_LIST_DIR}/savestate.cpp
    ${CMAKE_CURRENT_LIST_DIR}/replay.cpp
    ${CMAKE_CURRENT_LIST_DIR}/physics.cpp
    ${CMAKE_CURRENT_LIST_DIR}/particles.cpp
    ${CMAKE_CURRENT_LIST_DIR}/environment.cpp
    ${CMAKE_CURRENT_LIST_DIR}/dialog.cpp
    ${CMAKE_CURRENT_LIST_DIR}/speedrun.cpp
    ${CMAKE_CURRENT_LIST_DIR}/profiler.cpp
    ${CMAKE_CURRENT_LIST_DIR}/achievements.cpp
    ${CMAKE_CURRENT_LIST_DIR}/console.cpp
    ${CMAKE_CURRENT_LIST_DIR}/track.cpp
    ${CMAKE_CURRENT_LIST_DIR}/photomode.cpp
    ${CMAKE_CURRENT_LIST_DIR}/objects.cpp
    ${CMAKE_CURRENT_LIST_DIR}/gameobjects.cpp
    ${CMAKE_CURRENT_LIST_DIR}/steam.cpp
)

add_library(${API_TARGET} SHARED ${API_SOURCES})

# Auto-export all public symbols so all 18 mod DLLs share ONE set of
# singletons, ONE EventBus, and ONE set of installed IAT hooks.
set_target_properties(${API_TARGET} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

target_include_directories(${API_TARGET}
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${SOURCE_DIR}
)

target_link_libraries(${API_TARGET}
    PUBLIC
        winhttp
        user32
        gdi32
        shell32
        psapi
        winmm
)

