include(FetchContent)

find_package(Vulkan)
find_package(glfw3 QUIET)
find_package(glm QUIET)
find_package(Freetype)
find_package(Protobuf)
find_package(PNG)
find_package(nlohmann_json)

if (NOT Vulkan_FOUND)
    message("Vulkan library not found, searching manually for the Vulkan SDK...")
    find_library(Vulkan_LIBRARY NAMES vulkan vulkan-1 PATHS $ENV{VULKAN_SDK}/Lib/ REQUIRED)
    if (Vulkan_LIBRARY)
        set(Vulkan_FOUND ON)
    endif ()
endif ()
if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan not found!")
endif ()

if (NOT glfw3_FOUND)
    message("No external GLFW found; download and build it with torch.")
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        master
    )
    FetchContent_MakeAvailable(glfw)
endif ()

if (NOT glm_FOUND)
    message("No external GLM found; download and build it with torch.")
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        master
    )
    FetchContent_MakeAvailable(glm)
endif ()

if (NOT Freetype_FOUND)
    message("No external Freetype found; download and build it with torch.")
    FetchContent_Declare(
        freetype
        GIT_REPOSITORY https://github.com/freetype/freetype
        GIT_TAG        master
    )
    FetchContent_MakeAvailable(freetype)
    torch_default_compile_options(freetype)
    add_library(Freetype::Freetype ALIAS freetype)
endif ()

if (NOT Protobuf_FOUND)
    message("No external Protocol Buffers found; download and build it with torch.")
    FetchContent_Declare(
        protobuf
        GIT_REPOSITORY https://github.com/protocolbuffers/protobuf
        GIT_TAG        main
    )
    set(protobuf_BUILD_TESTS OFF)
    set(protobuf_BUILD_CONFORMANCE OFF)
    set(protobuf_BUILD_EXAMPLES OFF)
    option(protobuf_MSVC_STATIC_RUNTIME "" OFF)
    option(protobuf_BUILD_SHARED_LIBS "" OFF)
    FetchContent_MakeAvailable(protobuf)
endif ()

if (NOT PNG_FOUND)
    message("No external PNG found; download and build PNG and ZLIB with torch.")

    # PNG requires ZLIB
    find_package(ZLIB)
    if (NOT ZLIB_FOUND)
        FetchContent_Declare(
            ZLIB
            GIT_REPOSITORY https://github.com/madler/zlib.git
            GIT_TAG        master
        )
        FetchContent_MakeAvailable(ZLIB)
        add_library(ZLIB::ZLIB ALIAS zlib)

        # I need both `set` and `option` calls to get it to find the custom-built ZLIB. Don't ask me why.
        set(PNG_BUILD_ZLIB TRUE)
        option(PNG_BUILD_ZLIB "" ON)

        set(ZLIB_LIBRARIES "ZLIB::ZLIB")
        FetchContent_GetProperties(ZLIB SOURCE_DIR ZLIB_INCLUDE_DIRS)
        file(RENAME ${ZLIB_INCLUDE_DIRS}/zconf.h.included ${ZLIB_INCLUDE_DIRS}/zconf.h)

        set(ZLIB_FOUND TRUE)
    endif ()

    FetchContent_Declare(
        libpng
        GIT_REPOSITORY https://github.com/glennrp/libpng.git
        GIT_TAG        libpng16
    )
    set(SKIP_INSTALL_ALL TRUE)
    FetchContent_MakeAvailable(libpng)

    FetchContent_GetProperties(PNG SOURCE_DIR PNG_SOURCE_DIR)
    # Include binary dir for the pnglibconf.h:
    target_include_directories(png PUBLIC ${libpng_SOURCE_DIR} ${libpng_BINARY_DIR})

    add_library(PNG::PNG ALIAS png)
endif ()

if (NOT nlohmann_json_FOUND)
    message("No external nlohmann-json found; download and build it with torch.")
    FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json
        GIT_TAG        master
    )

    set(JSON_BuildTests OFF CACHE INTERNAL "")
    FetchContent_MakeAvailable(json)
endif ()

FetchContent_Declare(
    cimg
    GIT_REPOSITORY https://github.com/dtschump/CImg.git
    GIT_TAG        927fee511fe4fcc1ae5cdf2365a60048fe4ff935
)
FetchContent_MakeAvailable(cimg)
