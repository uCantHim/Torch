project(torch_util_lib)

add_library(torch_util STATIC
    src/ArgParse.cpp
    src/StringManip.cpp
    src/Timer.cpp
    src/async/ThreadPool.cpp
    src/data/ObjectId.cpp
)
target_include_directories(torch_util PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
torch_default_compile_options(torch_util)
