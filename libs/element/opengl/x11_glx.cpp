
#include <cstdio>
#include <cstdlib>
#include <functional>

#include "helpers.hpp"
#include "opengl.hpp"
#define GLAD_GLX_IMPLEMENTATION 1
#include <glad/glx.h>

#include <X11/Xlib-xcb.h>

namespace gl {

enum class GLXSwapType {
    NORMAL = 0,
    EXT,
    MESA,
    SGI
};

static const int ctx_attribs[] = {
#ifdef _DEBUG
    GLX_CONTEXT_FLAGS_ARB,
    GLX_CONTEXT_DEBUG_BIT_ARB,
#endif
    GLX_CONTEXT_PROFILE_MASK_ARB,
    GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
    GLX_CONTEXT_MAJOR_VERSION_ARB,
    3,
    GLX_CONTEXT_MINOR_VERSION_ARB,
    3,
    None,
};

static int ctx_pbuffer_attribs[] = { GLX_PBUFFER_WIDTH, 2, GLX_PBUFFER_HEIGHT, 2, None };

static int ctx_visual_attribs[] = { GLX_STENCIL_SIZE,
                                    0,
                                    GLX_DEPTH_SIZE,
                                    0,
                                    GLX_BUFFER_SIZE,
                                    32,
                                    GLX_ALPHA_SIZE,
                                    8,
                                    GLX_DOUBLEBUFFER,
                                    GL_TRUE,
                                    GLX_X_RENDERABLE,
                                    GL_TRUE,
                                    None };

//=============================================================================
using XDeleter = int (*) (void*);
template <typename T>
class unique_xptr : public std::unique_ptr<T, XDeleter> {
public:
    using parent_type = std::unique_ptr<T, XDeleter>;
    unique_xptr() : parent_type (nullptr, __delete) {}
    unique_xptr (T* ptr) : parent_type (ptr, __delete) {}

private:
    static int __delete (void* data) {
        return XFree (data);
    }
};

//=============================================================================
struct GLXSwap final : public gl::Swap {
    xcb_window_t ID { 0 };
    GLXFBConfig config { 0 };
    int screen { -1 };

    explicit GLXSwap (const evgSwapInfo* s)
    {
        memcpy (&this->setup, s, sizeof (evgSwapInfo));
    }

    inline constexpr bool valid() const noexcept { return config != 0 && ID != 0 && screen >= 0; }
};

//=============================================================================
static Display* glx_display_open();
static xcb_get_geometry_reply_t* get_window_geometry (xcb_connection_t* xcb_conn, xcb_drawable_t drawable);
static inline int get_screen_number_from_root (xcb_connection_t* xcb_conn, xcb_window_t root);

static int log_x11_error (Display* display, XErrorEvent* error)
{
    char err[512], request[512], minor[512];
    XGetErrorText (display, error->error_code, err, sizeof (err));
    XGetErrorText (display, error->request_code, request, sizeof (request));
    XGetErrorText (display, error->minor_code, minor, sizeof (minor));
    fprintf (stderr, "X Error: %s, Major: %s, Minor: %s, Serial: %lu", err, request, minor, error->serial);
    return 0;
}



class GLXPlatform final : public Platform {
public:
    explicit GLXPlatform (Display* d, GLXContext c, GLXPbuffer pb)
        : display (d),
          context (c),
          pbuffer (pb)
    {
    }

    ~GLXPlatform() override = default;

    bool initialize() override
    {
        return true;
    }

    //========================================================================
    Swap* create_swap (const evgSwapInfo* setup) override
    {
        auto swap = std::make_unique<GLXSwap> (setup);
        if (! attach_swap (swap.get()))
            return nullptr;
        return swap.release();
    }

    void load_swap (const Swap* s) override
    {
        auto swap = dynamic_cast<const GLXSwap*> (s);
        if (active_swap == swap)
            return;

        active_swap = swap;

        if (active_swap) {
            if (! glXMakeContextCurrent (display, active_swap->ID, active_swap->ID, context))
                std::clog << "Failed to make context current." << std::endl;
        } else {
            if (! glXMakeContextCurrent (display, pbuffer, pbuffer, context)) {
                std::clog << "Failed to make context current." << std::endl;
            }
        }
    }

    //=========================================================================
    void* context_handle() const noexcept override
    {
        return static_cast<void*> (context);
    }

    void enter_context() override
    {
        if (active_swap != nullptr) {
            if (! glXMakeContextCurrent (display, active_swap->ID, active_swap->ID, context))
                std::clog << "Failed to make swap current (swap)" << std::endl;
        } else {
            if (! glXMakeContextCurrent (display, pbuffer, pbuffer, context))
                std::clog << "Failed to make context current (internal)" << std::endl;
        }
    }

    void leave_context() override
    {
        if (! glXMakeContextCurrent (display, None, None, context))
            std::clog << "Failed to leave context" << std::endl;
    }

    void clear_context() override
    {
        if (! glXMakeContextCurrent (display, None, None, NULL))
            std::clog << "Failed to clear contex" << std::endl;
    }

    //=========================================================================
    void create_window() {}
    void destroy_window() {}

    //=========================================================================
    void get_size (const GLXSwap* swap, uint32_t* width, uint32_t* height)
    {
        xcb_connection_t* xcb_conn = XGetXCBConnection (display);
        if (auto geometry = get_window_geometry (xcb_conn, swap->ID)) {
            *width = geometry->width;
            *height = geometry->height;
            std::free (geometry);
        }
    }

    //=========================================================================
    void swap_buffers() override
    {
        if (active_swap == nullptr)
            return;

        if (! present_initialized) {
            if (GLAD_GLX_EXT_swap_control)
                swap_type = GLXSwapType::EXT;
            else if (GLAD_GLX_MESA_swap_control)
                swap_type = GLXSwapType::MESA;
            else if (GLAD_GLX_SGI_swap_control)
                swap_type = GLXSwapType::SGI;
            else
                swap_type = GLXSwapType::NORMAL;
            present_initialized = true;
        }

        // xcb_connection_t* xcb_conn = XGetXCBConnection (display);
        // xcb_generic_event_t* xcb_event = nullptr;
        // while ((xcb_event = xcb_poll_for_event (xcb_conn))) {
        //     /* TODO: Handle XCB events. */
        //     free (xcb_event);
        // }

        switch (swap_type) {
            case GLXSwapType::EXT:
                glXSwapIntervalEXT (display, active_swap->ID, 0);
                break;
            case GLXSwapType::MESA:
                glXSwapIntervalMESA (0);
                break;
            case GLXSwapType::SGI:
                glXSwapIntervalSGI (0);
                break;
            case GLXSwapType::NORMAL:
            default:
                break;
        }

        glXSwapBuffers (display, active_swap->ID);
    }

private:
    Display* display { nullptr };
    GLXContext context { 0 };
    GLXPbuffer pbuffer { 0 };

    const GLXSwap* active_swap { nullptr };
    GLXSwapType swap_type = GLXSwapType::NORMAL;
    bool present_initialized = false;

    friend void destroy_platform (Platform* p);

    bool attach_swap (GLXSwap* swap)
    {
        if (swap == nullptr)
            return false;

        auto xcb_conn = XGetXCBConnection (display);
        auto wid = xcb_generate_id (xcb_conn);
        auto parent = swap->setup.window.xwindow;
        auto geometry = get_window_geometry (xcb_conn, parent);

        if (nullptr == geometry)
            return false;

        bool status = false;
        int screen_num = -1;
        int visual = -1;
        GLXFBConfig* fb_config = nullptr;

#define free_xobjs()          \
    if (geometry != nullptr)  \
        std::free (geometry); \
    if (fb_config != nullptr) \
        XFree (fb_config);

        screen_num = get_screen_number_from_root (xcb_conn, geometry->root);
        if (screen_num < 0) {
            free_xobjs();
            return false;
        }

        {
            // find best config
            int nconfigs = 0;
            fb_config = glXChooseFBConfig (display, screen_num, ctx_visual_attribs, &nconfigs);

            if (fb_config == nullptr || nconfigs < 1) {
                std::clog << "Failed to find FBConfig!" << std::endl;
                free_xobjs();
                return false;
            }
        }

        {
            // get a visual
            int error = glXGetFBConfigAttrib (display, fb_config[0], GLX_VISUAL_ID, &visual);
            if (error) {
                std::clog << "Bad call to GetFBConfigAttrib!" << std::endl;
                free_xobjs();
                return false;
            }
        }

        xcb_colormap_t colormap = xcb_generate_id (xcb_conn);
        uint32_t mask = XCB_CW_BORDER_PIXEL | XCB_CW_COLORMAP;
        uint32_t mask_values[] = { 0, colormap, 0 };

        const uint8_t depth = 24;
        xcb_create_colormap (xcb_conn, XCB_COLORMAP_ALLOC_NONE, colormap, parent, visual);
        xcb_create_window (xcb_conn, depth, wid, parent, 0, 0, geometry->width, geometry->height, 0, 0, visual, mask, mask_values);

        swap->config = fb_config[0];
        swap->ID = wid;
        swap->screen = screen_num;
        xcb_map_window (xcb_conn, swap->ID);

        free_xobjs();
        return true;
    }
};

/* Returns -1 on invalid screen. */
static int get_screen_number (xcb_connection_t* xcb_conn, xcb_screen_t* screen)
{
    xcb_screen_iterator_t iter =
        xcb_setup_roots_iterator (xcb_get_setup (xcb_conn));
    int screen_num = 0;

    for (; iter.rem; xcb_screen_next (&iter), ++screen_num)
        if (iter.data == screen)
            return screen_num;

    return -1;
}

static xcb_screen_t* get_screen_from_root (xcb_connection_t* xcb_conn, xcb_window_t root)
{
    auto setup = xcb_get_setup (xcb_conn);
    auto iter = xcb_setup_roots_iterator (setup);

    while (iter.rem) {
        if (iter.data->root == root)
            return iter.data;
        xcb_screen_next (&iter);
    }

    return 0;
}

static inline int get_screen_number_from_root (xcb_connection_t* xcb_conn, xcb_window_t root)
{
    auto screen = get_screen_from_root (xcb_conn, root);

    if (! screen)
        return -1;

    return get_screen_number (xcb_conn, screen);
}

static xcb_get_geometry_reply_t* get_window_geometry (xcb_connection_t* xcb_conn, xcb_drawable_t drawable)
{
    xcb_generic_error_t* error = nullptr;
    auto cookie = xcb_get_geometry (xcb_conn, drawable);
    auto reply = xcb_get_geometry_reply (xcb_conn, cookie, &error);

    if (error != nullptr) {
        std::clog << "Failed to fetch parent window geometry!" << std::endl;
        std::free (error);
        if (reply != nullptr)
            std::free (reply);
        return nullptr;
    }

    if (error)
        std::free (error);
    return reply;
}

static Display* glx_display_open()
{
    Display* display = XOpenDisplay (NULL);
    xcb_connection_t* xcb_conn = nullptr;
    xcb_screen_iterator_t screen_iterator;
    xcb_screen_t* screen = nullptr;
    int screen_num = -1;

    if (display == nullptr) {
        std::clog << "Unable to open new X connection!" << std::endl;
        return nullptr;
    }

    xcb_conn = XGetXCBConnection (display);
    if (! xcb_conn) {
        std::clog << "Unable to get XCB connection to main display" << std::endl;
        goto error;
    }

    screen_iterator = xcb_setup_roots_iterator (xcb_get_setup (xcb_conn));
    screen = screen_iterator.data;
    if (! screen) {
        std::clog << "[opengl] unable to get screen root" << std::endl;
        goto error;
    }

    screen_num = get_screen_number_from_root (xcb_conn, screen->root);
    if (screen_num < 0) {
        std::clog << "[opengl] unable to get screen number from root" << std::endl;
        goto error;
    }

    if (! gladLoaderLoadGLX (display, screen_num)) {
        std::clog << "[opengl] Unable to load GLX entry functions" << std::endl;
        goto error;
    }

    return display;

error:
    if (display)
        XCloseDisplay (display);
    return nullptr;
}



static bool glx_create_new (Display* display, GLXContext& context, GLXPbuffer& pbuffer)
{
    if (nullptr == display)
        return false;

    bool success = false;
    int nconfigs = 0;

    unique_xptr<GLXFBConfig> config;

    if (! GLAD_GLX_ARB_create_context) {
        // blog (LOG_ERROR, "ARB_glx_create_new not supported!");
        return false;
    }

    config.reset (glXChooseFBConfig (display, DefaultScreen (display), ctx_visual_attribs, &nconfigs));
    if (config == nullptr) {
        // blog (LOG_ERROR, "Failed to create OpenGL frame buffer config");
        return false;
    }

    context = glXCreateContextAttribsARB (display, config.get()[0], NULL, true, ctx_attribs);
    pbuffer = context != 0 ? glXCreatePbuffer (display, config.get()[0], ctx_pbuffer_attribs) : 0;

    XSync (display, false);
    return context != 0 && pbuffer != 0;
}

static bool glx_context_free (Display* display, GLXContext context)
{
    if (display == nullptr || context == 0)
        return false;
    glXMakeContextCurrent (display, None, None, NULL);
    glXDestroyContext (display, context);
    return true;
}

Platform* create_platform()
{
    auto display = glx_display_open();
    if (display == nullptr)
        return nullptr;

    GLXContext context = 0;
    GLXPbuffer pbuffer = 0;
    if (! glx_create_new (display, context, pbuffer)) {
        return nullptr;
    }

    if (! glXMakeContextCurrent (display, pbuffer, pbuffer, context)) {
        std::clog << "can't activate context.\n";
        return nullptr;
    }

    if (! gladLoaderLoadGL()) {
        std::clog << "could not load GL\n";
        return nullptr;
    }

    XSetEventQueueOwner (display, XCBOwnsEventQueue);
    XSetErrorHandler (log_x11_error);

    return new GLXPlatform (display, context, pbuffer);
}

void destroy_platform (Platform* p)
{
    std::clog << "closing GLX" << std::endl;
    auto glx = dynamic_cast<GLXPlatform*> (p);
    auto display = glx->display;
    auto pbuffer = glx->pbuffer;
    auto context = glx->context;
    delete glx;

    glx_context_free (display, context);
    XCloseDisplay (display);
    XSetErrorHandler (nullptr);
}

}; // namespace gl
