#include "Presets.h"
#include "ExternalTranslation.h"
#include "TranslationKeys.h"
#include <fstream>
#include <map>
#include <iostream>
namespace SKSE::log {
template<class... Args> void info(const char*,Args&&...) {}
template<class... Args> void warn(const char*,Args&&...) {}
}
namespace Gallery {
#include "Localization.inc"
}
int main() {
    using namespace Gallery;
    const std::string original="Outfit restored: My outfit Registered enchantment not restored: My ring";
    language=0;
    if(StatusText(original)!=original) return 1;
    language=1;
    if(StatusText(original)!="装備の復元を確認しました：My outfit 登録した付呪は再現されていません：My ring") return 2;
    language=2;
    externalTranslation=Translation::Parse(R"({"schema":1,"strings":{"Outfit restored: ":"恢复：","Registered enchantment not restored: ":"附魔未恢复："}})",Translation::Keys());
    if(StatusText(original)!="恢复：My outfit 附魔未恢复：My ring") return 3;
    externalTranslation={};
    if(StatusText(original)!=original) return 4;
    std::cout<<"Enchantment notices: English, Japanese, external and fallback passed\n";
}
