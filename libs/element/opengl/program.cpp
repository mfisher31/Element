
#include "program.hpp"
#include "helpers.hpp"
#include "opengl.hpp"

namespace gl {

Program::Program (Device& d)
    : device (d)
{
    create_program();
}

Program::~Program()
{
    delete_program();
    atts.clear();
    unis.clear();
    for (auto& r : res) {
        r.uniform.reset();
        r.attribute.reset();
    }
    res.clear();
}

bool Program::link (Shader* vs, Shader* fs)
{
    if (! gl_program || vs == nullptr || fs == nullptr) {
        return false;
    }

    if (vs->get_type() != EVG_SHADER_VERTEX || fs->get_type() != EVG_SHADER_FRAGMENT) {
        return false;
    }

    glAttachShader (gl_program, vs->object());
    if (! check_ok ("glAttachShader (vert)"))
        return false;

    glAttachShader (gl_program, fs->object());
    if (! check_ok ("glAttachShader (frag)"))
        return false;

    GLint linked = 0;
    glLinkProgram (gl_program);
    glGetProgramiv (gl_program, GL_LINK_STATUS, &linked);

    if (! check_ok ("glGetProgramiv"))
        return false;

    if (linked == GL_FALSE) {
        return false;
    }

    int vb_slot = 0, tex_slot = 0;
    res.reserve (vs->resources().size() + fs->resources().size());

    std::vector<gl::Shader::Resource> shres;
    shres.reserve (res.size());
    shres.insert (std::begin(shres), std::begin(vs->resources()), std::end (vs->resources()));
    shres.insert (std::begin(shres), std::begin(fs->resources()), std::end (fs->resources()));

    for (auto const& vsres : shres) {
        res.push_back (Resource());
        auto& r = res.back();
        r.symbol = vsres.symbol;
        r.resource = vsres.resource;
        r.resource.symbol = r.symbol.c_str();
        r.resource.key = res.size() - 1;

        if (r.resource.type == EVG_ATTRIBUTE) {
            r.attribute = std::make_unique<Attribute>();
            atts.push_back (r.attribute.get());
            auto& att = *r.attribute;
            att.location = glGetAttribLocation (gl_program, r.symbol.c_str());
            if (! check_ok ("glGetAttribLocation"))
                return false;

            if (att.location == -1) {
                printf ("glGetAttribLocation: Could not find "
                        "attribute '%s'",
                        r.symbol.c_str());
                return false;
            }

            att.size = uniform_vec_size (r.resource.value_type);
            att.type = uniform_data_type (r.resource.value_type);
            att.stride = uniform_stride (r.resource.value_type);
            att.normalized = true;
            att.offset = 0;
            att.slot = vb_slot++;

        } else if (r.resource.type == EVG_UNIFORM) {
            r.uniform = std::make_unique<Uniform>();
            unis.push_back (r.uniform.get());
            auto& u = *r.uniform;
            u.location = glGetUniformLocation (gl_program, r.symbol.c_str());
            u.value_type = r.resource.value_type;
            u.value_size = uniform_data_size (u.value_type);
            u.current_value.reset (new uint8_t[u.value_size]);
            u.default_value.reset (new uint8_t[u.value_size]);
            memset (u.current_value.get(), 0, u.value_size);
            memset (u.default_value.get(), 0, u.value_size);
            if (r.resource.value_type == EVG_VALUE_TEXTURE) {
                memcpy (u.current_value.get(), &tex_slot, sizeof (int));
                memcpy (u.default_value.get(), &tex_slot, sizeof (int));
                ++tex_slot;
            }
        }
    }

    glDetachShader (gl_program, vs->object());
    glDetachShader (gl_program, fs->object());

    gl_vert = vs->object();
    gl_frag = vs->object();
    return true;
}

bool Program::create_program()
{
    if (gl_program > 0)
        return true;
    glGenVertexArrays (1, &VAO);
    gl_program = glCreateProgram();
    return check_ok ("glCreateProgram");
}

bool Program::delete_program()
{
    bool res = true;
    if (gl_program) {
        glDeleteProgram (gl_program);
        gl_program = 0;
        if (! check_ok ("glDeleteProgram"))
            res = false;
    }

    if (VAO) {
        glDeleteVertexArrays (1, &VAO);
        VAO = 0;
        if (! check_ok ("Program::delete_program(): glDeleteVertexArrays()"))
            res = false;
    }

    return res;
}

void Program::load_buffers (Buffer** vb, Buffer* ib)
{
    glBindVertexArray (VAO);
    if (! gl::check_ok ("glBindVertexArray"))
        return;

    for (Attribute* a : atts) {
        auto& att = *a;
        auto buffer = vb[att.slot];
        if (bind_buffer (GL_ARRAY_BUFFER, buffer->object())) {
            bool success = true;
            glVertexAttribPointer (att.location, att.size, att.type, att.normalized ? GL_TRUE : GL_FALSE, att.stride, (void*) att.offset);
            if (! check_ok ("glVertexAttribPointer"))
                success = false;
            glEnableVertexAttribArray (att.location);
            if (! check_ok ("glEnableVertexAttribArray"))
                success = false;

            if (! success)
                return;
        }
    }

    if (ib) {
        gl::bind_buffer (GL_ELEMENT_ARRAY_BUFFER, ib->object());
    }
}

void Program::process_uniforms()
{
    for (Uniform* u : unis) {
        if (! u->changed)
            continue;
        switch (u->value_type) {
            case EVG_VALUE_TEXTURE:
            case EVG_VALUE_INT:
                glUniform1i (u->location, *(GLint*) u->current_value.get());
                break;
            case EVG_VALUE_FLOAT:
                glUniform1f (u->location, *(GLfloat*) u->current_value.get());
                break;
            case EVG_VALUE_MAT4X4:
                glUniformMatrix4fv (u->location, 1, GL_FALSE, (GLfloat*)u->current_value.get());
                break;
            default:
                break;
        }
        u->changed = false;
        check_ok ("process_uniforms: set value");
    }
}

evgHandle Program::_create (evgHandle dh)
{
    auto device = (Device*) dh;
    if (auto program = std::make_unique<Program> (*device))
        if (program->create_program())
            return program.release();
    return nullptr;
}

void Program::_destroy (evgHandle ph)
{
    if (auto program = static_cast<Program*> (ph)) {
        program->delete_program();
        delete program;
    }
}

void Program::_link (evgHandle ph, evgHandle vs, evgHandle fs)
{
    (static_cast<Program*> (ph))->link ((Shader*) vs, (Shader*) fs);
}

const evgResource* Program::_resource (evgHandle ph, uint32_t index)
{
    auto& self = *static_cast<Program*> (ph);
    return index < self.res.size() ? &self.res[index].resource : nullptr;
}

void Program::_update_resource (evgHandle ph, int key, uint32_t size, const void* data) {
    auto& self = *static_cast<Program*> (ph);
    if (auto u = self.res[key].uniform.get()) {
        u->changed = true;
        memcpy (u->current_value.get(), data, size);
    } else if (auto a = self.res[key].attribute.get()) {
        a->slot = *(int*) data;
    }
}

} // namespace gl
