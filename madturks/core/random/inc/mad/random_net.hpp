/******************************************************
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/

#pragma once

#include <mad/random_arithmetic.hpp>

#ifdef __has_include
#if __has_include(<experimental/net>)
#include <experimental/net>

namespace mad::random { namespace detail {
    namespace net_namespace = std::experimental::net;
}} // namespace mad::random::detail
#elif __has_include(<boost/asio/ts/net.hpp>)
#define BOOST_ASIO_HAS_STD_INVOKE_RESULT 1
#include <boost/asio/ts/net.hpp>

namespace mad::random { namespace detail {
    namespace net_namespace = boost::asio;
}} // namespace mad::random::detail
#else
#error "Missing networking ts impl!"
#endif
#else
#error "C++20 is required for networking ts support!"
#endif

namespace mad {
namespace net = detail::net_namespace;

/**
 * Generate a single random net::ip::address_v4.
 *
 * @return net::ip::address_v4 Random IPv4 address
 */
template <typename T>
requires(std::is_same_v<net::ip::address_v4, T>)
NCF_NODISCARD inline auto generate() -> net::ip::address_v4 {
    net::ip::address_v4::bytes_type data;
    fill(data.begin(), data.end());
    return net::ip::make_address_v4(data);
}

/**
 * Generate a single random net::ip::address_v6.
 *
 * @return net::ip::address_v4 Random IPv6 address
 */
template <typename T>
requires(std::is_same_v<net::ip::address_v6, T>)
NCF_NODISCARD inline auto generate() -> net::ip::address_v6 {
    net::ip::address_v6::bytes_type data;
    fill(data.begin(), data.end());
    return net::ip::make_address_v6(data);
}

/**
 * Fill between [begin, end] with random elements of type T.
 *
 * (container type element overload)
 *
 * @tparam Iter Auto-deduced iterator type
 * @tparam ValueType Auto-deduced iterator value type
 *
 * @param [in,out] begin Iterator to the start of the fill range
 * @param [in,out] end  Iterator to the end of the fill range
 * @param [in] lower_bound Minimum length of the value generated
 * @param [in] upper_bound Maximum length of the value generated
 *
 */
template <typename Iter, typename ValueType = typename Iter::value_type>
requires(std::is_same_v<net::ip::address_v4, ValueType> ||
         std::is_same_v<net::ip::address_v6, ValueType>)
inline auto fill(Iter begin, Iter end) {
    while (begin != end) {
        *begin++ = generate<ValueType>();
    }
}

/**
 * Make a container filled with random `net::ip::address_v4` or
 * `net::ip::address_v6` elements.
 *
 * (.push_back() overload)
 *
 * @tparam Container Container type
 * @tparam ValueType (auto-deduced) Value type
 *
 * @param [in] count Number of elements generated
 *
 * @return Container filled with `count` number of random ipv4 or ipv6 addresses
 */
template <typename Container,
          typename ValueType = typename Container::value_type>
requires((std::is_same_v<net::ip::address_v4, ValueType> ||
          std::is_same_v<net::ip::address_v6, ValueType>) &&
         has_push_back<Container> && std::is_default_constructible_v<Container>)
NCF_NODISCARD inline auto make(std::size_t count) {
    Container container;

    while (count--) {
        container.push_back(generate<ValueType>());
    }

    return container;
}

/**
 * Make a container filled with random `net::ip::address_v4` or
 * `net::ip::address_v6` elements.
 *
 * (.insert() overload)
 *
 * @tparam Container Container type
 * @tparam ValueType (auto-deduced) Value type
 *
 * @param [in] count Number of elements generated
 *
 * @return Container filled with `count` number of random ipv4 or ipv6 addresses
 */
template <typename Container,
          typename ValueType = typename Container::value_type>
requires((std::is_same_v<net::ip::address_v4, ValueType> ||
          std::is_same_v<net::ip::address_v6, ValueType>) &&
         has_unary_insert<Container> &&
         std::is_default_constructible_v<Container>)
NCF_NODISCARD inline auto make(std::size_t count) {
    Container container;
    while (count) {
        while (container.insert(generate<ValueType>()).second == false)
            ;
        count--;
    }
    return container;
}

} // namespace mad
