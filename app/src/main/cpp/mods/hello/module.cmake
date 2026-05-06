set(MOD_TARGET ${CURRENT_MOD_NAME})

set(MODULE_SOURCES
    ${MOD_DIR}/main.cpp
)

add_library(${MOD_TARGET} SHARED ${MODULE_SOURCES})

target_include_directories(${MOD_TARGET}
    PRIVATE
        ${MOD_DIR}
        ${SOURCE_DIR}
)

if(WIN32)
    target_compile_definitions(${MOD_TARGET} PRIVATE MODULE_EXPORT=__declspec(dllexport))
endif()