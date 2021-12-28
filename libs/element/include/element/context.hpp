
#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <element/element.h>
#include <element/graphics.hpp>

namespace element {

class Modules;
class Scripting;
class Video;

class EL_API Context {
public:
    Context();
    virtual ~Context();

    //=========================================================================
    void open_module (const std::string& path);
    void load_modules();
    void add_module_path (const std::string&);
    void discover_modules();

    //=========================================================================
    int test_main (const std::string module_id, int argc, const char* argv[]);
    void* test_lua_state() const;

    //=========================================================================
    evg::Display* test_create_video_display (const evgSwapInfo*);

private:
    EL_DISABLE_COPY(Context);
    EL_DISABLE_MOVE(Context);
    friend class Module; // FIXME
    std::unique_ptr<Modules> modules;
    std::unique_ptr<Scripting> scripting;
    std::unique_ptr<Video> video;
};

}
