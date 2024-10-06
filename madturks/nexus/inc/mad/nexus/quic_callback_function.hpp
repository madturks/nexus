#pragma once

#include <cassert>
#include <utility>

namespace mad::nexus {

template <typename>
struct quic_callback_function;

template <typename FunctionReturnType, typename... FunctionArgs>
struct quic_callback_function<FunctionReturnType(FunctionArgs...)> {
    using callback_fn_t = FunctionReturnType (*)(void *, FunctionArgs...);
    using context_ptr_t = void *;

    quic_callback_function(callback_fn_t fn, context_ptr_t in) :
        fun_ptr(fn), ctx_ptr(in) {}

    quic_callback_function() = default;
    quic_callback_function(const quic_callback_function & other) = default;
    quic_callback_function(quic_callback_function && other) = default;
    quic_callback_function &
    operator=(const quic_callback_function & other) = default;
    quic_callback_function &
    operator=(quic_callback_function && other) = default;

    // Allow both lvalues and rvalues
    inline FunctionReturnType operator()(FunctionArgs... args) {
        assert(fun_ptr);
        return fun_ptr(ctx_ptr, std::forward<FunctionArgs>(args)...);
    }

    callback_fn_t fun_ptr = { nullptr };
    context_ptr_t ctx_ptr = { nullptr };

    inline operator bool() {
        return fun_ptr;
    }

    void reset() {
        fun_ptr = nullptr;
        ctx_ptr = nullptr;
    }
};

// Deduction guide for CTAD
template <typename FunctionReturnType, typename... FunctionArgs>
quic_callback_function(FunctionReturnType (*)(void *, FunctionArgs...), void *)
    -> quic_callback_function<FunctionReturnType(FunctionArgs...)>;

} // namespace mad::nexus
