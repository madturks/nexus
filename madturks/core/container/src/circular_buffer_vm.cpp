/**
 * ______________________________________________________
 *
 * @file 		circular_buffer_vm.cpp
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

#include <mad/circular_buffer_vm.hpp>

// system
#include <random>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/syscall.h>
// boost
// #include <boost/uuid/random_generator.hpp>
// #include <boost/uuid/uuid_io.hpp>
// cppstd
#include <cstring>
#include <iostream>
#include <type_traits>
#include <algorithm>

#include <uuid.h>
// cstd
#include <unistd.h>

namespace {
    int spcfw_memfd_create(const char * name, unsigned int flags) {
        return static_cast<int>(syscall(__NR_memfd_create, name, flags));
    }

    auto uuid_generator() {
        thread_local std::mt19937 generator = []() {
            std::random_device rd;
            auto seed_data = std::array<int, std::mt19937::state_size>{};
            std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
            std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
            return std::mt19937{seq};
        }();
        thread_local uuids::uuid_random_generator uuid_generator{generator};
        return uuid_generator;
    }
} // namespace

namespace mad {

    template <circular_buffer_backend BT>
    circular_buffer_vm<BT>::circular_buffer_vm(const size_t size) : circular_buffer_base(size) {
        /**
         * In order to perform our trick, we need to allocate multiples of
         * page size.
         */
        if (size % static_cast<size_t>(getpagesize()) != 0)
            throw std::bad_exception{};

        element_t * vaddr = nullptr;

        if constexpr (cb_has_shm_backend<BT>) {
            /**
             * Get an address for allocation.
             */
            vaddr = static_cast<element_t *>(mmap(NULL, 2 * size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0));

            if (vaddr == MAP_FAILED) {
                throw std::bad_exception{};
            }

            auto shm_id = shmget(IPC_PRIVATE, size, IPC_CREAT | 0700);
            munmap(vaddr, 2 * size);

            if (shm_id < 0)
                throw std::bad_exception{};

            if (vaddr != shmat(shm_id, vaddr, 0)) {
                shmctl(shm_id, IPC_RMID, nullptr);
                throw std::bad_exception{};
            }

            if (vaddr + size != shmat(shm_id, vaddr + size, 0)) {
                shmdt(vaddr);
                shmctl(shm_id, IPC_RMID, nullptr);
                throw std::bad_exception{};
            }

            if (shmctl(shm_id, IPC_RMID, nullptr) < 0) {
                shmdt(vaddr);
                shmdt(vaddr + size);
                throw std::bad_exception{};
            }
        } else if constexpr (cb_has_mmap_backend<BT>) {

            /**
             * @brief uuid
             * Anonymous files require a name, and we might allocate a dozen of them.
             * Therefore the name should be unique in order to prevent collisions.
             */
            // auto uuid     = boost::uuids::random_generator()();
            // auto uuid_str = boost::uuids::to_string(uuid);

            const uuids::uuid uuid = uuid_generator()();
            auto uuid_str          = uuids::to_string(uuid);

            /**
             * Create the anonymous file
             */
            if ((anonymous_fd_ = spcfw_memfd_create(uuid_str.c_str(), 0)) < 0)
                throw std::bad_exception{};

            /*
             * Truncate the anonymous file we've acquired
             * */
            if (ftruncate(anonymous_fd_, static_cast<long>(size)) < 0)
                throw std::bad_exception{};

            /**
             * Get an address for allocation.
             */
            vaddr = static_cast<element_t *>(mmap(NULL, 2 * size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));

            if (vaddr == MAP_FAILED)
                throw std::bad_exception{};

            /*
             * Map the first page of the memory to our anonymous file
             * */
            if (mmap(vaddr, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, anonymous_fd_, 0) == MAP_FAILED)
                throw std::bad_exception{};
            /*
             * Map the second page of the memory to our anonymous file
             * */
            if (mmap(vaddr + size, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, anonymous_fd_, 0) == MAP_FAILED)
                throw std::bad_exception{};
        }

        /*
         * Our sinister buffer is ready to serve.
         * */
        native_buffer_ = buffer_ptr_t{vaddr, noop_free};
        assert(native_buffer_ != nullptr);
    }

    template <circular_buffer_backend BT>
    circular_buffer_vm<BT>::circular_buffer_vm(const size_t size, auto_align_to_page) :
        circular_buffer_vm(size + (static_cast<size_t>(getpagesize()) - (size % static_cast<size_t>(getpagesize())))) {}

    template <circular_buffer_backend BT>
    circular_buffer_vm<BT>::circular_buffer_vm(circular_buffer_vm &&) = default;

    template <circular_buffer_backend BT>
    circular_buffer_vm<BT> & circular_buffer_vm<BT>::operator=(circular_buffer_vm &&) = default;

    template <circular_buffer_backend BT>
    circular_buffer_vm<BT>::~circular_buffer_vm() {
        if (native_buffer_) {
            auto map_buf = native_buffer_.get();
            if constexpr (cb_has_mmap_backend<BT>) {
                munmap(map_buf + total_size(), total_size());
                munmap(map_buf, total_size());
                // this might work ?
                if (anonymous_fd_ > -1) {
                    close(anonymous_fd_);
                }
            } else if constexpr (cb_has_shm_backend<BT>) {
                shmdt(map_buf);
                shmdt(map_buf + total_size());
            }
        }
    }

    template <circular_buffer_backend BT>
    auto circular_buffer_vm<BT>::put(const element_t * buffer, const size_t size) -> bool {
        // Prevent overcommit
        if (empty_space() < size) [[unlikely]]
            return false;

        // In this version, we don't need to worry about adjusting head region and tail region
        // as we mapped the adjacent region of memory to same physical memory.
        // Therefore, memcpy call will automatically wrap around the buffer.

        memcpy(&native_buffer_.get() [tail_], buffer, size);

        // Mark the bytes as written
        mark_as_write(size);

        assert(empty_space() < total_size());
        assert(consumed_space() <= total_size());

        return true;
    }

    template <circular_buffer_backend BT>
    auto circular_buffer_vm<BT>::clear() -> void {
        if (consumed_space()) {
            mark_as_read(consumed_space());
        }
    }

    template <circular_buffer_backend BT>
    auto circular_buffer_vm<BT>::mark_as_read(const size_t amount) -> void {
        assert(tail_ > head_);

        head_ += amount;
        if (head_ >= total_size()) {
            head_ -= total_size();
            assert(tail_ - total_size() <= (total_size() * 2));
            tail_ -= total_size();

            assert(head_ < (total_size() * 2));
            assert(tail_ < (total_size() * 2));
        }

        assert(head_ < total_size() * 2);
    }

    template <circular_buffer_backend BT>
    auto circular_buffer_vm<BT>::mark_as_write(const size_t amount) -> void {

        // Move the tail committed amount of bytes,
        // We don't need to wrap it this time.

        tail_ += amount;

        assert(tail_ < (total_size() * 2));
        assert(tail_ != head_);
    } 

    template <circular_buffer_backend BT>
    auto circular_buffer_vm<BT>::peek(element_t * dst, const size_t amount) const -> bool {
        // Check if we've got the requested amount of data
        if (consumed_space() < amount) [[unlikely]]
            return false;
        memcpy(dst, native_buffer_.get() + head_, amount);
        assert(dst);
        assert(consumed_space() >= amount);
        return true;
    }

    template <circular_buffer_backend BT>
    auto circular_buffer_vm<BT>::transfer(circular_buffer_vm & other) -> void {
        if (consumed_space() == 0 || other.empty_space() == 0) {
            return;
        }
        const size_t transfer_amount = (consumed_space() > other.empty_space() ? other.empty_space() : consumed_space());
        get(other.native_buffer_head(), transfer_amount);
        other.mark_as_write(transfer_amount);
    }

    template <circular_buffer_backend BT>
    auto circular_buffer_vm<BT>::get(element_t * dst, const size_t amount) -> bool {
        if (peek(dst, amount)) {
            mark_as_read(amount);
            return true;
        }
        return false;
    }
} // namespace mad

// The thing with explicit template instantiation is, it must be in same translation unit with
// template class definition. For me, it's absolute bullshit.
template struct mad::circular_buffer_vm<mad::vm_cb_backend_mmap>;
template struct mad::circular_buffer_vm<mad::vm_cb_backend_shm>;
