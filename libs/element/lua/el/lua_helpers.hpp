#pragma once

#include <ios>
#include <string>
#include <sstream>

#include <element/element.h>

namespace element {
namespace lua {

template<class T>
inline static std::string to_string (T& self, const char* name) {
    std::stringstream s;
    s << "el." << name << ": 0x" << std::hex << (intptr_t) &self;
    return s.str();
}

}
}
