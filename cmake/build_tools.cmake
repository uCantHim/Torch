# Provide out own version of this convenience function because it's deprecated
# in the newest version of Protobuf.
#
# Call it like this:
#
#     protobuf_compile_cpp(
#         MY_PROTO_SOURCES
#         MY_PROTO_HEADERS
#         SRC_DIR <include-directory>
#         OUT_DIR <output-directory>
#         <proto-files>...
#     )
#
# Generated source files are stored in MY_PROTO_SOURCES, header files in
# MY_PROTO_HEADERS.
function(protobuf_compile_cpp SRCS HDRS)
    cmake_parse_arguments(
        PARSE_ARGV 2
        arg           # prefix
        ""            # options
        "TARGET;SRC_DIR;OUT_DIR;OUT_DIR_HEADERS;OUT_DIR_SOURCES"    # one-value keywords
        ""            # multi-value keywords
    )

    # Process args
    set(proto_files "${arg_UNPARSED_ARGUMENTS}")
    if (NOT proto_files)
        message(SEND_ERROR "Error: protobuf_compile_cpp() expects at least one proto file.")
        return()
    endif()

    if (NOT arg_SRC_DIR)
        set(arg_SRC_DIR ${CMAKE_CURRENT_LIST_DIR})
    endif()
    if (NOT arg_OUT_DIR)
        set(arg_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
    endif()

    # Compile input files to C++
    set(out_srcs)
    set(out_hdrs)
    foreach (file ${proto_files})
        string(REGEX REPLACE "[.]proto$" ".pb.cc" OUT_SRC ${arg_OUT_DIR}/${file})
        string(REGEX REPLACE "[.]proto$" ".pb.h" OUT_HDR ${arg_OUT_DIR}/${file})
        list(APPEND out_hdrs ${OUT_HDR})
        list(APPEND out_srcs ${OUT_SRC})

        # Call protoc
        add_custom_command(
            OUTPUT ${OUT_HDR} ${OUT_SRC}
            COMMAND protobuf::protoc
                --cpp_out ${arg_OUT_DIR}
                -I${arg_SRC_DIR}
                ${file}
            DEPENDS ${arg_SRC_DIR}/${file}
            COMMENT "Generating C++ from ${file}: ${OUT_HDR}"
            VERBATIM
        )
    endforeach()

    set(${HDRS} ${out_hdrs} PARENT_SCOPE)
    set(${SRCS} ${out_srcs} PARENT_SCOPE)
endfunction()

function (torch_compile_pipelines_cpp)
    cmake_parse_arguments(
        ARG
        ""
        "TARGET;FILE"
        ""
        ${ARGN}
    )
    if (NOT ARG_FILE)
        message(FATAL_ERROR "Function _torch_compile_pipelines_cpp expects argument 'FILE'.")
    endif ()
    if (NOT ARG_TARGET)
        message(FATAL_ERROR "Function _torch_compile_pipelines_cpp expects argument 'TARGET'.")
    endif ()

    if (${TORCH_FLIP_Y_AXIS})
        set(SHADER_MACROS --shader-macro TRC_FLIP_Y_AXIS)
    endif ()

    set(_OUT_DIR "${TORCH_GENERATED_DIR}/include/trc")
    set(_DEPFILE ${TORCH_GENERATED_DIR}/${ARG_FILE}_depfile)
    add_custom_command(
        OUTPUT
            ${_OUT_DIR}/${ARG_FILE}.cpp
        COMMAND pipeline_compiler ${TORCH_CONFIG_DIR}/${ARG_FILE}.se
            -o ${_OUT_DIR} --spv --spv-version 1.5 --spv-target-env vulkan1.2
            --shader-input ${TORCH_SHADER_DIR} --shader-output ${TORCH_SHADER_OUTPUT_DIR}
            --shader-db ${TORCH_INTERNAL_SHADER_DB}
            --shader-db-append
            ${SHADER_MACROS}
        DEPENDS
            pipeline_compiler
            ${TORCH_CONFIG_DIR}/${ARG_FILE}.se
        #DEPFILE ${_DEPFILE}
        VERBATIM
    )

    target_sources(${ARG_TARGET} PRIVATE ${_OUT_DIR}/${ARG_FILE}.cpp)
endfunction()
