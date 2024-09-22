/**
 * ______________________________________________________
 *
 * @file 		circular_buffer_vm.hpp
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
#include <mad/has_assign.hpp>
// cppstd
#include <cassert>
#include <span>

// take a look at :https://github.com/lava/linear_ringbuffer/blob/master/include/bev/linear_ringbuffer.hpp
// also: https://github.com/google/wuffs/blob/main/script/mmap-ring-buffer.c

namespace mad {

    struct vm_cb_backend_base {};

    /**
     * @brief Use mmap to map memory pages.
     */
    struct vm_cb_backend_mmap : public vm_cb_backend_base {};

    /**
     * @brief Use shared memory to map pages.
     */
    struct vm_cb_backend_shm : public vm_cb_backend_base {};

    /**
     * @brief Standard memory
     */
    struct cb_backend_standard : public vm_cb_backend_base {};

    template <typename T>
    concept cb_has_mmap_backend = std::is_same<vm_cb_backend_mmap, T>::value;

    template <typename T>
    concept cb_has_shm_backend = std::is_same<vm_cb_backend_shm, T>::value;

    template <typename T>
    concept circular_buffer_backend = std::is_base_of<vm_cb_backend_base, T>::value;

    /**
     * @brief
     * There's some memory trickery going on here, and I'll try to explain.
     * In normal scenario, we allocate a linear, contiguous chunk of memory for our circular buffer
     * and add some code to use it as if it was a circular buffer. To do this, we calculate the amount of
     * data free from front of the buffer and the end of the buffer seperately, then split the data we're going
     * to write between these two memory areas(I call it as region). This costs around two memcpy calls in worst
     * scenario.
     *
     * But what if the buffer we've allocated was indeed a non-linear, circular buffer?
     * The trick you're going to see below, exactly simulates this by using a memory mapping trick.
     *
     * I'm not going to dive in much details of how operating system's memory management works, but it manages
     * the physical memory by dividing it to pages. This pages are held in physical page table. When a process request
     * some memory from the operating system, some pages are dedicated to the process.
     *
     * After the starting of multitasking era, memory fragmentation become a real issue, because every single process
     * was using the same page table. This caused gaps between free pages, causing some allocations fail even if we had
     * enough pages to provide the requested memory.
     *
     * Thus, the UNIX developers introduced `virtual page`, assigned to each process independently. In its' core, this allowed
     * out-of-order physical page mapping on the virtual page.
     *
     * Assume that we've got the following physical page table, and two virtual page tables for each;
     *
     * PHYSICAL PAGE TABLE      PROCESS 1 VIRTUAL PT        PROCESS 2 VIRTUAL PT
     * |PP1  -- free |          |VP1P1  -- free |           |VP2P1    -- free |
     * |PP2  -- free |          |VP1P2  -- free |           |VP2P2    -- free |
     * |PP3  -- free |          |VP1P3  -- free |           |VP2P3    -- free |
     * |PP4  -- free |          |VP1P4  -- free |           |VP2P4    -- free |
     * |PP5  -- free |          |VP1P5  -- free |           |VP2P5    -- free |
     * |PP6  -- free |          |VP1P6  -- free |           |VP2P6    -- free |
     *
     * Process 1 now request 4 contiguous pages from the operating system, and the operating system happily give it.
     *
     * PHYSICAL PAGE TABLE      PROCESS 1 VIRTUAL PT        PROCESS 2 VIRTUAL PT
     * |PP1  -- VP1P1 |          |VP1P1  -- PP1  |           |VP2P1    -- free |
     * |PP2  -- VP1P2 |          |VP1P2  -- PP2  |           |VP2P2    -- free |
     * |PP3  -- VP1P3 |          |VP1P3  -- PP3  |           |VP2P3    -- free |
     * |PP4  -- VP1P4 |          |VP1P4  -- PP4  |           |VP2P4    -- free |
     * |PP5  -- free |           |VP1P5  -- free |           |VP2P5    -- free |
     * |PP6  -- free |           |VP1P6  -- free |           |VP2P6    -- free |
     *
     * Process 1 then decides to free first 2 pages.
     *
     * PHYSICAL PAGE TABLE      PROCESS 1 VIRTUAL PT        PROCESS 2 VIRTUAL PT
     * |PP1  -- free  |          |VP1P1  -- free  |          |VP2P1    -- free |
     * |PP2  -- free  |          |VP1P2  -- free  |          |VP2P2    -- free |
     * |PP3  -- VP1P3 |          |VP1P3  -- PP3  |           |VP2P3    -- free |
     * |PP4  -- VP1P4 |          |VP1P4  -- PP4  |           |VP2P4    -- free |
     * |PP5  -- free  |          |VP1P5  -- free |           |VP2P5    -- free |
     * |PP6  -- free  |          |VP1P6  -- free |           |VP2P6    -- free |
     *
     * Now, process 2 requests 4 contiguous pages from the operating system.
     *
     * PHYSICAL PAGE TABLE      PROCESS 1 VIRTUAL PT        PROCESS 2 VIRTUAL PT
     * |PP2  -- VP2P2 |          |VP1P2  -- free  |          |VP2P2    -- PP1 |
     * |PP1  -- VP2P1 |          |VP1P1  -- free  |          |VP2P1    -- PP2 |
     * |PP3  -- VP1P3 |          |VP1P3  -- PP3  |           |VP2P3    -- PP5 |
     * |PP4  -- VP1P4 |          |VP1P4  -- PP4  |           |VP2P4    -- PP6 |
     * |PP5  -- VP2P3 |          |VP1P5  -- free |           |VP2P5    -- free |
     * |PP6  -- VP2P4 |          |VP1P6  -- free |           |VP2P6    -- free |
     *
     * As for point of view of process 2, the pages are contiguous, but in reality, they're seperated by
     * half in physical memory. If we tried the scenario above without the virtual page table, the operating
     * system will return nothing for process 2's request, because it would lack thre required amount of adjacent pages.
     *
     * (**I said briefly, but explained throughly..)
     *
     * The following hack enables us to exploit the virtual table of our process. If we go back the first circular buffer scenario,
     * we had to wrap around the buffer and calculate the amount of free space from beginning to head, from tail to end and put our data
     * there. This costs two memcpy calls. In the code below, we allocate an anonymous file in the memory, and map two of our adjacent
     * page blocks to it. Thus, when we overflow our index, we actually wrap around the underlying anonymous file. Therefore, we can write
     * and read data from our buffer with a single memcpy call.
     *
     * Pretty neat trick.
     *
     *
     * @tparam cb_backend_shm Type of the backend to use perform sequential virtual page mapping
     */
    template <circular_buffer_backend BE = vm_cb_backend_shm>
    struct circular_buffer_vm : public circular_buffer_base {

        /**
         * @brief The auto_align_to_page struct
         * Enables automatic page size alignment for circular buffer fast constructor.
         * Assuming the page size is 1024 bytes and requested amount is eg. 1017 bytes,
         * the constructor will round it up to nearest multiple of 1024, which is 1024.
         * This is neccessary to this circular buffer optimization to work.
         */
        struct auto_align_to_page {};

        /**
         * @brief circular_buffer_fast
         * Construct a `size` amount circular buffer.
         * The requested size must be multiple of system's page size.
         * @param size
         */
        circular_buffer_vm(const std::size_t size);

        /**
         * @brief circular_buffer_fast
         * Auto-round up size to next multiple of page size.
         * @param size
         */
        circular_buffer_vm(const std::size_t size, auto_align_to_page);

        /**
         * @brief Move constructor for circular_buffer_fast
         *
         * @param mv    The circular_buffer_fast, which is being moved into this
         */
        circular_buffer_vm(circular_buffer_vm && mv);

        /**
         * @brief
         *
         * @param    mv
         * @return
         */
        circular_buffer_vm & operator=(circular_buffer_vm && mv);

        /**
         * @brief Destroy the circular buffer shm object
         *
         */
        ~circular_buffer_vm();

        /**
         * @brief get
         * Try to retrieve `amount` of byte(s) from circular buffer into `dst`. This is actually implemented
         * as "if `peek' then `increase_read`".
         *
         * @param dst       The destination buffer
         * @param amount    The amount of data requested to be read
         * @return          True if the circular buffer has `amount` bytes ready to be read. False otherwise.
         *
         */
        auto get(element_t * dst, const std::size_t amount) -> bool;

        template <class T>
        bool get(T & dst, const std::size_t amount) {
            if (peek(dst, amount)) {
                mark_as_read(amount);
                return true;
            }
            return false;
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

        /**
         * @brief Put `size` bytes to circular buffer from `buffer`.
         *
         * @param    buffer     Pointer to buffer which contains the content to be put
         * @param    size       Amount of bytes to be put starting from buffer[0]
         *
         * @return Boolean value indicating result of the operation.
         */
        auto put(const element_t * buffer, const size_t size) -> bool;

        /**
         * @brief Put `size` bytes to circular buffer from `buffer`.
         *
         * @param  buf     Span, which contains the content to be put
         *
         * @return Boolean value indicating result of the operation.
         */
        auto put(std::span<const element_t> buf) -> bool{
            return put(buf.data(), buf.size_bytes());
        }

        /**
         * @brief Put given std::array's contents into circular buffer.
         *
         * @tparam Size     std::array's size (deduced)
         * @param    buffer std::array instance
         *
         * @return Boolean value indicating result of the operation.
         */
        template <size_t Size>
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
        template <size_t Size>
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
         * @brief peek
         * Peek the `amount` byte(s) of data from circular buffer to the `dst`.
         * This is useful when we just want to 'peek' the data. This does not mark
         * the data being read as 'read'. `increase_read` is needed to be explicitly
         * called if marking is required afterwards. If the intention is not peeking
         * consider calling `get` instead.
         *
         * @param dst       The destination buffer
         * @param amount    The amount of data requested to be read
         * @return          True if the circular buffer has `amount` bytes ready to be read. False otherwise.
         *
         */
        auto peek(element_t * dst, const size_t amount) const -> bool;

        /**
         * @brief Peek `Size` byte(s) from circular buffer into given std::array (`buffer`)
         *
         * @tparam Size         Size of the std::array (deduced)
         * @param    buffer     Mutable reference to std::array
         *
         * @return Boolean value indicating result of the operation.
         */
        template <size_t Size>
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
        auto peek_assign(T & dst, const size_t amount) const -> bool {
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
         * @brief transfer
         * Transfer the available amount of data to `other` buffer.
         * If `other` buffer does not have enough space to hold the whole data,
         * then only the amount that can fit will be transferred to the other buffer,
         * rest will still be in this buffer.
         * @param other     Buffer to transfer the available data
         */
        auto transfer(circular_buffer_vm & other) -> void;

        /**
         * @brief mark_as_read
         * Move head to the forward by `amount` bytes and decrease the available bytes.
         * WARNING: This does not perform `avail_size` checking. The callee must ensure that
         * amount bytes are actually exist in the buffer, otherwise the buffer will be out-of-sync.
         *
         * @param amount    The amount of bytes that being marked as free
         */
        auto mark_as_read(const size_t amount) -> void;

        /**
         * @brief
         *
         * @param    amount
         */
        auto mark_as_write(const size_t amount) -> void;

        /**
         * @brief clear
         * Mark all data available as read.
         */
        auto clear() -> void;

        inline size_t empty_space() const noexcept {
            return total_size_ - consumed_space();
        }

        inline size_t consumed_space() const noexcept {
            return tail_ - head_;
        }

    private:
        /**
         * @brief noop_free
         * Dummy free function does nothing.
         */
        static void noop_free(void *) noexcept {}

        /**
         * File descriptor to anonymous file allocated for
         * the circular buffer.
         */
        int anonymous_fd_ = {-1};
    };

    extern template struct circular_buffer_vm<vm_cb_backend_mmap>;
    extern template struct circular_buffer_vm<vm_cb_backend_shm>;

} // namespace mad
