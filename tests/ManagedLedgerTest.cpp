#include "ManagedLedger.h"
#include "FollowerOutfitState.h"
#include <iostream>
#include <stdexcept>

void Require(bool condition,const char* message) {
    if(!condition) throw std::runtime_error(message);
}
int main() {
    using namespace Gallery;
    constexpr std::uint32_t player=0x14, alice=0x123456, bob=0x234567, armor=0xABC;
    ManagedLedger ledger;
    // Identical item/instance numbers on distinct actors must remain isolated.
    ledger.items={{armor,player,1},{armor,alice,1},{armor,bob,1},{0xDEF,alice,2}};
    Require(ledger.ForActor(player).size()==1,"player scope");
    Require(ledger.ForActor(alice).size()==2,"follower scope");
    Require(ledger.ForActor(0x999).empty(),"untracked inventory cannot be claimed");
    ledger.pending[{alice,armor}]=1;
    ledger.pending[{bob,armor}]=1;
    Require(ledger.Transfer(armor,0,alice)==0,"creation is not a transfer out");
    Require(ledger.pending.contains({alice,armor}),"creation preserves pending claim");
    Require(ledger.Transfer(armor,alice,alice)==0,"same-container notification");
    Require(ledger.Transfer(armor,alice,player)==1,"give relinquishes source claim");
    Require(!ledger.pending.contains({alice,armor}),"outgoing transfer cancels pending tag");
    Require(ledger.ForActor(alice).size()==1,"other armor remains tracked");
    Require(ledger.ForActor(bob).size()==1,"other follower unaffected");
    Require(ledger.ForActor(player).size()==1,"incoming item not claimed by receiver");
    ledger.Transfer(armor,player,alice);
    Require(ledger.ForActor(alice).size()==1,"transfer back never resurrects ownership");
    Require(ledger.ForActor(bob).size()==1,"roundtrip cannot erase third actor claim");
    ledger.pending[{alice,0xDEF}]=1;
    auto pending=ledger.TakePending(alice);
    Require(pending.size()==1 && pending.at(0xDEF)==1,"take correct actor additions");
    Require(ledger.pending.contains({bob,armor}),"pending additions isolate actors");
    ledger.Begin(alice);
    Require(ledger.pending.contains({bob,armor}),"begin does not clear another actor");
    Require(ledger.Transfer(armor,bob,0)==1,"drop relinquishes claim");
    Require(!ledger.pending.contains({bob,armor}),"drop cancels pending claim");
    ledger.Clear();
    Require(ledger.items.empty() && ledger.pending.empty(),"revert clears save state");
    FollowerOutfitState maintained;
    Require(maintained.Observe(false,1000),"first travel correction allowed");
    Require(maintained.nextCheck==6000,"correction schedules backoff");
    Require(maintained.Observe(false,6000),"second correction allowed");
    Require(maintained.Observe(false,11000),"third correction allowed");
    Require(!maintained.Observe(false,16000),"repeated external outfit control stops retries");
    Require(!maintained.Observe(true,20000),"matching gear needs no mutation");
    Require(!maintained.Observe(true,50000),"stable period needs no mutation");
    Require(maintained.corrections==0,"stable outfit clears conflict budget");
    Require(maintained.Observe(false,55000),"later travel can be corrected again");
    auto identity=[](auto old,auto& resolved){resolved=old; return true;};
    const std::vector<std::uint32_t> saved{alice,2,armor,0xDEF};
    auto loaded=DecodeFollowerOutfit(saved,identity);
    Require(loaded && loaded->first==alice && loaded->second==std::vector<std::uint32_t>{armor,0xDEF},"outfit state load");
    loaded=DecodeFollowerOutfit(saved,[](auto old,auto& resolved){resolved=old+0x1000000; return true;});
    Require(loaded && loaded->first==alice+0x1000000 && loaded->second.front()==armor+0x1000000,"load-order remaps actor AND armor");
    Require(!DecodeFollowerOutfit(saved,[](auto old,auto& resolved){resolved=old; return old!=0xDEF;}),"missing armor rejects whole outfit");
    Require(!DecodeFollowerOutfit(std::vector<std::uint32_t>{player,1,armor},identity),"player cannot be a maintained follower");
    Require(!DecodeFollowerOutfit(std::vector<std::uint32_t>{alice,2,armor},identity),"truncated outfit rejected");
    Require(!DecodeFollowerOutfit(std::vector<std::uint32_t>{alice,0},identity),"empty outfit rejected");
    std::cout << "Managed ledger actor isolation and transfers PASS\n";
}
