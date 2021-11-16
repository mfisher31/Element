
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <vector>

#include "element/context.hpp"
#include "element/plugin.h"
#include "dynlib.h"

namespace element {

Context::Context() {}
Context::~Context() {}

class DefaultBackend : public Backend {
public:
    DefaultBackend() {}
    ~DefaultBackend() {}

    const FeatureStore& features() const noexcept { return feats; }

private:
    friend class Context;
    FeatureStore feats;
};

}

//=============================================================================
namespace fs = std::filesystem;

namespace element {

//=============================================================================
class Module
{
public:
    Module (const std::string& _bundlePath, Backend& b)
        : bundlePath (_bundlePath),
          backend (b)
    {
    }

    ~Module()
    {
        close();
    }

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
        return library != nullptr &&
               mod != nullptr && 
               handle != nullptr; 
    }

    bool open()
    {
        close();
        if (! is_open())
        {
            fs::path libraryFile (bundlePath);
            libraryFile /= libraryFile.filename()
                .replace_extension (library_extension());
            if (! fs::exists (libraryFile))
                return false;
            library = element_openlib (libraryFile.string().c_str());
            
            if (library != nullptr)
            {
                moduleFunction = (ELDescriptorFunction)
                    element_getsym (library, "eds_descriptor");
            }
            else
            {
                std::cout << "library couldn't open\n";
                if (auto str = dlerror())
                {
                    std::cout << "error: " << str << std::endl;
                    std::free (str);
                }
            }

            if (moduleFunction)
            {
                mod = moduleFunction();
            }
            else
            {
                std::cout << "no descriptor function\n";
            }

            if (mod && mod->create)
            {
                handle = mod->create (backend.features());
            }
        }

        return is_open();
    }

    void close()
    {
        if (handle != nullptr) 
        {
            if (mod != nullptr && mod->destroy != nullptr)
                mod->destroy (handle);
            handle = nullptr;
        }
        mod = nullptr;

        moduleFunction = nullptr;

        if (library != nullptr)
        {
            element_closelib (library);
            library = nullptr;
        }
    }

    const std::string& bundle_path() const noexcept { return bundlePath; }

private:
    Backend& backend;
    const std::string bundlePath;
    void* library = nullptr;
    const elDescriptor* mod = nullptr;
    elHandle handle;
    ELDescriptorFunction moduleFunction = nullptr;
};

class Modules {
public:
    using ptr_type      = std::unique_ptr<Module>;
    using vector_type   = std::vector<ptr_type>;
    Modules (Backend& c) : backend (c) {}

    void clear()
    {
        for (auto& m : mods)
            m->close();
        mods.clear();
    }

    void add (ptr_type mod)
    {
        mods.push_back (std::move (mod));
    }

    void add (Module* mod) { add (ptr_type (mod)); }

    auto begin() const noexcept { return mods.begin(); }
    auto end()   const noexcept { return mods.end(); }

    bool contains (const std::string& bp) const noexcept {
        auto result = std::find_if (begin(), end(), [bp](const ptr_type& ptr) {
            return ptr->bundle_path() == bp;
        });
        
        return result != mods.end();
    }

private:
    Backend& backend;
    vector_type mods;
};

Backend::Backend() {
    modules.reset (new Modules (*this));
}

Backend::~Backend() {
    modules.reset();
}

void Backend::load_module (const std::string& path) {
    if (modules->contains (path))
        return;

    if (auto mod = std::make_unique<Module> (path, *this)) {
        if (mod->open())
            modules->add (std::move (mod));
    }
}

}
