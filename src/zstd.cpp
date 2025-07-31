#include <internal.hpp>

namespace maxzip
{
    template<
        class ContextType, 
        class ParameterType, 
        class SetParameterFuncType, 
        SetParameterFuncType SetParameterFunc,
        class DeleterType,
        DeleterType DeleterFunc>
    class zstd_context
    {
    public:
        static void deleter(ContextType *ctx)
        {
            static_cast<void>(DeleterFunc(ctx));
        }

        zstd_context(ContextType *ctx) : _ctx(ctx, deleter)
        {
        }

        void set_parameter(ParameterType key, int value)
        {
            const size_t ret = SetParameterFunc(_ctx.get(), key, value);
            const bool is_error = (ZSTD_isError(ret) == 1);
            if (is_error)
            {
                throw std::runtime_error("Failed to set Zstandard parameter: " + std::to_string(key));
            }
        }

        void set_flag(ParameterType key, bool value)
        {
            static_cast<void>(SetParameterFunc(_ctx.get(), key, value ? 1 : 0));
        }

    protected:
        std::unique_ptr<ContextType, std::function<void(ContextType*)>> _ctx;
    };

    class zstd_compressor : public compressor, public zstd_context<ZSTD_CCtx, ZSTD_cParameter, decltype(&ZSTD_CCtx_setParameter), &ZSTD_CCtx_setParameter, decltype(&ZSTD_freeCCtx), &ZSTD_freeCCtx>
    {
    public:
        zstd_compressor() : zstd_context(ZSTD_createCCtx())
        {
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
                static_cast<void>(ZSTD_CCtx_reset(_ctx.get(), ZSTD_reset_session_only));
                compressed_size = ZSTD_compressCCtx(
                    _ctx.get(),
                    output,
                    output_size,
                    input,
                    input_size,
                    0);
                if(ZSTD_isError(compressed_size))
                {
                    throw std::runtime_error("Zstandard compression failed: " + std::string(ZSTD_getErrorName(compressed_size)));
                }
            }
            else
            {
                output_size = ZSTD_compressBound(input_size);
            }
            return compressed_size;
        }
    };

    class zstd_decompressor : public decompressor, public zstd_context<ZSTD_DCtx, ZSTD_dParameter, decltype(&ZSTD_DCtx_setParameter), &ZSTD_DCtx_setParameter, decltype(&ZSTD_freeDCtx), &ZSTD_freeDCtx>
    {
    public:
        zstd_decompressor() : zstd_context(ZSTD_createDCtx())
        {
        }

        size_t decompress(
            const uint8_t *input,
            size_t input_size,
            uint8_t *output,
            size_t output_size) override
        {
            size_t decompressed_size = ZSTD_decompressDCtx(
                _ctx.get(),
                output,
                output_size,
                input,
                input_size);
            if (ZSTD_isError(decompressed_size))
            {
                throw std::runtime_error("Zstandard decompression failed: " + std::string(ZSTD_getErrorName(decompressed_size)));
            }
            return decompressed_size;
        }
    };

    compressor *create_zstd_compressor(const zstd_compressor_params &params)
    {
        std::unique_ptr<zstd_compressor> compressor = std::make_unique<zstd_compressor>();

        std::unordered_map<ZSTD_cParameter, std::optional<int>> param_map; 
        param_map[ZSTD_c_compressionLevel] = params.level;
        param_map[ZSTD_c_windowLog] = params.window_log;
        param_map[ZSTD_c_hashLog] = params.hash_log;
        param_map[ZSTD_c_chainLog] = params.chain_log;
        param_map[ZSTD_c_searchLog] = params.search_log;
        param_map[ZSTD_c_minMatch] = params.min_match;
        param_map[ZSTD_c_targetLength] = params.target_length;
        param_map[ZSTD_c_strategy] = params.strategy;

        std::unordered_map<ZSTD_cParameter, std::optional<bool>> flag_map;
        flag_map[ZSTD_c_enableLongDistanceMatching] = params.enable_long_distance_matching;
        flag_map[ZSTD_c_contentSizeFlag] = params.enable_content_size;
        flag_map[ZSTD_c_checksumFlag] = params.enable_checksum;
        flag_map[ZSTD_c_dictIDFlag] = params.enable_dict_id;

        for (const auto &[key, value] : param_map)
        {
            if (value.has_value())
            {
                compressor->set_parameter(key, value.value());
            }
        }

        for (const auto &[key, value] : flag_map)
        {
            if (value.has_value())
            {
                compressor->set_flag(key, value.value());
            }
        }

        return compressor.release();
    }

    decompressor *create_zstd_decompressor(const zstd_decompressor_params &params)
    {
        std::unique_ptr<zstd_decompressor> decompressor = std::make_unique<zstd_decompressor>();
        
        if(params.window_log_max.has_value())
        {
            decompressor->set_parameter(ZSTD_d_windowLogMax, params.window_log_max.value());
        }

        return decompressor.release();
    }
}