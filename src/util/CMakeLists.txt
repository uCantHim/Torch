target_sources(
    torch
    PRIVATE
)

add_subdirectory(data)
add_subdirectory(functional)
add_subdirectory(async)
