target_sources(
    torch
    PUBLIC
        Drawable.h
        Engine.h
        Framebuffer.h
        Pipeline.h
        Renderpass.h
        Scene.h
        Shader.h
        StaticDrawable.h
        StaticDrawable.inl
    PRIVATE
        Drawable.cpp
        Framebuffer.cpp
        Renderpass.cpp
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
        Framebuffer.h
        Pipeline.h
        Renderpass.h
        Scene.h
        Shader.h
        StaticDrawable.h
        StaticDrawable.inl
    PRIVATE
        Drawable.cpp
        Framebuffer.cpp
        Renderpass.cpp
        Scene.cpp
)

target_link_libraries(NewArchTest PUBLIC vk_base)
target_include_directories(NewArchTest PUBLIC ../vk_base/include)
