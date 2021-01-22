target_sources(
    torch PRIVATE
    Window.cpp
)

# This one is for the concrete implementation that integrates the gui
# library into torch. I will probably move this some place else when I'm
# done.
add_subdirectory(torch)
