cmake_minimum_required(VERSION 3.14)

add_library(libmaterial_editor SHARED)

torch_default_compile_options(libmaterial_editor)
target_link_libraries(libmaterial_editor PUBLIC torch torch-imgui component)

target_sources(
    libmaterial_editor
    PRIVATE
        src/CameraDescriptor.cpp
        src/Controls.cpp
        src/GraphLayout.cpp
        src/GraphManipulator.cpp
        src/GraphRenderer.cpp
        src/GraphScene.cpp
        src/GraphSerializer.cpp
        src/MaterialEditorGui.cpp
        src/MaterialEditorRenderConfig.cpp
        src/MaterialGraph.cpp
        src/MaterialNode.cpp
)
target_include_directories(libmaterial_editor INTERFACE ${CMAKE_CURRENT_LIST_DIR}/src/)

# Build protobuf sources
protobuf_generate_cpp(
    MEDIT_PROTO_SRCS MEDIT_PROTO_HDRS
    "${CMAKE_CURRENT_LIST_DIR}/material_graph.proto"
)
target_sources(libmaterial_editor PRIVATE ${MEDIT_PROTO_SRCS})
target_include_directories(libmaterial_editor PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# Build pipeline sources
set(PIPELINE_SRC_DIR "${CMAKE_BINARY_DIR}/material_editor-generated/pipelines")
set(PIPELINE_SRC ${PIPELINE_SRC_DIR}/pipelines.cpp)
add_custom_command(
    OUTPUT ${PIPELINE_SRC}
    COMMAND
        pipeline_compiler "${CMAKE_CURRENT_LIST_DIR}/pipelines.se"
        -o ${PIPELINE_SRC_DIR}
        --shader-input "${CMAKE_CURRENT_LIST_DIR}/shaders"
        --shader-output "${PIPELINE_SRC_DIR}/../shaders"
        --spv --spv-version=1.5 --spv-target-env=vulkan1.3
    DEPENDS ${CMAKE_CURRENT_LIST_DIR}/pipelines.se pipeline_compiler
    VERBATIM
)
target_sources(libmaterial_editor PRIVATE ${PIPELINE_SRC})
target_include_directories(libmaterial_editor PRIVATE ${PIPELINE_SRC_DIR})
