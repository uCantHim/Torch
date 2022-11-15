include(FetchContent)

if (NOT TARGET shaderc)
    FetchContent_Declare(
        shaderc
        GIT_REPOSITORY https://github.com/google/shaderc
        GIT_TAG        main
        PATCH_COMMAND  ${FETCHCONTENT_BASE_DIR}/shaderc-src/utils/git-sync-deps
    )

    set(SHADERC_SKIP_TESTS ON)
    set(SHADERC_SKIP_EXAMPLES ON)
    FetchContent_MakeAvailable(shaderc)
endif()

add_library(spirv_compiler)
torch_default_compile_options(spirv_compiler)

target_sources(spirv_compiler PRIVATE
    src/CompileSpirv.cpp
    src/FileIncluder.cpp
)

target_link_libraries(spirv_compiler PUBLIC shaderc)
target_include_directories(spirv_compiler PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include/")
target_compile_features(spirv_compiler PUBLIC cxx_std_20)
