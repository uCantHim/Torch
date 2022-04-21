#pragma once

template<typename... Ts> struct VariantVisitor : Ts... { using Ts::operator()...; };
template<typename... Ts> VariantVisitor(Ts...) -> VariantVisitor<Ts...>;
