#include "TransferObservation.h"
#include <stdexcept>
#include <string_view>
#include <iostream>
int main() {
    using namespace Gallery;
    auto check=[](bool pass){if(!pass) throw std::runtime_error("transfer observation regression");};
    try {
        ManagedIdentity key{42,0x14,7}; ManagedLedger ledger; ledger.items.insert(key);
        TransferObservation observation;
        check(observation.Capture(ledger,42,0x14,0x14)==0);
        check(observation.Capture(ledger,42,0,0x14)==0);
        check(observation.Capture(ledger,42,0x14,99)==1);
        check(ledger.items.contains(key)); // observation itself cannot mutate ownership
        check(ledger.Transfer(42,0x14,99)==1 && ledger.items.empty());
        ledger.Transfer(42,99,0x14);
        check(ledger.items.empty() && observation.departed.contains(key)); // no resurrection
        auto result=[&](int total,std::vector<TransferCandidate> c,std::string_view expected){check(ClassifyTransfer(key,total,c)==expected);};
        result(0,{},"absent");
        result(1,{{0,0,1,false,false}},"identity-not-found");
        result(1,{{0x14,8,1,false,false}},"identity-not-found"); // same type, private item
        result(1,{{99,7,1,false,false}},"identity-not-found");
        result(1,{{0x14,7,1,false,false}},"matching-unworn-observation-only");
        result(2,{{0x14,7,1,false,false},{0x14,8,1,false,false}},"matching-unworn-observation-only");
        result(2,{{0x14,7,1,false,false},{0x14,7,1,false,false}},"ambiguous");
        result(2,{{0x14,7,2,false,false}},"ambiguous");
        result(1,{{0x14,7,1,true,false}},"matching-protected");
        result(1,{{0x14,7,1,false,true}},"matching-worn");
        for(unsigned n=0;n<200;++n) {ledger.items.insert({n,0x14,static_cast<std::uint16_t>(n+1)});observation.Capture(ledger,n,0x14,99);}
        check(observation.departed.size()==TransferObservation::limit);
        observation.Clear();check(observation.departed.empty());
        std::cout<<"PASS: departure retention without reclaim, private identity mismatch, count/duplicate/protection checks, bound and reset\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
