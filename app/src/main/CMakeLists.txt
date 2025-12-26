cmake_minimum_required(VERSION 3.21)
project(SENSE_THE_GAME_MODLOADER LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/cpp)

set(PROJECT_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(SOURCE_DIR ${PROJECT_ROOT}/cpp)

include(sdl2_import.cmake)
include(minhook_import.cmake)

add_library(${PROJECT_NAME} SHARED
    ${SOURCE_DIR}/main.cpp
    ${SOURCE_DIR}/proxy.def
)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "SDL2")

install(
    TARGETS
        ${PROJECT_NAME}
        SDL2
        SDL2_image
        SDL2_mixer
        SDL2_ttf        

    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}
)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${PROJECT_ROOT}/external/SDL2/include
    ${PROJECT_ROOT}/external/SDL2_image
    ${PROJECT_ROOT}/external/SDL2_mixer
    ${PROJECT_ROOT}/external/SDL2_ttf
    ${PROJECT_ROOT}/external/MinHook/include
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        SDL2
        SDL2_image
        SDL2_mixer
        SDL2_ttf
        minhook
)

if(MSVC)
    include(msvc_config.cmake)
endif()