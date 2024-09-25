#pragma once

#include <algorithm>

namespace mad::meta {

    /**
     * @brief A constant-evaluated type which can be used as
     * non-type template parameter.
     */
    struct source_location {

        /**
         * @brief Meta type constructor
         *
         * @param [in] file_name Source location file name
         * @param [in] file_name_size Size of source location file name string
         * @param [in] function_name Source location function name
         * @param [in] function_name_size Size of source location function name string
         * @param [in] cline Source line index on file
         */
        inline consteval source_location(const char * file_name = __builtin_FILE(), int file_name_size = __builtin_strlen(__builtin_FILE()),
                                         const char * function_name = __builtin_FUNCTION(),
                                         int function_name_size = __builtin_strlen(__builtin_FUNCTION()), int cline = __builtin_LINE()) {
            std::copy_n(file_name, file_name_size, file);
            std::copy_n(function_name, function_name_size, function);
            line = cline;
        }

        char file [4096]     = {};
        char function [8192] = {};
        int line             = {};
    };
} // namespace mad
