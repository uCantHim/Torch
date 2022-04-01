#pragma once

#include <glm/glm.hpp>

namespace trc::util
{
    template<typename... Ts> struct VariantVisitor : Ts... { using Ts::operator()...; };
    template<typename... Ts> VariantVisitor(Ts...) -> VariantVisitor<Ts...>;
} // namespace trc::util



//////////////////////////////////////
// Structured bindings for GLM vectors

namespace glm
{
  template<std::size_t I, auto N, class T, auto Q>
  constexpr auto& get(glm::vec<N, T, Q>& v) noexcept { return v[I]; }

  template<std::size_t I, auto N, class T, auto Q>
  constexpr const auto& get(const glm::vec<N, T, Q>& v) noexcept { return v[I]; }

  template<std::size_t I, auto N, class T, auto Q>
  constexpr auto&& get(glm::vec<N, T, Q>&& v) noexcept { return std::move(v[I]); }

  template<std::size_t I, auto N, class T, auto Q>
  constexpr const auto&& get(const glm::vec<N, T, Q>&& v) noexcept { return std::move(v[I]); }
} // namespace glm

namespace std
{
    template <auto N, class T, auto Q>
    struct tuple_size<glm::vec<N, T, Q>> : std::integral_constant<std::size_t, N> { };

    template <std::size_t I, auto N, class T, auto Q>
    struct tuple_element<I, glm::vec<N, T, Q>> {
        using type = decltype(get<I>(declval<glm::vec<N,T,Q>>()));
    };
} // namespace std
