/**
 * ______________________________________________________
 *
 * @file 		circular_buffer.base.cpp
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

#include <mad/circular_buffer_base.hpp>

namespace mad {

circular_buffer_base::circular_buffer_base(const size_t total_size) :
    total_size_(total_size) {}

circular_buffer_base::circular_buffer_base(circular_buffer_base && mv) =
    default;
circular_buffer_base &
circular_buffer_base::operator=(circular_buffer_base && mv) = default;
} // namespace mad
