// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_GPU_TRACE_GL_FUCTIONS_H_
#define CLAY_SHELL_GPU_TRACE_GL_FUCTIONS_H_

#include <tuple>
#include <unordered_map>
#include <vector>

#include "third_party/skia/include/gpu/gl/GrGLInterface.h"
#include "third_party/skia/include/gpu/gl/GrGLTypes.h"

#undef MemoryBarrier

namespace clay {

class TraceGlFuctions {
 public:
  static void ReplaceFunctions(GrGLInterface* interface);

  explicit TraceGlFuctions(GrGLInterface* interface);
  ~TraceGlFuctions() = default;

  GrGLvoid ActiveTexture(GrGLenum texture);
  GrGLvoid AttachShader(GrGLuint program, GrGLuint shader);
  GrGLvoid BeginQuery(GrGLenum target, GrGLuint id);
  GrGLvoid BindAttribLocation(GrGLuint program, GrGLuint index,
                              const char* name);
  GrGLvoid BindBuffer(GrGLenum target, GrGLuint buffer);
  GrGLvoid BindFramebuffer(GrGLenum target, GrGLuint framebuffer);
  GrGLvoid BindRenderbuffer(GrGLenum target, GrGLuint renderbuffer);
  GrGLvoid BindTexture(GrGLenum target, GrGLuint texture);
  GrGLvoid BindFragDataLocation(GrGLuint program, GrGLuint colorNumber,
                                const GrGLchar* name);
  GrGLvoid BindFragDataLocationIndexed(GrGLuint program, GrGLuint colorNumber,
                                       GrGLuint index, const GrGLchar* name);
  GrGLvoid BindSampler(GrGLuint unit, GrGLuint sampler);
  GrGLvoid BindVertexArray(GrGLuint array);
  GrGLvoid BlendBarrier();
  GrGLvoid BlendColor(GrGLclampf red, GrGLclampf green, GrGLclampf blue,
                      GrGLclampf alpha);
  GrGLvoid BlendEquation(GrGLenum mode);
  GrGLvoid BlendFunc(GrGLenum sfactor, GrGLenum dfactor);
  GrGLvoid BlitFramebuffer(GrGLint srcX0, GrGLint srcY0, GrGLint srcX1,
                           GrGLint srcY1, GrGLint dstX0, GrGLint dstY0,
                           GrGLint dstX1, GrGLint dstY1, GrGLbitfield mask,
                           GrGLenum filter);
  GrGLvoid BufferData(GrGLenum target, GrGLsizeiptr size, const GrGLvoid* data,
                      GrGLenum usage);
  GrGLvoid BufferSubData(GrGLenum target, GrGLintptr offset, GrGLsizeiptr size,
                         const GrGLvoid* data);
  GrGLenum CheckFramebufferStatus(GrGLenum target);
  GrGLvoid Clear(GrGLbitfield mask);
  GrGLvoid ClearColor(GrGLclampf red, GrGLclampf green, GrGLclampf blue,
                      GrGLclampf alpha);
  GrGLvoid ClearStencil(GrGLint s);
  GrGLvoid ClearTexImage(GrGLuint texture, GrGLint level, GrGLenum format,
                         GrGLenum type, const GrGLvoid* data);
  GrGLvoid ClearTexSubImage(GrGLuint texture, GrGLint level, GrGLint xoffset,
                            GrGLint yoffset, GrGLint zoffset, GrGLsizei width,
                            GrGLsizei height, GrGLsizei depth, GrGLenum format,
                            GrGLenum type, const GrGLvoid* data);
  GrGLvoid ColorMask(GrGLboolean red, GrGLboolean green, GrGLboolean blue,
                     GrGLboolean alpha);
  GrGLvoid CompileShader(GrGLuint shader);
  GrGLvoid CompressedTexImage2D(GrGLenum target, GrGLint level,
                                GrGLenum internalformat, GrGLsizei width,
                                GrGLsizei height, GrGLint border,
                                GrGLsizei imageSize, const GrGLvoid* data);
  GrGLvoid CompressedTexSubImage2D(GrGLenum target, GrGLint level,
                                   GrGLint xoffset, GrGLint yoffset,
                                   GrGLsizei width, GrGLsizei height,
                                   GrGLenum format, GrGLsizei imageSize,
                                   const GrGLvoid* data);
  GrGLvoid CopyTexSubImage2D(GrGLenum target, GrGLint level, GrGLint xoffset,
                             GrGLint yoffset, GrGLint x, GrGLint y,
                             GrGLsizei width, GrGLsizei height);
  GrGLuint CreateProgram();
  GrGLuint CreateShader(GrGLenum type);
  GrGLvoid CullFace(GrGLenum mode);
  GrGLvoid DeleteBuffers(GrGLsizei n, const GrGLuint* buffers);
  GrGLvoid DeleteFences(GrGLsizei n, const GrGLuint* fences);
  GrGLvoid DeleteFramebuffers(GrGLsizei n, const GrGLuint* framebuffers);
  GrGLvoid DeleteProgram(GrGLuint program);
  GrGLvoid DeleteQueries(GrGLsizei n, const GrGLuint* ids);
  GrGLvoid DeleteRenderbuffers(GrGLsizei n, const GrGLuint* renderbuffers);
  GrGLvoid DeleteSamplers(GrGLsizei count, const GrGLuint* samplers);
  GrGLvoid DeleteShader(GrGLuint shader);
  GrGLvoid DeleteTextures(GrGLsizei n, const GrGLuint* textures);
  GrGLvoid DeleteVertexArrays(GrGLsizei n, const GrGLuint* arrays);
  GrGLvoid DepthMask(GrGLboolean flag);
  GrGLvoid Disable(GrGLenum cap);
  GrGLvoid DisableVertexAttribArray(GrGLuint index);
  GrGLvoid DrawArrays(GrGLenum mode, GrGLint first, GrGLsizei count);
  GrGLvoid DrawArraysInstanced(GrGLenum mode, GrGLint first, GrGLsizei count,
                               GrGLsizei primcount);
  GrGLvoid DrawArraysIndirect(GrGLenum mode, const GrGLvoid* indirect);
  GrGLvoid DrawBuffer(GrGLenum mode);
  GrGLvoid DrawBuffers(GrGLsizei n, const GrGLenum* bufs);
  GrGLvoid DrawElements(GrGLenum mode, GrGLsizei count, GrGLenum type,
                        const GrGLvoid* indices);
  GrGLvoid DrawElementsInstanced(GrGLenum mode, GrGLsizei count, GrGLenum type,
                                 const GrGLvoid* indices, GrGLsizei primcount);
  GrGLvoid DrawElementsIndirect(GrGLenum mode, GrGLenum type,
                                const GrGLvoid* indirect);
  GrGLvoid DrawRangeElements(GrGLenum mode, GrGLuint start, GrGLuint end,
                             GrGLsizei count, GrGLenum type,
                             const GrGLvoid* indices);
  GrGLvoid Enable(GrGLenum cap);
  GrGLvoid EnableVertexAttribArray(GrGLuint index);
  GrGLvoid EndQuery(GrGLenum target);
  GrGLvoid Finish();
  GrGLvoid FinishFence(GrGLuint fence);
  GrGLvoid Flush();
  GrGLvoid FlushMappedBufferRange(GrGLenum target, GrGLintptr offset,
                                  GrGLsizeiptr length);
  GrGLvoid FramebufferRenderbuffer(GrGLenum target, GrGLenum attachment,
                                   GrGLenum renderbuffertarget,
                                   GrGLuint renderbuffer);
  GrGLvoid FramebufferTexture2D(GrGLenum target, GrGLenum attachment,
                                GrGLenum textarget, GrGLuint texture,
                                GrGLint level);
  GrGLvoid FramebufferTexture2DMultisample(GrGLenum target, GrGLenum attachment,
                                           GrGLenum textarget, GrGLuint texture,
                                           GrGLint level, GrGLsizei samples);
  GrGLvoid FrontFace(GrGLenum mode);
  GrGLvoid GenBuffers(GrGLsizei n, GrGLuint* buffers);
  GrGLvoid GenFences(GrGLsizei n, GrGLuint* fences);
  GrGLvoid GenFramebuffers(GrGLsizei n, GrGLuint* framebuffers);
  GrGLvoid GenerateMipmap(GrGLenum target);
  GrGLvoid GenQueries(GrGLsizei n, GrGLuint* ids);
  GrGLvoid GenRenderbuffers(GrGLsizei n, GrGLuint* renderbuffers);
  GrGLvoid GenSamplers(GrGLsizei count, GrGLuint* samplers);
  GrGLvoid GenTextures(GrGLsizei n, GrGLuint* textures);
  GrGLvoid GenVertexArrays(GrGLsizei n, GrGLuint* arrays);
  GrGLvoid GetBufferParameteriv(GrGLenum target, GrGLenum pname,
                                GrGLint* params);
  GrGLenum GetError();
  GrGLvoid GetFramebufferAttachmentParameteriv(GrGLenum target,
                                               GrGLenum attachment,
                                               GrGLenum pname, GrGLint* params);
  GrGLvoid GetIntegerv(GrGLenum pname, GrGLint* params);
  GrGLvoid GetMultisamplefv(GrGLenum pname, GrGLuint index, GrGLfloat* val);
  GrGLvoid GetProgramBinary(GrGLuint program, GrGLsizei bufsize,
                            GrGLsizei* length, GrGLenum* binaryFormat,
                            void* binary);
  GrGLvoid GetProgramInfoLog(GrGLuint program, GrGLsizei bufsize,
                             GrGLsizei* length, char* infolog);
  GrGLvoid GetProgramiv(GrGLuint program, GrGLenum pname, GrGLint* params);
  GrGLvoid GetQueryiv(GrGLenum GLtarget, GrGLenum pname, GrGLint* params);
  GrGLvoid GetQueryObjecti64v(GrGLuint id, GrGLenum pname, GrGLint64* params);
  GrGLvoid GetQueryObjectiv(GrGLuint id, GrGLenum pname, GrGLint* params);
  GrGLvoid GetQueryObjectui64v(GrGLuint id, GrGLenum pname, GrGLuint64* params);
  GrGLvoid GetQueryObjectuiv(GrGLuint id, GrGLenum pname, GrGLuint* params);
  GrGLvoid GetRenderbufferParameteriv(GrGLenum target, GrGLenum pname,
                                      GrGLint* params);
  GrGLvoid GetShaderInfoLog(GrGLuint shader, GrGLsizei bufsize,
                            GrGLsizei* length, char* infolog);
  GrGLvoid GetShaderiv(GrGLuint shader, GrGLenum pname, GrGLint* params);
  GrGLvoid GetShaderPrecisionFormat(GrGLenum shadertype, GrGLenum precisiontype,
                                    GrGLint* range, GrGLint* precision);
  const GrGLubyte* GetString(GrGLenum name);
  const GrGLubyte* GetStringi(GrGLenum name, GrGLuint index);
  GrGLvoid GetTexLevelParameteriv(GrGLenum target, GrGLint level,
                                  GrGLenum pname, GrGLint* params);
  GrGLint GetUniformLocation(GrGLuint program, const char* name);
  GrGLvoid InsertEventMarker(GrGLsizei length, const char* marker);
  GrGLvoid InvalidateBufferData(GrGLuint buffer);
  GrGLvoid InvalidateBufferSubData(GrGLuint buffer, GrGLintptr offset,
                                   GrGLsizeiptr length);
  GrGLvoid InvalidateFramebuffer(GrGLenum target, GrGLsizei numAttachments,
                                 const GrGLenum* attachments);
  GrGLvoid InvalidateSubFramebuffer(GrGLenum target, GrGLsizei numAttachments,
                                    const GrGLenum* attachments, GrGLint x,
                                    GrGLint y, GrGLsizei width,
                                    GrGLsizei height);
  GrGLvoid InvalidateTexImage(GrGLuint texture, GrGLint level);
  GrGLvoid InvalidateTexSubImage(GrGLuint texture, GrGLint level,
                                 GrGLint xoffset, GrGLint yoffset,
                                 GrGLint zoffset, GrGLsizei width,
                                 GrGLsizei height, GrGLsizei depth);
  GrGLboolean IsTexture(GrGLuint texture);
  GrGLvoid LineWidth(GrGLfloat width);
  GrGLvoid LinkProgram(GrGLuint program);
  GrGLvoid* MapBuffer(GrGLenum target, GrGLenum access);
  GrGLvoid* MapBufferRange(GrGLenum target, GrGLintptr offset,
                           GrGLsizeiptr length, GrGLbitfield access);
  GrGLvoid* MapBufferSubData(GrGLuint target, GrGLintptr offset,
                             GrGLsizeiptr size, GrGLenum access);
  GrGLvoid* MapTexSubImage2D(GrGLenum target, GrGLint level, GrGLint xoffset,
                             GrGLint yoffset, GrGLsizei width, GrGLsizei height,
                             GrGLenum format, GrGLenum type, GrGLenum access);
  GrGLvoid* MemoryBarrier(GrGLbitfield barriers);
  GrGLvoid PatchParameteri(GrGLenum pname, GrGLint value);
  GrGLvoid PixelStorei(GrGLenum pname, GrGLint param);
  GrGLvoid PolygonMode(GrGLenum face, GrGLenum mode);
  GrGLvoid PopGroupMarker();
  GrGLvoid ProgramBinary(GrGLuint program, GrGLenum binaryFormat, void* binary,
                         GrGLsizei length);
  GrGLvoid ProgramParameteri(GrGLuint program, GrGLenum pname, GrGLint value);
  GrGLvoid PushGroupMarker(GrGLsizei length, const char* marker);
  GrGLvoid QueryCounter(GrGLuint id, GrGLenum target);
  GrGLvoid ReadBuffer(GrGLenum src);
  GrGLvoid ReadPixels(GrGLint x, GrGLint y, GrGLsizei width, GrGLsizei height,
                      GrGLenum format, GrGLenum type, GrGLvoid* pixels);
  GrGLvoid RenderbufferStorage(GrGLenum target, GrGLenum internalformat,
                               GrGLsizei width, GrGLsizei height);
  GrGLvoid RenderbufferStorageMultisample(GrGLenum target, GrGLsizei samples,
                                          GrGLenum internalformat,
                                          GrGLsizei width, GrGLsizei height);
  GrGLvoid ResolveMultisampleFramebuffer();
  GrGLvoid SamplerParameteri(GrGLuint sampler, GrGLenum pname, GrGLint params);
  GrGLvoid SamplerParameteriv(GrGLuint sampler, GrGLenum pname,
                              const GrGLint* params);
  GrGLvoid Scissor(GrGLint x, GrGLint y, GrGLsizei width, GrGLsizei height);
  // GL_CHROMIUM_bind_uniform_location
  GrGLvoid BindUniformLocation(GrGLuint program, GrGLint location,
                               const char* name);
  GrGLvoid SetFence(GrGLuint fence, GrGLenum condition);
  GrGLvoid ShaderSource(GrGLuint shader, GrGLsizei count,
                        const char* const* str, const GrGLint* length);
  GrGLvoid StencilFunc(GrGLenum func, GrGLint ref, GrGLuint mask);
  GrGLvoid StencilFuncSeparate(GrGLenum face, GrGLenum func, GrGLint ref,
                               GrGLuint mask);
  GrGLvoid StencilMask(GrGLuint mask);
  GrGLvoid StencilMaskSeparate(GrGLenum face, GrGLuint mask);
  GrGLvoid StencilOp(GrGLenum fail, GrGLenum zfail, GrGLenum zpass);
  GrGLvoid StencilOpSeparate(GrGLenum face, GrGLenum fail, GrGLenum zfail,
                             GrGLenum zpass);
  GrGLvoid TexBuffer(GrGLenum target, GrGLenum internalformat, GrGLuint buffer);
  GrGLvoid TexBufferRange(GrGLenum target, GrGLenum internalformat,
                          GrGLuint buffer, GrGLintptr offset,
                          GrGLsizeiptr size);
  GrGLvoid TexImage2D(GrGLenum target, GrGLint level, GrGLint internalformat,
                      GrGLsizei width, GrGLsizei height, GrGLint border,
                      GrGLenum format, GrGLenum type, const GrGLvoid* pixels);
  GrGLvoid TexParameterf(GrGLenum target, GrGLenum pname, GrGLfloat param);
  GrGLvoid TexParameterfv(GrGLenum target, GrGLenum pname,
                          const GrGLfloat* params);
  GrGLvoid TexParameteri(GrGLenum target, GrGLenum pname, GrGLint param);
  GrGLvoid TexParameteriv(GrGLenum target, GrGLenum pname,
                          const GrGLint* params);
  GrGLvoid TexStorage2D(GrGLenum target, GrGLsizei levels,
                        GrGLenum internalformat, GrGLsizei width,
                        GrGLsizei height);
  GrGLvoid DiscardFramebuffer(GrGLenum target, GrGLsizei numAttachments,
                              const GrGLenum* attachments);
  GrGLboolean TestFence(GrGLuint fence);
  GrGLvoid TexSubImage2D(GrGLenum target, GrGLint level, GrGLint xoffset,
                         GrGLint yoffset, GrGLsizei width, GrGLsizei height,
                         GrGLenum format, GrGLenum type,
                         const GrGLvoid* pixels);
  GrGLvoid TextureBarrier();
  GrGLvoid Uniform1f(GrGLint location, GrGLfloat v0);
  GrGLvoid Uniform1i(GrGLint location, GrGLint v0);
  GrGLvoid Uniform1fv(GrGLint location, GrGLsizei count, const GrGLfloat* v);
  GrGLvoid Uniform1iv(GrGLint location, GrGLsizei count, const GrGLint* v);
  GrGLvoid Uniform2f(GrGLint location, GrGLfloat v0, GrGLfloat v1);
  GrGLvoid Uniform2i(GrGLint location, GrGLint v0, GrGLint v1);
  GrGLvoid Uniform2fv(GrGLint location, GrGLsizei count, const GrGLfloat* v);
  GrGLvoid Uniform2iv(GrGLint location, GrGLsizei count, const GrGLint* v);
  GrGLvoid Uniform3f(GrGLint location, GrGLfloat v0, GrGLfloat v1,
                     GrGLfloat v2);
  GrGLvoid Uniform3i(GrGLint location, GrGLint v0, GrGLint v1, GrGLint v2);
  GrGLvoid Uniform3fv(GrGLint location, GrGLsizei count, const GrGLfloat* v);
  GrGLvoid Uniform3iv(GrGLint location, GrGLsizei count, const GrGLint* v);
  GrGLvoid Uniform4f(GrGLint location, GrGLfloat v0, GrGLfloat v1, GrGLfloat v2,
                     GrGLfloat v3);
  GrGLvoid Uniform4i(GrGLint location, GrGLint v0, GrGLint v1, GrGLint v2,
                     GrGLint v3);
  GrGLvoid Uniform4fv(GrGLint location, GrGLsizei count, const GrGLfloat* v);
  GrGLvoid Uniform4iv(GrGLint location, GrGLsizei count, const GrGLint* v);
  GrGLvoid UniformMatrix2fv(GrGLint location, GrGLsizei count,
                            GrGLboolean transpose, const GrGLfloat* value);
  GrGLvoid UniformMatrix3fv(GrGLint location, GrGLsizei count,
                            GrGLboolean transpose, const GrGLfloat* value);
  GrGLvoid UniformMatrix4fv(GrGLint location, GrGLsizei count,
                            GrGLboolean transpose, const GrGLfloat* value);
  GrGLboolean UnmapBuffer(GrGLenum target);
  GrGLvoid UnmapBufferSubData(const GrGLvoid* mem);
  GrGLvoid UnmapTexSubImage2D(const GrGLvoid* mem);
  GrGLvoid UseProgram(GrGLuint program);
  GrGLvoid VertexAttrib1f(GrGLuint indx, const GrGLfloat value);
  GrGLvoid VertexAttrib2fv(GrGLuint indx, const GrGLfloat* values);
  GrGLvoid VertexAttrib3fv(GrGLuint indx, const GrGLfloat* values);
  GrGLvoid VertexAttrib4fv(GrGLuint indx, const GrGLfloat* values);
  GrGLvoid VertexAttribDivisor(GrGLuint index, GrGLuint divisor);
  GrGLvoid VertexAttribIPointer(GrGLuint indx, GrGLint size, GrGLenum type,
                                GrGLsizei stride, const GrGLvoid* ptr);
  GrGLvoid VertexAttribPointer(GrGLuint indx, GrGLint size, GrGLenum type,
                               GrGLboolean normalized, GrGLsizei stride,
                               const GrGLvoid* ptr);
  GrGLvoid Viewport(GrGLint x, GrGLint y, GrGLsizei width, GrGLsizei height);

  /* GL_NV_framebuffer_mixed_samples */
  GrGLvoid CoverageModulation(GrGLenum components);

  /* EXT_base_instance */
  GrGLvoid DrawArraysInstancedBaseInstance(GrGLenum mode, GrGLint first,
                                           GrGLsizei count,
                                           GrGLsizei instancecount,
                                           GrGLuint baseinstance);
  GrGLvoid DrawElementsInstancedBaseVertexBaseInstance(
      GrGLenum mode, GrGLsizei count, GrGLenum type, const void* indices,
      GrGLsizei instancecount, GrGLint basevertex, GrGLuint baseinstance);

  /* EXT_multi_draw_indirect */
  GrGLvoid MultiDrawArraysIndirect(GrGLenum mode, const GrGLvoid* indirect,
                                   GrGLsizei drawcount, GrGLsizei stride);
  GrGLvoid MultiDrawElementsIndirect(GrGLenum mode, GrGLenum type,
                                     const GrGLvoid* indirect,
                                     GrGLsizei drawcount, GrGLsizei stride);

  /* ANGLE_base_vertex_base_instance */
  GrGLvoid MultiDrawArraysInstancedBaseInstance(GrGLenum mode,
                                                const GrGLint* firsts,
                                                const GrGLsizei* counts,
                                                const GrGLsizei* instanceCounts,
                                                const GrGLuint* baseInstances,
                                                const GrGLsizei drawcount);
  GrGLvoid MultiDrawElementsInstancedBaseVertexBaseInstance(
      GrGLenum mode, const GrGLint* counts, GrGLenum type,
      const GrGLvoid* const* indices, const GrGLsizei* instanceCounts,
      const GrGLint* baseVertices, const GrGLuint* baseInstances,
      const GrGLsizei drawcount);

  /* ARB_sync */
  GrGLsync FenceSync(GrGLenum condition, GrGLbitfield flags);
  GrGLboolean IsSync(GrGLsync sync);
  GrGLenum ClientWaitSync(GrGLsync sync, GrGLbitfield flags,
                          GrGLuint64 timeout);
  GrGLvoid WaitSync(GrGLsync sync, GrGLbitfield flags, GrGLuint64 timeout);
  GrGLvoid DeleteSync(GrGLsync sync);

  /* ARB_internalformat_query */
  GrGLvoid GetInternalformativ(GrGLenum target, GrGLenum internalformat,
                               GrGLenum pname, GrGLsizei bufSize,
                               GrGLint* params);

  /* KHR_debug */
  GrGLvoid DebugMessageControl(GrGLenum source, GrGLenum type,
                               GrGLenum severity, GrGLsizei count,
                               const GrGLuint* ids, GrGLboolean enabled);
  GrGLvoid DebugMessageInsert(GrGLenum source, GrGLenum type, GrGLuint id,
                              GrGLenum severity, GrGLsizei length,
                              const GrGLchar* buf);
  GrGLvoid DebugMessageCallback(GRGLDEBUGPROC callback,
                                const GrGLvoid* userParam);
  GrGLuint GetDebugMessageLog(GrGLuint count, GrGLsizei bufSize,
                              GrGLenum* sources, GrGLenum* types, GrGLuint* ids,
                              GrGLenum* severities, GrGLsizei* lengths,
                              GrGLchar* messageLog);
  GrGLvoid PushDebugGroup(GrGLenum source, GrGLuint id, GrGLsizei length,
                          const GrGLchar* message);
  GrGLvoid PopDebugGroup();
  GrGLvoid ObjectLabel(GrGLenum identifier, GrGLuint name, GrGLsizei length,
                       const GrGLchar* label);

  /** EXT_window_rectangles */
  GrGLvoid WindowRectangles(GrGLenum mode, GrGLsizei count,
                            const GrGLint box[]);

  /** GL_QCOM_tiled_rendering */
  GrGLvoid StartTiling(GrGLuint x, GrGLuint y, GrGLuint width, GrGLuint height,
                       GrGLbitfield preserveMask);
  GrGLvoid EndTiling(GrGLbitfield preserveMask);

 private:
  struct TextureInfo {
    bool inited = false;
    GrGLsizei width;
    GrGLsizei height;
    GrGLenum format;
    uint32_t size;
  };

  // If current target has no binding texture, returns null.
  // Otherwise returns pointer to an TextureInfo.
  TextureInfo* GetTextureInfo(GrGLenum target, GrGLint level);

  TextureInfo* SetTextureInfo(GrGLenum target, GrGLint level,
                              GrGLint internalformat, GrGLsizei width,
                              GrGLsizei height);

  void LogTexUsage();
  void LogTexUploadSize();

  GrGLInterface::Functions real_functions_;
  uint32_t fbo_changed_;
  uint64_t current_texture_size_;
  uint64_t total_tex_upload_size_;
  std::unordered_map<GrGLenum, GrGLuint> target_to_texture_;
  std::unordered_map<GrGLuint, std::vector<TextureInfo>>
      texture_infos_;  // maps texture_id, level to TextureInfo
};

}  // namespace clay
#endif  // CLAY_SHELL_GPU_TRACE_GL_FUCTIONS_H_
