target_sources(scene_editor PRIVATE
    App.cpp
    AssetManager.cpp
    Scene.cpp
)

add_subdirectory(input)
add_subdirectory(gui)
add_subdirectory(object)

target_include_directories(scene_editor PRIVATE ${CMAKE_CURRENT_LIST_DIR})
