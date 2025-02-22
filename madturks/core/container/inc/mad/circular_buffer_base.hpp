/******************************************************
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/

#pragma once

#include <cstdint> // for std::*int*_t, std::size_t
#include <cstdlib> // for ?
#include <memory>  // for std::unique_ptr

namespace mad {

class circular_buffer_base {
protected:
    using element_t = std::uint8_t;
    using buffer_ptr_t = std::unique_ptr<element_t, decltype(&free)>;
    using offset_t = std::int64_t;
    using size_t = std::size_t;

public:
    /******************************************************
     * Construct a new circular buffer base object
     *
     * @param total_size
     ******************************************************/
    circular_buffer_base(const size_t total_size);

    /*****************************************************
     * @brief Move constructor for circular buffer base
     *
     * @param mv
     */
    circular_buffer_base(circular_buffer_base && mv);

    /******************************************************/

    circular_buffer_base & operator=(circular_buffer_base && mv);

    /******************************************************/

    inline size_t total_size() const noexcept {
        return total_size_;
    }

    /******************************************************/

    /**
     * @brief native_buffer
     * The underlying buffer, do not mess with it unless you're
     * confident that you know what are you doing with it.
     * @return The pointer to the natively allocated buffer resource
     */
    element_t * native_buffer_tail() const noexcept {
        return (&native_buffer_.get() [tail_]);
    }

    /******************************************************/

    /**
     * @brief native_buffer
     * The underlying buffer, do not mess with it unless you're
     * confident that you know what are you doing with it.
     * @return The pointer to the natively allocated buffer resource
     */
    element_t * native_buffer_head() const noexcept {
        return (&native_buffer_.get() [head_]);
    }

protected:
    static inline auto min(const auto a, const auto b) {
        return a < b ? a : b;
    }

    /******************************************************/

    buffer_ptr_t native_buffer_ = { nullptr, nullptr };
    size_t total_size_ = { 0 }, head_ = { 0 }, tail_ = { 0 };
    bool overwrite_ = { false };
};

} // namespace mad
