#pragma once

#include <ranges>

#ifndef NDEBUG
#define DEBUG_PRINT(x) do { std::cout << (x) << std::endl; } while(0)
#else
#define DEBUG_PRINT(x) do { /*DO NOTHING*/ } while(0)
#endif

template<typename A, typename B>
bool isAllPresent(const std::vector<A>& a, const std::vector<B>& b) {
    return std::ranges::all_of(a,
            [&b] (const A& _a) {
                return std::ranges::any_of(b,
                        [&_a] (const B& _b) {
                            return _a == _b;
                        });
                });
}

using ByteArray = std::vector<std::byte>;

enum class ReadFileError {
    NOT_FOUND = 0,
    FAILED_TO_READ,
    NEGATIVE_FILESIZE,
    UNKNOWN_ERROR
};

inline std::string to_string(const ReadFileError& err) {
    switch (err) {
        case ReadFileError::NOT_FOUND: return "File not found";
        case ReadFileError::FAILED_TO_READ: return "Failed to read file";
        case ReadFileError::NEGATIVE_FILESIZE: return "File size is not positive";
        default: return "Unknown error";
    }
    return {};
}

std::expected<ByteArray, ReadFileError> readRawFile(const std::filesystem::path& filePath);
ByteArray readRawFileFast(const std::filesystem::path& filePath);
