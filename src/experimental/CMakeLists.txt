if (${TORCH_INTEGRATE_IMGUI})
    target_sources(torch PRIVATE
        ImguiIntegration.cpp
    )
endif ()
