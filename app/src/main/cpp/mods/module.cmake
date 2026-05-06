set(MODS_ROOT ${CMAKE_CURRENT_LIST_DIR})
file(GLOB MOD_DIRS CONFIGURE_DEPENDS LIST_DIRECTORIES true "${MODS_ROOT}/*")

set(ALL_MOD_TARGETS "" CACHE INTERNAL "All mod targets")

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
    include("${MOD_DIR}/module.cmake")
    
    set_target_properties(${MOD_TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/mods/${MOD_NAME}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/mods/${MOD_NAME}"
    )
    
    list(APPEND ALL_MOD_TARGETS ${MOD_TARGET})
endforeach()

set(ALL_MOD_TARGETS ${ALL_MOD_TARGETS} CACHE INTERNAL "All mod targets" FORCE)
message(STATUS "Found mods: ${ALL_MOD_TARGETS}")