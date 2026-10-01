#include "ExternalTranslation.h"
#include "TranslationKeys.h"
#include <iostream>
void Check(bool ok) {if(!ok) throw std::runtime_error("Translation test failed");}
int main() {
    using namespace Gallery::Translation;
    try {
        Check(Valid("Follower: %s","対象: %s"));
        Check(Valid("%u / %.1f","%u 個 / %.1f"));
        for(const auto& bad:{"%n","%ls","%s %s","%1$s","%*s","%999s","%","abc##id",""}) Check(!Valid("%s",bad));
        Check(!Valid("%s %u","%u %s"));
        Check(!Valid("Hello",std::string("x\0y",3)));
        Check(!Valid("Hello",std::string(8193,'x')));
        Check(Valid("100%%","100%%"));
        Check(!Valid("Hello","100%"));
        auto p=Parse(R"({"schema":1,"strings":{"Options":"設定","Follower: %s":"%n","Save outfit":123,"unknown":"x"}})",Keys());
        Check(p.strings.at("Options")=="設定" && p.strings.size()==1 && p.rejected==3);
        for(const std::string input:{"{", "[]", R"({"schema":2,"strings":{}})",R"({"schema":1,"strings":[]})"}) {
            bool rejected=false;try{(void)Parse(input,Keys());}catch(...){rejected=true;}Check(rejected);
        }
        auto english=nlohmann::json{{"schema",1},{"strings",nlohmann::json::object()}};
        for(const auto& key:Keys()) english["strings"][key]=key;
        auto full=Parse(english.dump(),Keys());
        Check(full.rejected==0 && full.strings.size()==Keys().size());
        Check(Parse(R"({"schema":1,"strings":{}})",Keys()).strings.empty());
        std::cout<<"Translation validation passed: "<<Keys().size()<<" template entries\n";
    } catch(const std::exception& e) {std::cerr<<e.what();return 1;}
}
