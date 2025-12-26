include(FetchContent)

cmake_policy(SET CMP0077 NEW)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    MinHook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG master
    SOURCE_DIR ${PROJECT_ROOT}/external/MinHook
)

FetchContent_MakeAvailable(MinHook)

