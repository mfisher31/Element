/*
    This file is part of Element
    Copyright (C) 2020  Kushview, LLC.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#pragma once

#include "sol/sol.hpp"

namespace element {

class ScriptInstance
{
public:
    ScriptInstance() = default;
    virtual ~ScriptInstance() = default;

    void cleanup()
    {
        if (! object.valid())
            return;

        switch (object.get_type())
        {
            case sol::type::table: {
                auto tbl = object.as<sol::table>();
                if (sol::function f = tbl["cleanup"])
                    f (object);
                break;
            }
            default:
                break;
        }
    }

private:
    sol::object object;
};

} // namespace element
