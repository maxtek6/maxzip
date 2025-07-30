#ifndef MAXZIP_DECOMPRESSOR_HPP
#define MAXZIP_DECOMPRESSOR_HPP

#include "common.hpp"

namespace maxzip
{
    /**
     * @class decompressor
     * @brief Abstract base class for block decompression
     */
    class decompressor
    {
    public:
        /**
         * @brief Decompress a block of data
         * @param input Pointer to the compressed input data
         * @param input_size Size of the compressed input data in bytes
         * @param output Pointer to the output buffer.
         * @param output_size Size of the output buffer. This may be ignored,
         * depending on the implementation.
         * @return The size of the decompressed data in bytes.
         */
        virtual size_t decompress(
            const uint8_t *input,
            size_t input_size,
            uint8_t *output,
            size_t output_size) = 0;
    };

    struct brotli_decompressor_params
    {
        std::optional<int> unused;
    };

    struct zlib_decompressor_params
    {
        std::optional<int> window_bits;
    };

    struct zstd_decompressor_params
    {
        std::optional<int> window_log_max;
        std::optional<int> decoder_format;
        std::optional<bool> stable_output_buffer;
        std::optional<bool> ignore_checksum;
        std::optional<bool> multiple_dictionaries;
        std::optional<bool> disable_huffman_assembly;
    };

    decompressor *create_brotli_decompressor(const brotli_decompressor_params &params = {});
    decompressor *create_zlib_decompressor(const zlib_decompressor_params &params = {});
    decompressor *create_zstd_decompressor(const zstd_decompressor_params &params = {});
}

#endif