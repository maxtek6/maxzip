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
    class zlib_compressor : public compressor
    {
    public:
        zlib_compressor(int level, int window_bits, int mem_level, int strategy)
        {
            _stream = {};
            _stream.zalloc = Z_NULL;
            _stream.zfree = Z_NULL;
            _stream.opaque = Z_NULL;

            int ret = deflateInit2(&_stream, level, Z_DEFLATED, window_bits, mem_level, strategy);
            if (ret != Z_OK)
            {
                throw std::runtime_error("Failed to initialize zlib compressor");
            }
        }

        ~zlib_compressor()
        {
            deflateEnd(&_stream);
        }

        size_t compress(
            const uint8_t *input,
            size_t input_size,
            uint8_t *output,
            size_t &output_size)
        {
            size_t compressed_size(0);
            deflateReset(&_stream);
            _stream.avail_in = static_cast<uInt>(input_size);
            if (output != nullptr)
            {
                _stream.next_in = const_cast<Bytef *>(input);
                _stream.next_out = reinterpret_cast<Bytef *>(output);
                _stream.avail_out = static_cast<uInt>(output_size);
                int ret = deflate(&_stream, Z_FINISH);
                if (ret != Z_STREAM_END)
                {
                    throw std::runtime_error("Zlib compression failed");
                }
                compressed_size = output_size - _stream.avail_out;
            }
            else
            {
                output_size = deflateBound(&_stream, input_size);
            }
            return compressed_size;
        }

    private:
        z_stream _stream;
    };

    class zlib_decompressor : public decompressor
    {
    public:
        zlib_decompressor(int window_bits)
        {
            _stream = {};
            _stream.zalloc = Z_NULL;
            _stream.zfree = Z_NULL;
            _stream.opaque = Z_NULL;
            int ret = inflateInit2(&_stream, window_bits);
            if (ret != Z_OK)
            {
                throw std::runtime_error("Failed to initialize zlib decompressor");
            }
        }

        ~zlib_decompressor()
        {
            inflateEnd(&_stream);
        }

        size_t decompress(
            const uint8_t *input,
            size_t input_size,
            uint8_t *output,
            size_t output_size) override
        {
            (void)inflateReset(&_stream);
            
            _stream.avail_in = static_cast<uInt>(input_size);
            _stream.next_in = const_cast<Bytef *>(input);
            _stream.avail_out = static_cast<uInt>(output_size);
            _stream.next_out = reinterpret_cast<Bytef *>(output);

            int ret = inflate(&_stream, Z_FINISH);
            if (ret != Z_STREAM_END && ret != Z_OK)
            {
                throw std::runtime_error("Zlib decompression failed");
            }

            return output_size - _stream.avail_out;
        }

    private:
        z_stream _stream;
    };

    compressor *create_zlib_compressor(
        const zlib_compressor_params &params)
    {
        std::unique_ptr<compressor> compressor = std::make_unique<zlib_compressor>(
            params.level.value_or(Z_DEFAULT_COMPRESSION),
            params.window_bits.value_or(15),
            params.mem_level.value_or(8),
            params.strategy.value_or(Z_DEFAULT_STRATEGY));
        return compressor.release();
    }

    decompressor *create_zlib_decompressor(
        const zlib_decompressor_params &params)
    {
        return new zlib_decompressor(params.window_bits.value_or(15));
    }
}