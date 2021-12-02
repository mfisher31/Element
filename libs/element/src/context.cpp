
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "dynlib.h"
#include "element/context.hpp"
#include "element/graphics.h"
#include "element/plugin.h"
#include "scripting.hpp"
#include "video.hpp"

#include <sol/sol.hpp>

#include "manifest.hpp"
#include "search_path.hpp"

namespace element {

using FeatureMap = std::map<std::string, const void*>;
static FeatureMap map_features (elFeatures features)
{
    FeatureMap fm;
    EL_FEATURES_FOREACH (features, f)
        fm.insert ({ f->ID, f->data });
    return std::move (fm);
}

} // namespace element

//=============================================================================
namespace fs = std::filesystem;

namespace element {

class Module {
public:
    Module (const std::string& _bundlePath, Context& b, Scripting& s)
        : bundlePath (_bundlePath),
          backend (b),
          scripting (s)
    {
        manifest = read_module_manifest (bundlePath);
    }

    ~Module()
    {
        close();
    }

    std::string name() const noexcept { return manifest.name; }

    static const std::string library_extension()
    {
#if __APPLE__
        return ".dylib";
#elif _WIN32
        return ".dll";
#else
        return ".so";
#endif
    }

    bool is_open() const
    {
        return library != nullptr && mod != nullptr && handle != nullptr;
    }

    const void* extension (const std::string& ID) const noexcept {
        return mod && handle && mod->extension ? mod->extension (handle, ID.c_str())
            : nullptr;
    }

    bool open()
    {
        close();
        if (! is_open()) {
            fs::path libraryFile (bundlePath);
            libraryFile /= libraryFile.filename()
                               .replace_extension (library_extension());
            if (! fs::exists (libraryFile))
                return false;
            library = element_openlib (libraryFile.string().c_str());

            if (library != nullptr) {
                moduleFunction = (elDescriptorFunction)
                    element_getsym (library, "element_descriptor");
            } else {
                std::cout << "library couldn't open\n";
                if (auto str = dlerror()) {
                    std::cout << "error: " << str << std::endl;
                    std::free (str);
                }
            }

            if (moduleFunction) {
                mod = moduleFunction();
            } else {
                std::cout << "no descriptor function\n";
            }

            if (mod && mod->create) {
                handle = mod->create();
            }

            if (handle && mod->extension) {
                std::vector<std::string> mids;
                mids.push_back (EL_EXTENSION__Main);
                mids.push_back (EL_EXTENSION__LuaPackages);
                mids.push_back ("el.GraphicsDevice");
                for (auto& s : mids) {
                    if (auto data = mod->extension (handle, s.c_str())) {
                        elFeature feature = {
                            .ID = s.c_str(),
                            .data = (void*) data
                        };
                        handle_module_extension (feature);
                    }
                }
            }
        }

        return is_open();
    }

    void handle_module_extension (const elFeature& f)
    {
        bool handled = true;
        if (strcmp (f.ID, EL_EXTENSION__LuaPackages) == 0) {
            for (auto reg = (const luaL_Reg*) f.data; reg != nullptr && reg->name != nullptr && reg->func != nullptr; ++reg) {
                scripting.add_package (reg->name, reg->func);
            }
        } else if (strcmp (f.ID, EL_EXTENSION__Main) == 0) {
            main = (const elMain*) f.data;
        } else if (strcmp (f.ID, "el.GraphicsDevice") == 0) {
            backend.video->load_device_descriptor ((const egDeviceDescriptor*) f.data);
        } else {
            handled = false;
            for (const auto& ex : manifest.provides) {
                if (ex == f.ID) {
                    handled = true;
                    break;
                }
            }
        }

        if (! handled)
            std::clog << "unhandled module feature: " << f.ID << std::endl;
    }

    bool loaded() const noexcept { return has_loaded; }

    void load (elFeatures features)
    {
        if (has_loaded)
            return;

        has_loaded = true;
        if (handle && mod && mod->load) {
            mod->load (handle, features);
        }
    }

    void unload()
    {
        if (! has_loaded)
            return;

        has_loaded = false;
        if (handle && mod && mod->unload) {
            mod->unload (handle);
        }
    }

    FeatureMap public_extensions() const
    {
        FeatureMap e;
        for (const auto& exp : manifest.provides)
            if (auto data = mod->extension (handle, exp.c_str()))
                e.insert ({ exp, data });
        return std::move (e);
    }

    void close()
    {
        unload();

        if (handle != nullptr) {
            if (mod != nullptr && mod->destroy != nullptr)
                mod->destroy (handle);
            handle = nullptr;
        }

        mod = nullptr;
        main = nullptr;
        moduleFunction = nullptr;

        if (library != nullptr) {
            element_closelib (library);
            library = nullptr;
        }
    }

    const std::string& bundle_path() const noexcept { return bundlePath; }

    bool have_main() const noexcept { return handle != nullptr && main != nullptr; }
    int run_main (int argc, const char* argv[])
    {
        return have_main() ? main->main (handle, argc, argv) : -1;
    }

private:
    Context& backend;
    Scripting& scripting;
    Manifest manifest;
    const std::string bundlePath;
    void* library = nullptr;
    const elDescriptor* mod = nullptr;
    elHandle handle;
    elDescriptorFunction moduleFunction = nullptr;
    const elMain* main = nullptr;
    sol::state config;
    bool has_loaded = false;
};

class Modules {
public:
    using ptr_type = std::unique_ptr<Module>;
    using vector_type = std::vector<ptr_type>;
    Modules (Context& c) : backend (c) {}

    void add (ptr_type mod)
    {
        mods.push_back (std::move (mod));
    }

    void add (Module* mod) { add (ptr_type (mod)); }

    auto begin() const noexcept { return mods.begin(); }
    auto end() const noexcept { return mods.end(); }

    bool contains (const std::string& bp) const noexcept
    {
        auto result = std::find_if (begin(), end(), [bp] (const ptr_type& ptr) {
            return ptr->name() == bp;
        });

        return result != mods.end();
    }

    int discover()
    {
        if (discovered.size() > 0)
            return (int) discovered.size();

        for (auto const& entry : searchpath.find_folders (false, "*.element")) {
            Manifest manifest = read_module_manifest (entry.string());
            if (! manifest.name.empty())
                discovered.insert ({ manifest.name, entry.string() });
        }

        return (int) discovered.size();
    }

    void unload_all() {
        for (const auto& mod : mods) {
            mod->unload();
            mod->close();
        }
    }

private:
    friend class Context;
    Context& backend;
    vector_type mods;
    SearchPath searchpath;
    std::map<std::string, std::string> discovered;
};

Context::Context()
{
    modules.reset (new Modules (*this));
    scripting.reset (new Scripting());
    video.reset (new Video());
    video->start_thread();
}

Context::~Context()
{
    video->stop_thread();
    scripting.reset();
    video.reset();
    modules->unload_all();
    modules.reset();   
}

void Context::test_open_module (const std::string& ID)
{
    if (modules->contains (ID))
        return;

    auto it = modules->discovered.find (ID);
    if (it == modules->discovered.end()) {
        std::clog << "module not found: " << ID << std::endl;
        return;
    }

    if (auto mod = std::make_unique<Module> (it->second, *this, *scripting)) {
        if (mod->open()) {
            std::clog << "module opened: " << mod->name() << std::endl;
            modules->add (std::move (mod));
        } else {
            std::clog << "could not open module: " << mod->name() << std::endl;
        }
    }
}

void Context::test_load_modules()
{
    std::vector<const elFeature*> features;

    for (const auto& mod : *modules) {
        for (const auto& e : mod->public_extensions()) {
            auto f = (elFeature*) std::malloc (sizeof (elFeature));
            features.push_back (f);
            f->ID = strdup (e.first.c_str());
            f->data = (void*) e.second;
        }
    }

    features.push_back ((elFeature*) nullptr);
    elFeatures fptr = &features.front();

    for (const auto& mod : *modules) {
        if (! mod->loaded())
            mod->load (fptr);
    }

    for (auto f : features) {
        if (nullptr == f)
            continue;
        std::free ((void*) f->ID);
        std::free ((void*) f);
    }
    features.clear();
}

void* Context::test_lua_state() const
{
    return scripting->root_state();
}

int Context::test_main (const std::string module_id, int argc, const char* argv[])
{
    for (const auto& m : *modules) {
        if (m->name().find (module_id) != std::string::npos)
            return m->run_main (argc, argv);
    }

    return -1;
}

void Context::test_add_module_search_path (const std::string& path)
{
    auto& sp = modules->searchpath;
    sp.add (path);
}

void Context::test_discover_modules()
{
    modules->discover();
}

} // namespace element

struct elContext {
    elContext()
        : impl (new element::Context(), delete_ptr) {}

    elContext (element::Context* ctx)
        : impl (ctx, release_ptr) {}

    ~elContext()
    {
        impl.reset();
    }

    bool object_is_owned() const noexcept { return impl.get_deleter() == delete_ptr; }
    element::Context& object() noexcept { return *impl; }

private:
    using Deleter = void (*) (element::Context*);
    using Pointer = std::unique_ptr<element::Context, Deleter>;
    Pointer impl;
    static void release_ptr (element::Context*) {}
    static void delete_ptr (element::Context* obj) { delete obj; }
};

elContext* element_context_new()
{
    return new elContext();
}

void element_context_free (elContext* ctx)
{
    auto& obj = ctx->object();
    delete ctx;
}
