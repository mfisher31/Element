
#include "opengl.hpp"

namespace gl {

class Program final {
public:
    Program (Device& d);
    ~Program();

    bool link (Shader* vs, Shader* fs);
    inline GLuint object() const noexcept { return gl_program; }
    inline bool have_program() const noexcept { return gl_program > 0; }
    inline bool can_run() const noexcept { return have_program() && gl_vert != 0 && gl_frag != 0; }

    void set_uniform_value (int index, uint32_t size, const void* data)
    {
        memcpy (res[index].uniform->current_value.get(), data, size);;
    }

    void load_buffers (Buffer** vb, Buffer* ib);
    void process_uniforms();
    bool create_program();
    bool delete_program();

    inline static const evgProgramInterface* interface()
    {
        static const evgProgramInterface I = {
            .create = _create,
            .destroy = _destroy,
            .link = _link,
            .resource = _resource,
            .update_resource = _update_resource
        };

        return &I;
    }

private:
    Device& device;
    GLuint gl_vert = 0;
    GLuint gl_frag = 0;
    GLuint gl_program = 0;
    GLuint VAO = 0;

    struct Attribute {
        uint32_t location;
        uint32_t type;
        uint32_t size;
        uint32_t stride;
        bool normalized = true;
        uintptr_t offset;
        int slot = 0;
        bool changed = true;
    };

    struct Uniform {
        uint32_t location;
        evgValueType value_type;
        size_t value_size = 0;
        std::unique_ptr<uint8_t[]> current_value;
        std::unique_ptr<uint8_t[]> default_value;
        bool changed = true;
    };

    struct Resource : public Shader::Resource {
        std::unique_ptr<Attribute> attribute;
        std::unique_ptr<Uniform> uniform;
    };

    std::vector<Resource> res;
    std::vector<Attribute*> atts;
    std::vector<Uniform*> unis;

    static evgHandle _create (evgHandle dh);
    static void _destroy (evgHandle ph);
    static void _link (evgHandle ph, evgHandle vs, evgHandle fs);
    static const evgResource* _resource (evgHandle program, uint32_t index);
    static void _update_resource (evgHandle program, int key, uint32_t size, const void* data);
};

}
