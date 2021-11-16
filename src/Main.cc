

#if EL_JUCE_MAIN
#include "Application.h"
START_JUCE_APPLICATION (Element::Application)
#else

#include <filesystem>
#include <iostream>
#include <memory>

#include <element/context.hpp>
#include <sol/sol.hpp>

namespace fs = std::filesystem;

static void setup_dll_dirs();

static std::string make_module_path (const char* name) {
    auto path = fs::current_path() / "build/modules";
    path /= name;
    return path.replace_extension ("element")
               .make_preferred()
               .string();
}

void parse_commandline (int ac, char* av[]) {
    
}


int main (int ac, char* argv[]) {
    setup_dll_dirs();
    // auto backend = std::make_unique<element::Backend>();
    sol::state state;
    state["test"] = []() {
        std::clog << "hello world\n";
    };
    state.script ("test()");
    return 0;
}

#if _WIN32
 #include <windows.h>
#endif

static void setup_dll_dirs() {
   #if _WIN32
    auto path = fs::current_path() / fs::path ("build/bin");
    std::clog << int (fs::exists (path)) << " " << path.string() << std::endl;
    SetDefaultDllDirectories (LOAD_LIBRARY_SEARCH_USER_DIRS);
    AddDllDirectory (path.make_preferred().c_str());
   #endif
}


#endif
