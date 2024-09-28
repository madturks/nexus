/**
 * ______________________________________________________
 *
 * @file 		concurrent_resource.hpp
 * @project 	spectre/framework/
 * @author 		mkg <hello@mkg.dev>
 * @date 		15.09.2019
 *
 * @brief
 *
 * @disclaimer
 * This file is part of SPECTRE MMORPG game server project.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 *
 * @copyright	2012-2019 Mustafa K. GILOR, All rights reserved.
 *
 * ______________________________________________________
 */
#pragma once

#if !defined(SPECTRE_CONCURRENT_RESOURCE_USE_BOOST_SHARED_MUTEX) && (__cplusplus >= 201703L)
#include <shared_mutex> // prerequisite : C++17
#include <mutex>

namespace mad {
    using shared_mutex_t = std::shared_mutex;
    using read_lock      = std::shared_lock<shared_mutex_t>;
    using write_lock     = std::unique_lock<shared_mutex_t>;
} // namespace mad

#else
#include <boost/thread/shared_mutex.hpp>

namespace mad {
    using shared_mutex_t        = boost::shared_mutex;
    using read_lock             = boost::shared_lock<shared_mutex_t>;
    using write_lock            = boost::unique_lock<shared_mutex_t>;
    // boost's rwlock implementation has two additional lock kinds, which can be useful
    // in several scenarios.
    using upgrade_lock          = boost::upgrade_lock<shared_mutex_t>;
    using upgrade_to_write_lock = boost::upgrade_to_unique_lock<shared_mutex_t>;
} // namespace mad

#define SPECTRE_LOCKABLE_USING_BOOST
#endif

// xproj
#include <mad/concepts>

namespace mad {

    /**
     * @brief Library implementation details.
     *
     * @warning The code/class/api in this namespace might change drastically over the time.
     *          (not reliable)
     */
    namespace detail {

        /**
         * @brief This is a simple workaround for the member access operator overload behaviour.
         * By default, member access operator requires return value to be `dereferenceable`, meaning
         * that, the return type must be a pointer to an object. As we intend to return
         * references from our accessors, this behaviour prevents us to do so.
         *
         * As a simple solution, we wrap our real type in our decorator class, exposing type pointer to
         * only wrapper (in our scenario, wrapper would be accessor class). The wrapper itself returns
         * this decorator by value, resulting the desired behaviour.
         *
         * @tparam TypeToDecorate  Type to add member access operator to.
         */
        template <typename TypeToDecorate>
        struct member_access_operator_decorator {
            constexpr member_access_operator_decorator(TypeToDecorate val) : value(val) {}

            constexpr decltype(auto) operator->() const noexcept {
                return &value;
            }

        private:
            TypeToDecorate value;
        };

        /**
         * @brief Wraps a resource with lock(read,write,upgrade), leaving only desired
         * access level to the resource. The lock is acquired on construction, released on
         * destruction of `accessor' type.
         *
         * To access the resource, just treat `accessor` object as a pointer to
         * the resource. The class has similar semantics with std|boost::optional.
         * @tparam LockType     Lock type indicating the desired access (read|write|upgrade)
         * @tparam ResourceType Type of the resource to wrap with desired access
         */
        template <typename LockType, typename ResourceType>
        struct accessor {
            using base_t = accessor<LockType, ResourceType>;

            /**
             * @brief Construct a new  accessor object
             *
             * @param resource  Resource to grant desired access to
             * @param mtx       Resource's read-write lock
             */
            constexpr accessor(ResourceType res, shared_mutex_t & mtx) : lock(mtx), resource(res) {}

            constexpr member_access_operator_decorator<ResourceType> operator->() noexcept {
                return member_access_operator_decorator<ResourceType>(resource);
            }

            decltype(auto) begin()
            requires mad::has_begin_end<ResourceType>
            {
                return const_cast<std::decay_t<ResourceType> &>(resource).begin();
            }

            decltype(auto) end()
            requires mad::has_begin_end<ResourceType>
            {
                return const_cast<std::decay_t<ResourceType> &>(resource).end();
            }

            decltype(auto) cbegin()
            requires mad::has_const_begin_end<ResourceType>
            {
                return resource.cbegin();
            }

            decltype(auto) cend()
            requires mad::has_const_begin_end<ResourceType>
            {
                return resource.cend();
            }

            operator std::add_const_t<ResourceType>() {
                return resource;
            }

        private:
            LockType lock;
            ResourceType resource;
        };

        /**
         * @brief Non-thread safe accessor to the resource.
         *
         * @tparam ResourceType Resource to provide non-thread safe access.
         */
        template <typename ResourceType>
        struct unsafe_accessor {
            constexpr unsafe_accessor(ResourceType res) : resource(res) {}

            constexpr member_access_operator_decorator<ResourceType> operator->() noexcept {
                return member_access_operator_decorator<ResourceType>(resource);
            }

        private:
            ResourceType resource;
        };

        struct const_wrapper {};

        template <typename ResourceType>
        using shared_accessor = accessor<read_lock, const ResourceType &>;

        template <typename ResourceType>
        using mutable_shared_accessor = accessor<read_lock, ResourceType &>;

        template <typename ResourceType>
        using exclusive_accessor = accessor<write_lock, ResourceType &>;

    } // namespace detail

    /**
     * @brief A concept, which adds a read-write lock
     * to inheriting class.
     */
    template <typename SharedMutexType = shared_mutex_t>
    struct lockable {
    protected:
        mutable SharedMutexType rwlock;
    };

    /**
     * @brief This class wraps a resource, and exposes accessors only as an interface.
     * The only way to access the underlying resource is, through the accessors.
     *
     * @tparam ResourceType Resource type to wrap
     */
    template <typename ResourceType>
    struct concurrent : lockable<> {
    public:
        using shared_accessor_t         = detail::shared_accessor<ResourceType>;
        using mutable_shared_accessor_t = detail::mutable_shared_accessor<ResourceType>;
        using exclusive_accessor_t      = detail::exclusive_accessor<ResourceType>;

        /**
         * @brief Grab shared_lock and grant access to the resource.
         *
         * Note that this is not
         */
        shared_accessor_t shared_access() const noexcept {
            return {resource, rwlock};
        }

        /**
         * @brief Grab shared_lock and grant access to the resource.
         *
         * Note that this is not
         */
        mutable_shared_accessor_t mutable_shared_access() noexcept {
            return {resource, rwlock};
        }

        /**
         * @brief Grab an exclusive lock. All subsequent read-write access requests
         * will wait until the grabbed write access object is released.
         */
        exclusive_accessor_t exclusive_access() noexcept {
            return {resource, rwlock};
        }

    private:
        /**
         * @brief The underlying resource.
         */
        ResourceType resource;
    };

} // namespace mad
