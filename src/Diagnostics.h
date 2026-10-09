#pragma once
#include "DiagnosticSettings.h"
#include <chrono>
// Guard arguments as well as the write: OFF never formats diagnostic strings.
#define GALLERY_DIAG(...) do { if(::Gallery::DetailEnabled()) { try { SKSE::log::info(__VA_ARGS__); } catch(...) {} } } while(false)
namespace Gallery {
struct CleanupTiming {
    const char* stage;
    unsigned items{};
    bool inventoryScan{};
    bool enabled=DetailEnabled();
    std::chrono::steady_clock::time_point start=enabled?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};
    ~CleanupTiming() {
        if(!enabled || !DetailEnabled()) return;
        const auto us=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start).count();
        GALLERY_DIAG("OG-DIAG timing stage={} items={} inventoryScan={} elapsed_us={}",stage,items,inventoryScan,us);
    }
};
}
