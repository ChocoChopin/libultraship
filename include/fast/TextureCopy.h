#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace Fast {

struct TextureResourceSpan {
    const uint8_t* imageData;
    size_t imageBytes;
    const uint8_t* ownedBufferData;
    size_t ownedBufferBytes;
    bool hasOwnedBuffer;
};

// Bound a resource by both its declared image and its retained file allocation.
// Compare integer addresses without forming a pointer outside either object.
inline size_t AvailableTextureResourceBytes(const uint8_t* source, const TextureResourceSpan& resource) noexcept {
    const uintptr_t maxAddress = (std::numeric_limits<uintptr_t>::max)();
    const uintptr_t imageAddress = reinterpret_cast<uintptr_t>(resource.imageData);
    const uintptr_t sourceAddress = reinterpret_cast<uintptr_t>(source);
    if (resource.imageData == nullptr || source == nullptr || resource.imageBytes > maxAddress - imageAddress ||
        sourceAddress < imageAddress || sourceAddress - imageAddress > resource.imageBytes) {
        return 0;
    }
    size_t availableBytes = resource.imageBytes - (sourceAddress - imageAddress);
    if (resource.hasOwnedBuffer) {
        const uintptr_t bufferAddress = reinterpret_cast<uintptr_t>(resource.ownedBufferData);
        if (resource.ownedBufferData == nullptr || resource.ownedBufferBytes > maxAddress - bufferAddress ||
            sourceAddress < bufferAddress || sourceAddress - bufferAddress > resource.ownedBufferBytes) {
            return 0;
        }
        const size_t bufferRemaining = resource.ownedBufferBytes - (sourceAddress - bufferAddress);
        if (bufferRemaining < availableBytes) {
            availableBytes = bufferRemaining;
        }
    }
    return availableBytes;
}

// The writable destination must not overlap the source. Copy represented bytes
// and zero only the unavailable part of the requested write.
inline size_t CopyTextureResourceBytes(uint8_t* destination, size_t requestedBytes, const uint8_t* source,
                                       size_t sourceOffset, const TextureResourceSpan& resource) noexcept {
    if (requestedBytes == 0) {
        return 0;
    }
    const size_t availableBytes = AvailableTextureResourceBytes(source, resource);
    const uintptr_t sourceAddress = reinterpret_cast<uintptr_t>(source);
    size_t copiedBytes = 0;
    if (sourceOffset < availableBytes && sourceOffset <= (std::numeric_limits<uintptr_t>::max)() - sourceAddress) {
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

struct TextureRowCopyResult {
    bool valid;
    size_t copiedBytes;
};

// Pack strided image rows into a caller-sized buffer. Missing bytes become zero
// without changing the requested row dimensions. Reject size arithmetic before
// writing anything; source row offsets stay integers until a bounded read exists.
inline TextureRowCopyResult CopyTextureResourceRows(uint8_t* destination, size_t destinationBytes, size_t rowBytes,
                                                    size_t rowCount, const uint8_t* source, size_t sourceStride,
                                                    const TextureResourceSpan& resource) noexcept {
    if (rowBytes == 0 || rowCount == 0) {
        return { true, 0 };
    }
    if (destination == nullptr || rowCount > destinationBytes / rowBytes ||
        (sourceStride != 0 && rowCount - 1 > (std::numeric_limits<size_t>::max)() / sourceStride)) {
        return { false, 0 };
    }
    size_t copiedBytes = 0;
    for (size_t row = 0; row < rowCount; ++row) {
        copiedBytes += CopyTextureResourceBytes(destination + row * rowBytes, rowBytes, source, row * sourceStride,
                                                resource);
    }
    return { true, copiedBytes };
}

} // namespace Fast
