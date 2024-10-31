/******************************************************
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/

#pragma once

#include <mad/macro>
#include <mad/meta/source_location.hpp>

#include <gmock/gmock.h>

#include <concepts>
#include <print>
#include <source_location>
#include <utility>

namespace mad {

/******************************************************
 * Static mock object.
 *
 * Each initialization of the class produces an unique
 * type. This class allows you to mock function pointers.
 * To do so, each static_mock instance has a static pod_fn
 * function pointer, and a mock object. Each initialization
 * gets own static variables due to the "UniqueTag" is different
 * for each template instance.
 *
 * There's one limitation, though -- which is when the template instances
 * are initiated from the exact same line, they'll get the exactly the same
 * lambda type. So, no same line initializations.
 *
 * The class has user-defined conversion operators for both
 * mock type and the function pointer type. Also, in order to
 * be able to use the object directly in gmock macro calls like
 * ON_CALL, EXPECT_CALL, operator* returns the mock function.
 ******************************************************/
template <typename FnSig, typename Unique = decltype([] {
                          })>
struct static_mock;

template <typename ReturnType, typename... Args, typename Unique>
struct static_mock<ReturnType (*)(Args...), Unique> {
    static_mock() {
        // std::print("constructor called {} {}!\n",
        // std::source_location::current().function_name(), Unique);
        MAD_ASSERT(nullptr == mock);
        mock = new ::testing::MockFunction<ReturnType(Args...)>;
    }

    static_mock(static_mock &&) = delete;
    static_mock(const static_mock &) = delete;

    ~static_mock() {
        MAD_ASSERT(nullptr != mock);
        delete mock;
        mock = nullptr;
        MAD_ASSERT(nullptr == mock);
    }

    inline operator decltype(auto)() {
        return pod_fn;
    }

    inline operator ::testing::MockFunction<ReturnType(Args...)> &() {
        MAD_ASSERT(mock);
        return *mock;
    }

    inline decltype(auto) operator*() {
        MAD_ASSERT(mock);
        return *mock;
    }

    decltype(auto) fn() {
        return pod_fn;
    }

private:
    // This is static just because the static function needs to be able to
    // access to the mock object. Every class instance is an unique type
    // so this is acting like a class member variable.
    static inline ::testing::MockFunction<ReturnType(Args...)> * mock{
        nullptr
    };

    /******************************************************
     * An unique POD function entry point for the mock object.
     ******************************************************/
    static inline ReturnType (*pod_fn)(Args...) = +[](Args... args) {
        MAD_ASSERT(mock);
        return mock->Call(std::forward<Args>(args)...);
    };
};

} // namespace mad
