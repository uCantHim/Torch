target_sources(
    torch
    PRIVATE
        Camera.cpp
        Transformation.cpp
)

add_subdirectory(data)
add_subdirectory(functional)
add_subdirectory(async)
