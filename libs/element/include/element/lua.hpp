#pragma once

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include <map>
#include <string>

namespace element {
namespace lua {

using CFunction = lua_CFunction;
using PackageLoaderMap = std::map<std::string, CFunction>;

}}
