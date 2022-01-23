target_sources(scene_editor PRIVATE
    App.cpp
    AssetManager.cpp
    Hitbox.cpp
    ObjectSelection.cpp
    Scene.cpp
)

add_subdirectory(input)
add_subdirectory(gui)

target_include_directories(scene_editor PRIVATE ${CMAKE_CURRENT_LIST_DIR})
