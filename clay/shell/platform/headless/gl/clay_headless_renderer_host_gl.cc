// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/headless/gl/clay_headless_renderer_host_gl.h"

#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/trace/native/trace_event.h"
#include "build/build_config.h"
#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/shared_image_backing.h"
#include "clay/gfx/shared_image/shared_image_sink.h"
#include "clay/shell/platform/embedder/embedder_struct_macros.h"
#include "clay/shell/platform/headless/clay_headless_engine.h"
#ifndef ENABLE_SKITY
#include "third_party/skia/include/core/SkImageInfo.h"
#endif

namespace clay {

// The host environment may only provide OpenGL ES 2.0, so use raw GL APIs to
// blit the shared-image CPU readback to the host framebuffer.
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

constexpr GLenum GL_NEAREST = 0x2600;
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
constexpr GLenum GL_VERTEX_ATTRIB_ARRAY_ENABLED = 0x8622;
constexpr GLenum GL_VERTEX_ATTRIB_ARRAY_SIZE = 0x8623;
constexpr GLenum GL_VERTEX_ATTRIB_ARRAY_STRIDE = 0x8624;
constexpr GLenum GL_VERTEX_ATTRIB_ARRAY_TYPE = 0x8625;
constexpr GLenum GL_VERTEX_ATTRIB_ARRAY_POINTER = 0x8645;
constexpr GLenum GL_VERTEX_ATTRIB_ARRAY_NORMALIZED = 0x886A;
constexpr GLenum GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING = 0x889F;
constexpr GLenum GL_UNPACK_ALIGNMENT = 0x0CF5;
constexpr GLenum GL_VIEWPORT = 0x0BA2;
constexpr GLenum GL_COLOR_CLEAR_VALUE = 0x0C22;
constexpr GLenum GL_COLOR_WRITEMASK = 0x0C23;
constexpr GLenum GL_BLEND = 0x0BE2;
constexpr GLenum GL_DEPTH_TEST = 0x0B71;
constexpr GLenum GL_STENCIL_TEST = 0x0B90;
constexpr GLenum GL_SCISSOR_TEST = 0x0C11;

// For glClear
constexpr GLenum GL_COLOR_BUFFER_BIT = 0x00004000;

struct HostGLReadbackPixmap {
  uint32_t width = 0;
  uint32_t height = 0;
  size_t row_bytes = 0;
  const void* pixels = nullptr;
};

struct VertexAttribState {
  GLint enabled = GL_FALSE;
  GLint size = 4;
  GLint stride = 0;
  GLint type = GL_FLOAT;
  GLint normalized = GL_FALSE;
  GLint buffer_binding = 0;
  void* pointer = nullptr;
};

constexpr size_t kRGBABytesPerPixel = 4;

bool CheckedMulSize(size_t lhs, size_t rhs, size_t* result) {
  if (lhs != 0 && rhs > std::numeric_limits<size_t>::max() / lhs) {
    return false;
  }
  *result = lhs * rhs;
  return true;
}

template <typename Fn>
Fn ResolveGLFunction(const GPUSurfaceGLDelegate::GLProcResolver& resolver,
                     const char* name) {
  return reinterpret_cast<Fn>(resolver(name));
}

constexpr char kCpuBlitVertexShaderSource[] = R"(
    precision highp float;
    attribute highp vec2 aPosition;
    attribute mediump vec2 aTexCoord;
    varying mediump vec2 vTexCoord;
    void main() {
      gl_Position = vec4(aPosition, 0.0, 1.0);
      vTexCoord = aTexCoord;
    }
)";

// Explicit attribute location indices
constexpr GLuint kAttribLocationPosition = 0;
constexpr GLuint kAttribLocationTexCoord = 1;

constexpr char kCpuBlitFragmentShaderSource[] = R"(
#ifdef GL_FRAGMENT_PRECISION_HIGH
    precision highp float;
#else
    precision mediump float;
#endif
    varying mediump vec2 vTexCoord;
    uniform sampler2D uTexture;
    uniform bool uRepeatTexCoord;
    void main() {
      vec2 texCoord = vTexCoord;
      if (uRepeatTexCoord) {
        texCoord = fract(texCoord);
      }
      gl_FragColor = texture2D(uTexture, texCoord).bgra;
    }
)";

constexpr GLfloat kFullscreenTrianglePositions[] = {
    -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f,
    -1.0f, 1.0f,  1.0f, -1.0f, 1.0f,  1.0f,
};

}  // namespace

class HostGLRenderer {
 public:
  explicit HostGLRenderer(GPUSurfaceGLDelegate::GLProcResolver resolver)
      : resolver_(std::move(resolver)) {}

  bool Draw(const HostGLReadbackPixmap& pixmap,
            const skity::Matrix& transformation, GLuint framebuffer) {
    if (!Initialize()) {
      return false;
    }
    if (!pixmap.pixels || pixmap.width == 0 || pixmap.height == 0) {
      FML_LOG(ERROR) << "Invalid HostGLRenderer pixmap";
      return false;
    }
    size_t packed_row_bytes = 0;
    if (!CheckedMulSize(static_cast<size_t>(pixmap.width), kRGBABytesPerPixel,
                        &packed_row_bytes)) {
      FML_LOG(ERROR) << "HostGLRenderer pixmap row bytes overflow";
      return false;
    }
    if (pixmap.row_bytes < packed_row_bytes) {
      FML_LOG(ERROR) << "HostGLRenderer pixmap row bytes too small: "
                     << pixmap.row_bytes << " < " << packed_row_bytes;
      return false;
    }
    size_t packed_pixels_size = 0;
    if (!CheckedMulSize(packed_row_bytes, static_cast<size_t>(pixmap.height),
                        &packed_pixels_size)) {
      FML_LOG(ERROR) << "HostGLRenderer pixmap buffer size overflow";
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
    GLfloat previous_clear_color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    GLboolean previous_color_mask[4] = {GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE};
    GLboolean previous_blend = gl_is_enabled_(GL_BLEND);
    GLboolean previous_depth_test = gl_is_enabled_(GL_DEPTH_TEST);
    GLboolean previous_stencil_test = gl_is_enabled_(GL_STENCIL_TEST);
    GLboolean previous_scissor_test = gl_is_enabled_(GL_SCISSOR_TEST);
    VertexAttribState previous_position_attrib;
    VertexAttribState previous_tex_coord_attrib;

    gl_get_integerv_(GL_CURRENT_PROGRAM, &previous_program);
    gl_get_integerv_(GL_ACTIVE_TEXTURE, &previous_active_texture);
    gl_get_integerv_(GL_FRAMEBUFFER_BINDING, &previous_framebuffer);
    if (supports_vertex_array_object_) {
      gl_get_integerv_(GL_VERTEX_ARRAY_BINDING, &previous_vertex_array);
    }
    gl_get_integerv_(GL_ARRAY_BUFFER_BINDING, &previous_array_buffer);
    gl_get_integerv_(GL_UNPACK_ALIGNMENT, &previous_unpack_alignment);
    gl_get_integerv_(GL_VIEWPORT, previous_viewport);
    gl_get_float_v_(GL_COLOR_CLEAR_VALUE, previous_clear_color);
    gl_get_boolean_v_(GL_COLOR_WRITEMASK, previous_color_mask);
    gl_active_texture_(GL_TEXTURE0);
    gl_get_integerv_(GL_TEXTURE_BINDING_2D, &previous_texture0_binding);
    if (!supports_vertex_array_object_) {
      SaveVertexAttrib(kAttribLocationPosition, &previous_position_attrib);
      SaveVertexAttrib(kAttribLocationTexCoord, &previous_tex_coord_attrib);
    }

    gl_bind_framebuffer_(GL_FRAMEBUFFER, framebuffer);
    gl_viewport_(0, 0, static_cast<GLsizei>(pixmap.width),
                 static_cast<GLsizei>(pixmap.height));
    gl_disable_(GL_SCISSOR_TEST);
    gl_disable_(GL_BLEND);
    gl_disable_(GL_DEPTH_TEST);
    gl_disable_(GL_STENCIL_TEST);
    gl_color_mask_(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    gl_clear_color_(0.0f, 0.0f, 0.0f, 0.0f);
    gl_clear_(GL_COLOR_BUFFER_BIT);

    gl_use_program_(program_);
    gl_bind_texture_(GL_TEXTURE_2D, texture_);
    const bool repeat_tex_coord = !transformation.IsIdentity();
    if (repeat_tex_coord_dirty_ ||
        repeat_tex_coord_enabled_ != repeat_tex_coord) {
      gl_uniform_1_i_(repeat_tex_coord_location_, repeat_tex_coord ? 1 : 0);
      repeat_tex_coord_enabled_ = repeat_tex_coord;
      repeat_tex_coord_dirty_ = false;
    }
    if (supports_vertex_array_object_) {
      gl_bind_vertex_array_(vertex_array_);
    }
    gl_pixel_store_i_(GL_UNPACK_ALIGNMENT, 1);

    const void* pixels = pixmap.pixels;
    if (pixmap.row_bytes != packed_row_bytes) {
      packed_pixels_.resize(packed_pixels_size);
      for (uint32_t row = 0; row < pixmap.height; ++row) {
        std::memcpy(
            packed_pixels_.data() + row * packed_row_bytes,
            static_cast<const uint8_t*>(pixmap.pixels) + row * pixmap.row_bytes,
            packed_row_bytes);
      }
      pixels = packed_pixels_.data();
    }

    if (texture_width_ != pixmap.width || texture_height_ != pixmap.height) {
      gl_tex_image_2d_(GL_TEXTURE_2D, 0, GL_RGBA,
                       static_cast<GLsizei>(pixmap.width),
                       static_cast<GLsizei>(pixmap.height), 0, GL_RGBA,
                       GL_UNSIGNED_BYTE, pixels);
      texture_width_ = pixmap.width;
      texture_height_ = pixmap.height;
    } else {
      gl_tex_sub_image_2d_(GL_TEXTURE_2D, 0, 0, 0,
                           static_cast<GLsizei>(pixmap.width),
                           static_cast<GLsizei>(pixmap.height), GL_RGBA,
                           GL_UNSIGNED_BYTE, pixels);
    }

    // GL uploads the first CPU row to low-v texels, matching the old Skia
    // path's canvas Y-flip without an extra texcoord flip here.
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
    } else {
      RestoreVertexAttrib(kAttribLocationPosition, previous_position_attrib);
      RestoreVertexAttrib(kAttribLocationTexCoord, previous_tex_coord_attrib);
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
    gl_clear_color_(previous_clear_color[0], previous_clear_color[1],
                    previous_clear_color[2], previous_clear_color[3]);
    gl_color_mask_(previous_color_mask[0], previous_color_mask[1],
                   previous_color_mask[2], previous_color_mask[3]);
    RestoreCapability(GL_BLEND, previous_blend);
    RestoreCapability(GL_DEPTH_TEST, previous_depth_test);
    RestoreCapability(GL_STENCIL_TEST, previous_stencil_test);
    RestoreCapability(GL_SCISSOR_TEST, previous_scissor_test);
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
    texture_width_ = 0;
    texture_height_ = 0;
    repeat_tex_coord_enabled_ = false;
    repeat_tex_coord_dirty_ = true;
    packed_pixels_.clear();
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
  using GLTexSubImage2DProc = void (*)(GLenum, GLint, GLint, GLint, GLsizei,
                                       GLsizei, GLenum, GLenum, const void*);
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
  using GLGetVertexAttribivProc = void (*)(GLuint, GLenum, GLint*);
  using GLGetVertexAttribPointervProc = void (*)(GLuint, GLenum, void**);
  using GLGetIntegervProc = void (*)(GLenum, GLint*);
  using GLGetBooleanvProc = void (*)(GLenum, GLboolean*);
  using GLGetFloatvProc = void (*)(GLenum, GLfloat*);
  using GLIsEnabledProc = GLboolean (*)(GLenum);
  using GLEnableProc = void (*)(GLenum);
  using GLDisableProc = void (*)(GLenum);
  using GLColorMaskProc = void (*)(GLboolean, GLboolean, GLboolean, GLboolean);
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
    gl_tex_sub_image_2d_ =
        ResolveGLFunction<GLTexSubImage2DProc>(resolver_, "glTexSubImage2D");
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
    gl_get_vertex_attrib_iv_ = ResolveGLFunction<GLGetVertexAttribivProc>(
        resolver_, "glGetVertexAttribiv");
    gl_get_vertex_attrib_pointer_v_ =
        ResolveGLFunction<GLGetVertexAttribPointervProc>(
            resolver_, "glGetVertexAttribPointerv");
    gl_get_integerv_ =
        ResolveGLFunction<GLGetIntegervProc>(resolver_, "glGetIntegerv");
    gl_get_boolean_v_ =
        ResolveGLFunction<GLGetBooleanvProc>(resolver_, "glGetBooleanv");
    gl_get_float_v_ =
        ResolveGLFunction<GLGetFloatvProc>(resolver_, "glGetFloatv");
    gl_is_enabled_ =
        ResolveGLFunction<GLIsEnabledProc>(resolver_, "glIsEnabled");
    gl_enable_ = ResolveGLFunction<GLEnableProc>(resolver_, "glEnable");
    gl_disable_ = ResolveGLFunction<GLDisableProc>(resolver_, "glDisable");
    gl_color_mask_ =
        ResolveGLFunction<GLColorMaskProc>(resolver_, "glColorMask");
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
        !gl_tex_parameter_i_ || !gl_tex_image_2d_ || !gl_tex_sub_image_2d_ ||
        !gl_gen_buffers_ || !gl_delete_buffers_ || !gl_buffer_data_ ||
        !gl_bind_framebuffer_ || !gl_viewport_ ||
        !gl_enable_vertex_attrib_array_ || !gl_disable_vertex_attrib_array_ ||
        !gl_vertex_attrib_pointer_ || !gl_draw_arrays_ ||
        !gl_get_vertex_attrib_iv_ || !gl_get_vertex_attrib_pointer_v_ ||
        !gl_get_integerv_ || !gl_bind_buffer_ || !gl_get_boolean_v_ ||
        !gl_get_float_v_ || !gl_is_enabled_ || !gl_enable_ || !gl_disable_ ||
        !gl_color_mask_ || !gl_pixel_store_i_ || !gl_clear_ ||
        !gl_clear_color_) {
      FML_LOG(ERROR) << "Failed to resolve GL functions for HostGLRenderer";
      return false;
    }

    program_ =
        CreateProgram(kCpuBlitVertexShaderSource, kCpuBlitFragmentShaderSource);
    if (program_ == 0) {
      return false;
    }

    GLint sampler_location = gl_get_uniform_location_(program_, "uTexture");
    repeat_tex_coord_location_ =
        gl_get_uniform_location_(program_, "uRepeatTexCoord");
    if (sampler_location < 0 || repeat_tex_coord_location_ < 0) {
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
    GLint previous_array_buffer = 0;
    gl_get_integerv_(GL_CURRENT_PROGRAM, &previous_program);
    gl_get_integerv_(GL_ACTIVE_TEXTURE, &previous_active_texture);
    if (supports_vertex_array_object_) {
      gl_get_integerv_(GL_VERTEX_ARRAY_BINDING, &previous_vertex_array);
    }
    gl_get_integerv_(GL_ARRAY_BUFFER_BINDING, &previous_array_buffer);
    gl_active_texture_(GL_TEXTURE0);
    gl_get_integerv_(GL_TEXTURE_BINDING_2D, &previous_texture0_binding);

    gl_use_program_(program_);
    if (supports_vertex_array_object_) {
      gl_bind_vertex_array_(vertex_array_);
    }
    gl_bind_buffer_(GL_ARRAY_BUFFER, position_buffer_);
    gl_buffer_data_(GL_ARRAY_BUFFER, sizeof(kFullscreenTrianglePositions),
                    kFullscreenTrianglePositions, GL_STATIC_DRAW);
    gl_bind_texture_(GL_TEXTURE_2D, texture_);
    gl_tex_parameter_i_(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl_tex_parameter_i_(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl_tex_parameter_i_(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl_tex_parameter_i_(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl_uniform_1_i_(sampler_location, 0);
    gl_uniform_1_i_(repeat_tex_coord_location_, 0);
    repeat_tex_coord_enabled_ = false;
    repeat_tex_coord_dirty_ = false;
    if (supports_vertex_array_object_) {
      gl_bind_vertex_array_(static_cast<GLuint>(previous_vertex_array));
    }
    gl_bind_buffer_(GL_ARRAY_BUFFER,
                    static_cast<GLuint>(previous_array_buffer));
    gl_bind_texture_(GL_TEXTURE_2D,
                     static_cast<GLuint>(previous_texture0_binding));
    gl_active_texture_(static_cast<GLenum>(previous_active_texture));
    gl_use_program_(static_cast<GLuint>(previous_program));

    initialized_ = true;
    return true;
  }

  void RestoreCapability(GLenum capability, GLboolean enabled) {
    if (enabled) {
      gl_enable_(capability);
    } else {
      gl_disable_(capability);
    }
  }

  void SaveVertexAttrib(GLuint index, VertexAttribState* state) {
    gl_get_vertex_attrib_iv_(index, GL_VERTEX_ATTRIB_ARRAY_ENABLED,
                             &state->enabled);
    gl_get_vertex_attrib_iv_(index, GL_VERTEX_ATTRIB_ARRAY_SIZE, &state->size);
    gl_get_vertex_attrib_iv_(index, GL_VERTEX_ATTRIB_ARRAY_STRIDE,
                             &state->stride);
    gl_get_vertex_attrib_iv_(index, GL_VERTEX_ATTRIB_ARRAY_TYPE, &state->type);
    gl_get_vertex_attrib_iv_(index, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED,
                             &state->normalized);
    gl_get_vertex_attrib_iv_(index, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING,
                             &state->buffer_binding);
    gl_get_vertex_attrib_pointer_v_(index, GL_VERTEX_ATTRIB_ARRAY_POINTER,
                                    &state->pointer);
  }

  void RestoreVertexAttrib(GLuint index, const VertexAttribState& state) {
    gl_bind_buffer_(GL_ARRAY_BUFFER, static_cast<GLuint>(state.buffer_binding));
    gl_vertex_attrib_pointer_(
        index, state.size, static_cast<GLenum>(state.type),
        static_cast<GLboolean>(state.normalized), state.stride, state.pointer);
    if (state.enabled) {
      gl_enable_vertex_attrib_array_(index);
    } else {
      gl_disable_vertex_attrib_array_(index);
    }
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
  uint32_t texture_width_ = 0;
  uint32_t texture_height_ = 0;
  GLint repeat_tex_coord_location_ = -1;
  bool repeat_tex_coord_enabled_ = false;
  bool repeat_tex_coord_dirty_ = true;
  std::vector<uint8_t> packed_pixels_;
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
  GLTexSubImage2DProc gl_tex_sub_image_2d_ = nullptr;
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
  GLGetVertexAttribivProc gl_get_vertex_attrib_iv_ = nullptr;
  GLGetVertexAttribPointervProc gl_get_vertex_attrib_pointer_v_ = nullptr;
  GLGetIntegervProc gl_get_integerv_ = nullptr;
  GLGetBooleanvProc gl_get_boolean_v_ = nullptr;
  GLGetFloatvProc gl_get_float_v_ = nullptr;
  GLIsEnabledProc gl_is_enabled_ = nullptr;
  GLEnableProc gl_enable_ = nullptr;
  GLDisableProc gl_disable_ = nullptr;
  GLColorMaskProc gl_color_mask_ = nullptr;
  GLBindBufferProc gl_bind_buffer_ = nullptr;
  GLPixelStoreiProc gl_pixel_store_i_ = nullptr;
  GLClearProc gl_clear_ = nullptr;
  GLClearColorProc gl_clear_color_ = nullptr;
};

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
      draw_tasks_enabled_(std::make_shared<std::atomic_bool>(true)),
      config_(renderer_config) {
  FML_LOG(ERROR) << "Starting Clay in [Host GL+SharedImage] mode. "
                    "Maybe slow in large views";

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

  shared_image_sink_->SetFrameAvailableCallback(
      [host_gl_task_runner = host_gl_thread_.GetTaskRunner(),
       draw_tasks_enabled = draw_tasks_enabled_, this] {
        if (!draw_tasks_enabled->load()) {
          return;
        }
        // The callback is triggered in Clay Raster thread
        host_gl_task_runner->PostTask([draw_tasks_enabled, this] {
          if (!draw_tasks_enabled->load()) {
            return;
          }
          Draw();
        });
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
  draw_tasks_enabled_->store(false);
  {
    std::lock_guard<std::mutex> lock(shared_image_sink_mutex_);
    // shared_image_sink internally keeps ref to D3D device and mutex,
    // which means it should be reset before destroy renderer
    if (shared_image_sink_) {
      shared_image_sink_->SetFrameAvailableCallback(nullptr);
    }
    shared_image_sink_ = nullptr;
  }
  renderer_.reset();
  DestroyHostGLRendererSync();
}

void ClayHeadlessRendererSharedImageHostGL::CleanupGPUResources() {
  if (renderer_) {
    renderer_->CleanupGPUResources();
  }
  DestroyHostGLRendererSync();
}

void ClayHeadlessRendererSharedImageHostGL::DestroyHostGLRendererSync() {
  auto task_runner = host_gl_thread_.GetTaskRunner();
  if (!task_runner) {
    FML_LOG(ERROR) << "No host GL task runner for HostGLRenderer destruction";
    return;
  }
  if (task_runner->RunsTasksOnCurrentThread()) {
    DestroyHostGLRendererOnHostThread();
    return;
  }
  task_runner->PostSyncTask([this] { DestroyHostGLRendererOnHostThread(); });
}

void ClayHeadlessRendererSharedImageHostGL::
    DestroyHostGLRendererOnHostThread() {
  if (!host_gl_renderer_) {
    return;
  }
  auto context_switch = GLContextMakeCurrent();
  if (context_switch && context_switch->GetResult()) {
    host_gl_renderer_->Destroy();
    GLContextClearCurrent();
  } else {
    FML_LOG(ERROR) << "Failed to make current while destroying HostGLRenderer";
  }
  host_gl_renderer_.reset();
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

  skity::Matrix transformation;
  std::unique_ptr<clay::SharedImageReadbackPixmap> readback_pixmap;
#ifndef ENABLE_SKITY
  std::vector<uint8_t> readback_pixels;
#endif
  HostGLReadbackPixmap host_pixmap;
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

    {
      TRACE_EVENT("clay", "SharedImageBacking::ReadbackToMemory");
      const uint32_t width = static_cast<uint32_t>(backing->GetSize().x);
      const uint32_t height = static_cast<uint32_t>(backing->GetSize().y);
#ifndef ENABLE_SKITY
      size_t row_bytes = 0;
      size_t readback_pixels_size = 0;
      if (!CheckedMulSize(static_cast<size_t>(width), kRGBABytesPerPixel,
                          &row_bytes) ||
          !CheckedMulSize(row_bytes, static_cast<size_t>(height),
                          &readback_pixels_size)) {
        FML_LOG(ERROR) << "SharedImage readback pixmap size overflow";
        return;
      }
      readback_pixels.resize(readback_pixels_size);
      auto image_info =
          SkImageInfo::Make(SkISize::Make(static_cast<int32_t>(width),
                                          static_cast<int32_t>(height)),
                            kBGRA_8888_SkColorType, kPremul_SkAlphaType);
      readback_pixmap = std::make_unique<clay::SharedImageReadbackPixmap>(
          image_info, readback_pixels.data(), row_bytes);
#else
      readback_pixmap = std::make_unique<clay::SharedImageReadbackPixmap>(
          width, height, skity::AlphaType::kUnpremul_AlphaType,
          skity::ColorType::kBGRA);
#endif
      if (!backing->ReadbackToMemory(readback_pixmap.get(), 1)) {
        FML_LOG(ERROR) << "Failed to ReadbackToMemory";
        return;
      }
      transformation = backing->GetTransformation();
      host_pixmap = {
          .width = width,
          .height = height,
          .row_bytes = SharedImagePixmapRowBytes(*readback_pixmap),
          .pixels = SharedImagePixmapWritableAddr(*readback_pixmap),
      };
    }
  }

  if (!host_pixmap.pixels) {
    FML_LOG(ERROR) << "No pixmap for Host GL CPU blit";
    return;
  }

  auto context_switch = GLContextMakeCurrent();
  if (!context_switch || !context_switch->GetResult()) {
    FML_LOG(ERROR) << "Failed to make current for Host GL CPU blit";
    return;
  }
  struct AutoClearCurrent {
    explicit AutoClearCurrent(ClayHeadlessRendererSharedImageHostGL& renderer)
        : renderer_(renderer) {}

    ~AutoClearCurrent() { renderer_.GLContextClearCurrent(); }

    ClayHeadlessRendererSharedImageHostGL& renderer_;
  };
  AutoClearCurrent auto_clear_current(*this);

  if (!host_gl_renderer_) {
    GLProcResolver proc_resolver = GetGLProcResolver();
    if (!proc_resolver) {
      FML_LOG(ERROR) << "No GL proc resolver for Host GL CPU blit";
      return;
    }
    host_gl_renderer_ =
        std::make_unique<HostGLRenderer>(std::move(proc_resolver));
  }

  GLFrameInfo frame_info = {host_pixmap.width, host_pixmap.height};
  const GLFBOInfo fbo_info = GLContextFBO(frame_info);
  if (fbo_info.fbo_id < 0 ||
      fbo_info.fbo_id >
          static_cast<int64_t>(std::numeric_limits<uint32_t>::max())) {
    FML_LOG(ERROR) << "Invalid FBO id for Host GL renderer: "
                   << fbo_info.fbo_id;
    return;
  }
  const GLuint target_fbo = static_cast<GLuint>(fbo_info.fbo_id);

  if (!host_gl_renderer_->Draw(host_pixmap, transformation, target_fbo)) {
    FML_LOG(ERROR) << "Failed to draw Host GL CPU blit";
    return;
  }

  if (!GLContextPresent(
          {target_fbo, std::nullopt, std::nullopt, std::nullopt})) {
    FML_LOG(ERROR) << "Failed to present Host GL CPU blit";
  }
}

}  // namespace clay
