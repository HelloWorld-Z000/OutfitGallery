#pragma once
#include <bit>
#include <cmath>
#include <nlohmann/json.hpp>

namespace Gallery {
// Compare data, never reconstruct or modify effects. Dynamic enchantment FormIDs
// are not stable across sessions; base effects use plugin + local ID instead.
inline std::string EnchantmentSignature(RE::ExtraDataList* extra) {
    if(!extra) return {};
    const auto* e=extra->GetByType<RE::ExtraEnchantment>();
    if(!e || !e->enchantment) return {};
    const auto* enchant=e->enchantment;
    if(enchant->effects.empty() || enchant->effects.size()>32) return {};
    auto key=[](RE::TESForm* form)->nlohmann::json {
        if(!form) return nullptr;
        auto* file=form->GetFile(0);
        if(!file || (form->GetFormID()>>24)==0xFF) throw std::runtime_error("Unstable effect reference");
        return {std::string(file->GetFilename()),form->GetFormID() & (file->IsLight()?0xFFFu:0xFFFFFFu)};
    };
    try {
        nlohmann::json effects=nlohmann::json::array();
        for(const auto* effect:enchant->effects) {
            if(!effect || !effect->baseEffect || effect->conditions.head ||
                !std::isfinite(effect->effectItem.magnitude) || !std::isfinite(effect->cost)) return {};
            effects.push_back({key(effect->baseEffect),std::bit_cast<std::uint32_t>(effect->effectItem.magnitude),
                effect->effectItem.area,effect->effectItem.duration,std::bit_cast<std::uint32_t>(effect->cost)});
        }
        const auto& d=enchant->data;
        if(!std::isfinite(d.chargeTime)) return {};
        nlohmann::json result={1,effects,d.costOverride,d.flags.underlying(),static_cast<int>(d.castingType),
            d.chargeOverride,static_cast<int>(d.delivery),static_cast<int>(d.spellType),
            std::bit_cast<std::uint32_t>(d.chargeTime),key(d.baseEnchantment),key(d.wornRestrictions),
            e->charge,e->removeOnUnequip};
        // Distinguish tempering when the same armor has several enchanted copies.
        const auto* health=extra->GetByType<RE::ExtraHealth>();
        const float value=health?health->health:1.f;
        if(!std::isfinite(value)) return {};
        result.push_back(std::bit_cast<std::uint32_t>(value));
        auto signature=result.dump();
        return signature.size()<=16384?signature:std::string{};
    } catch(const std::exception&) { return {}; }
}
}
