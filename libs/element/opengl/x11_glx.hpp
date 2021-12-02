#pragma once
#include "opengl.hpp"
namespace gl {
    extern Platform* glx_platform_new();
    extern void glx_platform_delete (Platform* p);
}
