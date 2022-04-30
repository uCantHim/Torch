#include "Compiler.h"

#include "Exceptions.h"
#include "Util.h"



Compiler::Compiler(std::vector<Stmt> _statements, ErrorReporter& errorReporter)
    :
    converter(std::move(_statements)),
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
auto Compiler::makeReference(const compiler::Value& val) -> ObjectReference<T>
{
    return std::visit(VariantVisitor{
        [this](const compiler::Literal& lit) -> ObjectReference<T>
        {
            error({}, "Cannot create an artificial reference to a literal value \""
                      + lit.value + "\".");
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
            VariantGroup<T> items{ .baseName=name };
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

    res.source = expectLiteral(expectSingle(expectField(obj, "Source"))).value;
    auto it = obj.fields.find("Variable");
    if (it != obj.fields.end())
    {
        for (const auto& [name, val] : expectMap(it->second).values)
        {
            res.variables.try_emplace(name, expectLiteral(*val).value);
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
    return {
        .program=makeReference<ProgramDesc>(expectSingle(expectField(obj, "Program")))
    };
}
