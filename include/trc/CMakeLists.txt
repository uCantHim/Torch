target_sources(
    torch
    PUBLIC
        CommandCollector.h
        DrawableBase.h
        Node.h
        Pipeline.h
        PipelineBuilder.h
        Renderpass.h
)

add_subdirectory(base)
add_subdirectory(utils)
add_subdirectory(data_utils)

target_include_directories(torch PUBLIC ${CMAKE_CURRENT_LIST_DIR})
