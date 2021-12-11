
#include "helpers.hpp"
#include "opengl.hpp"

namespace gl {

VertexBuffer::VertexBuffer (evgVertexData* data, uint32_t flags)
{
    this->data = data;
    this->dynamic = (flags & EVG_OPT_DYNAMIC) != 0;
}

bool VertexBuffer::create_buffers()
{
    if (data == nullptr || data->points == nullptr)
        return false;

    const GLenum type = is_dynamic() ? GL_STREAM_DRAW : GL_STATIC_DRAW;
    size_t data_size = data->size * sizeof (evgVec3);
    if (! gl::buffer_bind_data (GL_ARRAY_BUFFER, &points, data_size, data->points, type))
        return false;

    if (data->normals)
        if (! gl::buffer_bind_data (GL_ARRAY_BUFFER, &normals, data_size, data->normals, type))
            return false;
    if (data->tangents)
        if (! gl::buffer_bind_data (GL_ARRAY_BUFFER, &tangents, data_size, data->tangents, type))
            return false;
    if (data->colors)
        if (! gl::buffer_bind_data (GL_ARRAY_BUFFER, &colors, data_size, data->colors, type))
            return false;

    uv_buffers.reserve (data->ntextures);
    for (uint32_t i = 0; i < data->ntextures; i++) {
        GLuint tbuf;
        auto tverts = data->texdata + i;
        size_t size = data->size * sizeof (float) * tverts->width;
        if (! gl::buffer_bind_data (GL_ARRAY_BUFFER, &tbuf, size, tverts->array, type)) {
            return false;
        }

        uv_buffers.push_back ({ tbuf, static_cast<GLuint> (tverts->width) });
    }

    if (! is_dynamic()) {
        // evg_vertex_data_free (data);
        // data = nullptr;
    }

    if (! gl::gen_vertex_arrays (1, &vao))
        return false;

    return true;
}

void VertexBuffer::flush() {
	if (! is_dynamic()) {
        return;
	}

    gl::bind_vertex_array (vao);

    size_t field_size = data->size * sizeof (evgVec3);
	if (points && data->points) {
		if (! gl::update_buffer (GL_ARRAY_BUFFER, points, data->points, field_size))
			return;
	}

	if (normals && data->normals) {
		if (! gl::update_buffer (GL_ARRAY_BUFFER, normals, data->normals, field_size))
			return;
	}

	if (tangents && data->tangents) {
		if (! gl::update_buffer (GL_ARRAY_BUFFER, tangents, data->tangents, field_size))
			return;
	}

	if (colors && data->colors) {
		if (! gl::update_buffer (GL_ARRAY_BUFFER, colors, data->colors, field_size))
			return;
	}

    size_t i = 0;
	for (auto const& uv : uv_buffers) {
		GLuint buffer = uv.buffer;
		auto *tv = data->texdata + i;
		size_t size = data->size * tv->width * sizeof(float);
		if (! gl::update_buffer (GL_ARRAY_BUFFER, buffer, tv->array, size))
			return;
        ++i;
	}

	return;
}

bool VertexBuffer::destroy_buffers()
{
    if (points)
        gl::delete_buffers (1, &points);
    if (normals)
        gl::delete_buffers (1, &normals);
    if (tangents)
        gl::delete_buffers (1, &tangents);
    if (colors)
        gl::delete_buffers (1, &colors);

    for (auto& uv : uv_buffers)
        gl::delete_buffers (1, &uv.buffer);

    if (vao)
        gl::delete_vertex_arrays (1, &vao);

    uv_buffers.clear();

    return true;
}

}
