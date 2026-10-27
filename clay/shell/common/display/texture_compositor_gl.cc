// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include <algorithm>
#include <cstring>
#include <memory>
#include <utility>
#include <vector>

#include "base/include/fml/message_loop.h"
#include "base/trace/native/trace_event.h"
#include "clay/common/graphics/gl/gl_shader_program.h"
#include "clay/fml/logging.h"
#include "clay/shell/common/display/raster_output_surface.h"
#include "clay/shell/common/display/texture_compositor.h"
#include "clay/shell/common/output_surface.h"

namespace clay::display {
namespace {
constexpr GLenum kCapabilities[] = {GL_BLEND,
                                    GL_CULL_FACE,
                                    GL_DEPTH_TEST,
                                    GL_STENCIL_TEST,
                                    GL_SCISSOR_TEST,
                                    GL_RASTERIZER_DISCARD,
                                    GL_SAMPLE_ALPHA_TO_COVERAGE,
                                    GL_SAMPLE_COVERAGE,
                                    GL_DITHER};
constexpr GLenum kBlendParameters[] = {
    GL_BLEND_SRC_RGB,   GL_BLEND_DST_RGB,      GL_BLEND_SRC_ALPHA,
    GL_BLEND_DST_ALPHA, GL_BLEND_EQUATION_RGB, GL_BLEND_EQUATION_ALPHA};

// Restore actual GL state so Canvas fallback's cached state remains valid.
// Scissor bounds, blend color and depth/stencil parameters are never changed.
struct GLState {
  GLint draw, read, program, vao, array, renderbuffer, active, texture, sampler;
  GLint viewport[4], blend[6];
  GLboolean color[4], enabled[9];
  GLState() {
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &array);
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &renderbuffer);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
    glGetIntegerv(GL_SAMPLER_BINDING, &sampler);
    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetBooleanv(GL_COLOR_WRITEMASK, color);
    for (size_t i = 0; i < 6; ++i)
      glGetIntegerv(kBlendParameters[i], &blend[i]);
    for (size_t i = 0; i < 9; ++i) enabled[i] = glIsEnabled(kCapabilities[i]);
  }
  ~GLState() {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, read);
    glUseProgram(program);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, array);
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindSampler(0, sampler);
    glActiveTexture(active);
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glColorMask(color[0], color[1], color[2], color[3]);
    glBlendFuncSeparate(blend[0], blend[1], blend[2], blend[3]);
    glBlendEquationSeparate(blend[4], blend[5]);
    for (size_t i = 0; i < 9; ++i)
      enabled[i] ? glEnable(kCapabilities[i]) : glDisable(kCapabilities[i]);
  }
};

struct GLContext {
  std::weak_ptr<GrContext> gpu;
  fml::RefPtr<OutputSurface> owner;
  fml::RefPtr<fml::TaskRunner> runner;
  EGLDisplay display;
  EGLContext context;
  bool IsCurrent() const {
    return eglGetCurrentDisplay() == display &&
           eglGetCurrentContext() == context;
  }
};
template <typename T>
std::shared_ptr<T> ContextOwned(const std::shared_ptr<GLContext>& context) {
  auto* value = new T;
  value->context = context;
  return std::shared_ptr<T>(value, [context](T* value) {
    fml::TaskRunner::RunNowOrPostTask(context->runner, [context, value] {
      auto current = context->owner->MakeCurrent();
      if (!current || !current->GetResult() || !context->IsCurrent()) {
        // The independent object retains its context/storage and budget.
        FML_LOG(ERROR) << "Could not bind the GL compositor resource owner.";
        return;
      }
      delete value;
    });
  });
}

struct Pipeline {
  std::shared_ptr<GLContext> context;
  GLuint program = 0, vao = 0, vbo = 0, sampler = 0;
  GLint clip = -1, uv = -1;
  ~Pipeline() {
    glDeleteSamplers(1, &sampler);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);
  }
  bool Initialize() {
    static constexpr char vertex[] = R"(#version 300 es
layout(location = 0) in vec2 corner;
uniform vec4 clip_rect;
uniform vec4 uv_rect;
out vec2 tex_coord;
void main() {
  gl_Position = vec4(mix(clip_rect.xy, clip_rect.zw, corner), 0.0, 1.0);
  tex_coord = mix(uv_rect.xy, uv_rect.zw, corner);
})";
    static constexpr char fragment[] = R"(#version 300 es
precision highp float;
uniform sampler2D image;
in vec2 tex_coord;
out vec4 color;
void main() { color = texture(image, tex_coord); }
)";
    program = CreateProgram(vertex, fragment);
    if (!program) return false;
    clip = glGetUniformLocation(program, "clip_rect");
    uv = glGetUniformLocation(program, "uv_rect");
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenSamplers(1, &sampler);
    if (clip < 0 || uv < 0 || !vao || !vbo || !sampler) return false;
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    static constexpr GLfloat corners[] = {0, 0, 1, 0, 0, 1, 1, 1};
    glBufferData(GL_ARRAY_BUFFER, sizeof(corners), corners, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "image"), 0);
    return true;
  }
};
struct ReadView {
  std::shared_ptr<GLContext> context;
  std::weak_ptr<ResourceRecord> record;
  std::shared_ptr<void> reservation;
  fml::RefPtr<SharedImageBacking> backing;
  fml::RefPtr<SharedImageRepresentation> representation;
  ClayOpenGLTexture texture{};
  bool reading = false;
  ~ReadView() {
    if (reading) representation->EndRead();
    if (texture.destruction_callback)
      texture.destruction_callback(texture.user_data);
  }
};
struct Quad {
  std::shared_ptr<ReadView> view;
  float clip[4], uv[4];
};
struct PrepareWrite {
  fml::RefPtr<SharedImageRepresentation> representation;
  bool accepted = false;
  ~PrepareWrite() {
    if (!accepted) representation->EndWrite();
  }
};
bool IsGL(const SharedImageRepresentation& representation) {
  return representation.GetType() == ImageRepresentationType::kGL ||
         representation.GetType() == ImageRepresentationType::kEGL;
}

class GLTextureCompositor final : public TextureCompositor {
 public:
  explicit GLTextureCompositor(GrContextPtr gpu) : gpu_(std::move(gpu)) {}
  ~GLTextureCompositor() override {
    for (auto& cached : views_) {
      if (auto view = cached.lock()) {
        if (auto record = view->record.lock()) {
          auto buffer =
              std::static_pointer_cast<RasterBuffer>(record->storage());
          if (buffer->native_consumer_read_view == view)
            buffer->native_consumer_read_view.reset();
        }
      }
    }
  }
  TextureComposition Prepare(const RasterBuffer& target,
                             const RasterResult& result,
                             OutputSurface& output) override {
    const auto& config = result.config;
    auto* loop = fml::MessageLoop::IsInitializedForCurrentThread();
    if (!loop || eglGetCurrentContext() == EGL_NO_CONTEXT ||
        eglGetCurrentDisplay() == EGL_NO_DISPLAY)
      return {};
    auto version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    if (!version || !std::strstr(version, "OpenGL ES 3.") ||
        !config.IsValid() || config.format != PixelFormat::kRGBA8888 ||
        config.alpha != AlphaMode::kPremultiplied ||
        config.color != ColorSpace::kSRGB || result.outputs.empty() ||
        !target.backing || !target.representation ||
        !IsGL(*target.representation) ||
        target.origin != ImageOrigin::kBottomLeft ||
        target.backing->GetSize() != skity::Vec2(config.width, config.height))
      return {};
    if (!context_) {
      context_ = std::make_shared<GLContext>(
          GLContext{gpu_, fml::Ref(&output), loop->GetTaskRunner(),
                    eglGetCurrentDisplay(), eglGetCurrentContext()});
    }
    if (context_->owner.get() != &output || !context_->IsCurrent()) return {};
    views_.erase(
        std::remove_if(views_.begin(), views_.end(),
                       [](const auto& view) { return view.expired(); }),
        views_.end());
    GLState restore;
    if (!pipeline_) {
      if (pipeline_attempted_) return {};
      pipeline_attempted_ = true;
      auto pipeline = ContextOwned<Pipeline>(context_);
      if (!pipeline->Initialize()) return {};
      pipeline_ = std::move(pipeline);
    }
    ClaySharedImageWriteResult write{};
    if (!target.representation->BeginWrite(&write)) return {};
    // Only accepted batches transfer EndWrite to the pool's encode/abandon.
    PrepareWrite pending_write{target.representation};
    const bool native =
        write.struct_size >= sizeof(write) &&
        write.type == kClaySharedImageRepresentationTypeGL &&
        write.opengl_framebuffer.struct_size >= sizeof(ClayOpenGLFramebuffer);
    auto framebuffer = native ? write.opengl_framebuffer.name : 0;
    if (write.type == kClaySharedImageRepresentationTypeGL &&
        write.opengl_framebuffer.destruction_callback)
      write.opengl_framebuffer.destruction_callback(
          write.opengl_framebuffer.user_data);
    if (!framebuffer) return {};
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
    if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) !=
        GL_FRAMEBUFFER_COMPLETE)
      return {};
    std::vector<Quad> quads;
    quads.reserve(result.outputs.size());
    for (const auto& input : result.outputs) {
      if (!input.texture_ref) return {};
      const auto& image = input.texture_ref->image();
      const auto& rect = image.valid_rect;
      if (image.format != PixelFormat::kRGBA8888 ||
          image.alpha != AlphaMode::kPremultiplied ||
          image.color != ColorSpace::kSRGB || !image.width || !image.height ||
          (image.origin != ImageOrigin::kTopLeft &&
           image.origin != ImageOrigin::kBottomLeft) ||
          rect.x < 0 || rect.y < 0 || !rect.width || !rect.height ||
          uint64_t(rect.x) + rect.width > image.width ||
          uint64_t(rect.y) + rect.height > image.height)
        return {};
      const bool bottom = image.origin == ImageOrigin::kBottomLeft;
      const int64_t x = int64_t(input.properties.x) + rect.x;
      const int64_t y = int64_t(input.properties.y) +
                        (bottom ? image.height - rect.y - rect.height : rect.y);
      const int64_t left = std::max<int64_t>(0, x);
      const int64_t top = std::max<int64_t>(0, y);
      const int64_t right = std::min<int64_t>(config.width, x + rect.width);
      const int64_t end = std::min<int64_t>(config.height, y + rect.height);
      if (left >= right || top >= end) continue;
      auto view = GetReadView(input.texture_ref->resource());
      if (!view || view->texture.size.width != image.width ||
          view->texture.size.height != image.height)
        return {};
      const float first_v = bottom ? rect.y + rect.height : rect.y;
      const float direction = bottom ? -1.f : 1.f;
      quads.push_back(
          {std::move(view),
           {2.f * left / config.width - 1.f, 1.f - 2.f * top / config.height,
            2.f * right / config.width - 1.f, 1.f - 2.f * end / config.height},
           {float(rect.x + left - x) / image.width,
            (first_v + direction * (top - y)) / image.height,
            float(rect.x + right - x) / image.width,
            (first_v + direction * (end - y)) / image.height}});
    }
    pending_write.accepted = true;
    return {[pipeline = pipeline_, framebuffer, quads = std::move(quads),
             backing = target.backing, representation = target.representation,
             reservation = target.reservation, width = config.width,
             height = config.height]() -> TextureComposition::Result {
      auto context = pipeline->context;
      // Keep native storage and its accounting alive through GPU retirement.
      (void)backing;
      (void)representation;
      (void)reservation;
      auto gpu = context->gpu.lock();
      if (!gpu || !context->IsCurrent()) return {};
      TRACE_EVENT("clay", "TextureCompositor::NativeCompose");
      GLState restore;
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
      glViewport(0, 0, width, height);
      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      for (size_t i = 1; i < 9; ++i) glDisable(kCapabilities[i]);
      glEnable(GL_BLEND);
      glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
      glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE,
                          GL_ONE_MINUS_SRC_ALPHA);
      static constexpr GLfloat clear[] = {0, 0, 0, 0};
      glClearBufferfv(GL_COLOR, 0, clear);
      glUseProgram(pipeline->program);
      glBindVertexArray(pipeline->vao);
      glBindSampler(0, pipeline->sampler);
      for (const auto& quad : quads) {
        glBindTexture(GL_TEXTURE_2D, quad.view->texture.name);
        glUniform4fv(pipeline->clip, 1, quad.clip);
        glUniform4fv(pipeline->uv, 1, quad.uv);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
      }
      auto completion = context->owner->CreateDisplayCompletion(gpu.get());
      return {completion != nullptr, std::move(completion)};
    }};
  }

 private:
  std::shared_ptr<ReadView> GetReadView(
      const std::shared_ptr<ResourceRecord>& record) {
    auto buffer = std::static_pointer_cast<RasterBuffer>(record->storage());
    auto view =
        std::static_pointer_cast<ReadView>(buffer->native_consumer_read_view);
    if (view && view->context == context_ && view->record.lock() == record)
      return view;
    if (!buffer->backing) return nullptr;
    ClaySharedImageRepresentationConfig config{};
    config.struct_size = sizeof(config);
    config.type = kClaySharedImageRepresentationTypeGL;
    auto representation = buffer->backing->CreateRepresentation(&config);
    if (!representation || !IsGL(*representation)) return nullptr;
    view = ContextOwned<ReadView>(context_);
    view->record = record;
    view->reservation = buffer->reservation;
    view->backing = buffer->backing;
    view->representation = std::move(representation);
    ClaySharedImageReadResult read{};
    view->reading = view->representation->BeginRead(&read);
    if (read.type == kClaySharedImageRepresentationTypeGL)
      view->texture = read.opengl_texture;
    if (!view->reading || read.struct_size < sizeof(read) ||
        read.type != kClaySharedImageRepresentationTypeGL ||
        view->texture.struct_size < sizeof(ClayOpenGLTexture) ||
        view->texture.target != GL_TEXTURE_2D || !view->texture.name ||
        (view->texture.format != GL_RGBA && view->texture.format != GL_RGBA8))
      return nullptr;
    buffer->native_consumer_read_view = view;
    views_.push_back(view);
    return view;
  }
  std::weak_ptr<GrContext> gpu_;
  std::shared_ptr<GLContext> context_;
  std::shared_ptr<Pipeline> pipeline_;
  bool pipeline_attempted_ = false;
  std::vector<std::weak_ptr<ReadView>> views_;
};
}  // namespace

std::unique_ptr<TextureCompositor> CreateTextureCompositor(
    GrContextPtr context) {
  if (!context || context->GetBackendType() != skity::GPUBackendType::kOpenGL)
    return nullptr;
  return std::make_unique<GLTextureCompositor>(std::move(context));
}
}  // namespace clay::display
