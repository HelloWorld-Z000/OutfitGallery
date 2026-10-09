#pragma once
#include <string_view>
namespace Gallery {
// LookupLoadedModByName scans only full plugins. LookupModByName includes light
// plugins, but also inactive files, so reject the unassigned compile index.
template<class Handler>
auto FindActivePlugin(Handler* data,std::string_view name) {
 auto* file=data?data->LookupModByName(name):nullptr;
 return file && file->compileIndex!=0xFF?file:nullptr;
}
}
