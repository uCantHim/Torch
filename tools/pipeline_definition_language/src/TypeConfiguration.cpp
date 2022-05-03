#include "TypeConfiguration.h"



auto makeDefaultTypeConfig() -> TypeConfiguration
{
    return TypeConfiguration({
        // Fundamental string type
        { stringTypeName, StringType{} },
        { floatTypeName, FloatType{} },
        { intTypeName, IntType{} },

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
                { "Binding", { "Int" } },
                { "Stride", { "Int" } },
                { "InputRate", { "VertexInputRate" } },
                { "Locations", { "Format", FieldType::eList } },
            }
        }},
        { "Multisampling", ObjectType{
            .typeName="Multisampling",
            .fields={
                { "Samples", { "Int" } },
            }
        }},
        { "ColorBlendAttachment", ObjectType{
            .typeName="ColorBlendAttachment",
            .fields={
                { "ColorBlending",   { "Bool" } },
                { "SrcColorFactor",  { "BlendFactor" } },
                { "DstColorFactor",  { "BlendFactor" } },
                { "ColorBlendOp",    { "BlendOp" } },
                { "SrcAlphaFactor",  { "BlendFactor" } },
                { "DstAlphaFactor",  { "BlendFactor" } },
                { "AlphaBlendOp",    { "BlendOp" } },
                { "ColorComponents", { "ColorComponent", FieldType::eList } },
            }
        }},
        { "Pipeline", ObjectType{
            .typeName="Pipeline",
            .fields={
                { "Base",    { "Pipeline" } },
                { "Program", { "Program" } },

                // Vertex input
                { "VertexInput", { "VertexAttribute", FieldType::eList } },

                // Input assembly
                { "PrimitiveTopology", { "PrimitiveTopology" } },

                // Rasterization
                { "FillMode", { "PolygonFillMode" } },
                { "CullMode", { "CullMode" } },
                { "FaceWinding", { "FaceWinding" } },
                { "DepthBiasConstant", { "Float" } },
                { "DepthBiasSlope", { "Float" } },
                { "DepthBiasClamp", { "Float" } },
                { "LineWidth", { "Float" } },

                // Multisampling
                { "Multisampling", { "Multisampling" } },

                // Depth/stencil state
                { "DepthTest", { "Bool" } },
                { "DepthWrite", { "Bool" } },
                { "StencilTest", { "Bool" } },

                // Color blending
                { "DisableBlendAttachments", { "Int" } },
                { "ColorBlendAttachments", { "ColorBlendAttachment", FieldType::eList } },

                // Dynamic state
                { "DynamicState", { "String", FieldType::eList } },
            }
        }},
    });
}
