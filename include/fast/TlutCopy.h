#pragma once

#include "fast/TextureCopy.h"

namespace Fast {

using TlutResourceSpan = TextureResourceSpan;

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
    return CopyTextureResourceBytes(destination, requestedBytes, source, sourceOffset, resource);
}

} // namespace Fast
