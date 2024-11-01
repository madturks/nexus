/******************************************************
 * Copyright (c) 2024 The Madturks Organization
 * SPDX-License-Identifier: GPL-3.0-or-later
 ******************************************************/

#include <mad/circular_buffer_base.hpp>

namespace mad {

circular_buffer_base::circular_buffer_base(const size_t total_size) :
    total_size_(total_size) {}

circular_buffer_base::circular_buffer_base(circular_buffer_base && mv) =
    default;
circular_buffer_base &
circular_buffer_base::operator=(circular_buffer_base && mv) = default;
} // namespace mad
