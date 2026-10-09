#include "TransferChain.h"
#include <stdexcept>
#include <string_view>
#include <iostream>
int main() {
    using namespace Gallery;
    auto check=[](bool b){if(!b) throw std::runtime_error("chain regression");};
    try {
        ManagedLedger ledger; ManagedIdentity a{42,20,11}, box{42,99,1}, back{42,20,3};
        ledger.items.insert(a); TransferChain c;
        c.Change(ledger,a,box); check(ledger.items.contains(a));
        c.Container(42,20,99,1,1); ledger.Transfer(42,20,99);
        check(std::string_view(c.Classify(a,0,{}))=="still-outside");
        // Same-form private item returns; it cannot advance the generated item's chain.
        c.Change(ledger,{42,88,8},{42,20,7}); c.Container(42,88,20,1,7);
        check(std::string_view(c.Classify(a,1,{{20,7,1,false,false}}))=="still-outside");
        c.Change(ledger,box,back); c.Container(42,99,20,1,3);
        check(ledger.items.empty());
        check(c.Ready(back,true) && !c.Ready(back,false));
        check(!c.Consume(back,false));
        check(!c.Ready({42,20,7},true)); // same-form private item is not authorized
        auto result=[&](std::vector<TransferCandidate> items,std::string_view expected){check(c.Classify(a,2,items)==expected);};
        result({{20,3,1,false,false},{20,7,1,false,false}},"matching-unworn-observation-only");
        result({{20,7,1,false,false}},"identity-not-found");
        result({{20,3,1,false,false},{20,3,1,false,false}},"ambiguous");
        result({{20,3,1,true,false}},"matching-protected");
        result({{20,3,1,false,true}},"matching-worn");
        result({{20,3,2,false,false}},"ambiguous");
        // Second roundtrip, different box.
        c.Change(ledger,back,{42,88,4}); c.Container(42,20,88,1,4);
        c.Change(ledger,{42,88,4},{42,20,5}); c.Container(42,88,20,1,5);
        check(c.entries.at(a).hops==4);
        c.Container(42,20,99,1,9); check(c.entries.at(a).failure!=nullptr);
        c.Clear(); check(c.entries.empty() && !c.stopped);
        ledger.items.insert(a);
        c.Change(ledger,a,box); c.Change(ledger,box,back);
        check(std::string_view(c.entries.at(a).failure)=="missing-container-confirmation");
        c.Clear(); c.Change(ledger,a,box); c.Container(42,20,99,2,1);
        check(std::string_view(c.entries.at(a).failure)=="container-mismatch");
        c.Clear(); c.Change(ledger,a,{42,0,0}); check(c.entries.at(a).failure!=nullptr);
        c.Clear(); ManagedIdentity b{42,20,12}; ledger.items.insert(b);
        c.Change(ledger,a,box); c.Container(42,20,99,1,1);
        c.Change(ledger,b,box); check(c.entries.at(a).failure && c.entries.at(b).failure);
        c.Clear(); ledger.Clear();
        for(unsigned n=1;n<=129;++n) {ManagedIdentity root{n,20,1}; ledger.items.insert(root); c.Change(ledger,root,{n,99,2});}
        check(c.entries.size()==128 && c.stopped);
        c.Clear(); check(!c.stopped);
        // Replay the two-item event sequence from the user's diagnostic 2 log.
        ledger.Clear();
        ManagedIdentity corset{0xFE33F802,20,11}, heels{0xFE33F804,20,12};
        ledger.items.insert(corset); ledger.items.insert(heels);
        c.Change(ledger,corset,{corset.form,0x3804958B,1});
        c.Container(corset.form,20,0x3804958B,1,1); ledger.Transfer(corset.form,20,0x3804958B);
        c.Change(ledger,heels,{heels.form,0x3804958B,2});
        c.Container(heels.form,20,0x3804958B,1,2); ledger.Transfer(heels.form,20,0x3804958B);
        c.Change(ledger,{corset.form,0x3804958B,1},{corset.form,20,3});
        c.Container(corset.form,0x3804958B,20,1,3);
        c.Change(ledger,{heels.form,0x3804958B,2},{heels.form,20,4});
        c.Container(heels.form,0x3804958B,20,1,4);
        check(std::string_view(c.Classify(corset,1,{{20,3,1,false,false}}))=="matching-unworn-observation-only");
        check(std::string_view(c.Classify(heels,1,{{20,4,1,false,false}}))=="matching-unworn-observation-only");
        check(ledger.items.empty());
        check(c.Consume({corset.form,20,3},true));
        check(!c.Consume({corset.form,20,3},true)); // one-shot even before remove confirmation
        c.Retire({heels.form,20,4}); // protected/ambiguous items relinquish the transfer claim
        check(!c.Ready({heels.form,20,4},true));
        // Same form and same box, different private identity: never substitute it.
        c.Clear(); ledger.items.insert(a); c.Change(ledger,a,box); c.Container(42,20,99,1,1); ledger.Transfer(42,20,99);
        c.Change(ledger,{42,99,8},{42,20,7}); c.Container(42,99,20,1,7);
        check(std::string_view(c.Classify(a,1,{{20,7,1,false,false}}))!="matching-unworn-observation-only");
        check(!c.Consume({42,20,7},true));
        c.Clear(); check(!c.Ready(back,true)); // toggle/load cannot revive claims
        ledger.Clear(); ledger.items.insert(a);
        c.Change(ledger,a,box); c.Container(42,20,99,1,1); ledger.Transfer(42,20,99);
        c.Change(ledger,box,back); c.Container(42,99,20,1,3);
        c.Change(ledger,{42,88,8},back); // unrelated item collides with current identity
        check(!c.Ready(back,true));
        c.Clear(); ledger.items.insert(a);
        c.Change(ledger,a,box); c.Container(42,20,99,1,1); ledger.Transfer(42,20,99);
        ledger.items.insert(a); // another generated item reused the original identity
        c.Change(ledger,box,a); check(!c.Ready(a,true));
        std::cout<<"PASS: ID chain roundtrips, private item isolation, ambiguity, broken events, bounds; no ledger adoption\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
