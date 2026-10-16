#pragma once
#include <cstdint>

namespace Fast {
// Suballocate the existing dynamic vertex upload, never overwrite a submitted
// draw's range. A wrap starts a new DISCARD generation owned by the D3D driver.
struct VertexUploadCursor {
    static constexpr uint32_t Capacity = 256 * 32 * 3 * sizeof(float);
    struct Range { uint32_t offset; bool discard; bool valid; };
    uint32_t next = 0;

    Range Reserve(uint32_t bytes) {
        if (bytes > Capacity) return {0, false, false};
        uint32_t offset = (next + 15u) & ~15u;
        if (offset > Capacity - bytes) offset = 0;
        next = offset + bytes;
        return {offset, offset == 0, true};
    }
    void Reset() { next = 0; }
};
}
