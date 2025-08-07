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

#include <maxtest.hpp>
#include <maxzip.hpp>

#include <algorithm>
#include <memory>
#include <random>

template <typename ObjectType, typename CreateFunction, typename... Args>
static std::pair<bool, std::unique_ptr<ObjectType>> try_create(CreateFunction create_func, Args &&...args)
{
    std::pair<bool, std::unique_ptr<ObjectType>> result;
    try
    {
        result.second = std::unique_ptr<ObjectType>(create_func(std::forward<Args>(args)...));
        result.first = true;
    }
    catch (...)
    {
        result.first = false;
    }

    return result;
}

static bool try_func(const std::function<void()> &func)
{
    bool result;
    try
    {
        func();
        result = true;
    }
    catch (...)
    {
        result =  false;
    }
    return result;
}

class stream_processor
{
public:
    stream_processor()
    {
        std::default_random_engine generator(std::random_device{}());
        std::sample(
            characters.begin(), characters.end(),
            std::back_inserter(_input),
            input_size,
            generator);
    }

    void test_encode_decode(const std::unique_ptr<maxzip::stream> &encoder,
                            const std::unique_ptr<maxzip::stream> &decoder,
                            bool flush)
    {
        std::istringstream input_stream(_input);
        std::stringstream compressed_stream;
        std::ostringstream output_stream;

        process_stream(encoder, input_stream, compressed_stream, flush);
        compressed_stream.seekg(0);
        process_stream(decoder, compressed_stream, output_stream, flush);
        output_stream.seekp(0);
        std::string output = output_stream.str();
        MAXTEST_ASSERT(std::equal(
            _input.begin(), _input.end(),
            output.begin(), output.end()));
    }

private:
    static void process_stream(
        const std::unique_ptr<maxzip::stream> &stream,
        std::istream &input_stream,
        std::ostream &output_stream,
        bool flush)
    {
        const auto block_sizes = stream->block_sizes();
        std::vector<uint8_t> input_buffer(block_sizes.first);
        std::vector<uint8_t> output_buffer(block_sizes.second / (flush ? 10 : 1));
        size_t available_input = 0;
        bool finalizing(true);
        size_t finalizing_write_size = 0;

        stream->initialize(flush);
        while(!input_stream.eof() || available_input > 0)
        {
            if (available_input > 0)
            {
                auto [read_size, write_size] = stream->update(
                input_buffer.data(), available_input,
                output_buffer.data(), output_buffer.size());
                if (write_size > 0)
                {
                    output_stream.write(reinterpret_cast<const char *>(output_buffer.data()), write_size);
                }
                available_input -= read_size;
            }
            else
            {
                input_stream.read(reinterpret_cast<char *>(input_buffer.data()), input_buffer.size());
                available_input = static_cast<size_t>(input_stream.gcount());
            }
        }

        while(finalizing)
        {
            finalizing = stream->finalize(
                output_buffer.data(), output_buffer.size(), finalizing_write_size);
            if (finalizing_write_size > 0)
            {
                output_stream.write(reinterpret_cast<const char *>(output_buffer.data()), finalizing_write_size);
            }
        }
    }

    const size_t input_size = 1000000;
    const std::string characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::string _input;
};

static void test_block_compression(const std::unique_ptr<maxzip::compressor> &compressor,
                            const std::unique_ptr<maxzip::decompressor> &decompressor)
{
    std::vector<uint8_t> input_data(1024, 0xAA);
    std::vector<uint8_t> compressed_data;
    std::vector<uint8_t> decompressed_data(1024);
    size_t max_compressed_size;
    size_t actual_compressed_size;
    size_t decompressed_size;

    // determine maximum compressed size
    MAXTEST_ASSERT(try_func([&]() {
        actual_compressed_size = compressor->compress(input_data.data(), input_data.size(), nullptr, max_compressed_size);
    }));
    MAXTEST_ASSERT(actual_compressed_size == 0);

    compressed_data.resize(max_compressed_size);

    // try to force an error by passing an incorrect output size
    MAXTEST_ASSERT(!try_func([&]() {
        size_t incorrect_output_size = 0;
        actual_compressed_size = compressor->compress(input_data.data(), input_data.size(), compressed_data.data(), incorrect_output_size);
    }));

    // compress the data
    MAXTEST_ASSERT(try_func([&]() {
        actual_compressed_size = compressor->compress(input_data.data(), input_data.size(), compressed_data.data(), max_compressed_size);
    }));
    MAXTEST_ASSERT(actual_compressed_size > 0);
    MAXTEST_ASSERT(actual_compressed_size <= max_compressed_size);
    compressed_data.resize(actual_compressed_size);

    // try to force a decompression error
    MAXTEST_ASSERT(!try_func([&]() {
        decompressed_size = decompressor->decompress(compressed_data.data(), actual_compressed_size, nullptr, 0);
    }));

    // decompress the data
    MAXTEST_ASSERT(try_func([&]() {
        decompressed_data.resize(input_data.size());
        decompressed_size = decompressor->decompress(compressed_data.data(), actual_compressed_size, decompressed_data.data(), decompressed_data.size());
    }));
    MAXTEST_ASSERT(decompressed_size == input_data.size());
    MAXTEST_ASSERT(std::equal(decompressed_data.begin(), decompressed_data.end(), input_data.begin()));
}

static void test_stream_compression(const std::unique_ptr<maxzip::stream> &encoder,
                            const std::unique_ptr<maxzip::stream> &decoder)
{
    stream_processor processor;
    processor.test_encode_decode(encoder, decoder, false);
    processor.test_encode_decode(encoder, decoder, true);
}

MAXTEST_MAIN
{
    MAXTEST_TEST_CASE(brotli::block)
    {
        maxzip::brotli_compressor_params params;
        params.quality = -100;
        auto compressor_result = try_create<maxzip::compressor>(maxzip::create_brotli_compressor, params);
        MAXTEST_ASSERT(!compressor_result.first && (compressor_result.second == nullptr));
        params.quality.reset();
        params.window_size = -100;
        compressor_result = try_create<maxzip::compressor>(maxzip::create_brotli_compressor, params);
        MAXTEST_ASSERT(!compressor_result.first && (compressor_result.second == nullptr));
        params.window_size.reset();
        params.mode = -100;
        compressor_result = try_create<maxzip::compressor>(maxzip::create_brotli_compressor, params);
        MAXTEST_ASSERT(!compressor_result.first && (compressor_result.second == nullptr));
        params.mode.reset();
        compressor_result = try_create<maxzip::compressor>(maxzip::create_brotli_compressor, params);
        MAXTEST_ASSERT(compressor_result.first && (compressor_result.second != nullptr));
        auto decompressor_result = try_create<maxzip::decompressor>(maxzip::create_brotli_decompressor, maxzip::brotli_decompressor_params{});
        MAXTEST_ASSERT(decompressor_result.first && (decompressor_result.second != nullptr));
        test_block_compression(compressor_result.second, decompressor_result.second);
    };

    MAXTEST_TEST_CASE(zlib::block)
    {
        maxzip::zlib_compressor_params compress_params;
        maxzip::zlib_decompressor_params decompress_params;
        compress_params.level = -100;
        auto compressor_result = try_create<maxzip::compressor>(maxzip::create_zlib_compressor, compress_params);
        MAXTEST_ASSERT(!compressor_result.first && (compressor_result.second == nullptr));
        compress_params.level.reset();
        compress_params.window_bits = -100;
        compressor_result = try_create<maxzip::compressor>(maxzip::create_zlib_compressor, compress_params);
        MAXTEST_ASSERT(!compressor_result.first && (compressor_result.second == nullptr));
        compress_params.window_bits.reset();
        compressor_result = try_create<maxzip::compressor>(maxzip::create_zlib_compressor, compress_params);
        MAXTEST_ASSERT(compressor_result.first && (compressor_result.second != nullptr));
        auto decompressor_result = try_create<maxzip::decompressor>(maxzip::create_zlib_decompressor, decompress_params);
        MAXTEST_ASSERT(decompressor_result.first && (decompressor_result.second != nullptr));
        test_block_compression(compressor_result.second, decompressor_result.second);
    };

    MAXTEST_TEST_CASE(zstd::block)
    {
        maxzip::zstd_compressor_params compress_params;
        maxzip::zstd_decompressor_params decompress_params;
        compress_params.window_log = -100;
        auto compressor_result = try_create<maxzip::compressor>(maxzip::create_zstd_compressor, compress_params);
        MAXTEST_ASSERT(!compressor_result.first && (compressor_result.second == nullptr));
        compress_params.window_log = 0;
        compress_params.enable_checksum = true;
        compressor_result = try_create<maxzip::compressor>(maxzip::create_zstd_compressor, compress_params);
        MAXTEST_ASSERT(compressor_result.first && (compressor_result.second != nullptr));
        decompress_params.window_log_max = -100;
        auto decompressor_result = try_create<maxzip::decompressor>(maxzip::create_zstd_decompressor, decompress_params);
        MAXTEST_ASSERT(!decompressor_result.first && (decompressor_result.second == nullptr));
        decompress_params.window_log_max = 0;
        decompressor_result = try_create<maxzip::decompressor>(maxzip::create_zstd_decompressor, decompress_params);
        MAXTEST_ASSERT(decompressor_result.first && (decompressor_result.second != nullptr));
        decompress_params.window_log_max.reset();
        decompressor_result = try_create<maxzip::decompressor>(maxzip::create_zstd_decompressor, decompress_params);
        MAXTEST_ASSERT(decompressor_result.first && (decompressor_result.second != nullptr));
        test_block_compression(compressor_result.second, decompressor_result.second);
    };

    MAXTEST_TEST_CASE(brotli::stream)
    {
        maxzip::brotli_encoder_params encoder_params;
        maxzip::brotli_decoder_params decoder_params;
        auto encoder_result = try_create<maxzip::stream>(maxzip::create_brotli_encoder, encoder_params);
        MAXTEST_ASSERT(encoder_result.first && (encoder_result.second != nullptr));
        auto decoder_result = try_create<maxzip::stream>(maxzip::create_brotli_decoder, decoder_params);
        MAXTEST_ASSERT(decoder_result.first && (decoder_result.second != nullptr));
        test_stream_compression(encoder_result.second, decoder_result.second);
    };
}