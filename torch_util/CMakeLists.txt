project(torch_util_lib)

add_library(torch_util STATIC
    include/trc_util/ArgParse.h
    include/trc_util/Exception.h
    include/trc_util/Padding.h
    include/trc_util/Timer.h
    include/trc_util/Util.h
    include/trc_util/algorithm/IteratorRange.h
    include/trc_util/algorithm/VectorTransform.h
    include/trc_util/async/Async.h
    include/trc_util/async/ThreadPool.h
    include/trc_util/data/ExternalStorage.h
    include/trc_util/data/IndexMap.h
    include/trc_util/data/ObjectId.h
    include/trc_util/data/ObjectPool.h
    include/trc_util/data/TypesafeId.h
    include/trc_util/functional/Maybe.h
    src/ArgParse.cpp
    src/Timer.cpp
    src/async/ThreadPool.cpp
    src/data/ObjectId.cpp
)
target_include_directories(torch_util
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include/trc_util
)
target_compile_options(torch_util PUBLIC -Wall -Wextra -Wpedantic)
target_compile_features(torch_util PUBLIC cxx_std_20)
