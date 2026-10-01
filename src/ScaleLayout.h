#pragma once
#include <cmath>
namespace Gallery {
struct SurfaceSize { float width{}, height{}; };
struct LayoutSurface {
    SurfaceSize swap, buffer, viewport;
    bool valid{};
};
// Conservative experiment: only a uniform, smaller buffer with a matching
// full-surface viewport and an unscaled UI framebuffer is eligible.
inline SurfaceSize ResolveLayoutSize(SurfaceSize display, SurfaceSize framebuffer, const LayoutSurface& s) {
    const auto positive=[](SurfaceSize v){return std::isfinite(v.width)&&std::isfinite(v.height)&&v.width>0&&v.height>0;};
    const auto same=[](float a,float b){return std::abs(a-b)<1.f;};
    if(!s.valid || !positive(display) || !positive(s.swap) || !positive(s.buffer) || !positive(s.viewport)) return display;
    if(std::abs(framebuffer.width-1.f)>.001f || std::abs(framebuffer.height-1.f)>.001f) return display;
    if(!same(display.width,s.swap.width) || !same(display.height,s.swap.height) ||
       !same(s.buffer.width,s.viewport.width) || !same(s.buffer.height,s.viewport.height)) return display;
    const float x=s.buffer.width/display.width, y=s.buffer.height/display.height;
    if(x<.25f || x>=.999f || y<.25f || y>=.999f || std::abs(x-y)>.002f) return display;
    return s.buffer;
}
}
