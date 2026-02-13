#include "pch.hpp"
#include "Utils.hpp"

namespace fs = std::filesystem;

std::expected<ByteArray, ReadFileError> readRawFile(const fs::path& filePath) {
    if (!fs::exists(filePath)) {
        return std::unexpected(ReadFileError::NOT_FOUND);
    }

    std::ifstream file(filePath, std::ios::binary | std::ios::ate);  
    if (!file.is_open()) {
        return std::unexpected(ReadFileError::UNKNOWN_ERROR);
    }

    auto endPos = file.tellg();
    if (endPos <= 0) {
        return std::unexpected(ReadFileError::NEGATIVE_FILESIZE);
    }

    size_t fileSize = static_cast<size_t>(endPos);

    ByteArray rawData(fileSize);
    file.seekg(0, std::ios::beg);
    if (!file.read(reinterpret_cast<char*>(rawData.data()), fileSize)) {
        return std::unexpected(ReadFileError::FAILED_TO_READ);
    }

    return rawData;
}

ByteArray readRawFileFast(const fs::path& filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);

    size_t fileSize = file.tellg();
    ByteArray rawData(fileSize);

    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(rawData.data()), fileSize);

    return rawData;
}
