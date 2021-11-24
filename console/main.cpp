

#include <cstring>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

#include <element/context.hpp>
#include <element/lua.hpp>
#include <sol/sol.hpp>

namespace fs = std::filesystem;

static void setup_dll_dirs();

static std::string make_module_path (const char* name)
{
    auto path = fs::current_path() / "build/modules";
    path /= name;
    return path.replace_extension ("element")
        .make_preferred()
        .string();
}

class MainBackend : public element::Context {
};
std::unique_ptr<MainBackend> backend;

int run_ui (int ac, const char* av[])
{
    backend->test_open_module ("org.lvtk.JLV2");
    backend->test_open_module ("el.UI");
    backend->test_load_modules();
    return backend->test_main ("el.UI", ac, av);
}

extern int element_console_main (lua_State*, int, const char**);
int run_console (int ac, const char* av[])
{
    sol::state_view view ((lua_State*) backend->test_lua_state());
    
    view["import"] = [](const char* name) -> bool {
        backend->test_open_module (name);
        return true;
    };

    view["main"] = [](const char* name) -> int {
        std::vector<const char*> args;
        args.push_back ("element");
        backend->test_load_modules();
        return backend->test_main (name, 1, args.data());
    };

    return element_console_main (view, ac, av);
}

int main (int ac, char* av[])
{
    bool console = true;
    setup_dll_dirs();
    backend = std::make_unique<MainBackend>();
    backend->test_add_module_search_path (fs::path (
                                              fs::current_path() / "build/modules")
                                              .make_preferred()
                                              .string());
    backend->test_discover_modules();

    std::vector<const char*> cli;
    for (int i = 0; i < ac; ++i)
        cli.push_back (av[i]);

    return console ? run_console (cli.size(), cli.data())
                   : run_ui (cli.size(), cli.data());
}

#if _WIN32
#include <windows.h>
#endif

static void setup_dll_dirs()
{
#if _WIN32
    auto path = fs::current_path() / fs::path ("build/bin");
    std::clog << int (fs::exists (path)) << " " << path.string() << std::endl;
    SetDefaultDllDirectories (LOAD_LIBRARY_SEARCH_USER_DIRS);
    AddDllDirectory (path.make_preferred().c_str());
#endif
}
