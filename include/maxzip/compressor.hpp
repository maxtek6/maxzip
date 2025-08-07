/*
 * Copyright (c) 2025 Maxtek Consulting
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef MAXZIP_COMPRESSOR_HPP
#define MAXZIP_COMPRESSOR_HPP

#include "common.hpp"

namespace maxzip
{
    /**
     * @class compressor
     * @brief Abstract base class for block compression.
     */
    class compressor
    {
    public:
        /**
         * @brief Compress a block of data.
         * @param input Pointer to the input data.
         * @param input_size Size of the input data in bytes.
         * @param output Pointer to the output buffer (can be nullptr).
         * @param output_size Reference to the size of the output buffer on input. If
         * output is nullptr, this will be set to the maximum compressed size.
         * @return The size of the compressed data in bytes, or 0 if output is nullptr.
         */
        virtual size_t compress(
            const uint8_t *input,
            size_t input_size,
            uint8_t *output,
            size_t &output_size) = 0;
    };

    struct brotli_compressor_params
    {
        std::optional<int> quality;
        std::optional<int> window_size;
        std::optional<int> mode;
    };

    struct zlib_compressor_params
    {
        std::optional<int> level;
        std::optional<int> window_bits;
        std::optional<int> mem_level;
        std::optional<int> strategy;
    };

    struct zstd_compressor_params
    {
        std::optional<int> level;
        std::optional<int> window_log;
        std::optional<int> hash_log;
        std::optional<int> chain_log;
        std::optional<int> search_log;
        std::optional<int> min_match;
        std::optional<int> target_length;
        std::optional<int> strategy;
        std::optional<bool> enable_long_distance_matching;
        std::optional<bool> enable_content_size;
        std::optional<bool> enable_checksum;
        std::optional<bool> enable_dict_id;
    };

    compressor *create_brotli_compressor(const brotli_compressor_params &params = {});
    compressor *create_zlib_compressor(const zlib_compressor_params &params = {});
    compressor *create_zstd_compressor(const zstd_compressor_params &params = {});
}

#endif