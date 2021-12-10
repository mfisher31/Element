
#include "helpers.hpp"
#include "opengl.hpp"

namespace gl {

static void shader_info (GLuint shader, const char* file, char** error_string)
{
    GLint info_len = 0;
    GLsizei chars_written = 0;

    glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &info_len);
    if (! check_ok ("glGetProgramiv") || ! info_len)
        return;

    char* errors = (char*) std::malloc (info_len + 1);
    glGetShaderInfoLog (shader, info_len, &chars_written, errors);
    check_ok ("glGetShaderInfoLog");

    printf ("Compiler warnings/errors for %s:\n%s", file, errors);

    if (error_string)
        *error_string = errors;
    else
        std::free (errors);
}

bool Shader::parse (const char* program)
{
    if (gl_shader != 0)
        return false;

    auto shader = glCreateShader (shader_type (type));
    if (! check_ok ("glCreateShader") || ! shader)
        return false;

    glShaderSource (shader, 1, (const GLchar**) &program, 0);
    if (! check_ok ("glShaderSource"))
        return false;

    glCompileShader (shader);
    if (! check_ok ("glCompileShader"))
        return false;

    int compiled = 0;
    glGetShaderiv (shader, GL_COMPILE_STATUS, &compiled);
    if (! check_ok ("glGetShaderiv"))
        return false;

    bool success = compiled > 0;
    if (! success) {
        GLint len = 0;
        glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &len);

        char* log = (char*) std::malloc (sizeof (char) * len);

        GLsizei len2 = 0;
        glGetShaderInfoLog (shader, len, &len2, log);
        printf ("Error compiling shader:\n%s\n", log);
        free (log);
    }

    char* error_string = nullptr;
    shader_info (shader, "shader_label", &error_string);
    if (error_string) {
        std::clog << "error: " << error_string << std::endl;
        std::free (error_string);
    }

    if (success)
        gl_shader = shader;

    return success;
}

void Shader::release()
{
}


} // namespace gl
