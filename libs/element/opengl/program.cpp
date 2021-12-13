
#include "opengl.hpp"
#include "helpers.hpp"

namespace gl {

Program::Program (Device& d)
    : device (d)
{
    create_program();
}

Program::~Program()
{
    delete_program();
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
		// print_link_errors(program->obj);
		return false;
	}

    for (auto const& att : vs->attributes())
    {
        GLint gl_att = glGetAttribLocation (gl_program, att.name.c_str());
	    if (! check_ok ("glGetAttribLocation"))
		    return false;

	    if (gl_att == -1) {
		    printf ("glGetAttribLocation: Could not find "
		            "attribute '%s'", att.name.c_str());
		    return false;
        }
        std::clog << "[opengl] programm att: " << (int)gl_att << " : " << att.name << std::endl;
        vs_atts.push_back (gl_att);
        vs_types.push_back (attribute_data_type (att.type));
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

    gl_program = glCreateProgram();
    return check_ok ("glCreateProgram");
}

bool Program::delete_program()
{
    bool res = true;
    if (gl_program) {
        glDeleteProgram (gl_program);
        gl_program = 0;
        res = check_ok ("glDeleteProgram");
    }
    return res;
}

void Program::load_buffers (Buffer* vb, Buffer* ib)
{
    GLuint buffer = vb->object();
    GLuint VAO = vb->VAO();
    if (buffer == 0 || VAO == 0) {
        std::cerr << "buffer and/or VAO missing\n";
        return;
    }

    glBindVertexArray (VAO);
    if (! gl::check_ok ("glBindVertexArray"))
        return;

    if (! gl::bind_buffer (GL_ARRAY_BUFFER, buffer))
        return;
    
    for (size_t i = 0; i < vs_atts.size(); ++i) {
        auto location = vs_atts[i];
        auto type = GL_FLOAT; //vs_types[i];
        
        // FIXME:
        // if (type != GL_FLOAT)
        //     continue;
        
        GLuint size = 3;
        GLsizei stride = sizeof(evgVec3);// + sizeof(evgVec2);
        uintptr_t offset = 0;

        if (location == 1) {
            size = 2;
            offset = sizeof(evgVec3);
        }

        bool success = true;
        glVertexAttribPointer (location, size, type, GL_TRUE, stride, (void*)offset);
        if (! check_ok ("glVertexAttribPointer"))
        	success = false;
        glEnableVertexAttribArray (location);
        if (! check_ok ("glEnableVertexAttribArray"))
        	success = false;

        if (! success)
            return;
    }

    if (ib) {
        gl::bind_buffer (GL_ELEMENT_ARRAY_BUFFER, ib->object());
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

}
