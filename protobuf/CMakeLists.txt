protobuf_generate_cpp(
    TRC_PROTO_SRCS TRC_PROTO_HDRS
    protobuf/vector_types.proto
    protobuf/geometry.proto
    protobuf/texture.proto
    protobuf/material.proto
    protobuf/rig.proto
    protobuf/animation.proto
    protobuf/font.proto

    protobuf/asset.proto
    protobuf/asset_reference.proto
)

target_sources(torch PRIVATE ${TRC_PROTO_SRCS})
target_include_directories(torch PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
