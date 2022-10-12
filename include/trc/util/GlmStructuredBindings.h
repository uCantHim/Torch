#pragma once

#include <tuple>

#include "trc/Types.h"

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

template <auto N, class T, auto Q>
struct std::tuple_size<glm::vec<N, T, Q>> : std::integral_constant<std::size_t, N> { };

template <std::size_t I, auto N, class T, auto Q>
struct std::tuple_element<I, glm::vec<N, T, Q>> {
    using type = decltype(get<I>(declval<glm::vec<N,T,Q>>()));
};