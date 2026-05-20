// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/headless/gl/clay_headless_renderer_host_gl.h"

#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "base/trace/native/trace_event.h"
#include "build/build_config.h"
#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/shared_image_backing.h"
#include "clay/gfx/shared_image/shared_image_sink.h"
#include "clay/gfx/skity_to_skia_utils.h"
#include "clay/shell/platform/embedder/embedder_struct_macros.h"
#include "clay/shell/platform/headless/clay_headless_engine.h"

namespace clay {

// Skity depends on OpenGL ES 3.0, but the host environment may only provide
// OpenGL ES 2.0. When Skity is enabled, use raw OpenGL ES 2.0 APIs to blit the
// result to the screen.
#ifdef ENABLE_SKITY
namespace {

// GL Type Definitions
using GLenum = uint32_t;
using GLboolean = unsigned char;
using GLbitfield = uint32_t;
using GLbyte = int8_t;
using GLshort = int16_t;
using GLint = int32_t;
using GLsizei = int32_t;
using GLsizeiptr = intptr_t;
using GLubyte = uint8_t;
using GLushort = uint16_t;
using GLuint = uint32_t;
using GLfloat = float;
using GLclampf = float;
using GLchar = char;

// GL Enumeration Values
constexpr GLenum GL_FALSE = 0;
constexpr GLenum GL_TRUE = 1;

constexpr GLenum GL_VERTEX_SHADER = 0x8B31;
constexpr GLenum GL_FRAGMENT_SHADER = 0x8B30;

constexpr GLenum GL_COMPILE_STATUS = 0x8B81;
constexpr GLenum GL_LINK_STATUS = 0x8B82;

constexpr GLenum GL_TEXTURE_2D = 0x0DE1;
constexpr GLenum GL_TEXTURE0 = 0x84C0;

constexpr GLenum GL_TEXTURE_MIN_FILTER = 0x2801;
constexpr GLenum GL_TEXTURE_MAG_FILTER = 0x2800;
constexpr GLenum GL_TEXTURE_WRAP_S = 0x2802;
constexpr GLenum GL_TEXTURE_WRAP_T = 0x2803;

constexpr GLenum GL_LINEAR = 0x2601;
constexpr GLenum GL_CLAMP_TO_EDGE = 0x812F;

constexpr GLenum GL_RGBA = 0x1908;
constexpr GLenum GL_UNSIGNED_BYTE = 0x1401;

constexpr GLenum GL_FLOAT = 0x1406;

constexpr GLenum GL_TRIANGLES = 0x0004;

constexpr GLenum GL_FRAMEBUFFER = 0x8D40;
constexpr GLenum GL_ARRAY_BUFFER = 0x8892;
constexpr GLenum GL_STATIC_DRAW = 0x88E4;

constexpr GLenum GL_CURRENT_PROGRAM = 0x8B8D;
constexpr GLenum GL_ACTIVE_TEXTURE = 0x84E0;
constexpr GLenum GL_TEXTURE_BINDING_2D = 0x8069;
constexpr GLenum GL_FRAMEBUFFER_BINDING = 0x8CA6;
constexpr GLenum GL_ARRAY_BUFFER_BINDING = 0x8894;
constexpr GLenum GL_VERTEX_ARRAY_BINDING = 0x85B5;
constexpr GLenum GL_UNPACK_ALIGNMENT = 0x0CF5;
constexpr GLenum GL_VIEWPORT = 0x0BA2;

// For glClear
constexpr GLenum GL_COLOR_BUFFER_BIT = 0x00004000;
constexpr GLenum GL_DEPTH_BUFFER_BIT = 0x00000100;

template <typename Fn>
Fn ResolveGLFunction(const GPUSurfaceGLDelegate::GLProcResolver& resolver,
                     const char* name) {
  return reinterpret_cast<Fn>(resolver(name));
}

constexpr char kCpuBlitVertexShaderSource[] = R"(
    precision mediump float;
    attribute vec2 aPosition;
    attribute vec2 aTexCoord;
    varying vec2 vTexCoord;
    void main() {
      gl_Position = vec4(aPosition, 0.0, 1.0);
      vTexCoord = aTexCoord;
    }
)";

// Explicit attribute location indices
constexpr GLuint kAttribLocationPosition = 0;
constexpr GLuint kAttribLocationTexCoord = 1;

constexpr char kCpuBlitFragmentShaderSource[] = R"(
    precision mediump float;
    varying vec2 vTexCoord;
    uniform sampler2D uTexture;
    void main() {
      gl_FragColor = texture2D(uTexture, vTexCoord).bgra;
    }
)";

}  // namespace

class HostGLRenderer {
 public:
  explicit HostGLRenderer(GPUSurfaceGLDelegate::GLProcResolver resolver)
      : resolver_(std::move(resolver)) {}

  bool Draw(const skity::Pixmap& pixmap, const skity::Matrix& transformation,
            GLuint framebuffer) {
    if (!Initialize()) {
      return false;
    }

    GLint previous_program = 0;
    GLint previous_active_texture = 0;
    GLint previous_texture0_binding = 0;
    GLint previous_framebuffer = 0;
    GLint previous_vertex_array = 0;
    GLint previous_array_buffer = 0;
    GLint previous_unpack_alignment = 4;
    GLint previous_viewport[4] = {0, 0, 0, 0};

    gl_get_integerv_(GL_CURRENT_PROGRAM, &previous_program);
    gl_get_integerv_(GL_ACTIVE_TEXTURE, &previous_active_texture);
    gl_get_integerv_(GL_FRAMEBUFFER_BINDING, &previous_framebuffer);
    if (supports_vertex_array_object_) {
      gl_get_integerv_(GL_VERTEX_ARRAY_BINDING, &previous_vertex_array);
    }
    gl_get_integerv_(GL_ARRAY_BUFFER_BINDING, &previous_array_buffer);
    gl_get_integerv_(GL_UNPACK_ALIGNMENT, &previous_unpack_alignment);
    gl_get_integerv_(GL_VIEWPORT, previous_viewport);
    gl_get_integerv_(GL_TEXTURE_BINDING_2D, &previous_texture0_binding);

    gl_bind_framebuffer_(GL_FRAMEBUFFER, framebuffer);
    gl_viewport_(0, 0, static_cast<GLsizei>(pixmap.Width()),
                 static_cast<GLsizei>(pixmap.Height()));
    gl_clear_color_(0.0f, 0.0f, 0.0f, 0.0f);
    gl_clear_(GL_COLOR_BUFFER_BIT);

    gl_use_program_(program_);
    gl_active_texture_(GL_TEXTURE0);
    gl_bind_texture_(GL_TEXTURE_2D, texture_);
    if (supports_vertex_array_object_) {
      gl_bind_vertex_array_(vertex_array_);
    }
    gl_pixel_store_i_(GL_UNPACK_ALIGNMENT, 1);

    const void* pixels = pixmap.Addr();
    std::vector<uint8_t> contiguous_pixels;
    size_t packed_row_bytes = static_cast<size_t>(pixmap.Width()) * 4;
    if (pixmap.RowBytes() != packed_row_bytes) {
      contiguous_pixels.resize(packed_row_bytes * pixmap.Height());
      for (uint32_t row = 0; row < pixmap.Height(); ++row) {
        std::memcpy(contiguous_pixels.data() + row * packed_row_bytes,
                    static_cast<const uint8_t*>(pixmap.Addr()) +
                        row * pixmap.RowBytes(),
                    packed_row_bytes);
      }
      pixels = contiguous_pixels.data();
    }

    gl_tex_image_2d_(GL_TEXTURE_2D, 0, GL_RGBA,
                     static_cast<GLsizei>(pixmap.Width()),
                     static_cast<GLsizei>(pixmap.Height()), 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, pixels);

    const GLfloat positions[] = {
        -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f,
        -1.0f, 1.0f,  1.0f, -1.0f, 1.0f,  1.0f,
    };

    skity::Matrix uv_transform = transformation;
    skity::Vec2 src[4] = {
        {0.f, 0.f},
        {1.f, 0.f},
        {0.f, 1.f},
        {1.f, 1.f},
    };
    skity::Vec2 dst[4];
    uv_transform.MapPoints(dst, src, 4);

    const GLfloat tex_coords[] = {
        dst[0].x, dst[0].y, dst[1].x, dst[1].y, dst[2].x, dst[2].y,
        dst[2].x, dst[2].y, dst[1].x, dst[1].y, dst[3].x, dst[3].y,
    };

    gl_bind_buffer_(GL_ARRAY_BUFFER, position_buffer_);
    gl_buffer_data_(GL_ARRAY_BUFFER, sizeof(positions), positions,
                    GL_STATIC_DRAW);
    gl_enable_vertex_attrib_array_(kAttribLocationPosition);
    gl_vertex_attrib_pointer_(kAttribLocationPosition, 2, GL_FLOAT, GL_FALSE, 0,
                              nullptr);
    gl_bind_buffer_(GL_ARRAY_BUFFER, tex_coord_buffer_);
    gl_buffer_data_(GL_ARRAY_BUFFER, sizeof(tex_coords), tex_coords,
                    GL_STATIC_DRAW);
    gl_enable_vertex_attrib_array_(kAttribLocationTexCoord);
    gl_vertex_attrib_pointer_(kAttribLocationTexCoord, 2, GL_FLOAT, GL_FALSE, 0,
                              nullptr);

    gl_draw_arrays_(GL_TRIANGLES, 0, 6);
    gl_disable_vertex_attrib_array_(kAttribLocationPosition);
    gl_disable_vertex_attrib_array_(kAttribLocationTexCoord);

    gl_bind_framebuffer_(GL_FRAMEBUFFER,
                         static_cast<GLuint>(previous_framebuffer));
    if (supports_vertex_array_object_) {
      gl_bind_vertex_array_(static_cast<GLuint>(previous_vertex_array));
    }
    gl_bind_buffer_(GL_ARRAY_BUFFER,
                    static_cast<GLuint>(previous_array_buffer));
    gl_bind_texture_(GL_TEXTURE_2D,
                     static_cast<GLuint>(previous_texture0_binding));
    gl_active_texture_(static_cast<GLenum>(previous_active_texture));
    gl_use_program_(static_cast<GLuint>(previous_program));
    gl_pixel_store_i_(GL_UNPACK_ALIGNMENT, previous_unpack_alignment);
    gl_viewport_(previous_viewport[0], previous_viewport[1],
                 previous_viewport[2], previous_viewport[3]);
    return true;
  }

  void Destroy() {
    if (position_buffer_ != 0) {
      gl_delete_buffers_(1, &position_buffer_);
      position_buffer_ = 0;
    }
    if (tex_coord_buffer_ != 0) {
      gl_delete_buffers_(1, &tex_coord_buffer_);
      tex_coord_buffer_ = 0;
    }
    if (vertex_array_ != 0) {
      gl_delete_vertex_arrays_(1, &vertex_array_);
      vertex_array_ = 0;
    }
    if (texture_ != 0) {
      gl_delete_textures_(1, &texture_);
      texture_ = 0;
    }
    if (program_ != 0) {
      gl_delete_program_(program_);
      program_ = 0;
    }
    initialized_ = false;
  }

 private:
  using GLCreateShaderProc = GLuint (*)(GLenum);
  using GLShaderSourceProc = void (*)(GLuint, GLsizei, const GLchar* const*,
                                      const GLint*);
  using GLCompileShaderProc = void (*)(GLuint);
  using GLGetShaderivProc = void (*)(GLuint, GLenum, GLint*);
  using GLDeleteShaderProc = void (*)(GLuint);
  using GLCreateProgramProc = GLuint (*)();
  using GLAttachShaderProc = void (*)(GLuint, GLuint);
  using GLLinkProgramProc = void (*)(GLuint);
  using GLGetProgramivProc = void (*)(GLuint, GLenum, GLint*);
  using GLDeleteProgramProc = void (*)(GLuint);
  using GLUseProgramProc = void (*)(GLuint);
  using GLGetUniformLocationProc = GLint (*)(GLuint, const GLchar*);
  using GLUniform1iProc = void (*)(GLint, GLint);
  using GLGenTexturesProc = void (*)(GLsizei, GLuint*);
  using GLDeleteTexturesProc = void (*)(GLsizei, const GLuint*);
  using GLActiveTextureProc = void (*)(GLenum);
  using GLBindTextureProc = void (*)(GLenum, GLuint);
  using GLTexParameteriProc = void (*)(GLenum, GLenum, GLint);
  using GLTexImage2DProc = void (*)(GLenum, GLint, GLint, GLsizei, GLsizei,
                                    GLint, GLenum, GLenum, const void*);
  using GLGenBuffersProc = void (*)(GLsizei, GLuint*);
  using GLDeleteBuffersProc = void (*)(GLsizei, const GLuint*);
  using GLBufferDataProc = void (*)(GLenum, GLsizeiptr, const void*, GLenum);
  using GLBindFramebufferProc = void (*)(GLenum, GLuint);
  using GLViewportProc = void (*)(GLint, GLint, GLsizei, GLsizei);
  using GLGenVertexArraysProc = void (*)(GLsizei, GLuint*);
  using GLDeleteVertexArraysProc = void (*)(GLsizei, const GLuint*);
  using GLBindVertexArrayProc = void (*)(GLuint);
  using GLEnableVertexAttribArrayProc = void (*)(GLuint);
  using GLDisableVertexAttribArrayProc = void (*)(GLuint);
  using GLVertexAttribPointerProc = void (*)(GLuint, GLint, GLenum, GLboolean,
                                             GLsizei, const void*);
  using GLDrawArraysProc = void (*)(GLenum, GLint, GLsizei);
  using GLGetIntegervProc = void (*)(GLenum, GLint*);
  using GLBindBufferProc = void (*)(GLenum, GLuint);
  using GLPixelStoreiProc = void (*)(GLenum, GLint);
  using GLClearProc = void (*)(GLbitfield);
  using GLClearColorProc = void (*)(GLclampf, GLclampf, GLclampf, GLclampf);
  using GLBindAttribLocationProc = void (*)(GLuint, GLuint, const GLchar*);

  bool Initialize() {
    if (initialized_) {
      return true;
    }
    if (!resolver_) {
      FML_LOG(ERROR) << "No GL proc resolver for HostGLRenderer";
      return false;
    }

    gl_create_shader_ =
        ResolveGLFunction<GLCreateShaderProc>(resolver_, "glCreateShader");
    gl_shader_source_ =
        ResolveGLFunction<GLShaderSourceProc>(resolver_, "glShaderSource");
    gl_compile_shader_ =
        ResolveGLFunction<GLCompileShaderProc>(resolver_, "glCompileShader");
    gl_get_shader_iv_ =
        ResolveGLFunction<GLGetShaderivProc>(resolver_, "glGetShaderiv");
    gl_delete_shader_ =
        ResolveGLFunction<GLDeleteShaderProc>(resolver_, "glDeleteShader");
    gl_create_program_ =
        ResolveGLFunction<GLCreateProgramProc>(resolver_, "glCreateProgram");
    gl_attach_shader_ =
        ResolveGLFunction<GLAttachShaderProc>(resolver_, "glAttachShader");
    gl_link_program_ =
        ResolveGLFunction<GLLinkProgramProc>(resolver_, "glLinkProgram");
    gl_get_program_iv_ =
        ResolveGLFunction<GLGetProgramivProc>(resolver_, "glGetProgramiv");
    gl_delete_program_ =
        ResolveGLFunction<GLDeleteProgramProc>(resolver_, "glDeleteProgram");
    gl_use_program_ =
        ResolveGLFunction<GLUseProgramProc>(resolver_, "glUseProgram");
    gl_get_uniform_location_ = ResolveGLFunction<GLGetUniformLocationProc>(
        resolver_, "glGetUniformLocation");
    gl_uniform_1_i_ =
        ResolveGLFunction<GLUniform1iProc>(resolver_, "glUniform1i");
    gl_bind_attrib_location_ = ResolveGLFunction<GLBindAttribLocationProc>(
        resolver_, "glBindAttribLocation");
    gl_gen_textures_ =
        ResolveGLFunction<GLGenTexturesProc>(resolver_, "glGenTextures");
    gl_delete_textures_ =
        ResolveGLFunction<GLDeleteTexturesProc>(resolver_, "glDeleteTextures");
    gl_active_texture_ =
        ResolveGLFunction<GLActiveTextureProc>(resolver_, "glActiveTexture");
    gl_bind_texture_ =
        ResolveGLFunction<GLBindTextureProc>(resolver_, "glBindTexture");
    gl_tex_parameter_i_ =
        ResolveGLFunction<GLTexParameteriProc>(resolver_, "glTexParameteri");
    gl_tex_image_2d_ =
        ResolveGLFunction<GLTexImage2DProc>(resolver_, "glTexImage2D");
    gl_gen_buffers_ =
        ResolveGLFunction<GLGenBuffersProc>(resolver_, "glGenBuffers");
    gl_delete_buffers_ =
        ResolveGLFunction<GLDeleteBuffersProc>(resolver_, "glDeleteBuffers");
    gl_buffer_data_ =
        ResolveGLFunction<GLBufferDataProc>(resolver_, "glBufferData");
    gl_bind_framebuffer_ = ResolveGLFunction<GLBindFramebufferProc>(
        resolver_, "glBindFramebuffer");
    gl_viewport_ = ResolveGLFunction<GLViewportProc>(resolver_, "glViewport");
    gl_gen_vertex_arrays_ = ResolveGLFunction<GLGenVertexArraysProc>(
        resolver_, "glGenVertexArrays");
    gl_delete_vertex_arrays_ = ResolveGLFunction<GLDeleteVertexArraysProc>(
        resolver_, "glDeleteVertexArrays");
    gl_bind_vertex_array_ = ResolveGLFunction<GLBindVertexArrayProc>(
        resolver_, "glBindVertexArray");
    if (!gl_gen_vertex_arrays_ || !gl_delete_vertex_arrays_ ||
        !gl_bind_vertex_array_) {
      gl_gen_vertex_arrays_ = ResolveGLFunction<GLGenVertexArraysProc>(
          resolver_, "glGenVertexArraysOES");
      gl_delete_vertex_arrays_ = ResolveGLFunction<GLDeleteVertexArraysProc>(
          resolver_, "glDeleteVertexArraysOES");
      gl_bind_vertex_array_ = ResolveGLFunction<GLBindVertexArrayProc>(
          resolver_, "glBindVertexArrayOES");
    }
    supports_vertex_array_object_ = gl_gen_vertex_arrays_ &&
                                    gl_delete_vertex_arrays_ &&
                                    gl_bind_vertex_array_;
    gl_enable_vertex_attrib_array_ =
        ResolveGLFunction<GLEnableVertexAttribArrayProc>(
            resolver_, "glEnableVertexAttribArray");
    gl_disable_vertex_attrib_array_ =
        ResolveGLFunction<GLDisableVertexAttribArrayProc>(
            resolver_, "glDisableVertexAttribArray");
    gl_vertex_attrib_pointer_ = ResolveGLFunction<GLVertexAttribPointerProc>(
        resolver_, "glVertexAttribPointer");
    gl_draw_arrays_ =
        ResolveGLFunction<GLDrawArraysProc>(resolver_, "glDrawArrays");
    gl_get_integerv_ =
        ResolveGLFunction<GLGetIntegervProc>(resolver_, "glGetIntegerv");
    gl_bind_buffer_ =
        ResolveGLFunction<GLBindBufferProc>(resolver_, "glBindBuffer");
    gl_pixel_store_i_ =
        ResolveGLFunction<GLPixelStoreiProc>(resolver_, "glPixelStorei");
    gl_clear_ = ResolveGLFunction<GLClearProc>(resolver_, "glClear");
    gl_clear_color_ =
        ResolveGLFunction<GLClearColorProc>(resolver_, "glClearColor");

    if (!gl_create_shader_ || !gl_shader_source_ || !gl_compile_shader_ ||
        !gl_get_shader_iv_ || !gl_delete_shader_ || !gl_create_program_ ||
        !gl_attach_shader_ || !gl_link_program_ || !gl_get_program_iv_ ||
        !gl_delete_program_ || !gl_use_program_ || !gl_get_uniform_location_ ||
        !gl_uniform_1_i_ || !gl_bind_attrib_location_ || !gl_gen_textures_ ||
        !gl_delete_textures_ || !gl_active_texture_ || !gl_bind_texture_ ||
        !gl_tex_parameter_i_ || !gl_tex_image_2d_ || !gl_gen_buffers_ ||
        !gl_delete_buffers_ || !gl_buffer_data_ || !gl_bind_framebuffer_ ||
        !gl_viewport_ || !gl_enable_vertex_attrib_array_ ||
        !gl_disable_vertex_attrib_array_ || !gl_vertex_attrib_pointer_ ||
        !gl_draw_arrays_ || !gl_get_integerv_ || !gl_bind_buffer_ ||
        !gl_pixel_store_i_ || !gl_clear_ || !gl_clear_color_) {
      FML_LOG(ERROR) << "Failed to resolve GL functions for HostGLRenderer";
      return false;
    }

    program_ =
        CreateProgram(kCpuBlitVertexShaderSource, kCpuBlitFragmentShaderSource);
    if (program_ == 0) {
      return false;
    }

    GLint sampler_location = gl_get_uniform_location_(program_, "uTexture");
    if (sampler_location < 0) {
      FML_LOG(ERROR) << "Failed to query HostGLRenderer shader locations";
      Destroy();
      return false;
    }

    gl_gen_textures_(1, &texture_);
    if (texture_ == 0) {
      FML_LOG(ERROR) << "Failed to create texture for HostGLRenderer";
      Destroy();
      return false;
    }
    if (supports_vertex_array_object_) {
      gl_gen_vertex_arrays_(1, &vertex_array_);
    }
    gl_gen_buffers_(1, &position_buffer_);
    gl_gen_buffers_(1, &tex_coord_buffer_);
    if ((supports_vertex_array_object_ && vertex_array_ == 0) ||
        position_buffer_ == 0 || tex_coord_buffer_ == 0) {
      FML_LOG(ERROR) << "Failed to create vertex resources for HostGLRenderer";
      Destroy();
      return false;
    }

    GLint previous_program = 0;
    GLint previous_active_texture = 0;
    GLint previous_texture0_binding = 0;
    GLint previous_vertex_array = 0;
    gl_get_integerv_(GL_CURRENT_PROGRAM, &previous_program);
    gl_get_integerv_(GL_ACTIVE_TEXTURE, &previous_active_texture);
    if (supports_vertex_array_object_) {
      gl_get_integerv_(GL_VERTEX_ARRAY_BINDING, &previous_vertex_array);
    }
    gl_active_texture_(GL_TEXTURE0);
    gl_get_integerv_(GL_TEXTURE_BINDING_2D, &previous_texture0_binding);

    gl_use_program_(program_);
    if (supports_vertex_array_object_) {
      gl_bind_vertex_array_(vertex_array_);
    }
    gl_bind_texture_(GL_TEXTURE_2D, texture_);
    gl_tex_parameter_i_(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl_tex_parameter_i_(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl_tex_parameter_i_(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl_tex_parameter_i_(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl_uniform_1_i_(sampler_location, 0);
    if (supports_vertex_array_object_) {
      gl_bind_vertex_array_(static_cast<GLuint>(previous_vertex_array));
    }
    gl_bind_texture_(GL_TEXTURE_2D,
                     static_cast<GLuint>(previous_texture0_binding));
    gl_active_texture_(static_cast<GLenum>(previous_active_texture));
    gl_use_program_(static_cast<GLuint>(previous_program));

    initialized_ = true;
    return true;
  }

  GLuint CompileShader(GLenum type, const char* source) {
    GLuint shader = gl_create_shader_(type);
    if (shader == 0) {
      FML_LOG(ERROR) << "Failed to create GL shader";
      return 0;
    }
    gl_shader_source_(shader, 1, &source, nullptr);
    gl_compile_shader_(shader);

    GLint compiled = GL_FALSE;
    gl_get_shader_iv_(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
      FML_LOG(ERROR) << "Failed to compile GL shader";
      gl_delete_shader_(shader);
      return 0;
    }
    return shader;
  }

  GLuint CreateProgram(const char* vertex_source, const char* fragment_source) {
    GLuint vertex_shader = CompileShader(GL_VERTEX_SHADER, vertex_source);
    GLuint fragment_shader = CompileShader(GL_FRAGMENT_SHADER, fragment_source);
    if (vertex_shader == 0 || fragment_shader == 0) {
      if (vertex_shader != 0) {
        gl_delete_shader_(vertex_shader);
      }
      if (fragment_shader != 0) {
        gl_delete_shader_(fragment_shader);
      }
      return 0;
    }

    GLuint program = gl_create_program_();
    if (program == 0) {
      FML_LOG(ERROR) << "Failed to create GL program";
      gl_delete_shader_(vertex_shader);
      gl_delete_shader_(fragment_shader);
      return 0;
    }

    gl_attach_shader_(program, vertex_shader);
    gl_attach_shader_(program, fragment_shader);

    // Bind attribute locations BEFORE linking
    gl_bind_attrib_location_(program, kAttribLocationPosition, "aPosition");
    gl_bind_attrib_location_(program, kAttribLocationTexCoord, "aTexCoord");

    gl_link_program_(program);
    gl_delete_shader_(vertex_shader);
    gl_delete_shader_(fragment_shader);

    GLint linked = GL_FALSE;
    gl_get_program_iv_(program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
      FML_LOG(ERROR) << "Failed to link GL program";
      gl_delete_program_(program);
      return 0;
    }
    return program;
  }

  GPUSurfaceGLDelegate::GLProcResolver resolver_;
  bool initialized_ = false;
  GLuint program_ = 0;
  GLuint texture_ = 0;
  GLuint vertex_array_ = 0;
  GLuint position_buffer_ = 0;
  GLuint tex_coord_buffer_ = 0;
  bool supports_vertex_array_object_ = false;

  GLCreateShaderProc gl_create_shader_ = nullptr;
  GLShaderSourceProc gl_shader_source_ = nullptr;
  GLCompileShaderProc gl_compile_shader_ = nullptr;
  GLGetShaderivProc gl_get_shader_iv_ = nullptr;
  GLDeleteShaderProc gl_delete_shader_ = nullptr;
  GLCreateProgramProc gl_create_program_ = nullptr;
  GLAttachShaderProc gl_attach_shader_ = nullptr;
  GLLinkProgramProc gl_link_program_ = nullptr;
  GLGetProgramivProc gl_get_program_iv_ = nullptr;
  GLDeleteProgramProc gl_delete_program_ = nullptr;
  GLUseProgramProc gl_use_program_ = nullptr;
  GLGetUniformLocationProc gl_get_uniform_location_ = nullptr;
  GLUniform1iProc gl_uniform_1_i_ = nullptr;
  GLBindAttribLocationProc gl_bind_attrib_location_ = nullptr;
  GLGenTexturesProc gl_gen_textures_ = nullptr;
  GLDeleteTexturesProc gl_delete_textures_ = nullptr;
  GLActiveTextureProc gl_active_texture_ = nullptr;
  GLBindTextureProc gl_bind_texture_ = nullptr;
  GLTexParameteriProc gl_tex_parameter_i_ = nullptr;
  GLTexImage2DProc gl_tex_image_2d_ = nullptr;
  GLGenBuffersProc gl_gen_buffers_ = nullptr;
  GLDeleteBuffersProc gl_delete_buffers_ = nullptr;
  GLBufferDataProc gl_buffer_data_ = nullptr;
  GLBindFramebufferProc gl_bind_framebuffer_ = nullptr;
  GLViewportProc gl_viewport_ = nullptr;
  GLGenVertexArraysProc gl_gen_vertex_arrays_ = nullptr;
  GLDeleteVertexArraysProc gl_delete_vertex_arrays_ = nullptr;
  GLBindVertexArrayProc gl_bind_vertex_array_ = nullptr;
  GLEnableVertexAttribArrayProc gl_enable_vertex_attrib_array_ = nullptr;
  GLDisableVertexAttribArrayProc gl_disable_vertex_attrib_array_ = nullptr;
  GLVertexAttribPointerProc gl_vertex_attrib_pointer_ = nullptr;
  GLDrawArraysProc gl_draw_arrays_ = nullptr;
  GLGetIntegervProc gl_get_integerv_ = nullptr;
  GLBindBufferProc gl_bind_buffer_ = nullptr;
  GLPixelStoreiProc gl_pixel_store_i_ = nullptr;
  GLClearProc gl_clear_ = nullptr;
  GLClearColorProc gl_clear_color_ = nullptr;
};
#endif

std::unique_ptr<ClayHeadlessRenderer> ClayHeadlessRenderer::CreateHostGL(
    ClayHeadlessEngine* engine, const ClayOpenGLRendererConfig& config) {
  const ClayOpenGLRendererConfig* config_ptr = &config;
  if (SAFE_ACCESS(config_ptr, enable_shared_image_sink, false)) {
    ClaySharedImageSinkBufferMode buffer_mode =
        SAFE_ACCESS(config_ptr, shared_image_sink_buffer_mode,
                    kClaySharedImageSinkBufferModeDoubleBuffer);
    return std::make_unique<ClayHeadlessRendererSharedImageHostGL>(
        engine, config, buffer_mode);

  } else {
    return std::make_unique<ClayHeadlessRendererHostGL>(engine, config);
  }
}

ClayHeadlessRendererHostGL::ClayHeadlessRendererHostGL(
    ClayHeadlessEngine* engine, const ClayOpenGLRendererConfig& renderer_config)
    : ClayHeadlessRendererGL(engine), config_(renderer_config) {
  FML_LOG(ERROR) << "Starting Clay in [Host GL] mode. "
                    "Components using external textures will NOT work";
}

GPUSurfaceGLDelegate::GLProcResolver
ClayHeadlessRendererHostGL::GetGLProcResolver() const {
  return [this](const char* name) -> void* {
    return const_cast<ClayHeadlessRendererHostGL*>(this)->ResolveProc(name);
  };
}

bool ClayHeadlessRendererHostGL::MakeCurrent() {
  return config_.make_current(engine_->UserData());
}

bool ClayHeadlessRendererHostGL::ClearCurrent() {
  return config_.clear_current(engine_->UserData());
}

bool ClayHeadlessRendererHostGL::Present() {
  return config_.present(engine_->UserData());
}

int64_t ClayHeadlessRendererHostGL::FBO(const ClayFrameInfo& frame_info) {
  return config_.fbo_callback(engine_->UserData(), &frame_info);
}

void* ClayHeadlessRendererHostGL::ResolveProc(const char* name) {
  return config_.gl_proc_resolver(engine_->UserData(), name);
}

void ClayHeadlessRendererHostGL::CleanupGPUResources() {}

ClayHeadlessRendererSharedImageHostGL::ClayHeadlessRendererSharedImageHostGL(
    ClayHeadlessEngine* engine, const ClayOpenGLRendererConfig& renderer_config,
    ClaySharedImageSinkBufferMode buffer_mode)
    : ClayHeadlessRenderer(engine),
      host_gl_thread_("clay.headless.host-gl"),
      config_(renderer_config) {
  FML_LOG(ERROR) << "Starting Clay in [Host GL+SharedImage] mode. "
                    "Maybe slow in large views";

  host_gl_thread_.GetTaskRunner()->PostTask([&] {
#ifndef ENABLE_SKITY
    host_gl_surface_ = std::make_unique<GPUSurfaceGLSkia>(this, true);
#endif
  });

  ClayHeadlessRendererConfig hardware_config;

  ClaySharedImageBackingType image_backing_type;

#if OS_MACOSX
  image_backing_type = kClaySharedImageBackingTypeIOSurface;
  hardware_config.type = kClayRendererTypeMetal;
#elif OS_WIN
  image_backing_type = kClaySharedImageBackingTypeD3DTexture;
  hardware_config.type = kClayRendererTypeOpenGL;
#elif OS_LINUX
  image_backing_type = kClaySharedImageBackingTypeShmImage;
  hardware_config.type = kClayRendererTypeOpenGL;
#elif OS_HARMONY
  image_backing_type = kClaySharedImageBackingTypeNativeImage;
  hardware_config.type = kClayRendererTypeOpenGL;
#else
  FML_DCHECK(false) << "Shared Image Renderer not supported on this platform";
  return;
#endif

  ClaySharedImageSinkRef sink_ref =
      ClayCreateSharedImageSink(buffer_mode, image_backing_type,
                                kClaySharedImageBackingPixelFormatNative8888);
  shared_image_sink_ = fml::RefPtr<clay::SharedImageSink>(
      reinterpret_cast<clay::SharedImageSink*>(sink_ref));

  shared_image_sink_->SetFrameAvailableCallback([this] {
    // The callback is triggered in Clay Raster thread
    host_gl_thread_.GetTaskRunner()->PostTask([this] { Draw(); });
  });

  hardware_config.hardware.struct_size = sizeof(hardware_config.hardware);
  hardware_config.hardware.sink_ref = sink_ref;
  renderer_ = ClayHeadlessRenderer::Create(engine, hardware_config);

  FML_CHECK(renderer_);

  // sink_ref is owned by shared_image_sink_
  ClayReleaseSharedImageSink(sink_ref);
}

// |ClayHeadlessRenderer|
EmbedderSurfaceSoftwareDelegate*
ClayHeadlessRendererSharedImageHostGL::GetSoftwareRendererDelegate() {
  return renderer_->GetSoftwareRendererDelegate();
}
#ifdef SHELL_ENABLE_GL
// |ClayHeadlessRenderer|
GPUSurfaceGLDelegate*
ClayHeadlessRendererSharedImageHostGL::GetGLRendererDelegate() {
  return renderer_->GetGLRendererDelegate();
}
#endif
#ifdef SHELL_ENABLE_METAL
// |ClayHeadlessRenderer|
EmbedderSurfaceMetalDelegate*
ClayHeadlessRendererSharedImageHostGL::GetMetalRendererDelegate() {
  return renderer_->GetMetalRendererDelegate();
}
#endif

ClayHeadlessRenderer*
ClayHeadlessRendererSharedImageHostGL::GetEngineRenderer() {
  return renderer_.get();
}

ClayHeadlessRendererSharedImageHostGL::
    ~ClayHeadlessRendererSharedImageHostGL() {
  {
    std::lock_guard<std::mutex> lock(shared_image_sink_mutex_);
    // shared_image_sink internally keeps ref to D3D device and mutex,
    // which means it should be reset before destroy renderer
    shared_image_sink_->SetFrameAvailableCallback(nullptr);
    shared_image_sink_ = nullptr;
  }
  renderer_.reset();
  fml::AutoResetWaitableEvent latch;
  host_gl_thread_.GetTaskRunner()->PostTask([&] {
#ifdef ENABLE_SKITY
    if (host_gl_renderer_) {
      auto context_switch = GLContextMakeCurrent();
      if (context_switch && context_switch->GetResult()) {
        host_gl_renderer_->Destroy();
      }
      host_gl_renderer_.reset();
      GLContextClearCurrent();
    }
#endif
#ifndef ENABLE_SKITY
    host_gl_surface_.reset();
#endif
    latch.Signal();
  });
  latch.Wait();
}

void ClayHeadlessRendererSharedImageHostGL::CleanupGPUResources() {
  renderer_->CleanupGPUResources();
}

// |GPUSurfaceGLDelegate|
std::unique_ptr<GLContextResult>
ClayHeadlessRendererSharedImageHostGL::GLContextMakeCurrent() {
  return std::make_unique<GLContextDefaultResult>(
      config_.make_current(engine_->UserData()));
}

// |GPUSurfaceGLDelegate|
bool ClayHeadlessRendererSharedImageHostGL::GLContextClearCurrent() {
  return config_.clear_current(engine_->UserData());
}

// |GPUSurfaceGLDelegate|
bool ClayHeadlessRendererSharedImageHostGL::GLContextPresent(
    const GLPresentInfo& present_info) {
  return config_.present(engine_->UserData());
}

// |GPUSurfaceGLDelegate|
bool ClayHeadlessRendererSharedImageHostGL::GLContextFBOResetAfterPresent()
    const {
  return true;
}

// |GPUSurfaceGLDelegate|
GLFBOInfo ClayHeadlessRendererSharedImageHostGL::GLContextFBO(
    GLFrameInfo frame_info) const {
  ClayFrameInfo clay_frame_info{};
  clay_frame_info.struct_size = sizeof(clay_frame_info);
  clay_frame_info.width = frame_info.width;
  clay_frame_info.height = frame_info.height;
  return {.fbo_id = config_.fbo_callback(engine_->UserData(), &clay_frame_info),
          .existing_damage = {}};
}

// |GPUSurfaceGLDelegate|
GPUSurfaceGLDelegate::GLProcResolver
ClayHeadlessRendererSharedImageHostGL::GetGLProcResolver() const {
  return [gl_proc_resolver = config_.gl_proc_resolver,
          user_data = engine_->UserData()](const char* name) -> void* {
    return gl_proc_resolver(user_data, name);
  };
}

void ClayHeadlessRendererSharedImageHostGL::Draw() {
  TRACE_EVENT("clay", __FUNCTION__);
  std::lock_guard<std::mutex> lock(shared_image_sink_mutex_);
  if (!shared_image_sink_) {
    return;
  }
#ifndef ENABLE_SKITY
  if (!host_gl_surface_) {
    return;
  }
#endif

#ifdef ENABLE_SKITY
  skity::Matrix transformation;
  std::shared_ptr<skity::Pixmap> pixmap;
#else
  SkMatrix transformation;
  SkBitmap bitmap;
#endif
  {
    fml::RefPtr<clay::SharedImageBacking> backing =
        shared_image_sink_->UpdateFront(nullptr);
    if (!backing) {
      FML_LOG(ERROR) << "No front buffer";
      return;
    }

    // We don't need to hold the front, so we always release it
    struct AutoReleaseSink {
      explicit AutoReleaseSink(clay::SharedImageSink& sink) : sink_(sink) {}

      ~AutoReleaseSink() { sink_.ReleaseFront(nullptr); }

      clay::SharedImageSink& sink_;
    };

    AutoReleaseSink auto_release_sink(*shared_image_sink_);

    if (backing->GetPixelFormat() !=
        clay::SharedImageBacking::PixelFormat::kNative8888) {
      FML_LOG(ERROR) << "PixelFormat not supported: "
                     << static_cast<uint32_t>(backing->GetPixelFormat());
      return;
    }

    if (std::unique_ptr<clay::FenceSync> fence_sync = backing->GetFenceSync()) {
      if (!fence_sync->ClientWait()) {
        FML_LOG(ERROR) << "Failed to wait sync";
        return;
      }
    }

#ifndef ENABLE_SKITY
    // Currently, kNative8888 equals BGRA8888
    auto image_info = SkImageInfo::Make(
        SkISize::Make(backing->GetSize().x, backing->GetSize().y),
        kBGRA_8888_SkColorType, kPremul_SkAlphaType);
    if (!bitmap.tryAllocPixels(image_info, 0)) {
      FML_LOG(ERROR) << "Failed to allocate bitmap pixels";
      return;
    }
    SkPixmap pixmap;
    if (!bitmap.peekPixels(&pixmap)) {
      FML_LOG(ERROR) << "Failed to peek bitmap pixels";
      return;
    }
#endif

    {
      TRACE_EVENT("clay", "SharedImageBacking::ReadbackToMemory");
#ifdef ENABLE_SKITY
      pixmap = std::make_shared<skity::Pixmap>(
          backing->GetSize().x, backing->GetSize().y,
          skity::AlphaType::kUnpremul_AlphaType, skity::ColorType::kBGRA);
      if (!backing->ReadbackToMemory(pixmap.get(), 1)) {
        FML_LOG(ERROR) << "Failed to ReadbackToMemory";
        return;
      }
      transformation = backing->GetTransformation();
#else
      if (!backing->ReadbackToMemory(&pixmap, 1)) {
        FML_LOG(ERROR) << "Failed to ReadbackToMemory";
        return;
      }

      bitmap.setImmutable();

      transformation =
          clay::ConvertSkityMatrixToSkMatrix(backing->GetTransformation());
#endif
    }
  }

#ifdef ENABLE_SKITY
  if (!pixmap) {
    FML_LOG(ERROR) << "No pixmap for Host GL CPU blit";
    return;
  }

  auto context_switch = GLContextMakeCurrent();
  if (!context_switch || !context_switch->GetResult()) {
    FML_LOG(ERROR) << "Failed to make current for Host GL CPU blit";
    return;
  }

  if (!host_gl_renderer_) {
    GLProcResolver proc_resolver = GetGLProcResolver();
    if (!proc_resolver) {
      FML_LOG(ERROR) << "No GL proc resolver for Host GL CPU blit";
      return;
    }
    host_gl_renderer_ =
        std::make_unique<HostGLRenderer>(std::move(proc_resolver));
  }

  GLFrameInfo frame_info = {pixmap->Width(), pixmap->Height()};
  const GLFBOInfo fbo_info = GLContextFBO(frame_info);
  if (fbo_info.fbo_id < 0 ||
      fbo_info.fbo_id >
          static_cast<int64_t>(std::numeric_limits<uint32_t>::max())) {
    FML_LOG(ERROR) << "Invalid FBO id for Host GL renderer: "
                   << fbo_info.fbo_id;
    return;
  }
  const GLuint target_fbo = static_cast<GLuint>(fbo_info.fbo_id);

  if (!host_gl_renderer_->Draw(*pixmap, transformation, target_fbo)) {
    FML_LOG(ERROR) << "Failed to draw Host GL CPU blit";
    return;
  }

  if (!GLContextPresent(
          {target_fbo, std::nullopt, std::nullopt, std::nullopt})) {
    FML_LOG(ERROR) << "Failed to present Host GL CPU blit";
  }
  return;
#else
  std::unique_ptr<SurfaceFrame> frame = host_gl_surface_->AcquireFrame(
      {bitmap.dimensions().fWidth, bitmap.dimensions().fHeight});

  if (!frame) {
    FML_LOG(ERROR) << "Failed to AcquireFrame";
    return;
  }

  SkCanvas* canvas = frame->GetCanvas();
  canvas->clear(SK_ColorTRANSPARENT);

  SkAutoCanvasRestore autoRestore(canvas, true);

  sk_sp<SkImage> sk_image = bitmap.asImage();

  SkIRect bounds = sk_image->bounds();

  // The incoming texture is vertically flipped, so we flip it
  // back.
  // Maybe it's better to use SurfaceOrigin in AdoptTexture,
  // but on Electron this method doesn't work.
  SkMatrix flip_y_mat =
      SkMatrix::MakeAll(1, 0, 0, 0, -1, bounds.height(), 0, 0, 1);

  canvas->concat(flip_y_mat);

  if (!transformation.isIdentity()) {
    sk_sp<SkShader> shader =
        sk_image->makeShader(SkTileMode::kRepeat, SkTileMode::kRepeat,
                             SkSamplingOptions(), transformation);

    SkPaint paintWithShader;
    paintWithShader.setShader(shader);
    canvas->drawRect(SkRect::Make(sk_image->bounds()), paintWithShader);
  } else {
    canvas->drawImage(sk_image, 0, 0, SkSamplingOptions());
  }
  frame->Submit();

  host_gl_surface_->GetContext()->performDeferredCleanup(
      std::chrono::milliseconds(0));
#endif
}

}  // namespace clay
