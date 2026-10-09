#pragma once
#include <cstdint>
namespace Gallery::Craft {
enum class Kind {None, Smithing, Alchemy, Enchanting};
inline int SlotFor(Kind kind) {return kind==Kind::Smithing?23:kind==Kind::Alchemy?24:kind==Kind::Enchanting?25:-1;}
inline const char* Name(Kind kind) {return kind==Kind::Enchanting?"Enchanting":kind==Kind::Alchemy?"Alchemy":kind==Kind::Smithing?"Smithing":"None";}
// No engine pointers or persistence. One attempt per actual furniture session.
struct Session {
    std::uint32_t target{};
    std::uint64_t started{};
    unsigned menuSerial{};
    Kind kind=Kind::None;
    bool occupied{}, exited{}, attempted{}, changed{}, applied{};
    bool Begin(std::uint32_t id,std::uint64_t now,unsigned serial,Kind selected=Kind::Smithing) {
        if(!id || target || SlotFor(selected)<0) return false;
        *this={};target=id;started=now;menuSerial=serial;kind=selected;return true;
    }
    bool Finished(std::uint64_t now,bool sameFurniture,bool normal,bool menuOpen) {
        occupied|=sameFurniture;
        if(menuOpen || !normal) return false;
        if(exited) return true; // Some setups retain the last furniture handle after standing.
        if(sameFurniture) return false;
        return exited || occupied || now-started>=10000;
    }
    bool Try(bool sameFurniture,bool menuOpen,unsigned serial,bool allowed) {
        if(!target || attempted) return false;
        if(menuOpen || serial!=menuSerial || !allowed || exited) {attempted=true;return false;}
        if(!sameFurniture) return false;
        attempted=true;return true;
    }
};
}
