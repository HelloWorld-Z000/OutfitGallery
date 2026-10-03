#include "PreferredItem.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
using namespace Gallery;
void Check(bool ok) {if(!ok) throw std::runtime_error("preferred item selection failed");}
int main() {
    PreferredItem wanted{true,1234,42};
    Check(FindPreferred(wanted,1234,{{42,true,true,1}})==0);
    Check(FindPreferred(wanted,9999,{{42,true,true,1}})==-1);
    Check(FindPreferred(wanted,1234,{{42,true,true,1},{42,true,true,1}})==-1);
    Check(FindPreferred(wanted,1234,{{42,true,false,1}})==-1);
    Check(FindPreferred(wanted,1234,{{42,false,true,1}})==-1);
    Check(FindPreferred(wanted,1234,{{42,true,true,2}})==-1);
    Check(FindPreferred(wanted,0,{{42,true,true,1}})==-1);
    Check(FindPreferred({},1234,{{42,true,true,1}})==-1);
    PreferredItem fingerprinted{true,1234,42,"effects-A"};
    Check(FindPreferred(fingerprinted,1234,{{0,false,true,1,"effects-A"}})==0);
    Check(FindPreferred(fingerprinted,1234,{{7,false,true,1,"effects-A"}})==0);
    Check(FindPreferred(fingerprinted,1234,{{0,false,true,1,"effects-B"}})==-1);
    Check(FindPreferred(fingerprinted,1234,{{0,false,true,1,"effects-A"},{0,false,true,1,"effects-A"}})==-1);
    Check(FindPreferred(fingerprinted,1234,{{0,false,true,2,"effects-A"}})==-1);
    Check(FindPreferred(fingerprinted,1234,{{0,false,false,1,"effects-A"}})==-1);
    Check(FindPreferred(fingerprinted,9999,{{0,false,true,1,"effects-A"}})==-1);
    Check(FindPreferred(fingerprinted,0,{{0,false,true,1,"effects-A"}})==-1);
    // Exact identity still wins among otherwise equivalent copies.
    Check(FindPreferred(fingerprinted,1234,{{0,false,true,1,"effects-A"},{42,true,true,1,"effects-A"}})==1);
    // A broken/duplicate exact identity must not be bypassed with a signature.
    Check(FindPreferred(fingerprinted,1234,{{42,true,true,2,"effects-B"},{0,false,true,1,"effects-A"}})==-1);
    Check(FindPreferred(wanted,1234,{{0,false,true,1,"effects-A"}})==-1);
    for(int size:{100,10000,100000}) {
        std::vector<PreferredCandidate> candidates(size,{1,true,true,1});
        candidates.back().id=42;
        const auto start=std::chrono::steady_clock::now();
        for(int repeat=0;repeat<100;++repeat) Check(FindPreferred(wanted,1234,candidates)==size-1);
        const auto us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count()/100;
        std::cout<<"Synthetic selection candidates="<<size<<" mean_us="<<us<<" (not engine inventory/equip timing)\n";
    }
}
