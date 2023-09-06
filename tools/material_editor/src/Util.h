#pragma once

#include <variant>

/**
 * @brief Access a field of a variant's alternative by name
 *
 * If all alternatives of a variant have a field with the same name, this macro
 * accesses that field, no matter which alternative is currently stored in the
 * variant.
 */
#define access(_obj, _field) \
    std::visit([](auto&& obj){ return obj._field; }, _obj)

/**
 * @brief Try to access a field of a variant
 *
 * @return pointer `variant`'s alternative of type `Field` if that alternative
 *                 is currently stored in the variant. `nullptr` otherwise.
 */
template<typename Field>
constexpr auto try_get(auto&& variant) -> decltype(&std::get<Field>(variant))
{
    if (std::holds_alternative<Field>(variant)) {
        return &std::get<Field>(variant);
    }
    return nullptr;
}

