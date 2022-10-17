#include "trc/material/MaterialGraph.h"



namespace trc
{

class MaterialResultFunction : public MaterialFunction
{
public:
    explicit MaterialResultFunction()
        :
        MaterialFunction({
            .name="Material result node",
            .inputs={ { "color", vec4{} }, },
            .output={ "none", vec4{} }
        }, {})
    {
    }

    auto makeGlslCode(ShaderResourceInterface&) -> std::string override
    {
        return "RESULT NODE";
    }
};

MaterialResultNode::MaterialResultNode()
    : MaterialNode(std::make_unique<MaterialResultFunction>())
{
}

void MaterialResultNode::setColor(MaterialNode* colorNode)
{
    setInput(Input::eColor, colorNode);
}

auto MaterialResultNode::getColorNode() -> MaterialNode*
{
    return getInputs().at(Input::eColor);
}



/** A temporary hack */
class ConstantValueWrapperFunction : public MaterialFunction
{
public:
    ConstantValueWrapperFunction(Constant val)
        :
        MaterialFunction({
            .name="ConstantWrapperFunc_" + val.datatype() + "_" + std::to_string(uniqueIndex++),
            .inputs={},
            .output={ "constant", val.type }
        }, {}),
        value(val)
    {
    }

    auto makeGlslCode(ShaderResourceInterface& resources) -> std::string override
    {
        std::stringstream ss;
        ss << "return " << resources.makeScalarConstant(value) << ";";
        return ss.str();
    }

private:
    static inline ui32 uniqueIndex{ 0 };
    Constant value;
};

/** A temporary hack */
class BuiltinValueWrapperFunction : public MaterialFunction
{
public:
    BuiltinValueWrapperFunction(Builtin builtin, BasicType type)
        :
        MaterialFunction({
            .name="BuiltinWrapperFunc_" + std::to_string(uniqueIndex++),
            .inputs={},
            .output={ "constant", type }
        }, {}),
        builtin(builtin)
    {
    }

    auto makeGlslCode(ShaderResourceInterface& resources) -> std::string override
    {
        std::stringstream ss;
        ss << "return " << resources.queryConstant(builtin) << ";";
        return ss.str();
    }

private:
    static inline ui32 uniqueIndex{ 0 };
    Builtin builtin;
};

class TextureSampleFunction : public MaterialFunction
{
public:
    explicit TextureSampleFunction(TextureReference texture)
        :
        MaterialFunction(
            {
                .name="TextureSample",
                .inputs={
                    { "uv", vec2{} },
                },
                .output={ "color", vec4{} },
            },
            { Capability::eTextureSample }
        ),
        texture(std::move(texture))
    {
    }

    auto makeGlslCode(ShaderResourceInterface& resources) -> std::string
    {
        std::stringstream ss;
        ss << "return texture(" << resources.queryTexture(texture) << ", uv);";
        return ss.str();
    }

private:
    TextureReference texture;
    std::function<std::string(const std::string&)> textureAccessor;
};



auto MaterialGraph::getResultNode() -> MaterialResultNode&
{
    return resultNode;
}

auto MaterialGraph::makeConstant(Constant constant) -> MaterialNode*
{
    return makeNode(std::make_unique<ConstantValueWrapperFunction>(constant), {});
}

auto MaterialGraph::makeBuiltinConstant(Builtin constant) -> MaterialNode*
{
    return makeNode(
        std::make_unique<BuiltinValueWrapperFunction>(
            constant,
            ShaderCapabilityConfig::getConstantType(constant)
        ),
        {}
    );
}

auto MaterialGraph::makeTextureSample(TextureReference tex, MaterialNode* uvs)
    -> MaterialNode*
{
    return makeNode(std::make_unique<TextureSampleFunction>(tex), { uvs });
}

auto MaterialGraph::makeNode(
    u_ptr<MaterialFunction> func,
    std::initializer_list<MaterialNode*> args) -> MaterialNode*
{
    assert(func->getSignature().inputs.size() == args.size());

    auto node = nodes.emplace_back(new MaterialNode(std::move(func))).get();
    for (ui32 i = 0; auto input : args) {
        node->setInput(i++, input);
    }
    return node;
}

} // namespace trc
