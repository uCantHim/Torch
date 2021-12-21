add_executable(shader_compiler ${CMAKE_CURRENT_LIST_DIR}/shader_compiler.cpp)
target_link_libraries(shader_compiler PRIVATE torch nlohmann_json::nlohmann_json)
