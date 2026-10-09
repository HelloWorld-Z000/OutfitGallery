#include "ActivePluginLookup.h"
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
struct File {std::string_view name;std::uint8_t compileIndex;std::uint16_t smallFileCompileIndex;};
struct Handler {
 std::array<File,3> files{{{"Full.esp",0x20,0},{"OStim.esp",0xFE,0xABC},{"Inactive.esp",0xFF,0}}};
 const File* LookupModByName(std::string_view name){for(const auto& f:files)if(f.name==name)return &f;return nullptr;}
 // Reproduce CommonLib's full-plugin-only lookup: it cannot find a light ESP.
 const File* LookupLoadedModByName(std::string_view name){const auto* f=LookupModByName(name);return f && f->compileIndex<0xFE?f:nullptr;}
};
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){try{
 Handler h;
 check(!h.LookupLoadedModByName("OStim.esp"),"old path reproduces light-ESP miss");
 const auto* light=Gallery::FindActivePlugin(&h,"OStim.esp");
 check(light==&h.files[1] && light->compileIndex==0xFE && light->smallFileCompileIndex==0xABC,"active light ESP found without dropping sub-index");
 check(Gallery::FindActivePlugin(&h,"Full.esp")==&h.files[0],"full ESP still found");
 check(!Gallery::FindActivePlugin(&h,"Inactive.esp"),"inactive file does not count as installed runtime plugin");
 check(!Gallery::FindActivePlugin(&h,"Absent.esp"),"absent plugin optional");
 check(!Gallery::FindActivePlugin(static_cast<Handler*>(nullptr),"OStim.esp"),"missing data handler safe");
 h.files[1].compileIndex=0xFF;
 check(!Gallery::FindActivePlugin(&h,"OStim.esp"),"disabled light ESP excluded");
 std::cout<<"PASS: full/light/inactive/absent plugin lookup regression\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what();return 1;}}
