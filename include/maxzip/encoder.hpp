
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

#ifndef MAXZIP_ENCODER_HPP
#define MAXZIP_ENCODER_HPP

#include "common.hpp"

namespace maxzip
{
    /**
     * @class encoder
     * @brief Abstract base class for stream compression
     */
    class encoder
    {
    public:
        /**
         * @brief Initialize the encoder state.
         */
        virtual void init(bool flush = false) = 0;
        
        /**
         * @brief Advance encoder state.
         * @param input Pointer to input data. If no input is available, this should be nullptr.
         * @param input_size Size of input data. If no input is available, this should be 0.
         * @param read_size Size of input data processed.
         * @param output Pointer to output buffer.
         * @param output_size Size of output buffer.
         * @param write_size Size of output data written.
         * @return true if still processing, false if compression is complete.
         */
        virtual bool encode(
            const uint8_t *input, 
            size_t input_size,
            size_t &read_size,
            uint8_t *output,
            size_t output_size,
            size_t &write_size) = 0;
};


}

#endif