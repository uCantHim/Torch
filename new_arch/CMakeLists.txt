target_sources(
    torch
    PUBLIC
        Drawable.h
        Engine.h
        Pipeline.h
        Renderpass.h
        Scene.h
        Shader.h
        StaticDrawable.h
        StaticDrawable.inl
    PRIVATE
        Drawable.cpp
        Scene.cpp
)

add_subdirectory(data_utils)

target_include_directories(torch PUBLIC ${CMAKE_CURRENT_LIST_DIR})

add_executable(NewArchTest main.cpp)
target_sources(
    NewArchTest
    PUBLIC
        Drawable.h
        Engine.h
        Pipeline.h
        Renderpass.h
        Scene.h
        Shader.h
        StaticDrawable.h
        StaticDrawable.inl
    PRIVATE
        Drawable.cpp
        Scene.cpp
)
