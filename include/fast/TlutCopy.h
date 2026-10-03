#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace Fast {

struct TlutResourceSpan {
    const uint8_t* imageData;
    size_t imageBytes;
    const uint8_t* ownedBufferData;
    size_t ownedBufferBytes;
    bool hasOwnedBuffer;
};

// Cache identity only: the resulting address need not designate readable storage.
// Integer arithmetic avoids forming an out-of-object pointer with source + offset.
inline const uint8_t* TlutAddressToken(const uint8_t* source, size_t byteOffset) noexcept {
    const uintptr_t address = reinterpret_cast<uintptr_t>(source);
    if (byteOffset > (std::numeric_limits<uintptr_t>::max)() - address) {
        return nullptr;
    }
    return reinterpret_cast<const uint8_t*>(address + byteOffset);
}

// destination must contain requestedBytes writable bytes and must not overlap the
// source. Preserve every available source byte and zero only the unavailable part
// of this requested write, leaving other palette entries unchanged. A resource
// without an owned file buffer is still bounded by its declared image extent.
inline size_t CopyTlutResourceBytes(uint8_t* destination, size_t requestedBytes, const uint8_t* source,
                                    size_t sourceOffset, const TlutResourceSpan& resource) noexcept {
    if (requestedBytes == 0) {
        return 0;
    }

    const uintptr_t maxAddress = (std::numeric_limits<uintptr_t>::max)();
    const uintptr_t imageAddress = reinterpret_cast<uintptr_t>(resource.imageData);
    const uintptr_t sourceAddress = reinterpret_cast<uintptr_t>(source);
    size_t availableBytes = 0;

    if (resource.imageData != nullptr && source != nullptr && resource.imageBytes <= maxAddress - imageAddress &&
        sourceAddress >= imageAddress && sourceAddress - imageAddress <= resource.imageBytes) {
        availableBytes = resource.imageBytes - (sourceAddress - imageAddress);
        if (resource.hasOwnedBuffer) {
            const uintptr_t bufferAddress = reinterpret_cast<uintptr_t>(resource.ownedBufferData);
            if (resource.ownedBufferData == nullptr || resource.ownedBufferBytes > maxAddress - bufferAddress ||
                sourceAddress < bufferAddress || sourceAddress - bufferAddress > resource.ownedBufferBytes) {
                availableBytes = 0;
            } else {
                const size_t bufferRemaining = resource.ownedBufferBytes - (sourceAddress - bufferAddress);
                if (bufferRemaining < availableBytes) {
                    availableBytes = bufferRemaining;
                }
            }
        }
    }

    size_t copiedBytes = 0;
    // Do not form the offset source address at all when no bytes can be read.
    if (sourceOffset < availableBytes && sourceOffset <= maxAddress - sourceAddress) {
        copiedBytes = availableBytes - sourceOffset;
        if (copiedBytes > requestedBytes) {
            copiedBytes = requestedBytes;
        }
        std::memcpy(destination, reinterpret_cast<const void*>(sourceAddress + sourceOffset), copiedBytes);
    }
    if (copiedBytes < requestedBytes) {
        std::memset(destination + copiedBytes, 0, requestedBytes - copiedBytes);
    }
    return copiedBytes;
}

} // namespace Fast
