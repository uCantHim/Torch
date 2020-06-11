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
        Scene.h
        SelfManagedObject.h
        Shader.h
        SortedObjectPool.h
    PRIVATE
        IndexMap.inl
        Scene.cpp
)

target_include_directories(torch PUBLIC ${CMAKE_CURRENT_LIST_DIR})

add_executable(NewArchTest main.cpp)
target_sources(
    NewArchTest
    PUBLIC
        Drawable.h
        Engine.h
        IndexMap.h
        ObjectPool.h
        ObjectPool.inl
        Pipeline.h
        Renderpass.h
        Scene.h
        SelfManagedObject.h
        Shader.h
        SortedObjectPool.h
    PRIVATE
        Scene.cpp
)
