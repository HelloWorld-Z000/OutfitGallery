#pragma once
#include <nlohmann/json.hpp>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>
#include <regex>

namespace Gallery::Translation {
// Exact format tokens, in order. Do not allow translators to introduce varargs,
// positional arguments, %n, width changes or hidden ImGui IDs.
inline bool Tokens(std::string_view text,std::vector<std::string>& result) {
    static const std::regex conversion(R"(%[-+ #0]*[0-9]*(\.[0-9]+)?(hh|ll|h|l|j|z|t|L)?[diuoxXfFeEgGaAcsp])");
    for(std::size_t i=0;i<text.size();++i) {
        if(text[i]!='%') continue;
        if(i+1<text.size() && text[i+1]=='%') {result.emplace_back("%%");++i;continue;}
        std::match_results<std::string_view::const_iterator> match;
        if(!std::regex_search(text.begin()+i,text.end(),match,conversion,std::regex_constants::match_continuous)) return false;
        result.emplace_back(match.str());i+=match.length()-1;
    }
    return true;
}
inline bool Valid(std::string_view key,std::string_view value) {
    if(value.empty() || value.size()>8192 || value.find("##")!=value.npos) return false;
    for(unsigned char c:value) if(c<32 && c!='\n' && c!='\t') return false;
    std::vector<std::string> a,b;
    return Tokens(key,a) && Tokens(value,b) && a==b;
}
struct Pack {std::map<std::string,std::string> strings;std::size_t rejected{};};
inline Pack Parse(std::string_view input,const std::set<std::string>& keys) {
    if(input.size()>4*1024*1024) throw std::runtime_error("Translation file exceeds 4 MiB");
    const auto root=nlohmann::json::parse(input);
    if(!root.is_object() || root.value("schema",0)!=1 || !root.contains("strings") || !root.at("strings").is_object())
        throw std::runtime_error("Expected schema 1 and a strings object");
    Pack pack;
    for(const auto& [key,value]:root.at("strings").items()) {
        if(!keys.contains(key) || !value.is_string() || !Valid(key,value.get_ref<const std::string&>())) {++pack.rejected;continue;}
        pack.strings.emplace(key,value.get<std::string>());
    }
    return pack;
}
}
