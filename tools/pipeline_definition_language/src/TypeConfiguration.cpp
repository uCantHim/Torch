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
                { "Namespace", { stringTypeName } },
            }
        }},

        // The following are custom (non-built-in) types
        { "Variable", StringType{} },
        { "Shader", ObjectType{
            .typeName="Shader",
            .fields={
                { "Source",     { stringTypeName } },
                { "Target",     { stringTypeName } },
                { "TargetType", { "ShaderTargetType" } },
                { "Variable",   { "Variable", FieldType::eMap } },
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
        { "Descriptor", ObjectType{
            .typeName="Descriptor",
            .fields={
                { "Name", { stringTypeName } },
                { "Type", { "DescriptorType" } },
            }
        }},
        { "PushConstant", ObjectType{
            .typeName="PushConstant",
            .fields={
                { "Offset", { intTypeName } },
                { "Size", { intTypeName } },
                { "DefaultValue", { stringTypeName } },
            }
        }},
        { "Layout", ObjectType{
            .typeName="Layout",
            .fields={
                { "Descriptors", { "Descriptor", FieldType::eList } },

                { "VertexPushConstants",      { "PushConstant", FieldType::eList } },
                { "FragmentPushConstants",    { "PushConstant", FieldType::eList } },
                { "TessControlPushConstants", { "PushConstant", FieldType::eList } },
                { "TessEvalPushConstants",    { "PushConstant", FieldType::eList } },
                { "GeometryPushConstants",    { "PushConstant", FieldType::eList } },
                { "ComputePushConstants",     { "PushConstant", FieldType::eList } },
                { "MeshPushConstants",        { "PushConstant", FieldType::eList } },
                { "TaskPushConstants",        { "PushConstant", FieldType::eList } },
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
                { "Layout", { "Layout" } },
                { "RenderPass", { "String" } },

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
        { "ComputePipeline", ObjectType{
            .typeName="ComputePipeline",
            .fields={
                { "Layout", { "Layout" } },
                { "Shader", { "Shader" } },
            }
        }},
    });
}
