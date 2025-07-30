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

#include <internal.hpp>

namespace maxzip
{
    class brotli_compressor : public compressor
    {
    public:
        brotli_compressor(int quality, int window_size, int mode) : _quality(quality), _window_size(window_size), _mode(static_cast<BrotliEncoderMode>(mode))
        {
            if (!maxzip::in_range(_quality, BROTLI_MIN_QUALITY, BROTLI_MAX_QUALITY))
            {
                throw std::invalid_argument("Quality must be between " +
                                            std::to_string(BROTLI_MIN_QUALITY) + " and " +
                                            std::to_string(BROTLI_MAX_QUALITY));
            }

            if (!maxzip::in_range(_window_size, BROTLI_MIN_WINDOW_BITS, BROTLI_MAX_WINDOW_BITS))
            {
                throw std::invalid_argument("Window size must be between " +
                                            std::to_string(BROTLI_MIN_WINDOW_BITS) + " and " +
                                            std::to_string(BROTLI_MAX_WINDOW_BITS));
            }

            if (!maxzip::in_range(_mode, BROTLI_MODE_GENERIC, BROTLI_MODE_TEXT))
            {
                throw std::invalid_argument("Mode must be between " +
                                            std::to_string(BROTLI_MODE_GENERIC) + " and " +
                                            std::to_string(BROTLI_MODE_TEXT));
            }
        }

        size_t compress(
            const uint8_t *input,
            size_t input_size,
            uint8_t *output,
            size_t &output_size) override
        {
            size_t compressed_size(0);
            if (output != nullptr)
            {
                compressed_size = output_size;
                if (BrotliEncoderCompress(
                        _quality,
                        _window_size,
                        _mode,
                        input_size,
                        input,
                        &compressed_size,
                        output) != BROTLI_TRUE)
                {
                    throw std::runtime_error("Insufficient output buffer size.");
                }
            }
            else
            {
                output_size = BrotliEncoderMaxCompressedSize(input_size);
            }
            return compressed_size;
        }

    private:
        int _quality;
        int _window_size;
        BrotliEncoderMode _mode;
    };

    class brotli_decompressor : public decompressor
    {
    public:
        brotli_decompressor() {}
        size_t decompress(
            const uint8_t *input,
            size_t input_size,
            uint8_t *output,
            size_t output_size) override
        {

            size_t decompressed_size = output_size;
            if (BrotliDecoderDecompress(
                    input_size, input, &decompressed_size, output) != BROTLI_DECODER_RESULT_SUCCESS)
            {
                throw std::runtime_error("Decompression failed.");
            }

            return decompressed_size;
        }
    };

    compressor *create_brotli_compressor(
        const brotli_compressor_params &params)
    {
        std::unique_ptr<compressor> compressor = std::make_unique<brotli_compressor>(
            params.quality.value_or(BROTLI_DEFAULT_QUALITY),
            params.window_size.value_or(BROTLI_DEFAULT_WINDOW),
            params.mode.value_or(BROTLI_DEFAULT_MODE));
        return compressor.release();
    }

    decompressor *create_brotli_decompressor(
        const brotli_decompressor_params &params)
    {
        return new brotli_decompressor();
    }
}