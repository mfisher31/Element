
#pragma once

#include <memory>
#include <string>
#include <element/element.h>
#include <element/graphics.h>

namespace element {

class Modules;
class Scripting;
class Video;

class VideoDisplay {
public:
    ~VideoDisplay() = default;

    void get_size (int& width, int& height) const { width = 0; height = 0; }

private:
    VideoDisplay (egSwapChain* swap);
};

class EL_API Context {
public:
    Context();
    virtual ~Context();
    
    //=========================================================================
    void test_open_module (const std::string& path);
    void test_load_modules();
    int test_main (const std::string module_id, int argc, const char* argv[]);
    void* test_lua_state() const;
    void test_add_module_search_path (const std::string&);
    void test_discover_modules();

private:
    EL_DISABLE_COPY(Context);
    EL_DISABLE_MOVE(Context);
    friend class Module; // FIXME
    std::unique_ptr<Modules> modules;
    std::unique_ptr<Scripting> scripting;
    std::unique_ptr<Video> video;
};

}
