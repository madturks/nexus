/**
 * ______________________________________________________
 *
 * @file 		circular_buffer_p2.hpp
 * @project 	spectre/kol-framework/
 * @author 		mkg <hello@mkg.dev>
 * @date 		26.10.2019
 *
 * @brief
 *
 * @disclaimer
 * This file is part of SPECTRE MMORPG game server project.
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 *
 * @copyright		2012-2019 Mustafa K. GILOR, All rights reserved.
 *
 * ______________________________________________________
 */

#pragma once

// proj
#include <mad/circular_buffer_base.hpp>
#include <mad/concepts>
// xproj

// cppstd
#include <cassert>

namespace mad {

    /**
     * @brief An optimized version of regular circular buffer,
     * where size of circular buffer is power of two.
     *
     */
    struct circular_buffer_pow2 : public circular_buffer_base {

        struct auto_align {};

        /**
         * @brief Construct a new circular buffer p2 object
         * Usable size will be size-1.
         * @param    size
         */
        circular_buffer_pow2(std::size_t size);

        /**
         * @brief
         *
         * @param    size
         * @return
         */
        circular_buffer_pow2(std::size_t size, auto_align);

        /**
         * @brief Destroy the circular buffer p2 object
         *
         */
        ~circular_buffer_pow2();

        /**
         * @brief
         *
         * @param    buffer
         * @param    size
         * @return true
         * @return false
         */
        bool put(element_t * buffer, std::size_t size);

        inline void mark_as_read(const std::size_t amount) {
            tail_ = (tail_ + amount) & (total_size() - 1);
        }

        inline void mark_as_write(const std::size_t amount) {
            head_ = (head_ + amount) & (total_size() - 1);
        }

        /**
         * @brief
         *
         * @param    dst
         * @param    amount
         * @return true
         * @return false
         */
        bool peek(element_t * dst, std::size_t amount);

        /**
         * @brief Peek `Size` byte(s) from circular buffer into given std::array (`buffer`)
         *
         * @tparam Size         Size of the std::array (deduced)
         * @param    buffer     Mutable reference to std::array
         *
         * @return Boolean value indicating result of the operation.
         */
        template <std::size_t Size>
        inline auto peek(std::array<element_t, Size> & buffer) -> bool {
            return peek(buffer.data(), buffer.size());
        }

        /**
         * @brief
         *
         * @tparam T
         * @param    dst
         * @param    amount
         * @return true
         * @return false
         */
        template <has_assign T>
        auto peek_assign(T & dst, const std::size_t amount) const -> bool {
            // Check if we've got the requested amount of data
            if (consumed_space() < amount)
                return false;
            dst.assign(native_buffer_.get() + head_, native_buffer_.get() + head_ + amount);
            assert(consumed_space() >= amount);
            return true;
        }

        /**
         * @brief
         *
         * @tparam T
         * @param    dst
         * @return true
         * @return false
         */
        template <has_assign T>
        auto peek_assign(T & dst) const -> bool {
            if (consumed_space() < 1) {
                return false;
            }
            dst.assign(native_buffer_.get() + head_, native_buffer_.get() + head_ + consumed_space());
            return true;
        }

        /**
         * @brief clear
         * Mark all data available as read.
         */
        auto clear() -> void {
            mark_as_read(consumed_space());
        }

        /**
         * @brief
         *
         * @param    dst
         * @param    amount
         * @return true
         * @return false
         */
        bool get(element_t * dst, const std::size_t amount);

        std::size_t empty_space() const noexcept;
        std::size_t consumed_space() const noexcept;

        /**
         * @brief Put given std::array's contents into circular buffer.
         *
         * @tparam Size     std::array's size (deduced)
         * @param    buffer std::array instance
         *
         * @return Boolean value indicating result of the operation.
         */
        template <std::size_t Size>
        inline auto put(std::array<element_t, Size> buffer) -> bool {
            return put(buffer.data(), buffer.size());
        }

        /**
         * @brief Put given pod fixed array contents into circular buffer.
         *
         * @tparam Size     Size of the array (deduced)
         *
         * @return Boolean value indicating result of the operation.
         */
        template <std::size_t Size>
        inline auto put(element_t (&buffer) [Size]) -> bool {
            return put(&buffer [0], Size);
        }

        /**
         * @brief Put given multiple std::array's contents' into circular buffer.
         *
         * @tparam ArrayPack  (deduced)
         * @param    arrays   std::array's to be put
         *
         * @return Boolean value indicating result of the operation.
         */
        template <typename... ArrayPack>
        inline auto put_all(ArrayPack &&... arrays) -> bool {
            return (put(std::forward<ArrayPack>(arrays)) && ...);
        }

        /**
         * @brief
         *
         * @tparam T
         * @param    dst
         * @return true
         * @return false
         */
        template <typename T>
        inline auto get(T & dst) -> bool {
            return get(dst, consumed_space());
        }

        template <std::size_t Size>
        inline auto get(std::array<element_t, Size> & buffer) -> bool {
            return get(buffer.data(), buffer.size());
        }

        template <typename... Args>
        inline auto get_all(Args &&... args) -> bool {
            return (get(std::forward<Args>(args)) && ...);
        }

        template <std::size_t Size>
        inline auto get(element_t (&buffer) [Size]) -> bool {
            return get(&buffer [0], Size);
        }

        auto transfer(circular_buffer_pow2 & other) -> void {
            if (consumed_space() == 0 || other.empty_space() == 0) {
                return;
            }
            const size_t transfer_amount = (consumed_space() > other.empty_space() ? other.empty_space() : consumed_space());
            get(other.native_buffer_head(), transfer_amount);
            other.mark_as_write(transfer_amount);
        }
    };
} // namespace mad
