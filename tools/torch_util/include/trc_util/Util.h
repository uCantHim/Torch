#pragma once

namespace trc::util
{
    template<typename... Ts> struct VariantVisitor : Ts... { using Ts::operator()...; };
    template<typename... Ts> VariantVisitor(Ts...) -> VariantVisitor<Ts...>;
} // namespace trc::util