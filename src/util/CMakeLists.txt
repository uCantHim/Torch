target_sources(
    torch
    PRIVATE
        ArgParse.cpp
)

add_subdirectory(data)
add_subdirectory(functional)
add_subdirectory(async)
