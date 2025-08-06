#ifndef MAXZIP_STREAM_HPP
#define MAXZIP_STREAM_HPP

#include "common.hpp"

namespace maxzip
{
    class stream
    {
    public:
        virtual void initialize(
            bool flush = false) = 0;

        virtual std::pair<size_t, size_t> update(
            const uint8_t *input,
            size_t input_size,
            uint8_t *output,
            size_t output_size) = 0;

        virtual bool finalize(
            uint8_t *output,
            size_t output_size,
            size_t &write_size) = 0;

        virtual std::pair<size_t, size_t> block_sizes() const = 0;
    };

    struct brotli_encoder_params
    {
        std::optional<int> mode;
        std::optional<int> quality;
        std::optional<int> window_size;
        std::optional<int> block_size;
        std::optional<bool> literal_context_modeling;
        std::optional<int> size_hint;
        std::optional<bool> large_window;
        std::optional<int> postfix_bits;
        std::optional<int> num_direct_distance_codes;
        std::optional<int> stream_offset;
    };

    stream *create_brotli_encoder(
        const brotli_encoder_params &params);

}

#endif