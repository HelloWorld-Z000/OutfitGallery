#pragma once
#include <algorithm>
#include <cmath>

namespace Gallery {
struct PreviewLayout { float x, y, width, height; };
// Work in physical pixels so a native-size image samples texel centers even
// when the UI uses a framebuffer scale other than one.
inline PreviewLayout FitPreview(float x, float y, float width, float height,
    float sourceWidth, float sourceHeight, float scaleX, float scaleY) {
    scaleX=scaleX>0 ? scaleX : 1.f;
    scaleY=scaleY>0 ? scaleY : 1.f;
    const float left=std::ceil(x*scaleX), top=std::ceil(y*scaleY);
    const float roomX=std::max(0.f,std::floor((x+width)*scaleX)-left);
    const float roomY=std::max(0.f,std::floor((y+height)*scaleY)-top);
    if(sourceWidth<=0 || sourceHeight<=0) return {left/scaleX,top/scaleY,0,0};
    const float fit=std::min({1.f,roomX/sourceWidth,roomY/sourceHeight});
    const float w=std::floor(sourceWidth*fit), h=std::floor(sourceHeight*fit);
    return {(left+std::floor((roomX-w)*.5f))/scaleX,
        (top+std::floor((roomY-h)*.5f))/scaleY,w/scaleX,h/scaleY};
}
}
