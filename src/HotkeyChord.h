#pragma once
namespace Gallery::HotkeyChord {
constexpr unsigned Ctrl=1,Shift=2,Alt=4;
constexpr unsigned ModifierForKey(unsigned code) {
    return code==0x1D || code==0x9D?Ctrl:code==0x2A || code==0x36?Shift:code==0x38 || code==0xB8?Alt:0;
}
constexpr bool Matches(unsigned required,unsigned held) {return required<=7 && held==required;}
constexpr unsigned Pack(unsigned mode,unsigned code,unsigned modifiers) {return (modifiers<<24)|(mode<<16)|code;}
constexpr unsigned Mode(unsigned value) {return (value>>16)&0xFFu;}
constexpr unsigned Modifiers(unsigned value) {return (value>>24)&7u;}
}
