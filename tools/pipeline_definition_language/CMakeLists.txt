add_executable(pipeline_compiler main.cpp
    src/AstPrinter.cpp
    src/Compiler.cpp
    src/CompilerObjectRepresentation.cpp
    src/ErrorReporter.cpp
    src/FlagTable.cpp
    src/IdentifierTable.cpp
    src/Parser.cpp
    src/PipelineDefinitionLanguage.cpp
    src/Scanner.cpp
    src/TorchCppWriter.cpp
    src/TypeChecker.cpp
    src/TypeConfiguration.cpp
    src/UniqueName.cpp
    src/VariantResolver.cpp
)

target_include_directories(pipeline_compiler PRIVATE src)
target_link_libraries(pipeline_compiler PRIVATE shader_compiler_lib)

set_target_properties(pipeline_compiler PROPERTIES
    OUTPUT_NAME compile-pipeline
    RUNTIME_OUTPUT_DIRECTORY ${TORCH_EXECUTABLE_OUTPUT_PATH}
)
