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
#include <memory>

template <typename CreateFunction, typename ParamType>
static std::pair<bool, std::unique_ptr<maxzip::compressor>> try_create_compressor(CreateFunction create_func, ParamType &&param)
{
    std::pair<bool, std::unique_ptr<maxzip::compressor>> result;
    try
    {
        result.second = std::unique_ptr<maxzip::compressor>(create_func(std::forward<ParamType>(param)));
        result.first = true;
    }
    catch (...)
    {
        result.first = false;
    }

    return result;
}

template <typename CreateFunction, typename... Args>
static std::pair<bool, std::unique_ptr<maxzip::decompressor>> try_create_decompressor(CreateFunction create_func, Args &&...args)
{
    std::pair<bool, std::unique_ptr<maxzip::decompressor>> result;
    try
    {
        result.second = std::unique_ptr<maxzip::decompressor>(create_func(std::forward<Args>(args)...));
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

MAXTEST_MAIN
{
    MAXTEST_TEST_CASE(brotli::block)
    {
        maxzip::brotli_compressor_params params;
        params.quality = -100;
        auto compressor_result = try_create_compressor(maxzip::create_brotli_compressor, params);
        MAXTEST_ASSERT(!compressor_result.first && (compressor_result.second == nullptr));
        params.quality.reset();
        params.window_size = -100;
        compressor_result = try_create_compressor(maxzip::create_brotli_compressor, params);
        MAXTEST_ASSERT(!compressor_result.first && (compressor_result.second == nullptr));
        params.window_size.reset();
        params.mode = -100;
        compressor_result = try_create_compressor(maxzip::create_brotli_compressor, params);
        MAXTEST_ASSERT(!compressor_result.first && (compressor_result.second == nullptr));
        params.mode.reset();
        compressor_result = try_create_compressor(maxzip::create_brotli_compressor, params);
        MAXTEST_ASSERT(compressor_result.first && (compressor_result.second != nullptr));
        auto decompressor_result = try_create_decompressor(maxzip::create_brotli_decompressor, maxzip::brotli_decompressor_params{});
        MAXTEST_ASSERT(decompressor_result.first && (decompressor_result.second != nullptr));
        test_block_compression(compressor_result.second, decompressor_result.second);
    };

    MAXTEST_TEST_CASE(zlib::block)
    {
        maxzip::zlib_compressor_params compress_params;
        maxzip::zlib_decompressor_params decompress_params;
        compress_params.level = -100;
        auto compressor_result = try_create_compressor(maxzip::create_zlib_compressor, compress_params);
        MAXTEST_ASSERT(!compressor_result.first && (compressor_result.second == nullptr));
        compress_params.level.reset();
        compress_params.window_bits = -100;
        compressor_result = try_create_compressor(maxzip::create_zlib_compressor, compress_params);
        MAXTEST_ASSERT(!compressor_result.first && (compressor_result.second == nullptr));
        compress_params.window_bits.reset();
        compressor_result = try_create_compressor(maxzip::create_zlib_compressor, compress_params);
        MAXTEST_ASSERT(compressor_result.first && (compressor_result.second != nullptr));
        auto decompressor_result = try_create_decompressor(maxzip::create_zlib_decompressor, decompress_params);
        MAXTEST_ASSERT(decompressor_result.first && (decompressor_result.second != nullptr));
        test_block_compression(compressor_result.second, decompressor_result.second);
    };

    MAXTEST_TEST_CASE(zstd::block)
    {
        maxzip::zstd_compressor_params compress_params;
        maxzip::zstd_decompressor_params decompress_params;
        compress_params.window_log = -100;
        auto compressor_result = try_create_compressor(maxzip::create_zstd_compressor, compress_params);
        MAXTEST_ASSERT(!compressor_result.first && (compressor_result.second == nullptr));
        compress_params.window_log = 0;
        compress_params.enable_checksum = true;
        compressor_result = try_create_compressor(maxzip::create_zstd_compressor, compress_params);
        MAXTEST_ASSERT(compressor_result.first && (compressor_result.second != nullptr));
        decompress_params.window_log_max = -100;
        auto decompressor_result = try_create_decompressor(maxzip::create_zstd_decompressor, decompress_params);
        MAXTEST_ASSERT(!decompressor_result.first && (decompressor_result.second == nullptr));
        decompress_params.window_log_max = 0;
        decompressor_result = try_create_decompressor(maxzip::create_zstd_decompressor, decompress_params);
        MAXTEST_ASSERT(decompressor_result.first && (decompressor_result.second != nullptr));
        decompress_params.window_log_max.reset();
        decompressor_result = try_create_decompressor(maxzip::create_zstd_decompressor, decompress_params);
        MAXTEST_ASSERT(decompressor_result.first && (decompressor_result.second != nullptr));
        test_block_compression(compressor_result.second, decompressor_result.second);
    };
}