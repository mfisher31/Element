/// A rectangle.
// The value type for this is a 32 bit float.
// @classmod el.Rectangle
// @pragma nostrip

#include <element/lua/rectangle.hpp>
#include "lua_helpers.hpp"

#define LKV_TYPE_NAME_RECTANGLE "Rectangle"

using namespace juce;

extern "C" EL_API
int luaopen_el_Rectangle (lua_State* L) {
    using R = Rectangle<float>;

    auto M = element::lua::new_rectangle<float> (L, LKV_TYPE_NAME_RECTANGLE,
        sol::meta_method::to_string, [](R& self) {
            return element::lua::to_string (self, LKV_TYPE_NAME_RECTANGLE);
        }
    );

    sol::stack::push (L, M);
    return 1;
}
