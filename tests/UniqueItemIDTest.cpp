#include "UniqueItemID.h"
#include <stdexcept>
#include <iostream>
void Check(bool b) { if(!b) throw std::runtime_error("Unique item ID regression"); }
int main() {
    using Gallery::FindAvailableItemID;
    std::set<std::uint16_t> reserved{1,2,4};
    // A constant engine result must not prevent cleanup registration when the
    // lowest inventory-free ID is still reserved for an absent enchanted item.
    auto first=FindAvailableItemID(1,reserved); Check(first==3);
    reserved.insert(first); Check(FindAvailableItemID(1,reserved)==5);
    Check(FindAvailableItemID(0,reserved)==0);
    Check(FindAvailableItemID(65535,{65535,1})==2);
    std::set<std::uint16_t> full;
    for(unsigned n=1;n<=65535;++n) full.insert(static_cast<std::uint16_t>(n));
    Check(FindAvailableItemID(1,full)==0);
    full.erase(4096); Check(FindAvailableItemID(5000,full)==4096);
    Check(FindAvailableItemID(17,{})==17);
    std::cout << "ID collision, repeated candidate, wraparound and exhaustion checks passed\n";
}
