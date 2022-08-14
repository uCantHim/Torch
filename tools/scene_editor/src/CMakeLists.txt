target_sources(scene_editor PRIVATE
    App.cpp
    AssetFileIndex.cpp
    DefaultAssets.cpp
    Project.cpp
    ProjectDirectory.cpp
    Scene.cpp
)

add_subdirectory(asset)
add_subdirectory(command)
add_subdirectory(gui)
add_subdirectory(input)
add_subdirectory(object)

target_include_directories(scene_editor PRIVATE ${CMAKE_CURRENT_LIST_DIR})
