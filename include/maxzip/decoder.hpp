
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

#ifndef MAXZIP_DECODER_HPP
#define MAXZIP_DECODER_HPP

#include "common.hpp"

namespace maxzip
{
    /**
     * @class decoder
     * @brief Abstract base class for stream decompression
     */
    class decoder
    {
    public:
        /**
         * @brief Initialize the decoder.
         * @param flush if set, may flush incomplete frames. This is typically ignored,
         * with the exception of zlib.
         */
        virtual void init(bool flush = false) = 0;

        /**
         * @brief Decompress data stream.
         *
         * @param input_func Function to retrieve input buffer.
         * @param output_func Function to retrieve output buffer.
         * @param notify_func Function to notify actual output size.
         * @param flush Whether to flush incomplete frames. This is typically
         * ignored, with the exception of zlib.
         */
        virtual bool decode(
            const uint8_t *input, 
            size_t input_size,
            size_t &read_size,
            uint8_t *output,
            size_t output_size,
            size_t &write_size
        ) = 0;
    };

}

#endif