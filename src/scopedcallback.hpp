/*
    This file is part of Element
    Copyright (C) 2020 Kushview, LLC.  All rights reserved.
    
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
*/

#pragma once

namespace element {

/** A scoped function callback. Similar to goto, this calls a function when it goes out of scope.
 
    You should not perform complex operations with this.  It is meant to ensure a callback  when
    the calling function returns.
 */
class ScopedCallback
{
public:
    /** Create a new ScopedCallback.
       @param f    The function to call
    */
    explicit ScopedCallback (std::function<void()> f)
        : callback (f) {}

    ~ScopedCallback()
    {
        if (callback)
            callback();
    }

private:
    std::function<void()> callback;
};

} // namespace element
