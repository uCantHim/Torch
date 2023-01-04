project(torch_util_lib)

add_library(torch_util STATIC
    src/ArgParse.cpp
    src/InterProcessLock.cpp
    src/MemoryStream.cpp
    src/StringManip.cpp
    src/Timer.cpp
    src/Util.cpp
    src/async/ThreadPool.cpp
    src/data/ObjectId.cpp
)
target_include_directories(torch_util PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
torch_default_compile_options(torch_util)

if (WIN32)
    include(FetchContent)
    FetchContent_Declare(
        semaphore
        GIT_REPOSITORY https://github.com/DanielTillett/simple-windows-posix-semaphore
        GIT_TAG        master
    )
    FetchContent_MakeAvailable(semaphore)

    target_sources(torch_util PRIVATE ${semaphore_SOURCE_DIR}/semaphore.c)
    target_include_directories(torch_util PUBLIC ${semaphore_SOURCE_DIR})
endif ()
