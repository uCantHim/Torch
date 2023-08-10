#pragma once

namespace trc::util
{
    /**
     * @brief A convenient constructor for variant visitors from lambdas.
     */
    template<typename... Ts> struct VariantVisitor : Ts... { using Ts::operator()...; };
    template<typename... Ts> VariantVisitor(Ts...) -> VariantVisitor<Ts...>;

    /**
     * @brief Requires a type to be a complete type
     */
    template<typename T>
    concept CompleteType = requires { sizeof(T); };
} // namespace trc::util
