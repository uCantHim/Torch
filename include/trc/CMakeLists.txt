target_sources(
    torch
    PUBLIC
        AssetRegistry.h
        Boilerplate.h
        CommandCollector.h
        DescriptorProvider.h
        Drawable.h
        Geometry.h
        Light.h
        Material.h
        Node.h
        Pipeline.h
        PipelineBuilder.h
        Renderpass.h
        Vertex.h
)

add_subdirectory(base)
add_subdirectory(utils)
add_subdirectory(data_utils)

target_include_directories(torch PUBLIC ${CMAKE_CURRENT_LIST_DIR})
