
#pragma once

#include <memory>
#include <string>
#include <element/element.h>

namespace element {

class Modules;
class Scripting;

class EL_API Context {
public:
    virtual ~Context();
    
    //=========================================================================
    void test_open_module (const std::string& path);
    void test_load_modules();
    int test_main (const std::string module_id, int argc, const char* argv[]);
    void* test_lua_state() const;
    void test_add_module_search_path (const std::string&);
    void test_discover_modules();
    
protected:
    Context();

private:
    EL_DISABLE_COPY(Context);
    EL_DISABLE_MOVE(Context);
    std::unique_ptr<Modules> modules;
    std::unique_ptr<Scripting> scripting;
};

}
