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
    class brotli_compressor : public basic_compressor
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

    protected:
        size_t compress_data(
            const uint8_t *input,
            size_t input_size,
            uint8_t *output,
            size_t output_size) override
        {
            size_t compressed_size(output_size);
            const int rc = BrotliEncoderCompress(
                _quality,
                _window_size,
                _mode,
                input_size,
                input,
                &compressed_size,
                output);
            if (rc != BROTLI_TRUE)
            {
                throw std::runtime_error("Insufficient output buffer size.");
            }
            return compressed_size;
        }

        size_t compress_bound(
            const uint8_t * /*unused*/,
            size_t input_size) override
        {
            return BrotliEncoderMaxCompressedSize(input_size);
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
            const BrotliDecoderResult result = BrotliDecoderDecompress(
                input_size,
                input,
                &decompressed_size,
                output);
            if (result != BROTLI_DECODER_RESULT_SUCCESS)
            {
                throw std::runtime_error("Decompression failed.");
            }

            return decompressed_size;
        }
    };

    template <
        typename ContextType,
        typename ConstructorType,
        ConstructorType Constructor,
        typename DestructorType,
        DestructorType Destructor>
    class stream_handle
    {
    public:
        using context_type = ContextType;
        stream_handle() : _ctx(nullptr, Destructor) {}
        ~stream_handle() = default;
        void create()
        {
            _ctx.reset(Constructor(nullptr, nullptr, nullptr));
        }
        ContextType *get() const
        {
            return _ctx.get();
        }
        void reset()
        {
            _ctx.reset();
        }

    private:
        std::unique_ptr<ContextType, DestructorType> _ctx;
    };

    template <typename ParameterType, typename SetParameterType, SetParameterType SetParameter>
    class stream_config
    {
    public:
        stream_config() = default;

        template <typename ContextType>
        void configure(ContextType *ctx)
        {
            for (const auto &[param, value] : _params)
            {
                if (value.has_value())
                {
                    set_parameter(ctx, param, value.value());
                }
            }

            for (const auto &[param, value] : _flags)
            {
                if (value.has_value())
                {
                    set_flag(ctx, param, value.value());
                }
            }
        }

        std::unordered_map<ParameterType, std::optional<int>> _params;
        std::unordered_map<ParameterType, std::optional<bool>> _flags;

    private:
        template <typename ContextType>
        void set_parameter(
            ContextType *ctx,
            ParameterType param,
            int value)
        {
            const int rc = SetParameter(ctx, param, value);
            if (rc == BROTLI_FALSE)
            {
                throw std::invalid_argument("failed to set parameter " + std::to_string(param) + " to value " + std::to_string(value));
            }
        }

        template <typename ContextType>
        void set_flag(
            ContextType *ctx,
            ParameterType param,
            bool value)
        {
            static_cast<void>(SetParameter(ctx, param, value ? 1 : 0));
        }
    };

    template <
        typename QueryFuncType,
        QueryFuncType QueryFunc,
        typename ProcessFuncType,
        ProcessFuncType ProcessFunc,
        typename... ArgTypes>
    class stream_functions
    {
    public:
        template <typename ContextType>
        static void stream_process(
            ContextType *ctx,
            ArgTypes... args,
            const uint8_t *input,
            size_t input_size,
            size_t *read_size,
            uint8_t *output,
            size_t output_size,
            size_t *write_size)
        {
            size_t available_in = input_size;
            size_t available_out = output_size;
            const int rc = ProcessFunc(ctx, args..., &available_in, &input, &available_out, &output, nullptr);
            if (rc != BROTLI_TRUE)
            {
                throw std::runtime_error("stream processing failed.");
            }
            if (read_size)
            {
                *read_size = input_size - available_in;
            }
            *write_size = output_size - available_out;
        }

        template <typename ContextType>
        static bool stream_query(ContextType *ctx)
        {
            bool result(false);
            const int rc = QueryFunc(ctx);
            if (rc == BROTLI_FALSE)
            {
                result = true;
            }
            return result;
        }
    };

    using encoder_functions = stream_functions<
        decltype(&BrotliEncoderIsFinished),
        &BrotliEncoderIsFinished,
        decltype(&BrotliEncoderCompressStream),
        &BrotliEncoderCompressStream,
        BrotliEncoderOperation>;

    using decoder_functions = stream_functions<
        decltype(&BrotliDecoderIsFinished),
        &BrotliDecoderIsFinished,
        decltype(&BrotliDecoderDecompressStream),
        &BrotliDecoderDecompressStream>;

    template <typename HandleType, typename ConfigType, typename StreamFunctions>
    class brotli_stream : public basic_stream, public StreamFunctions
    {
    public:
        void setup() override
        {
            _handle.create();
            _config.configure(_handle.get());
        }

        bool finish(
            uint8_t *output,
            size_t output_size,
            size_t &write_size) override
        {
            finish_stream(output, output_size, write_size);
            return StreamFunctions::stream_query(_handle.get());
        }

    protected:
        virtual void finish_stream(
            uint8_t *output,
            size_t output_size,
            size_t &write_size) = 0;
        HandleType _handle;
        ConfigType _config;
    };

    using encoder_handle = stream_handle<
        BrotliEncoderState,
        decltype(&BrotliEncoderCreateInstance),
        &BrotliEncoderCreateInstance,
        decltype(&BrotliEncoderDestroyInstance),
        &BrotliEncoderDestroyInstance>;

    using encoder_config = stream_config<
        BrotliEncoderParameter,
        decltype(&BrotliEncoderSetParameter),
        &BrotliEncoderSetParameter>;

    using decoder_handle = stream_handle<
        BrotliDecoderState,
        decltype(&BrotliDecoderCreateInstance),
        &BrotliDecoderCreateInstance,
        decltype(&BrotliDecoderDestroyInstance),
        &BrotliDecoderDestroyInstance>;

    using decoder_config = stream_config<
        BrotliDecoderParameter,
        decltype(&BrotliDecoderSetParameter),
        &BrotliDecoderSetParameter>;

    class brotli_encoder : public brotli_stream<encoder_handle, encoder_config, encoder_functions>
    {
    public:
        brotli_encoder(const brotli_encoder_params &params)
        {
            _config._params[BROTLI_PARAM_MODE] = params.mode;
            _config._params[BROTLI_PARAM_QUALITY] = params.quality;
            _config._params[BROTLI_PARAM_LGWIN] = params.window_size;
            _config._params[BROTLI_PARAM_LGBLOCK] = params.block_size;
            _config._flags[BROTLI_PARAM_DISABLE_LITERAL_CONTEXT_MODELING] = params.literal_context_modeling;
            _config._params[BROTLI_PARAM_SIZE_HINT] = params.size_hint;
            _config._flags[BROTLI_PARAM_LARGE_WINDOW] = params.large_window;
            _config._params[BROTLI_PARAM_NPOSTFIX] = params.postfix_bits;
            _config._params[BROTLI_PARAM_NDIRECT] = params.num_direct_distance_codes;
            _config._params[BROTLI_PARAM_STREAM_OFFSET] = params.stream_offset;
            _handle.create();
            _config.configure(_handle.get());
            _handle.reset();
        }

    protected:
        void process(
            const uint8_t *input,
            size_t input_size,
            size_t &read_size,
            uint8_t *output,
            size_t output_size,
            size_t &write_size,
            bool flush) override
        {
            const BrotliEncoderOperation op = flush ? BROTLI_OPERATION_FLUSH : BROTLI_OPERATION_PROCESS;
            stream_process(
                _handle.get(),
                op,
                input,
                input_size,
                &read_size,
                output,
                output_size,
                &write_size);
        }

        void finish_stream(
            uint8_t *output,
            size_t output_size,
            size_t &write_size) override
        {
            stream_process(
                _handle.get(),
                BROTLI_OPERATION_FINISH,
                nullptr, 0, nullptr, output, output_size, &write_size);
        }

        size_t input_block_size() const override
        {
            return 16000;
        }

        size_t output_block_size() const override
        {
            return 16000;
        }
    };

    class brotli_decoder : public brotli_stream<decoder_handle, decoder_config, decoder_functions>
    {
    public:
        brotli_decoder(const brotli_decoder_params &params)
        {
            _config._flags[BROTLI_DECODER_PARAM_DISABLE_RING_BUFFER_REALLOCATION] = params.disable_ring_buffer_reallocation;
            _config._flags[BROTLI_DECODER_PARAM_LARGE_WINDOW] = params.large_window;
            _handle.create();
            _config.configure(_handle.get());
            _handle.reset();
        }

    protected:
        void process(
            const uint8_t *input,
            size_t input_size,
            size_t &read_size,
            uint8_t *output,
            size_t output_size,
            size_t &write_size,
            bool flush) override
        {
            stream_process(
                _handle.get(),
                input,
                input_size,
                &read_size,
                output,
                output_size,
                &write_size);
        }

        void finish_stream(
            uint8_t *output,
            size_t output_size,
            size_t &write_size) override
        {
            stream_process(
                _handle.get(),
                nullptr, 0, nullptr, output, output_size, &write_size);
        }

        size_t input_block_size() const override
        {
            return 16000;
        }

        size_t output_block_size() const override
        {
            return 16000;
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
        const brotli_decompressor_params & /* unused */)
    {
        return new brotli_decompressor();
    }

    stream *create_brotli_encoder(
        const brotli_encoder_params &params)
    {
        return new brotli_encoder(params);
    }

    stream *create_brotli_decoder(
        const brotli_decoder_params &params)
    {
        return new brotli_decoder(params);
    }
}