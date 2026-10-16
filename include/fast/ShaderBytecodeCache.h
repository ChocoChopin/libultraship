#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace Fast::ShaderBytecodeCache {
// Disposable compiler output only. Exact source/options are retained in each
// entry; a hash collision, truncation or damaged payload is a cache miss.
constexpr uint32_t Magic = 0x31434853; // SHC1, Windows little endian
constexpr uint32_t MaxEntryBytes = 256 * 1024;
constexpr uint64_t MaxDirectoryBytes = 16 * 1024 * 1024;
struct Bytecode { std::vector<uint8_t> vertex, pixel; };
inline uint64_t Hash(const void* data, size_t size, uint64_t hash = 14695981039346656037ull) {
    const auto* bytes = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < size; ++i) hash = (hash ^ bytes[i]) * 1099511628211ull;
    return hash;
}
inline uint64_t Hash(const Bytecode& code) {
    const std::array<uint32_t, 2> sizes{static_cast<uint32_t>(code.vertex.size()), static_cast<uint32_t>(code.pixel.size())};
    return Hash(code.pixel.data(), code.pixel.size(),
        Hash(code.vertex.data(), code.vertex.size(), Hash(sizes.data(), sizeof(sizes))));
}
inline std::filesystem::path Entry(const std::filesystem::path& directory, const std::string& identity) {
    return directory / (std::to_string(Hash(identity.data(), identity.size())) + ".shc");
}
inline bool IsDxbc(const std::vector<uint8_t>& bytes) {
    return bytes.size() >= 32 && bytes[0] == 'D' && bytes[1] == 'X' && bytes[2] == 'B' && bytes[3] == 'C';
}
inline bool Read(const std::filesystem::path& directory, const std::string& identity, Bytecode& result) {
    std::ifstream file(Entry(directory, identity), std::ios::binary | std::ios::ate);
    if (!file || file.tellg() < 24 || file.tellg() > MaxEntryBytes) return false;
    const auto size = static_cast<uint64_t>(file.tellg());
    file.seekg(0);
    std::array<uint32_t, 4> header{};
    uint64_t checksum = 0;
    file.read(reinterpret_cast<char*>(header.data()), sizeof(header));
    file.read(reinterpret_cast<char*>(&checksum), sizeof(checksum));
    if (!file || header[0] != Magic || header[1] != identity.size() ||
        uint64_t(24) + header[1] + header[2] + header[3] != size) return false;
    std::string stored(header[1], '\0');
    file.read(stored.data(), stored.size());
    if (!file || stored != identity) return false;
    Bytecode candidate;
    candidate.vertex.resize(header[2]); candidate.pixel.resize(header[3]);
    file.read(reinterpret_cast<char*>(candidate.vertex.data()), candidate.vertex.size());
    file.read(reinterpret_cast<char*>(candidate.pixel.data()), candidate.pixel.size());
    if (!file || !IsDxbc(candidate.vertex) || !IsDxbc(candidate.pixel) || Hash(candidate) != checksum) return false;
    result = std::move(candidate);
    return true;
}
inline bool Write(const std::filesystem::path& directory, const std::string& identity, const Bytecode& code) {
    const uint64_t size = uint64_t(24) + identity.size() + code.vertex.size() + code.pixel.size();
    if (size > MaxEntryBytes || !IsDxbc(code.vertex) || !IsDxbc(code.pixel)) return false;
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error) return false;
    uint64_t total = size;
    for (std::filesystem::directory_iterator it(directory, error), end; !error && it != end; it.increment(error)) {
        if (it->path().extension() == ".shc") total += it->file_size(error);
        if (error || total > MaxDirectoryBytes) return false;
    }
    if (error) return false;
    // No eviction, global paths, or required write access. A failed/partial write
    // is safely rejected on the next load and normal compilation remains available.
    std::ofstream file(Entry(directory, identity), std::ios::binary | std::ios::trunc);
    const std::array<uint32_t, 4> header{Magic, static_cast<uint32_t>(identity.size()),
        static_cast<uint32_t>(code.vertex.size()), static_cast<uint32_t>(code.pixel.size())};
    const uint64_t checksum = Hash(code);
    file.write(reinterpret_cast<const char*>(header.data()), sizeof(header));
    file.write(reinterpret_cast<const char*>(&checksum), sizeof(checksum));
    file.write(identity.data(), identity.size());
    file.write(reinterpret_cast<const char*>(code.vertex.data()), code.vertex.size());
    file.write(reinterpret_cast<const char*>(code.pixel.data()), code.pixel.size());
    file.close();
    return bool(file);
}
} // namespace Fast::ShaderBytecodeCache
