
/* Return count in buffer.  */
#define CIRC_CNT(head, tail, size) (((head) - (tail)) & ((size) - 1))

/* Return space available, 0..size-1.  We always leave one free char
   as a completely full buffer has head == tail, which is the same as
   empty.  */
#define CIRC_SPACE(head, tail, size) CIRC_CNT((tail), ((head) + 1), (size))

#define CIRC_EFFECTIVE_SIZE (total_size() - 1)

// proto
#include <mad/circular_buffer_pow2.hpp>
// cppstd
#include <cstring>

namespace mad {

circular_buffer_pow2::circular_buffer_pow2(size_t size) :
    circular_buffer_base(size) {
    native_buffer_ = buffer_ptr_t{
        static_cast<element_t *>(malloc(total_size_)), std::free
    };
}

circular_buffer_pow2::~circular_buffer_pow2() = default;

bool circular_buffer_pow2::put(element_t * buffer, size_t size) {
    // /* Return space available up to the end of the buffer.  */
    auto circ_space_to_end = [](auto head, auto tail, auto sz) {
        auto end = (sz) -1 - (head);
        auto n = (end + (tail)) & ((sz) -1);
        return n <= end ? n : end + 1;
    };

    auto es = empty_space();
    if (es >= size) {

        while (size) {
            auto wb = &native_buffer_.get() [head_];
            auto ste = circ_space_to_end(head_, tail_, total_size());
            auto n = min(size, ste);

            memcpy(wb, buffer, n);

            head_ = (head_ + n) & CIRC_EFFECTIVE_SIZE;
            size -= n;
            buffer += n;
        }

        return true;
    }
    return false;
}

bool circular_buffer_pow2::peek(element_t * dst, size_t amount) {
    /* Return count up to the end of the buffer.  Carefully avoid
    accessing head and tail more than once, so they can change
    underneath us without returning inconsistent results.  */

    auto circ_cnt_to_end = [](auto head, auto tail, auto size) {
        auto end = (size) - (tail);
        auto n = ((head) + end) & ((size) -1);
        return n < end ? n : end;
    };

    if (consumed_space() >= amount) {
        auto tmp_tail_ = tail_;
        while (amount) {
            auto rb = &native_buffer_.get() [tmp_tail_];
            auto ste = circ_cnt_to_end(head_, tmp_tail_, total_size());
            auto n = min(amount, ste);

            memcpy(dst, rb, n);
            tmp_tail_ = (tmp_tail_ + n) & CIRC_EFFECTIVE_SIZE;

            amount -= n;
            dst += n;
        }

        return true;
    }
    return false;
}

bool circular_buffer_pow2::get(element_t * dst, const size_t amount) {
    if (peek(dst, amount)) {
        mark_as_read(amount);
        return true;
    }
    return false;
}

size_t circular_buffer_pow2::empty_space() const noexcept {
    return CIRC_SPACE(head_, tail_, total_size());
}

size_t circular_buffer_pow2::consumed_space() const noexcept {
    return CIRC_CNT(head_, tail_, total_size());
}
} // namespace mad
