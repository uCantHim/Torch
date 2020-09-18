add_executable(TorchTest main.cpp)

target_link_libraries(TorchTest PUBLIC vk_base torch profiler)
# target_include_directories(TorchTest PUBLIC ../vk_base/include)
