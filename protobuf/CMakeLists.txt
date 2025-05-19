protobuf_compile_cpp(
    TRC_PROTO_SRCS TRC_PROTO_HDRS
    SRC_DIR ${CMAKE_CURRENT_LIST_DIR}
    OUT_DIR ${TORCH_PROTOBUF_OUTPUT_DIR}
    vector_types.proto
    geometry.proto
    texture.proto
    material.proto
    rig.proto
    animation.proto
    font.proto

    material_shader_program.proto

    asset.proto
    asset_reference.proto
)

target_sources(torch PRIVATE ${TRC_PROTO_SRCS})
target_include_directories(torch PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
