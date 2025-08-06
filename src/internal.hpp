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

#ifndef INTERNAL_HPP
#define INTERNAL_HPP

#include <maxzip.hpp>

#include <brotli/decode.h>
#include <brotli/encode.h>

#include <zlib.h>

#include <zstd.h>

#include <functional>
#include <memory>
#include <stdexcept>

namespace maxzip
{
    template<typename T>
    bool in_range(T value, T min, T max)
    {
        return (value >= min && value <= max);
    }

    // DRY principle for compressor
    class basic_compressor : public compressor
    {
    public:
        virtual size_t compress(
            const uint8_t *input,
            size_t input_size,
            uint8_t *output,
            size_t &output_size) override;

    protected:
        virtual size_t compress_bound(
            const uint8_t *input,
            size_t input_size) = 0;
        
        virtual size_t compress_data(
            const uint8_t *input,
            size_t input_size,
            uint8_t *output,
            size_t output_size) = 0;
    };

    // handle overlap in stream state management
    class basic_stream : public stream
    {
    public:
        virtual void initialize(
            bool flush = false) override final;

        virtual std::pair<size_t, size_t> update(
            const uint8_t *input,
            size_t input_size,
            uint8_t *output,
            size_t output_size) override final;

        virtual bool finalize(
            uint8_t *output,
            size_t output_size,
            size_t &write_size) override final;

        virtual std::pair<size_t, size_t> block_sizes() const override final;
    protected:
        virtual void setup() = 0;

        virtual void process(
            const uint8_t *input,
            size_t input_size,
            size_t &read_size,
            uint8_t *output,
            size_t output_size,
            size_t &write_size,
            bool flush) = 0;

        virtual bool finish(
            uint8_t *output,
            size_t output_size,
            size_t &write_size) = 0;

        virtual size_t input_block_size() const;

        virtual size_t output_block_size() const;
    private:
        enum class state
        {
            CREATED,
            PROCESSING,
            FINALIZING,
            FINALIZED
        };
        state _state = state::CREATED;
        bool _flush = false;
    };
}

#endif