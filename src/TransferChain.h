#pragma once
#include "TransferObservation.h"

namespace Gallery {
// Session-only transfer evidence; never serialized into the deletion ledger.
struct TransferChain {
    struct Entry {
        ManagedIdentity current{}, previous{};
        unsigned hops{};
        bool awaiting{};
        const char* failure{};
    };
    std::map<ManagedIdentity,Entry> entries;
    static constexpr unsigned limit=128;
    bool stopped{};
    void Clear() {entries.clear(); stopped=false;}
    void Stop() {stopped=true; for(auto& [root,e]:entries) e.failure="observation-stopped";}
    void Change(const ManagedLedger& ledger,ManagedIdentity oldID,ManagedIdentity newID) {
        if(stopped || oldID==newID) return;
        // An unrelated notification targeting a tracked identity destroys exclusivity.
        bool collision=false;
        for(auto& [root,e]:entries) if(e.current==newID && e.current!=oldID) {
            e.failure="identity-collision"; collision=true;
        }
        auto found=entries.end(); unsigned matches=0;
        for(auto it=entries.begin();it!=entries.end();++it)
            if(it->second.current==oldID && !it->second.failure) {found=it; ++matches;}
        if(matches>1) {for(auto& [root,e]:entries) if(e.current==oldID) e.failure="ambiguous-source"; return;}
        if(!matches) {
            // Seed before the outgoing container event releases the original claim.
            if(!ledger.items.contains(oldID) || entries.contains(oldID)) return;
            if(entries.size()>=limit) {Stop(); return;}
            found=entries.emplace(oldID,Entry{oldID}).first;
        }
        auto& e=found->second;
        if(collision) {e.failure="identity-collision"; return;}
        if(e.awaiting) {e.failure="missing-container-confirmation"; return;}
        if(!newID.owner || !newID.id || newID.form!=oldID.form) {e.failure="identity-ended"; return;}
        if(e.hops>=64) {e.failure="hop-limit"; return;}
        for(auto& [root,other]:entries) if(&other!=&e && other.current==newID && !other.failure) {
            other.failure=e.failure="identity-collision";
        }
        if(ledger.items.contains(newID)) e.failure="ledger-identity-collision";
        if(e.failure) return;
        e.previous=oldID; e.current=newID; e.awaiting=true; ++e.hops;
    }
    void Retire(const ManagedIdentity& current) {
        for(auto& [root,e]:entries) if(e.current==current) e.failure="claim-retired";
    }
    bool Ready(const ManagedIdentity& current,bool enabled) const {
        if(!enabled || stopped || !current.owner || !current.id) return false;
        unsigned matches=0;
        for(const auto& [root,e]:entries) if(e.current==current) {
            if(e.failure || e.awaiting || e.hops<2 || root.owner!=current.owner) return false;
            ++matches;
        }
        return matches==1;
    }
    bool Consume(const ManagedIdentity& current,bool enabled) {
        if(!Ready(current,enabled)) return false;
        Retire(current); return true;
    }
    void Container(std::uint32_t form,std::uint32_t from,std::uint32_t to,int count,std::uint16_t uid) {
        if(stopped || !from || from==to) return;
        for(auto& [root,e]:entries) {
            if(e.failure || e.current.form!=form) continue;
            if(e.awaiting && e.previous.owner==from) {
                if(e.current.owner==to && e.current.id==uid && count==1) e.awaiting=false;
                else e.failure="container-mismatch";
            } else if(!e.awaiting && e.current.owner==from) {
                // Includes container-first delivery; do not guess a connection.
                e.failure="missing-id-notification";
            }
        }
    }
    const char* Classify(const ManagedIdentity& root,int total,const std::vector<TransferCandidate>& candidates) const {
        if(stopped) return "observation-stopped";
        const auto it=entries.find(root);
        if(it==entries.end()) return "no-chain";
        const auto& e=it->second;
        if(e.failure) return e.failure;
        if(e.awaiting) return "awaiting-container";
        if(e.current.owner!=root.owner) return "still-outside";
        if(e.hops<2) return "incomplete-roundtrip";
        return ClassifyTransfer(e.current,total,candidates);
    }
};
}
