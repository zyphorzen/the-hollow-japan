#pragma once

#include "ecs/ComponentStorage.hpp"

#include <type_traits>

namespace thj::ecs {

namespace detail {

template <typename... Ts> struct UniqueTypes : std::true_type {};

template <typename First, typename... Rest>
struct UniqueTypes<First, Rest...>
    : std::bool_constant<((!std::is_same_v<First, Rest>) && ...) && UniqueTypes<Rest...>::value> {};
} // namespace detail

template <typename... Ts>
concept QueryComponents =
    (sizeof...(Ts) > 0) && (ComponentType<Ts> && ...) && detail::UniqueTypes<Ts...>::value;

} // namespace thj::ecs