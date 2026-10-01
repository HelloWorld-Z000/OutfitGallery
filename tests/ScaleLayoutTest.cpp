#include "ScaleLayout.h"
#include <iostream>
#include <limits>
int main() {
    using namespace Gallery;
    const SurfaceSize display{3840,2160};
    LayoutSurface s{display,{2560,1440},{2560,1440},true};
    int failures=0;
    auto check=[&](SurfaceSize fb,SurfaceSize expected,const char* name){
        const auto r=ResolveLayoutSize(display,fb,s);
        if(r.width!=expected.width || r.height!=expected.height){std::cerr<<name<<'\n'; ++failures;}
    };
    check({1,1},{2560,1440},"reported DLSS ON");
    s.buffer=s.viewport=display; check({1,1},display,"reported DLSS OFF");
    s.buffer=s.viewport={2560,1440}; check({1.5f,1.5f},display,"existing framebuffer compensation");
    s.valid=false; check({1,1},display,"missing surface"); s.valid=true;
    s.viewport=display; check({1,1},display,"different viewport");
    s.viewport=s.buffer={2560,1080}; check({1,1},display,"different aspect ratio");
    s.viewport=s.buffer={5120,2880}; check({1,1},display,"supersampling");
    s.viewport=s.buffer={0,0}; check({1,1},display,"minimized surface");
    s.viewport=s.buffer={std::numeric_limits<float>::quiet_NaN(),1440}; check({1,1},display,"invalid surface");
    s.viewport=s.buffer={2560,1440}; s.swap={1920,1080}; check({1,1},display,"unmatched swap dimensions");
    return failures?1:0;
}
