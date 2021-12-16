
#include "opengl.hpp"
#include "helpers.hpp"
#include "program.hpp"

namespace gl {

Program::Program (Device& d)
    : device (d)
{
    create_program();
}

Program::~Program()
{
    delete_program();
    for (auto& r : ress)
        if (r.symbol != nullptr)
            std::free ((void*) r.symbol);
}

bool Program::link (Shader* vs, Shader* fs)
{
    if (! gl_program || vs == nullptr || fs == nullptr) {
        std::clog << "no program\n";
        return false;
    }

    if (vs->get_type() != EVG_SHADER_VERTEX || fs->get_type() != EVG_SHADER_FRAGMENT) {
        std::clog << "wrong shader type(s)\n";
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

	if (!check_ok ("glGetProgramiv"))
		return false;

	if (linked == GL_FALSE) {
        std::clog << "got link errors\n";
		// TODO: log linker errors
		return false;
	}

    int slot = 0, tex_slot = 0;
    uint32_t att_key = 0, uni_key = 0;
    for (auto const& res : vs->resources())
    {
        ress.push_back ({});
        auto& res2 = ress.back();
        res2.symbol = strdup (res.symbol.c_str());
        res2.key = 0;
        res2.type = res.type;
        res2.value_type = res.value_type;
        
        if (res.type == EVG_ATTRIBUTE) {
            atts.push_back (Attribute());
            auto& att = atts.back();
            att.location = glGetAttribLocation (gl_program, res.symbol.c_str());
            if (! check_ok ("glGetAttribLocation"))
                return false;

            if (att.location == -1) {
                printf ("glGetAttribLocation: Could not find "
                        "attribute '%s'", res.symbol.c_str());
                return false;
            }

            std::clog << "[opengl] programm att: " << (int)att.location << " : " << res.symbol << std::endl;
            att.size        = uniform_vec_size (res.value_type);
            att.type        = uniform_data_type (res.value_type);
            att.stride      = uniform_stride (res.value_type);
            att.normalized  = true;
            att.offset = 0;
            att.buffer_slot = slot++;
            res2.key = att_key++;
        } else if (res.type == EVG_UNIFORM) {
            unis.push_back (Uniform());
            auto& u = unis.back();
            u.location = glGetUniformLocation (gl_program, res.symbol.c_str());
            u.value_type = res.value_type;
            u.value_size = uniform_data_size (u.value_type);
            u.current_value.reset (new uint8_t [u.value_size]);
            u.default_value.reset (new uint8_t [u.value_size]);
            memset (u.current_value.get(), 0, u.value_size);
            memset (u.default_value.get(), 0, u.value_size);
            if (u.value_type == EVG_UNIFORM_TEXTURE) {
                memcpy (u.current_value.get(), &tex_slot, sizeof (int));
                memcpy (u.default_value.get(), &tex_slot, sizeof (int));
                ++tex_slot;
            }

            res2.key = uni_key++;
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
    
    for (const auto& att : atts) {
        auto buffer = vb [att.buffer_slot];
        if (bind_buffer (GL_ARRAY_BUFFER, buffer->object()))
        {
            bool success = true;
            glVertexAttribPointer (att.location, att.size, att.type, 
                att.normalized ? GL_TRUE : GL_FALSE, 
                att.stride, (void*)att.offset);
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
    for (const auto& u : unis) {
        switch (u.value_type) {
            case EVG_UNIFORM_TEXTURE:
            case EVG_UNIFORM_INT:
                glUniform1i (u.location, *(GLint*) u.current_value.get());
                break;
            case EVG_UNIFORM_FLOAT:
                glUniform1f (u.location, *(GLfloat*) u.current_value.get());
                break;
            default: break;
        }

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

const evgResource* Program::_resource (evgHandle ph, uint32_t index) {
    auto& self = *static_cast<Program*> (ph);
    return index < self.ress.size() ? &self.ress[index] : nullptr;
}

}
