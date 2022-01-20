add_executable(scene_editor main.cpp)
torch_default_compile_options(scene_editor)

add_subdirectory(src)

target_link_libraries(scene_editor PRIVATE torch component)
