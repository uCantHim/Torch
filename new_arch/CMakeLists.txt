target_sources(
    torch
    PUBLIC
        Drawable.h
        Engine.h
        IndexMap.h
        ObjectPool.h
        ObjectPool.inl
        Pipeline.h
        Renderpass.h
        Scene.cpp
        Scene.h
        SelfManagedObject.h
        Shader.h
        SortedObjectPool.h
)

target_include_directories(torch PUBLIC ${CMAKE_CURRENT_LIST_DIR})
