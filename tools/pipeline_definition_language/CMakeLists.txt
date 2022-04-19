add_executable(pipeline_compiler main.cpp
    src/AstPrinter.cpp
    src/ErrorReporter.cpp
    src/Parser.cpp
    src/PipelineDefinitionLanguage.cpp
    src/Scanner.cpp
    src/TypeChecker.cpp
)

target_include_directories(pipeline_compiler PRIVATE src)
target_link_libraries(pipeline_compiler PRIVATE shader_compiler_lib)

set_target_properties(pipeline_compiler PROPERTIES
    OUTPUT_NAME compile-pipeline
    RUNTIME_OUTPUT_DIRECTORY ${TORCH_EXECUTABLE_OUTPUT_PATH}
)
