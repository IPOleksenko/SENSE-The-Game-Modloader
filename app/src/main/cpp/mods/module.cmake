set(MODS_ROOT ${CMAKE_CURRENT_LIST_DIR})
file(GLOB MOD_DIRS CONFIGURE_DEPENDS LIST_DIRECTORIES true "${MODS_ROOT}/*")

set(LOCAL_MOD_TARGETS "")

foreach(MOD_DIR ${MOD_DIRS})
    if(NOT IS_DIRECTORY ${MOD_DIR})
        continue()
    endif()

    if(NOT EXISTS "${MOD_DIR}/module.cmake")
        continue()
    endif()

    get_filename_component(MOD_NAME ${MOD_DIR} NAME)
    message(STATUS "Loading mod: ${MOD_NAME}")
    
    set(CURRENT_MOD_NAME ${MOD_NAME})
    set(MOD_TARGET "")
    include("${MOD_DIR}/module.cmake")
    if(NOT MOD_TARGET)
        continue()
    endif()
    
    target_link_libraries(${MOD_TARGET} PRIVATE ${PROJECT_NAME}_api)

    set_target_properties(${MOD_TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/mods/${MOD_NAME}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/mods/${MOD_NAME}"
    )
    
    list(APPEND LOCAL_MOD_TARGETS ${MOD_TARGET})
endforeach()

set(ALL_MOD_TARGETS ${LOCAL_MOD_TARGETS} CACHE INTERNAL "All mod targets" FORCE)
message(STATUS "Found mods: ${ALL_MOD_TARGETS}")
message(STATUS "Found mods: ${ALL_MOD_TARGETS}")