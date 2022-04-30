#include "TypeConfiguration.h"



auto makeDefaultTypeConfig() -> TypeConfiguration
{
    return TypeConfiguration({
        // Fundamental string type
        { stringTypeName, StringType{} },

        // Type of the "global" object, which has both arbitrary field names
        // and some pre-defined fields (e.g. 'Meta')
        { globalObjectTypeName, ObjectType{
            .typeName=globalObjectTypeName,
            .fields={
                { "Meta", { "CompilerMetaData" } },
            }
        }},
        { "CompilerMetaData", ObjectType{
            .typeName="CompilerMetaData",
            .fields={
                { "BaseDir", { stringTypeName } },
            }
        }},

        // The following are custom (non-built-in) types
        { "Variable", StringType{} },
        { "Shader", ObjectType{
            .typeName="Shader",
            .fields={
                { "Source",   { stringTypeName } },
                { "Variable", { "Variable", FieldType::eMap } },
            }
        }},
        { "Program", ObjectType{
            .typeName="Program",
            .fields={
                { "VertexShader", { "Shader" } },
                { "TessControlShader", { "Shader" } },
                { "TessEvalShader", { "Shader" } },
                { "GeometryShader", { "Shader" } },
                { "FragmentShader", { "Shader" } },
            }
        }},
        { "VertexAttribute", ObjectType{
            .typeName="VertexAttribute",
            .fields={
                { "Locations", { "Format", FieldType::eList } },
            }
        }},
        { "Pipeline", ObjectType{
            .typeName="Pipeline",
            .fields={
                { "Base",    { "Pipeline" } },
                { "Program", { "Program" } },
                { "VertexInput", { "VertexAttribute", FieldType::eList } },
            }
        }},
    });
}
