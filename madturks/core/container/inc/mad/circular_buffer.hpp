/**
 * ______________________________________________________
 *
 * @file 		circular_buffer_regular.hpp
 * @project 	spectre/kol-framework/
 * @author 		mkg <hello@mkg.dev>
 * @date 		17.10.2019
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

// cppstd
#include <array>
#include <cassert>
#include <cstring>

namespace mad {

class circular_buffer : public circular_buffer_base {
public:
    inline size_t empty_space() const noexcept {
        return total_size_ - bytes_avail_;
    }

    inline size_t consumed_space() const noexcept {
        return bytes_avail_;
    }

    circular_buffer(const size_t size, const bool allow_overwrite = false) :
        circular_buffer_base(size) {
        native_buffer_ = buffer_ptr_t{
            static_cast<element_t *>(malloc(total_size_)), std::free
        };
        overwrite_ = allow_overwrite;
        assert(native_buffer_ != nullptr);
    }

    inline bool put(element_t * buffer, const size_t size) {
        // Prevent overcommit
        if (empty_space() < size)
            return false;

        /*
         * I think I need to explain what's going on here.
         *
         * Assuming the put size: 6
         * circular buffer total size: 14
         * amount of previously committed data(avail_size): 5 (represented by X)
         * the data which will be committed now: (represented by *)
         *
         * Aliases:
         *
         * - head = The start position of used space
         * - tail = The end position of used space
         *
         * - region_tail = The empty space between tail and end.
         * - region_head = The empty space between begin and head.
         *
         *
         * We will put first 4 bytes of the data from tail to end (region_tail)
         *
         *        | reg_head  | used  | reg_tail|
         *        [ 0 0 0 0 0 X X X X X * * * * ]
         *  begin ^           ^       ^        ^ end
         *                    head    tail
         *
         *
         *
         * Then, we got 2 bytes leftover uncommitted, we will commit it to the
         * region [begin, head] [ * * 0 0 0 X X X X X * * * * ] begin ^ ^ ^ ^
         * end head    tail
         *
         */

        // How many bytes of the data we will write through tail to the end
        const size_t region_tail = min(size, tail_empty_size());
        // How many bytes of the data we will write through begin to head
        const size_t region_head = size - region_tail;

        // commit tail region data
        memcpy(native_buffer_.get() + tail_, buffer, region_tail);
        // commit head region data
        memcpy(native_buffer_.get(), buffer + region_tail, region_head);

        // Move the tail committed amount of bytes,
        // If tail overflows the buffer, wrap it around .
        tail_ = (tail_ + size) % total_size_;

        // Increase the amount of bytes ready to `get`
        bytes_avail_ += size;
        return true;
    }

    template <size_t Size>
    inline bool put(std::array<element_t, Size> buffer) {
        return put(buffer.data(), buffer.size());
    }

    template <size_t Size>
    inline bool put(element_t (&buffer) [Size]) {
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
     * @brief mark_as_read
     * Move head to the forward by `amount` bytes and decrease the available
     * bytes. WARNING: This does not perform `avail_size` checking. The callee
     * must ensure that amount bytes are actually exist in the buffer, otherwise
     * the buffer will be out-of-sync.
     *
     * @param amount    The amount of bytes that being marked as free
     */
    inline void mark_as_read(const size_t amount) {
        head_ = (head_ + amount) % total_size();
        bytes_avail_ -= amount;
    }

    inline void mark_as_write(const std::size_t amount) {
        head_ = (head_ + amount) & (total_size() - 1);
        bytes_avail_ += amount;
    }

    /**
     * @brief peek
     * Peek the `amount` byte(s) of data from circular buffer to the `dst`.
     * This is useful when we just want to 'peek' the data. This does not mark
     * the data being read as 'read'. `increase_read` is needed to be explicitly
     * called if marking is required afterwards. If the intention is not peeking
     * consider calling `get` instead.
     *
     * @param dst       The destination buffer
     * @param amount    The amount of data requested to be read
     * @return          True if the circular buffer has `amount` bytes ready to
     * be read. False otherwise.
     *
     */
    bool peek(element_t * dst, const size_t amount) {
        // Check if we've got the requested amount of data
        if (consumed_space() < amount)
            return false;

        // How many bytes we're going to read from head region
        const size_t region_head = min(amount, total_size() - head_);
        // How many bytes we're going to read from head region
        const size_t region_tail = amount - region_head;

        memcpy(dst, native_buffer_.get() + head_, region_head);

        memcpy(dst + region_head, native_buffer_.get(), region_tail);
        return true;
    }

    template <size_t Size>
    inline bool peek(std::array<element_t, Size> & buffer) {
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
        dst.assign(native_buffer_.get() + head_,
                   native_buffer_.get() + head_ + amount);
        assert(consumed_space() >= amount);
        return true;
    }

    /**
     * @brief get
     * Try to retrieve `amount` of byte(s) from circular buffer into `dst`. This
     * is actually implemented as "if `peek' then `increase_read`".
     *
     * @param dst       The destination buffer
     * @param amount    The amount of data requested to be read
     * @return          True if the circular buffer has `amount` bytes ready to
     * be read. False otherwise.
     *
     */
    bool get(element_t * dst, const size_t amount) {
        if (peek(dst, amount)) {
            mark_as_read(amount);
            return true;
        }
        return false;
    }

    template <size_t Size>
    inline bool get(std::array<element_t, Size> & buffer) {
        return get(buffer.data(), buffer.size());
    }

    template <typename... Args>
    inline bool get_all(Args &&... args) {
        return (get(std::forward<Args>(args)) && ...);
    }

    /**
     * @brief clear
     * Mark all data available as read.
     */
    auto clear() -> void {
        mark_as_read(consumed_space());
    }

    // TODO: Implement this if necessary.
    // auto transfer(circular_buffer & other) -> void {
    //     if (consumed_space() == 0 || other.empty_space() == 0) {
    //         return;
    //     }
    //     const size_t transfer_amount = (consumed_space() >
    //     other.empty_space() ? other.empty_space() : consumed_space());
    //     get(other.native_buffer_head(), transfer_amount);
    //     other.mark_as_write(transfer_amount);
    // }

private:
    size_t bytes_avail_ = { 0 };

    /**
     * @brief tail_available
     * Calculates the amount of bytes usable from current tail (committed data's
     * end) to the buffer's end.
     *
     * [ 0 0 0 0 0 X X X X X 0 0 0 0 ]
     *             ^       ^
     *             head    tail
     * In the scenario above, tail_available is 4.
     *
     * @return
     */
    inline size_t tail_empty_size() noexcept {
        return total_size_ - tail_;
    }
};
} // namespace mad
