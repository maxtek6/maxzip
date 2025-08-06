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

size_t maxzip::basic_compressor::compress(
    const uint8_t *input,
    size_t input_size,
    uint8_t *output,
    size_t &output_size)
{
    size_t result(0);
    if (output != nullptr)
    {
        result = compress_data(input, input_size, output, output_size);
    }
    else
    {
        output_size = compress_bound(input, input_size);
    }

    return result;
}

void maxzip::basic_stream::initialize(
    bool flush)
{
    _flush = flush;
    _state = state::PROCESSING;
    setup();
}

std::pair<size_t, size_t> maxzip::basic_stream::update(
    const uint8_t *input,
    size_t input_size,
    uint8_t *output,
    size_t output_size)
{
    size_t read_size = 0;
    size_t write_size = 0;

    // this should fail if we are not in PROCESSING state
    if(_state != state::PROCESSING)
    {
        throw std::runtime_error("invalid stream state " + std::to_string(static_cast<int>(_state)));
    }

    // TODO: validate input and output
 
    process(input, input_size, read_size, output, output_size, write_size, _flush);

    return {read_size, write_size};
}

bool maxzip::basic_stream::finalize(
    uint8_t *output,
    size_t output_size,
    size_t &write_size)
{
    bool finalizing(false);
    
    // nothing has been submitted, go to end
    if(_state == state::CREATED)
    {
        _state = state::FINALIZED;
    }

    // done processing, go to finalizing
    if(_state == state::PROCESSING)
    {
        _state = state::FINALIZING;
    }

    // try to finalize
    if(_state == state::FINALIZING)
    {
        finalizing = finish(output, output_size, write_size);
    }

    // if finalizing stage is done, go to end
    if (!finalizing)
    {
        _state = state::FINALIZED;
    }

    return finalizing;
}

std::pair<size_t, size_t> maxzip::basic_stream::block_sizes() const
{
    return {input_block_size(), output_block_size()};
}

size_t maxzip::basic_stream::input_block_size() const
{
    return 0; // Default implementation, can be overridden
}

size_t maxzip::basic_stream::output_block_size() const
{
    return 0; // Default implementation, can be overridden
}   