cmake_minimum_required(VERSION 3.21)
project(SENSE_THE_GAME_MODLOADER LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/cpp)

set(PROJECT_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(ASSETS_DIR ${PROJECT_ROOT}/assets)
set(SOURCE_DIR ${PROJECT_ROOT}/cpp)
set(MODS_DIR ${SOURCE_DIR}/mods)

include(${SOURCE_DIR}/api/module.cmake)
include(${MODS_DIR}/module.cmake)

include(${SOURCE_DIR}/application/module.cmake)
include(${SOURCE_DIR}/log/module.cmake)
include(${SOURCE_DIR}/inject/module.cmake)

set(SRC_FILES
    ${SOURCE_DIR}/main.cpp
)

add_executable(${PROJECT_NAME}
    ${SRC_FILES}
)

install(
    TARGETS
        ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_NAME}_application
)

if(MSVC)
    include(msvc_config.cmake)
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMENT "Copying mod DLLs to executable directory"
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${PROJECT_NAME}>/mods/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:${PROJECT_NAME}_api>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/$<TARGET_FILE_NAME:${PROJECT_NAME}_api>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:${PROJECT_NAME}_api>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/mods/$<TARGET_FILE_NAME:${PROJECT_NAME}_api>
)

foreach(MOD_TARGET ${ALL_MOD_TARGETS})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${MOD_TARGET}>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/mods/$<TARGET_FILE_NAME:${MOD_TARGET}>
    )
endforeach()

if(EXISTS "${ASSETS_DIR}/steam_api64.dll")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ASSETS_DIR}/steam_api64.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/steam_api64.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ASSETS_DIR}/steam_api64.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/mods/steam_api64.dll"
    )
endif()

# Also sync to app/src/main/build/Debug if that folder exists (where SENSE_THE_GAME.exe resides)
if(EXISTS "${PROJECT_ROOT}/build/Debug")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_ROOT}/build/Debug/mods/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJECT_NAME}> "${PROJECT_ROOT}/build/Debug/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJECT_NAME}_api> "${PROJECT_ROOT}/build/Debug/$<TARGET_FILE_NAME:${PROJECT_NAME}_api>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJECT_NAME}_api> "${PROJECT_ROOT}/build/Debug/mods/$<TARGET_FILE_NAME:${PROJECT_NAME}_api>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ASSETS_DIR}/steam_api64.dll" "${PROJECT_ROOT}/build/Debug/steam_api64.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ASSETS_DIR}/steam_api64.dll" "${PROJECT_ROOT}/build/Debug/mods/steam_api64.dll"
    )
    foreach(MOD_TARGET ${ALL_MOD_TARGETS})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:${MOD_TARGET}>
                "${PROJECT_ROOT}/build/Debug/mods/$<TARGET_FILE_NAME:${MOD_TARGET}>"
        )
    endforeach()
endif()