# MaxZip

C++ compression framework.

## About

This project is intended to provide an abstract C++ frontend for various 
compression libraries. Each implementation is exposed through a common 
API, along with configuration parameters.

## Usage

The API is designed to provide common interfaces for compressing and 
decompressing data.

### Block API

The block API provides the `compressor` and `decompressor` interfaces, 
which can be used for one shot processing. This is a clean and simple 
API, best suited for small to medium payloads where memory usage and 
IO complexity are not a major concern.

### Streaming API

The streaming API provides the `encoder` and `decoder` interfaces, to 
allow for incremental data processing. This is preferable when input 
is either too large, or the underlying IO is segmented.