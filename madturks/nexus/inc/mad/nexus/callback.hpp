/******************************************************
 * Generic callback function type with context pointer.
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/
#pragma once

#include <cassert>
#include <utility>

namespace mad::nexus {

template <typename>
struct callback;

/******************************************************
 * Generic callback function wrapper type with implicit
 * context pointer.
 *
 * @tparam FunctionReturnType Auto*deduced function return type
 * @tparam FunctionArgs Auto-deduced function arg types
 ******************************************************/
template <typename FunctionReturnType, typename... FunctionArgs>
struct callback<FunctionReturnType(FunctionArgs...)> {
    using callback_fn_t = FunctionReturnType (*)(void *, FunctionArgs...);
    using context_ptr_t = void *;

    /******************************************************
     * Construct a new callback object
     *
     * @param fn Callback function
     * @param in Implicit context pointer
     ******************************************************/
    callback(callback_fn_t fun, context_ptr_t in) : fun_ptr(fun), ctx_ptr(in) {}

    callback() = default;
    callback(const callback & other) = default;
    callback(callback && other) = default;
    callback & operator=(const callback & other) = default;
    callback & operator=(callback && other) = default;

    // Allow both lvalues and rvalues
    inline FunctionReturnType operator()(FunctionArgs... args) {
        assert(fun_ptr);
        return fun_ptr(ctx_ptr, std::forward<FunctionArgs>(args)...);
    }

    inline operator bool() {
        return fun_ptr;
    }

    void reset() {
        fun_ptr = nullptr;
        ctx_ptr = nullptr;
    }

    const auto & fn() const {
        return fun_ptr;
    }

private:
    callback_fn_t fun_ptr = { nullptr };
    context_ptr_t ctx_ptr = { nullptr };
};

// Deduction guide for CTAD
template <typename FunctionReturnType, typename... FunctionArgs>
callback(FunctionReturnType (*)(void *, FunctionArgs...),
         void *) -> callback<FunctionReturnType(FunctionArgs...)>;

} // namespace mad::nexus
