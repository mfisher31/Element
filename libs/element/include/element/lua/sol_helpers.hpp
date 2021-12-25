#pragma once

#include <lua.hpp>
#include <sol/sol.hpp>

namespace element {
namespace lua {

/** Removes a field from the table then clears it.
    @param tbl Input table
    @param field The field to remove. Lua type MUST be a table
*/
inline static sol::table remove_and_clear (sol::table tbl, const char* field) {
    auto F = tbl.get<sol::table> (field);
    tbl.clear();
    return F;
}

}
}
