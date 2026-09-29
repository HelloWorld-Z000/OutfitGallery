#pragma once
struct HoldButton {
    bool armed{}, fired{};
    bool Update(bool down,bool pressed,float duration,float threshold) {
        if(!pressed) {armed=false; fired=false; return false;}
        if(down) {armed=true; fired=false;}
        if(armed && !fired && duration>=threshold) {fired=true; return true;}
        return false;
    }
};
