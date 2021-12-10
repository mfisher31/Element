/** 
    This file is part of Element.
    Copyright (C) 2016-2021  Kushview, LLC.  All rights reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
**/

#include <element/lua.hpp>
#include <map>
#include <sol/sol.hpp>

extern "C" {
extern int luaopen_el_audio (lua_State*);
extern int luaopen_el_midi (lua_State*);
extern int luaopen_el_bytes (lua_State*);
extern int luaopen_el_round (lua_State*);
}

namespace element {
namespace lua {

void fill_builtins (PackageLoaderMap& pkgs)
{
    if (pkgs.find ("el.audio") != pkgs.end())
        return;
    pkgs.insert (
        { { "el.audio", luaopen_el_audio },
          { "el.bytes", luaopen_el_bytes },
          { "el.midi", luaopen_el_midi },
          { "el.round", luaopen_el_round } });
}

} // namespace lua
} // namespace element
