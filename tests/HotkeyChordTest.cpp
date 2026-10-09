#include "HotkeyChord.h"
#include <stdexcept>
#include <iostream>
using namespace Gallery::HotkeyChord;
int main(){try{
 for(unsigned expected=0;expected<8;++expected) for(unsigned held=0;held<8;++held)
    if(Matches(expected,held)!=(expected==held)) throw std::runtime_error("chord exact match");
 for(unsigned mode=1;mode<=8;++mode) for(unsigned mods=0;mods<8;++mods) {
    auto value=Pack(mode,33,mods);
    if(Mode(value)!=mode || Modifiers(value)!=mods || (value&0xFFFF)!=33) throw std::runtime_error("binding roundtrip");
 }
 if(ModifierForKey(0x1D)!=Ctrl || ModifierForKey(0x9D)!=Ctrl || ModifierForKey(0x2A)!=Shift || ModifierForKey(0x36)!=Shift || ModifierForKey(0x38)!=Alt || ModifierForKey(0xB8)!=Alt || ModifierForKey(33)!=0 || Matches(8,8)) throw std::runtime_error("modifier identification");
 std::cout<<"PASS modifier combinations, registration encoding and left/right keys";return 0;
}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
