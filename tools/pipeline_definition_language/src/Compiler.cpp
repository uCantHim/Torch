#include "Compiler.h"

#include "Exceptions.h"
#include "Util.h"
#include "PipelineDataWriter.h"



auto translatePolygonMode(const std::string& str) -> PipelineDesc::Rasterization::PolygonMode
{
    using Mode = PipelineDesc::Rasterization::PolygonMode;
    static std::unordered_map<std::string, Mode> map{
        { "points", Mode::ePoint },
        { "wireframe", Mode::eLine },
        { "fill", Mode::eFill },
    };

    return map.at(str);
}

auto translateCullMode(const std::string& str) -> PipelineDesc::Rasterization::CullMode
{
    using Mode = PipelineDesc::Rasterization::CullMode;
    static std::unordered_map<std::string, Mode> map{
        { "cullNone", Mode::eNone },
        { "cullFrontFace", Mode::eFront },
        { "cullBackFace", Mode::eBack },
        { "cullFrontAndBack", Mode::eBoth },
    };

    return map.at(str);
}

auto translateFaceWinding(const std::string& str) -> PipelineDesc::Rasterization::FaceWinding
{
    using Mode = PipelineDesc::Rasterization::FaceWinding;
    static std::unordered_map<std::string, Mode> map{
        { "clockwise", Mode::eClockwise },
        { "counterClockwise", Mode::eCounterClockwise },
    };

    return map.at(str);
}

auto translateBlendFactor(const std::string& str) -> PipelineDesc::BlendAttachment::BlendFactor
{
    using Mode = PipelineDesc::BlendAttachment::BlendFactor;
    static std::unordered_map<std::string, Mode> map{
        { "one",                   Mode::eOne },
        { "zero",                  Mode::eZero },
        { "constantAlpha",         Mode::eConstantAlpha },
        { "constantColor",         Mode::eConstantColor },
        { "dstAlpha",              Mode::eDstAlpha },
        { "dstColor",              Mode::eDstColor },
        { "srcAlpha",              Mode::eSrcAlpha },
        { "srcColor",              Mode::eSrcColor },
        { "oneMinusConstantAlpha", Mode::eOneMinusConstantAlpha },
        { "oneMinusConstantColor", Mode::eOneMinusConstantColor },
        { "oneMinusDstAlpha",      Mode::eOneMinusDstAlpha },
        { "oneMinusDstColor",      Mode::eOneMinusDstColor },
        { "oneMinusSrcAlpha",      Mode::eOneMinusSrcAlpha },
        { "oneMinusSrcColor",      Mode::eOneMinusSrcColor },
    };

    return map.at(str);
}

auto translateBlendOp(const std::string& str) -> PipelineDesc::BlendAttachment::BlendOp
{
    using Mode = PipelineDesc::BlendAttachment::BlendOp;
    static std::unordered_map<std::string, Mode> map{
        { "add", Mode::eAdd },
    };

    return map.at(str);
}

auto translateColorFlag(const std::string& flag) -> PipelineDesc::BlendAttachment::Color
{
    using Mode = PipelineDesc::BlendAttachment::Color;
    static std::unordered_map<std::string, Mode> map{
        { "red",   Mode::eR },
        { "green", Mode::eG },
        { "blue",  Mode::eB },
        { "alpha", Mode::eA },
    };

    return map.at(flag);
}



Compiler::Compiler(std::vector<Stmt> _statements, ErrorReporter& errorReporter)
    :
    converter(std::move(_statements), errorReporter),
    errorReporter(&errorReporter)
{
}

auto Compiler::compile() -> CompileResult
{
    globalObject = converter.convert();

    // Collect identifiers
    for (const auto& [name, field] : globalObject.fields)
    {
        if (std::holds_alternative<compiler::MapValue>(field))
        {
            const auto& map = std::get<compiler::MapValue>(field);
            for (const auto& [identifier, value] : map.values)
            {
                referenceTable.try_emplace(identifier, value);
            }
        }
    }

    // Compile objects
    auto it = globalObject.fields.find("Shader");
    if (it != globalObject.fields.end()) {
        result.shaders = compileMulti<ShaderDesc>(expectMap(it->second));
    }
    it = globalObject.fields.find("Program");
    if (it != globalObject.fields.end()) {
        result.programs = compileMulti<ProgramDesc>(expectMap(it->second));
    }
    it = globalObject.fields.find("Pipeline");
    if (it != globalObject.fields.end()) {
        result.pipelines = compileMulti<PipelineDesc>(expectMap(it->second));
    }

    result.flagTable = converter.getFlagTable();
    return result;
}

bool Compiler::isSubset(const VariantFlagSet& set, const VariantFlagSet& subset)
{
    std::unordered_set<VariantFlag> flags(subset.begin(), subset.end());
    for (const auto& flag : set) {
        flags.erase(flag);
    }

    return flags.empty();
}

auto Compiler::findVariant(
    const compiler::Variated& val,
    const VariantFlagSet& flags) -> const compiler::Variated::Variant*
{
    auto comp = [&flags](const auto& a){ return isSubset(flags, a.setFlags); };

    auto it = std::find_if(val.variants.begin(), val.variants.end(), comp);
    if (it == val.variants.end()) {
        return nullptr;
    }
    return &*it;
}

void Compiler::error(const Token& token, std::string message)
{
    errorReporter->error(Error{ .location=token.location, .message=std::move(message) });
}

auto Compiler::resolveReference(const compiler::Reference& ref) -> const compiler::Value&
{
    try {
        return *referenceTable.at(ref.name);
    }
    catch (const std::out_of_range&) {
        error({}, "Referenced value at identifier \"" + ref.name + "\" does not exist.");
        throw CompilerError{};
    }
}

bool Compiler::hasField(const compiler::Object& obj, const std::string& field)
{
    return obj.fields.contains(field);
}

auto Compiler::expectField(const compiler::Object& obj, const std::string& field)
    -> const compiler::FieldValueType&
{
    try {
        return obj.fields.at(field);
    }
    catch (const std::out_of_range& err) {
        error({}, "Expected field \"" + field + "\"");
        throw InternalLogicError(err.what());
    }
}

auto Compiler::expectSingle(const compiler::FieldValueType& field) -> const compiler::Value&
{
    return *std::get<compiler::SingleValue>(field).value;
}

auto Compiler::expectMap(const compiler::FieldValueType& field) -> const compiler::MapValue&
{
    return std::get<compiler::MapValue>(field);
}

auto Compiler::expectSingle(const compiler::Object& obj, const std::string& field)
    -> const compiler::Value&
{
    return expectSingle(expectField(obj, field));
}

auto Compiler::expectMap(const compiler::Object& obj, const std::string& field)
    -> const compiler::MapValue&
{
    return expectMap(expectField(obj, field));
}

template<typename T>
auto Compiler::expect(const compiler::Value& val) -> const T&
{
    const auto& newVal = [this, &val]() -> const compiler::Value& {
        if (std::holds_alternative<compiler::Reference>(val)) {
            return resolveReference(std::get<compiler::Reference>(val));
        }
        return val;
    }();

    if (std::holds_alternative<compiler::Variated>(newVal))
    {
        const auto& variated = std::get<compiler::Variated>(newVal);

        assert(currentVariant != nullptr);
        auto var = findVariant(variated, *currentVariant);
        if (var == nullptr)
        {
            error({}, "Expected variant does not exist in the variated field.");
            throw CompilerError{};
        }
        return expect<T>(*var->value);
    }

    try {
        return std::get<T>(newVal);
    }
    catch (const std::bad_variant_access&) {
        error({}, "Unexpected value type");
        throw CompilerError{};
    }
}

auto Compiler::expectLiteral(const compiler::Value& val) -> const compiler::Literal&
{
    return expect<compiler::Literal>(val);
}

auto Compiler::expectList(const compiler::Value& val) -> const compiler::List&
{
    return expect<compiler::List>(val);
}

auto Compiler::expectObject(const compiler::Value& val) -> const compiler::Object&
{
    return expect<compiler::Object>(val);
}

template<typename T>
auto Compiler::expect(const compiler::Literal& val) -> const T&
{
    try {
        return std::get<T>(val.value);
    }
    catch (const std::bad_variant_access&) {
        error({}, "Unexpected literal value type");
        throw CompilerError{};
    }
}

auto Compiler::expectString(const compiler::Value& val) -> const std::string&
{
    return expect<std::string>(expectLiteral(val));
}

auto Compiler::expectFloat(const compiler::Value& val) -> double
{
    return expect<double>(expectLiteral(val));
}

auto Compiler::expectInt(const compiler::Value& val) -> int64_t
{
    return expect<int64_t>(expectLiteral(val));
}

auto Compiler::expectBool(const compiler::Value& val) -> bool
{
    return expectString(val) == "true";
}

template<typename T>
auto Compiler::makeReference(const compiler::Value& val) -> ObjectReference<T>
{
    return std::visit(VariantVisitor{
        [this](const compiler::Literal&) -> ObjectReference<T>
        {
            error({}, "Cannot create an artificial reference to a literal value.");
            throw CompilerError{};
            return UniqueName("");
        },
        [this](const compiler::List&) -> ObjectReference<T>
        {
            error({}, "Cannot create an artificial reference to a list.");
            throw CompilerError{};
            return UniqueName("");
        },
        [this](const compiler::Object& obj) -> ObjectReference<T> {
            return compileSingle<T>(obj);
        },
        [this](const compiler::Variated& variated) -> ObjectReference<T>
        {
            auto var = findVariant(variated, *currentVariant);
            return makeReference<T>(*var->value);
        },
        [this](const compiler::Reference& ref) -> ObjectReference<T>
        {
            auto val = resolveReference(ref);
            if (std::holds_alternative<compiler::Variated>(val))
            {
                auto var = findVariant(std::get<compiler::Variated>(val), *currentVariant);
                return UniqueName(ref.name, var->setFlags);
            }
            return UniqueName(ref.name);
        },
    }, val);
}

template<typename T>
auto Compiler::compileMulti(const compiler::MapValue& val)
    -> std::unordered_map<std::string, CompileResult::SingleOrVariant<T>>
{
    std::unordered_map<std::string, CompileResult::SingleOrVariant<T>> res;
    for (const auto& [name, value] : val.values)
    {
        if (std::holds_alternative<compiler::Variated>(*value))
        {
            VariantGroup<T> items{ name };
            for (const auto& var : std::get<compiler::Variated>(*value).variants)
            {
                currentVariant = &var.setFlags;
                auto item = compileSingle<T>(expectObject(*var.value));
                items.variants.try_emplace(UniqueName(name, var.setFlags), std::move(item));
            }
            if (items.variants.empty()) {
                throw InternalLogicError("Variated value must have at least one variant");
            }
            // Collect all flag types
            for (auto& flag : items.variants.begin()->first.getFlags()) {
                items.flagTypes.emplace_back(flag.flagId);
            }
            res.try_emplace(name, std::move(items));
        }
        else {
            currentVariant = nullptr;
            auto item = compileSingle<T>(expectObject(*value));
            res.try_emplace(name, std::move(item));
        }
    }

    return res;
}

template<>
auto Compiler::compileSingle<ShaderDesc>(const compiler::Object& obj) -> ShaderDesc
{
    ShaderDesc res;

    res.source = expectString(expectLiteral(expectSingle(expectField(obj, "Source"))));
    auto it = obj.fields.find("Variable");
    if (it != obj.fields.end())
    {
        for (const auto& [name, val] : expectMap(it->second).values)
        {
            res.variables.try_emplace(name, expectString(expectLiteral(*val)));
        }
    }

    return res;
}

template<>
auto Compiler::compileSingle<ProgramDesc>(const compiler::Object& obj) -> ProgramDesc
{
    ProgramDesc prog;
    if (obj.fields.contains("VertexShader")) {
        prog.vert = makeReference<ShaderDesc>(expectSingle(expectField(obj, "VertexShader")));
    }
    if (obj.fields.contains("TessControlShader")) {
        prog.tesc = makeReference<ShaderDesc>(expectSingle(expectField(obj, "TessControlShader")));
    }
    if (obj.fields.contains("TessEvalShader")) {
        prog.tese = makeReference<ShaderDesc>(expectSingle(expectField(obj, "TessEvalShader")));
    }
    if (obj.fields.contains("GeometryShader")) {
        prog.geom = makeReference<ShaderDesc>(expectSingle(expectField(obj, "GeometryShader")));
    }
    if (obj.fields.contains("FragmentShader")) {
        prog.frag = makeReference<ShaderDesc>(expectSingle(expectField(obj, "FragmentShader")));
    }

    return prog;
}

template<>
auto Compiler::compileSingle<PipelineDesc>(const compiler::Object& obj) -> PipelineDesc
{
    // Compile vertex inputs
    std::vector<PipelineDesc::VertexAttribute> vertexInput;
    for (size_t binding = 0;
         const auto& value : expectList(expectSingle(obj, "VertexInput")).values)
    {
        assert(value != nullptr);
        const auto& in = expectObject(*value);
        auto& out = vertexInput.emplace_back();

        // Formats
        size_t calcStride = 0;  // Calculate the canonical stride for later
        for (const auto& opt : expectList(expectSingle(in, "Locations")).values)
        {
            assert(opt != nullptr);
            auto& format = out.locationFormats.emplace_back(expectString((*opt)));
            calcStride += util::getFormatByteSize(format);
        }

        // Binding (optional)
        if (hasField(in, "Binding")) {
            out.binding = expectInt(expectSingle(in, "Binding"));
        }
        else out.binding = binding;

        // Stride (optional)
        if (hasField(in, "Stride")) {
            out.stride = expectInt(expectSingle(in, "Stride"));
        }
        else out.stride = calcStride;

        ++binding;
    }

    // Compile input assembly state
    PipelineDesc::InputAssembly inputAssembly;
    if (hasField(obj, "PrimitiveTopology")) {
        inputAssembly.primitiveTopology = expectString(expectSingle(obj, "PrimitiveTopology"));
    }

    // Compile rasterization state
    PipelineDesc::Rasterization r;
    if (hasField(obj, "FillMode")) {
        r.polygonMode = translatePolygonMode(expectString(expectSingle(obj, "PolygonFillMode")));
    }
    if (hasField(obj, "CullMode")) {
        r.cullMode = translateCullMode(expectString(expectSingle(obj, "CullMode")));
    }
    if (hasField(obj, "FaceWinding")) {
        r.faceWinding = translateFaceWinding(expectString(expectSingle(obj, "FaceWinding")));
    }
    r.depthBiasEnable = hasField(obj, "DepthBiasConstant") || hasField(obj, "DepthBiasSlope");
    if (hasField(obj, "DepthBiasConstant")) {
        r.depthBiasConstantFactor = expectFloat(expectSingle(obj, "DepthBiasConstant"));
    }
    if (hasField(obj, "DepthBiasSlope")) {
        r.depthBiasSlopeFactor = expectFloat(expectSingle(obj, "DepthBiasSlope"));
    }
    if (hasField(obj, "DepthBiasClamp")) {
        r.depthBiasClamp = expectFloat(expectSingle(obj, "DepthBiasClamp"));
    }
    if (hasField(obj, "LineWidth")) {
        r.lineWidth = expectFloat(expectSingle(obj, "LineWidth"));
    }

    // Compile multisampling
    PipelineDesc::Multisampling multisampling;
    if (hasField(obj, "Multisampling"))
    {
        const auto& in = expectObject(expectSingle(obj, "Multisampling"));
        multisampling.samples = expectInt(expectSingle(in, "Samples"));
        if (!std::unordered_set<size_t>{ 1, 2, 4, 8, 16 }.contains(multisampling.samples))
        {
            error({}, "Number of samples in a Multisampling structure must be either"
                      " 1, 2, 4, 8, or 16!");
        }
    }

    // Compile depth/stencil state
    PipelineDesc::DepthStencil ds;
    if (hasField(obj, "DepthTest")) {
        ds.depthTestEnable = expectBool(expectSingle(obj, "DepthTest"));
    }
    if (hasField(obj, "DepthWrite")) {
        ds.depthWriteEnable = expectBool(expectSingle(obj, "DepthWrite"));
    }
    if (hasField(obj, "StencilTest")) {
        ds.stencilTestEnable = expectBool(expectSingle(obj, "StencilTest"));
    }

    // Compile color blend state
    std::vector<PipelineDesc::BlendAttachment> blendAttachments;
    if (hasField(obj, "DisableBlendAttachments")) {
        blendAttachments.resize(expectInt(expectSingle(obj, "DisableBlendAttachments")));
    }
    else if (hasField(obj, "ColorBlendAttachments"))
    {
        for (const auto& item : expectList(expectSingle(obj, "ColorBlendAttachments")).values)
        {
            const auto& att = expectObject(*item);
            auto str = [&, this](auto&& name){ return expectString(expectSingle(att, name)); };

            auto& newAtt = blendAttachments.emplace_back();
            if (hasField(att, "ColorBlending"))
                newAtt.blendEnable = expectBool(expectSingle(att, "ColorBlending"));
            if (hasField(att, "SrcColorFactor"))
                newAtt.srcColorFactor = translateBlendFactor(str("SrcColorFactor"));
            if (hasField(att, "DstColorFactor"))
                newAtt.dstColorFactor = translateBlendFactor(str("DstColorFactor"));
            if (hasField(att, "ColorBlendOp"))
                newAtt.colorBlendOp = translateBlendOp(str("ColorBlendOp"));
            if (hasField(att, "SrcAlphaFactor"))
                newAtt.srcAlphaFactor = translateBlendFactor(str("SrcAlphaFactor"));
            if (hasField(att, "DstAlphaFactor"))
                newAtt.dstAlphaFactor = translateBlendFactor(str("DstAlphaFactor"));
            if (hasField(att, "AlphaBlendOp"))
                newAtt.alphaBlendOp = translateBlendOp(str("AlphaBlendOp"));

            if (hasField(att, "ColorComponents"))
            {
                newAtt.colorComponentFlags = 0;
                for (const auto& item : expectList(expectSingle(att, "ColorComponents")).values) {
                    newAtt.colorComponentFlags |= translateColorFlag(expectString(*item));
                }
            }
        }
    }

    // Compile dynamic states
    std::vector<std::string> dynamicStates;
    if (hasField(obj, "DynamicState"))
    {
        for (const auto& item : expectList(expectSingle(obj, "DynamicState")).values) {
            dynamicStates.emplace_back(expectString(*item));
        }
    }

    return {
        .program=makeReference<ProgramDesc>(expectSingle(obj, "Program")),
        .vertexInput=std::move(vertexInput),
        .inputAssembly={},
        .tessellation={},
        .rasterization=r,
        .multisampling=multisampling,
        .depthStencil=ds,
        .blendAttachments=std::move(blendAttachments),
        .dynamicStates=std::move(dynamicStates),
    };
}
