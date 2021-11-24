
#include <map>
#include <element/lua.hpp>
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
