#include "VariantResolver.h"



struct Resolver
{
    auto operator()(const LiteralValue& val) const -> std::vector<FieldValueVariant>
    {
        std::vector<FieldValueVariant> res;
        res.push_back({ .setFlags={}, .value=val });
        return res;
    }

    auto operator()(const Identifier& id) const -> std::vector<FieldValueVariant>
    {
        std::vector<FieldValueVariant> res;
        res.push_back({ .setFlags={}, .value=id });
        return res;
    }

    auto operator()(const ObjectDeclaration& obj) const -> std::vector<FieldValueVariant>
    {
        std::vector<FieldValueVariant> results;
        for (const auto& [name, value] : obj.fields)
        {
            auto variants = std::visit(*this, *value);
        }

        return results;
    }

    auto operator()(const MatchExpression& expr) const -> std::vector<FieldValueVariant>
    {
        std::vector<FieldValueVariant> results;
        for (const auto& opt : expr.cases)
        {
            auto values = std::visit(*this, *opt.value);
        }
        return results;
    }
};



VariantResolver::VariantResolver(const FlagTable& flags)
    :
    flagTable(flags)
{
}

auto VariantResolver::resolve(FieldValue& value) -> std::vector<FieldValueVariant>
{
    return {};
}
