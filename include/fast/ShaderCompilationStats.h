#pragma once

#include <cstdint>

namespace Fast {
// The synchronous rendering thread owns these counters. No per-frame logging.
// Consumers can distinguish compiler work from ordinary frame pacing.
struct ShaderCompilationStats {
    uint64_t compilations = 0;
    uint64_t cacheHits = 0;
    double milliseconds = 0;
};
inline thread_local ShaderCompilationStats gShaderCompilationStats;
}
