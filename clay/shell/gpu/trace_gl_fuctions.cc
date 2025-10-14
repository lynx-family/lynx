// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/gpu/trace_gl_fuctions.h"

#include <algorithm>

#include "base/trace/native/trace_event.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

namespace {

inline uint64_t CalculateTexSize(GrGLenum target, GrGLenum internalformat,
                                 GrGLsizei width, GrGLsizei height) {
  GrBackendFormat format = GrBackendFormat::MakeGL(internalformat, target);
  return width * height * GrBackendFormatBytesPerPixel(format);
}

}  // namespace

// static
void TraceGlFuctions::ReplaceFunctions(GrGLInterface* interface) {
  TraceGlFuctions* trace_gl_fuctions = new TraceGlFuctions(interface);

  interface->fFunctions.fActiveTexture = [trace_gl_fuctions](GrGLenum texture) {
    trace_gl_fuctions->ActiveTexture(texture);
  };

  interface->fFunctions.fAttachShader = [trace_gl_fuctions](GrGLuint program,
                                                            GrGLuint shader) {
    trace_gl_fuctions->AttachShader(program, shader);
  };

  interface->fFunctions.fBeginQuery = [trace_gl_fuctions](GrGLenum target,
                                                          GrGLuint id) {
    trace_gl_fuctions->BeginQuery(target, id);
  };

  interface->fFunctions.fBindAttribLocation =
      [trace_gl_fuctions](GrGLuint program, GrGLuint index, const char* name) {
        trace_gl_fuctions->BindAttribLocation(program, index, name);
      };

  interface->fFunctions.fBindBuffer = [trace_gl_fuctions](GrGLenum target,
                                                          GrGLuint buffer) {
    trace_gl_fuctions->BindBuffer(target, buffer);
  };

  interface->fFunctions.fBindFramebuffer =
      [trace_gl_fuctions](GrGLenum target, GrGLuint framebuffer) {
        trace_gl_fuctions->BindFramebuffer(target, framebuffer);
      };

  interface->fFunctions.fBindRenderbuffer =
      [trace_gl_fuctions](GrGLenum target, GrGLuint renderbuffer) {
        trace_gl_fuctions->BindRenderbuffer(target, renderbuffer);
      };

  interface->fFunctions.fBindTexture = [trace_gl_fuctions](GrGLenum target,
                                                           GrGLuint texture) {
    trace_gl_fuctions->BindTexture(target, texture);
  };

  interface->fFunctions.fBindFragDataLocation =
      [trace_gl_fuctions](GrGLuint program, GrGLuint color_number,
                          const GrGLchar* name) {
        trace_gl_fuctions->BindFragDataLocation(program, color_number, name);
      };

  interface->fFunctions.fBindFragDataLocationIndexed =
      [trace_gl_fuctions](GrGLuint program, GrGLuint color_number,
                          GrGLuint index, const GrGLchar* name) {
        trace_gl_fuctions->BindFragDataLocationIndexed(program, color_number,
                                                       index, name);
      };

  interface->fFunctions.fBindSampler = [trace_gl_fuctions](GrGLuint unit,
                                                           GrGLuint sampler) {
    trace_gl_fuctions->BindSampler(unit, sampler);
  };

  interface->fFunctions.fBindVertexArray = [trace_gl_fuctions](GrGLuint array) {
    trace_gl_fuctions->BindVertexArray(array);
  };

  interface->fFunctions.fBlendBarrier = [trace_gl_fuctions]() {
    trace_gl_fuctions->BlendBarrier();
  };

  interface->fFunctions.fBlendColor = [trace_gl_fuctions](
                                          GrGLclampf red, GrGLclampf green,
                                          GrGLclampf blue, GrGLclampf alpha) {
    trace_gl_fuctions->BlendColor(red, green, blue, alpha);
  };

  interface->fFunctions.fBlendEquation = [trace_gl_fuctions](GrGLenum mode) {
    trace_gl_fuctions->BlendEquation(mode);
  };

  interface->fFunctions.fBlendFunc = [trace_gl_fuctions](GrGLenum sfactor,
                                                         GrGLenum dfactor) {
    trace_gl_fuctions->BlendFunc(sfactor, dfactor);
  };

  interface->fFunctions.fBlitFramebuffer =
      [trace_gl_fuctions](GrGLint srcX0, GrGLint srcY0, GrGLint srcX1,
                          GrGLint srcY1, GrGLint dstX0, GrGLint dstY0,
                          GrGLint dstX1, GrGLint dstY1, GrGLbitfield mask,
                          GrGLenum filter) {
        trace_gl_fuctions->BlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0,
                                           dstY0, dstX1, dstY1, mask, filter);
      };

  interface->fFunctions.fBufferData =
      [trace_gl_fuctions](GrGLenum target, GrGLsizeiptr size,
                          const GrGLvoid* data, GrGLenum usage) {
        trace_gl_fuctions->BufferData(target, size, data, usage);
      };

  interface->fFunctions.fBufferSubData =
      [trace_gl_fuctions](GrGLenum target, GrGLintptr offset, GrGLsizeiptr size,
                          const GrGLvoid* data) {
        trace_gl_fuctions->BufferSubData(target, offset, size, data);
      };

  interface->fFunctions.fCheckFramebufferStatus =
      [trace_gl_fuctions](GrGLenum target) {
        return trace_gl_fuctions->CheckFramebufferStatus(target);
      };

  interface->fFunctions.fClear = [trace_gl_fuctions](GrGLbitfield mask) {
    trace_gl_fuctions->Clear(mask);
  };

  interface->fFunctions.fClearColor = [trace_gl_fuctions](
                                          GrGLclampf red, GrGLclampf green,
                                          GrGLclampf blue, GrGLclampf alpha) {
    trace_gl_fuctions->ClearColor(red, green, blue, alpha);
  };

  interface->fFunctions.fClearStencil = [trace_gl_fuctions](GrGLint s) {
    trace_gl_fuctions->ClearStencil(s);
  };

  interface->fFunctions.fClearTexImage =
      [trace_gl_fuctions](GrGLuint texture, GrGLint level, GrGLenum format,
                          GrGLenum type, const GrGLvoid* data) {
        trace_gl_fuctions->ClearTexImage(texture, level, format, type, data);
      };

  interface->fFunctions.fClearTexSubImage =
      [trace_gl_fuctions](GrGLuint texture, GrGLint level, GrGLint xoffset,
                          GrGLint yoffset, GrGLint zoffset, GrGLsizei width,
                          GrGLsizei height, GrGLsizei depth, GrGLenum format,
                          GrGLenum type, const GrGLvoid* data) {
        trace_gl_fuctions->ClearTexSubImage(texture, level, xoffset, yoffset,
                                            zoffset, width, height, depth,
                                            format, type, data);
      };

  interface->fFunctions.fColorMask = [trace_gl_fuctions](
                                         GrGLboolean red, GrGLboolean green,
                                         GrGLboolean blue, GrGLboolean alpha) {
    trace_gl_fuctions->ColorMask(red, green, blue, alpha);
  };

  interface->fFunctions.fCompileShader = [trace_gl_fuctions](GrGLuint shader) {
    trace_gl_fuctions->CompileShader(shader);
  };

  interface->fFunctions.fCompressedTexImage2D =
      [trace_gl_fuctions](GrGLenum target, GrGLint level,
                          GrGLenum internalformat, GrGLsizei width,
                          GrGLsizei height, GrGLint border, GrGLsizei imageSize,
                          const GrGLvoid* data) {
        trace_gl_fuctions->CompressedTexImage2D(target, level, internalformat,
                                                width, height, border,
                                                imageSize, data);
      };

  interface->fFunctions.fCompressedTexSubImage2D =
      [trace_gl_fuctions](GrGLenum target, GrGLint level, GrGLint xoffset,
                          GrGLint yoffset, GrGLsizei width, GrGLsizei height,
                          GrGLenum format, GrGLsizei imageSize,
                          const GrGLvoid* data) {
        trace_gl_fuctions->CompressedTexSubImage2D(target, level, xoffset,
                                                   yoffset, width, height,
                                                   format, imageSize, data);
      };

  interface->fFunctions.fCopyTexSubImage2D =
      [trace_gl_fuctions](GrGLenum target, GrGLint level, GrGLint xoffset,
                          GrGLint yoffset, GrGLint x, GrGLint y,
                          GrGLsizei width, GrGLsizei height) {
        trace_gl_fuctions->CopyTexSubImage2D(target, level, xoffset, yoffset, x,
                                             y, width, height);
      };

  interface->fFunctions.fCreateProgram = [trace_gl_fuctions]() {
    return trace_gl_fuctions->CreateProgram();
  };

  interface->fFunctions.fCreateShader = [trace_gl_fuctions](GrGLenum type) {
    return trace_gl_fuctions->CreateShader(type);
  };

  interface->fFunctions.fCullFace = [trace_gl_fuctions](GrGLenum mode) {
    trace_gl_fuctions->CullFace(mode);
  };

  interface->fFunctions.fDeleteBuffers =
      [trace_gl_fuctions](GrGLsizei n, const GrGLuint* buffers) {
        trace_gl_fuctions->DeleteBuffers(n, buffers);
      };

  interface->fFunctions.fDeleteFences =
      [trace_gl_fuctions](GrGLsizei n, const GrGLuint* fences) {
        trace_gl_fuctions->DeleteFences(n, fences);
      };

  interface->fFunctions.fDeleteFramebuffers =
      [trace_gl_fuctions](GrGLsizei n, const GrGLuint* framebuffers) {
        trace_gl_fuctions->DeleteFramebuffers(n, framebuffers);
      };

  interface->fFunctions.fDeleteProgram = [trace_gl_fuctions](GrGLuint program) {
    trace_gl_fuctions->DeleteProgram(program);
  };

  interface->fFunctions.fDeleteQueries = [trace_gl_fuctions](
                                             GrGLsizei n, const GrGLuint* ids) {
    trace_gl_fuctions->DeleteQueries(n, ids);
  };

  interface->fFunctions.fDeleteRenderbuffers =
      [trace_gl_fuctions](GrGLsizei n, const GrGLuint* renderbuffers) {
        trace_gl_fuctions->DeleteRenderbuffers(n, renderbuffers);
      };

  interface->fFunctions.fDeleteSamplers =
      [trace_gl_fuctions](GrGLsizei count, const GrGLuint* samplers) {
        trace_gl_fuctions->DeleteSamplers(count, samplers);
      };

  interface->fFunctions.fDeleteShader = [trace_gl_fuctions](GrGLuint shader) {
    trace_gl_fuctions->DeleteShader(shader);
  };

  interface->fFunctions.fDeleteTextures =
      [trace_gl_fuctions](GrGLsizei n, const GrGLuint* textures) {
        trace_gl_fuctions->DeleteTextures(n, textures);
      };

  interface->fFunctions.fDeleteVertexArrays =
      [trace_gl_fuctions](GrGLsizei n, const GrGLuint* arrays) {
        trace_gl_fuctions->DeleteVertexArrays(n, arrays);
      };

  interface->fFunctions.fDepthMask = [trace_gl_fuctions](GrGLboolean flag) {
    trace_gl_fuctions->DepthMask(flag);
  };

  interface->fFunctions.fDisable = [trace_gl_fuctions](GrGLenum cap) {
    trace_gl_fuctions->Disable(cap);
  };

  interface->fFunctions.fDisableVertexAttribArray =
      [trace_gl_fuctions](GrGLuint index) {
        trace_gl_fuctions->DisableVertexAttribArray(index);
      };

  interface->fFunctions.fDrawArrays =
      [trace_gl_fuctions](GrGLenum mode, GrGLint first, GrGLsizei count) {
        trace_gl_fuctions->DrawArrays(mode, first, count);
      };

  interface->fFunctions.fDrawArraysInstanced =
      [trace_gl_fuctions](GrGLenum mode, GrGLint first, GrGLsizei count,
                          GrGLsizei primcount) {
        trace_gl_fuctions->DrawArraysInstanced(mode, first, count, primcount);
      };

  interface->fFunctions.fDrawArraysIndirect =
      [trace_gl_fuctions](GrGLenum mode, const GrGLvoid* indirect) {
        trace_gl_fuctions->DrawArraysIndirect(mode, indirect);
      };

  interface->fFunctions.fDrawBuffer = [trace_gl_fuctions](GrGLenum mode) {
    trace_gl_fuctions->DrawBuffer(mode);
  };

  interface->fFunctions.fDrawBuffers = [trace_gl_fuctions](
                                           GrGLsizei n, const GrGLenum* bufs) {
    trace_gl_fuctions->DrawBuffers(n, bufs);
  };

  interface->fFunctions.fDrawElements =
      [trace_gl_fuctions](GrGLenum mode, GrGLsizei count, GrGLenum type,
                          const GrGLvoid* indices) {
        trace_gl_fuctions->DrawElements(mode, count, type, indices);
      };

  interface->fFunctions.fDrawElementsInstanced =
      [trace_gl_fuctions](GrGLenum mode, GrGLsizei count, GrGLenum type,
                          const GrGLvoid* indices, GrGLsizei primcount) {
        trace_gl_fuctions->DrawElementsInstanced(mode, count, type, indices,
                                                 primcount);
      };

  interface->fFunctions.fDrawElementsIndirect =
      [trace_gl_fuctions](GrGLenum mode, GrGLenum type,
                          const GrGLvoid* indirect) {
        trace_gl_fuctions->DrawElementsIndirect(mode, type, indirect);
      };

  interface->fFunctions.fDrawRangeElements =
      [trace_gl_fuctions](GrGLenum mode, GrGLuint start, GrGLuint end,
                          GrGLsizei count, GrGLenum type,
                          const GrGLvoid* indices) {
        trace_gl_fuctions->DrawRangeElements(mode, start, end, count, type,
                                             indices);
      };

  interface->fFunctions.fEnable = [trace_gl_fuctions](GrGLenum cap) {
    trace_gl_fuctions->Enable(cap);
  };

  interface->fFunctions.fEnableVertexAttribArray =
      [trace_gl_fuctions](GrGLuint index) {
        trace_gl_fuctions->EnableVertexAttribArray(index);
      };

  interface->fFunctions.fEndQuery = [trace_gl_fuctions](GrGLenum target) {
    trace_gl_fuctions->EndQuery(target);
  };

  interface->fFunctions.fFinish = [trace_gl_fuctions]() {
    trace_gl_fuctions->Finish();
  };

  interface->fFunctions.fFinishFence = [trace_gl_fuctions](GrGLuint fence) {
    trace_gl_fuctions->FinishFence(fence);
  };

  interface->fFunctions.fFlush = [trace_gl_fuctions]() {
    trace_gl_fuctions->Flush();
  };

  interface->fFunctions.fFlushMappedBufferRange =
      [trace_gl_fuctions](GrGLenum target, GrGLintptr offset,
                          GrGLsizeiptr length) {
        trace_gl_fuctions->FlushMappedBufferRange(target, offset, length);
      };

  interface->fFunctions.fFramebufferRenderbuffer =
      [trace_gl_fuctions](GrGLenum target, GrGLenum attachment,
                          GrGLenum renderbuffertarget, GrGLuint renderbuffer) {
        trace_gl_fuctions->FramebufferRenderbuffer(
            target, attachment, renderbuffertarget, renderbuffer);
      };

  interface->fFunctions.fFramebufferTexture2D =
      [trace_gl_fuctions](GrGLenum target, GrGLenum attachment,
                          GrGLenum textarget, GrGLuint texture, GrGLint level) {
        trace_gl_fuctions->FramebufferTexture2D(target, attachment, textarget,
                                                texture, level);
      };

  interface->fFunctions.fFramebufferTexture2DMultisample =
      [trace_gl_fuctions](GrGLenum target, GrGLenum attachment,
                          GrGLenum textarget, GrGLuint texture, GrGLint level,
                          GrGLsizei samples) {
        trace_gl_fuctions->FramebufferTexture2DMultisample(
            target, attachment, textarget, texture, level, samples);
      };

  interface->fFunctions.fFrontFace = [trace_gl_fuctions](GrGLenum mode) {
    trace_gl_fuctions->FrontFace(mode);
  };

  interface->fFunctions.fGenBuffers = [trace_gl_fuctions](GrGLsizei n,
                                                          GrGLuint* buffers) {
    trace_gl_fuctions->GenBuffers(n, buffers);
  };

  interface->fFunctions.fGenFences = [trace_gl_fuctions](GrGLsizei n,
                                                         GrGLuint* fences) {
    trace_gl_fuctions->GenFences(n, fences);
  };

  interface->fFunctions.fGenFramebuffers =
      [trace_gl_fuctions](GrGLsizei n, GrGLuint* framebuffers) {
        trace_gl_fuctions->GenFramebuffers(n, framebuffers);
      };

  interface->fFunctions.fGenerateMipmap = [trace_gl_fuctions](GrGLenum target) {
    trace_gl_fuctions->GenerateMipmap(target);
  };

  interface->fFunctions.fGenQueries = [trace_gl_fuctions](GrGLsizei n,
                                                          GrGLuint* ids) {
    trace_gl_fuctions->GenQueries(n, ids);
  };

  interface->fFunctions.fGenRenderbuffers =
      [trace_gl_fuctions](GrGLsizei n, GrGLuint* renderbuffers) {
        trace_gl_fuctions->GenRenderbuffers(n, renderbuffers);
      };

  interface->fFunctions.fGenSamplers = [trace_gl_fuctions](GrGLsizei count,
                                                           GrGLuint* samplers) {
    trace_gl_fuctions->GenSamplers(count, samplers);
  };

  interface->fFunctions.fGenTextures = [trace_gl_fuctions](GrGLsizei n,
                                                           GrGLuint* textures) {
    trace_gl_fuctions->GenTextures(n, textures);
  };

  interface->fFunctions.fGenVertexArrays = [trace_gl_fuctions](
                                               GrGLsizei n, GrGLuint* arrays) {
    trace_gl_fuctions->GenVertexArrays(n, arrays);
  };

  interface->fFunctions.fGetBufferParameteriv =
      [trace_gl_fuctions](GrGLenum target, GrGLenum pname, GrGLint* params) {
        trace_gl_fuctions->GetBufferParameteriv(target, pname, params);
      };

  interface->fFunctions.fGetError = [trace_gl_fuctions]() {
    return trace_gl_fuctions->GetError();
  };

  interface->fFunctions.fGetFramebufferAttachmentParameteriv =
      [trace_gl_fuctions](GrGLenum target, GrGLenum attachment, GrGLenum pname,
                          GrGLint* params) {
        trace_gl_fuctions->GetFramebufferAttachmentParameteriv(
            target, attachment, pname, params);
      };

  interface->fFunctions.fGetIntegerv = [trace_gl_fuctions](GrGLenum pname,
                                                           GrGLint* params) {
    trace_gl_fuctions->GetIntegerv(pname, params);
  };

  interface->fFunctions.fGetMultisamplefv =
      [trace_gl_fuctions](GrGLenum pname, GrGLuint index, GrGLfloat* val) {
        trace_gl_fuctions->GetMultisamplefv(pname, index, val);
      };

  interface->fFunctions.fGetProgramBinary =
      [trace_gl_fuctions](GrGLuint program, GrGLsizei bufsize,
                          GrGLsizei* length, GrGLenum* binaryFormat,
                          void* binary) {
        trace_gl_fuctions->GetProgramBinary(program, bufsize, length,
                                            binaryFormat, binary);
      };

  interface->fFunctions.fGetProgramInfoLog =
      [trace_gl_fuctions](GrGLuint program, GrGLsizei bufsize,
                          GrGLsizei* length, char* infolog) {
        trace_gl_fuctions->GetProgramInfoLog(program, bufsize, length, infolog);
      };

  interface->fFunctions.fGetProgramiv =
      [trace_gl_fuctions](GrGLuint program, GrGLenum pname, GrGLint* params) {
        trace_gl_fuctions->GetProgramiv(program, pname, params);
      };

  interface->fFunctions.fGetQueryiv =
      [trace_gl_fuctions](GrGLenum GLtarget, GrGLenum pname, GrGLint* params) {
        trace_gl_fuctions->GetQueryiv(GLtarget, pname, params);
      };

  interface->fFunctions.fGetQueryObjecti64v =
      [trace_gl_fuctions](GrGLuint id, GrGLenum pname, GrGLint64* params) {
        trace_gl_fuctions->GetQueryObjecti64v(id, pname, params);
      };

  interface->fFunctions.fGetQueryObjectiv =
      [trace_gl_fuctions](GrGLuint id, GrGLenum pname, GrGLint* params) {
        trace_gl_fuctions->GetQueryObjectiv(id, pname, params);
      };

  interface->fFunctions.fGetQueryObjectui64v =
      [trace_gl_fuctions](GrGLuint id, GrGLenum pname, GrGLuint64* params) {
        trace_gl_fuctions->GetQueryObjectui64v(id, pname, params);
      };

  interface->fFunctions.fGetQueryObjectuiv =
      [trace_gl_fuctions](GrGLuint id, GrGLenum pname, GrGLuint* params) {
        trace_gl_fuctions->GetQueryObjectuiv(id, pname, params);
      };

  interface->fFunctions.fGetRenderbufferParameteriv =
      [trace_gl_fuctions](GrGLenum target, GrGLenum pname, GrGLint* params) {
        trace_gl_fuctions->GetRenderbufferParameteriv(target, pname, params);
      };

  interface->fFunctions.fGetShaderInfoLog =
      [trace_gl_fuctions](GrGLuint shader, GrGLsizei bufsize, GrGLsizei* length,
                          char* infolog) {
        trace_gl_fuctions->GetShaderInfoLog(shader, bufsize, length, infolog);
      };

  interface->fFunctions.fGetShaderiv =
      [trace_gl_fuctions](GrGLuint shader, GrGLenum pname, GrGLint* params) {
        trace_gl_fuctions->GetShaderiv(shader, pname, params);
      };

  interface->fFunctions.fGetShaderPrecisionFormat =
      [trace_gl_fuctions](GrGLenum shadertype, GrGLenum precisiontype,
                          GrGLint* range, GrGLint* precision) {
        trace_gl_fuctions->GetShaderPrecisionFormat(shadertype, precisiontype,
                                                    range, precision);
      };

  interface->fFunctions.fGetString = [trace_gl_fuctions](GrGLenum name) {
    return trace_gl_fuctions->GetString(name);
  };

  interface->fFunctions.fGetStringi = [trace_gl_fuctions](GrGLenum name,
                                                          GrGLuint index) {
    return trace_gl_fuctions->GetStringi(name, index);
  };

  interface->fFunctions.fGetTexLevelParameteriv =
      [trace_gl_fuctions](GrGLenum target, GrGLint level, GrGLenum pname,
                          GrGLint* params) {
        trace_gl_fuctions->GetTexLevelParameteriv(target, level, pname, params);
      };

  interface->fFunctions.fGetUniformLocation =
      [trace_gl_fuctions](GrGLuint program, const char* name) {
        return trace_gl_fuctions->GetUniformLocation(program, name);
      };

  interface->fFunctions.fInsertEventMarker =
      [trace_gl_fuctions](GrGLsizei length, const char* marker) {
        trace_gl_fuctions->InsertEventMarker(length, marker);
      };

  interface->fFunctions.fInvalidateBufferData =
      [trace_gl_fuctions](GrGLuint buffer) {
        trace_gl_fuctions->InvalidateBufferData(buffer);
      };

  interface->fFunctions.fInvalidateBufferSubData =
      [trace_gl_fuctions](GrGLuint buffer, GrGLintptr offset,
                          GrGLsizeiptr length) {
        trace_gl_fuctions->InvalidateBufferSubData(buffer, offset, length);
      };

  interface->fFunctions.fInvalidateFramebuffer =
      [trace_gl_fuctions](GrGLenum target, GrGLsizei numAttachments,
                          const GrGLenum* attachments) {
        trace_gl_fuctions->InvalidateFramebuffer(target, numAttachments,
                                                 attachments);
      };

  interface->fFunctions.fInvalidateSubFramebuffer =
      [trace_gl_fuctions](GrGLenum target, GrGLsizei numAttachments,
                          const GrGLenum* attachments, GrGLint x, GrGLint y,
                          GrGLsizei width, GrGLsizei height) {
        trace_gl_fuctions->InvalidateSubFramebuffer(
            target, numAttachments, attachments, x, y, width, height);
      };

  interface->fFunctions.fInvalidateTexImage =
      [trace_gl_fuctions](GrGLuint texture, GrGLint level) {
        trace_gl_fuctions->InvalidateTexImage(texture, level);
      };

  interface->fFunctions.fInvalidateTexSubImage =
      [trace_gl_fuctions](GrGLuint texture, GrGLint level, GrGLint xoffset,
                          GrGLint yoffset, GrGLint zoffset, GrGLsizei width,
                          GrGLsizei height, GrGLsizei depth) {
        trace_gl_fuctions->InvalidateTexSubImage(
            texture, level, xoffset, yoffset, zoffset, width, height, depth);
      };

  interface->fFunctions.fIsTexture = [trace_gl_fuctions](GrGLuint texture) {
    return trace_gl_fuctions->IsTexture(texture);
  };

  interface->fFunctions.fLineWidth = [trace_gl_fuctions](GrGLfloat width) {
    trace_gl_fuctions->LineWidth(width);
  };

  interface->fFunctions.fLinkProgram = [trace_gl_fuctions](GrGLuint program) {
    trace_gl_fuctions->LinkProgram(program);
  };

  interface->fFunctions.fMapBuffer = [trace_gl_fuctions](GrGLenum target,
                                                         GrGLenum access) {
    return trace_gl_fuctions->MapBuffer(target, access);
  };

  interface->fFunctions.fMapBufferRange =
      [trace_gl_fuctions](GrGLenum target, GrGLintptr offset,
                          GrGLsizeiptr length, GrGLbitfield access) {
        return trace_gl_fuctions->MapBufferRange(target, offset, length,
                                                 access);
      };

  interface->fFunctions.fMapBufferSubData =
      [trace_gl_fuctions](GrGLuint target, GrGLintptr offset, GrGLsizeiptr size,
                          GrGLenum access) {
        return trace_gl_fuctions->MapBufferSubData(target, offset, size,
                                                   access);
      };

  interface->fFunctions.fMapTexSubImage2D =
      [trace_gl_fuctions](GrGLenum target, GrGLint level, GrGLint xoffset,
                          GrGLint yoffset, GrGLsizei width, GrGLsizei height,
                          GrGLenum format, GrGLenum type, GrGLenum access) {
        return trace_gl_fuctions->MapTexSubImage2D(target, level, xoffset,
                                                   yoffset, width, height,
                                                   format, type, access);
      };

  interface->fFunctions.fMemoryBarrier =
      [trace_gl_fuctions](GrGLbitfield barriers) {
        return trace_gl_fuctions->MemoryBarrier(barriers);
      };

  interface->fFunctions.fPatchParameteri = [trace_gl_fuctions](GrGLenum pname,
                                                               GrGLint value) {
    trace_gl_fuctions->PatchParameteri(pname, value);
  };

  interface->fFunctions.fPixelStorei = [trace_gl_fuctions](GrGLenum pname,
                                                           GrGLint param) {
    trace_gl_fuctions->PixelStorei(pname, param);
  };

  interface->fFunctions.fPolygonMode = [trace_gl_fuctions](GrGLenum face,
                                                           GrGLenum mode) {
    trace_gl_fuctions->PolygonMode(face, mode);
  };

  interface->fFunctions.fPopGroupMarker = [trace_gl_fuctions]() {
    trace_gl_fuctions->PopGroupMarker();
  };

  interface->fFunctions.fProgramBinary =
      [trace_gl_fuctions](GrGLuint program, GrGLenum binaryFormat, void* binary,
                          GrGLsizei length) {
        trace_gl_fuctions->ProgramBinary(program, binaryFormat, binary, length);
      };

  interface->fFunctions.fProgramParameteri =
      [trace_gl_fuctions](GrGLuint program, GrGLenum pname, GrGLint value) {
        trace_gl_fuctions->ProgramParameteri(program, pname, value);
      };

  interface->fFunctions.fPushGroupMarker =
      [trace_gl_fuctions](GrGLsizei length, const char* marker) {
        trace_gl_fuctions->PushGroupMarker(length, marker);
      };

  interface->fFunctions.fQueryCounter = [trace_gl_fuctions](GrGLuint id,
                                                            GrGLenum target) {
    trace_gl_fuctions->QueryCounter(id, target);
  };

  interface->fFunctions.fReadBuffer = [trace_gl_fuctions](GrGLenum src) {
    trace_gl_fuctions->ReadBuffer(src);
  };

  interface->fFunctions.fReadPixels = [trace_gl_fuctions](
                                          GrGLint x, GrGLint y, GrGLsizei width,
                                          GrGLsizei height, GrGLenum format,
                                          GrGLenum type, GrGLvoid* pixels) {
    trace_gl_fuctions->ReadPixels(x, y, width, height, format, type, pixels);
  };

  interface->fFunctions.fRenderbufferStorage =
      [trace_gl_fuctions](GrGLenum target, GrGLenum internalformat,
                          GrGLsizei width, GrGLsizei height) {
        trace_gl_fuctions->RenderbufferStorage(target, internalformat, width,
                                               height);
      };

  interface->fFunctions.fRenderbufferStorageMultisample =
      [trace_gl_fuctions](GrGLenum target, GrGLsizei samples,
                          GrGLenum internalformat, GrGLsizei width,
                          GrGLsizei height) {
        trace_gl_fuctions->RenderbufferStorageMultisample(
            target, samples, internalformat, width, height);
      };

  interface->fFunctions.fResolveMultisampleFramebuffer = [trace_gl_fuctions]() {
    trace_gl_fuctions->ResolveMultisampleFramebuffer();
  };

  interface->fFunctions.fSamplerParameteri =
      [trace_gl_fuctions](GrGLuint sampler, GrGLenum pname, GrGLint params) {
        trace_gl_fuctions->SamplerParameteri(sampler, pname, params);
      };

  interface->fFunctions.fSamplerParameteriv =
      [trace_gl_fuctions](GrGLuint sampler, GrGLenum pname,
                          const GrGLint* params) {
        trace_gl_fuctions->SamplerParameteriv(sampler, pname, params);
      };

  interface->fFunctions.fScissor = [trace_gl_fuctions](GrGLint x, GrGLint y,
                                                       GrGLsizei width,
                                                       GrGLsizei height) {
    trace_gl_fuctions->Scissor(x, y, width, height);
  };

  // GL_CHROMIUM_bind_uniform_location
  interface->fFunctions.fBindUniformLocation =
      [trace_gl_fuctions](GrGLuint program, GrGLint location,
                          const char* name) {
        trace_gl_fuctions->BindUniformLocation(program, location, name);
      };

  interface->fFunctions.fSetFence = [trace_gl_fuctions](GrGLuint fence,
                                                        GrGLenum condition) {
    trace_gl_fuctions->SetFence(fence, condition);
  };

  interface->fFunctions.fShaderSource =
      [trace_gl_fuctions](GrGLuint shader, GrGLsizei count,
                          const char* const* str, const GrGLint* length) {
        trace_gl_fuctions->ShaderSource(shader, count, str, length);
      };

  interface->fFunctions.fStencilFunc =
      [trace_gl_fuctions](GrGLenum func, GrGLint ref, GrGLuint mask) {
        trace_gl_fuctions->StencilFunc(func, ref, mask);
      };

  interface->fFunctions.fStencilFuncSeparate = [trace_gl_fuctions](
                                                   GrGLenum face, GrGLenum func,
                                                   GrGLint ref, GrGLuint mask) {
    trace_gl_fuctions->StencilFuncSeparate(face, func, ref, mask);
  };

  interface->fFunctions.fStencilMask = [trace_gl_fuctions](GrGLuint mask) {
    trace_gl_fuctions->StencilMask(mask);
  };

  interface->fFunctions.fStencilMaskSeparate =
      [trace_gl_fuctions](GrGLenum face, GrGLuint mask) {
        trace_gl_fuctions->StencilMaskSeparate(face, mask);
      };

  interface->fFunctions.fStencilOp =
      [trace_gl_fuctions](GrGLenum fail, GrGLenum zfail, GrGLenum zpass) {
        trace_gl_fuctions->StencilOp(fail, zfail, zpass);
      };

  interface->fFunctions.fStencilOpSeparate =
      [trace_gl_fuctions](GrGLenum face, GrGLenum fail, GrGLenum zfail,
                          GrGLenum zpass) {
        trace_gl_fuctions->StencilOpSeparate(face, fail, zfail, zpass);
      };

  interface->fFunctions.fTexBuffer =
      [trace_gl_fuctions](GrGLenum target, GrGLenum internalformat,
                          GrGLuint buffer) {
        trace_gl_fuctions->TexBuffer(target, internalformat, buffer);
      };

  interface->fFunctions.fTexBufferRange =
      [trace_gl_fuctions](GrGLenum target, GrGLenum internalformat,
                          GrGLuint buffer, GrGLintptr offset,
                          GrGLsizeiptr size) {
        trace_gl_fuctions->TexBufferRange(target, internalformat, buffer,
                                          offset, size);
      };

  interface->fFunctions.fTexImage2D =
      [trace_gl_fuctions](GrGLenum target, GrGLint level,
                          GrGLint internalformat, GrGLsizei width,
                          GrGLsizei height, GrGLint border, GrGLenum format,
                          GrGLenum type, const GrGLvoid* pixels) {
        trace_gl_fuctions->TexImage2D(target, level, internalformat, width,
                                      height, border, format, type, pixels);
      };

  interface->fFunctions.fTexParameterf =
      [trace_gl_fuctions](GrGLenum target, GrGLenum pname, GrGLfloat param) {
        trace_gl_fuctions->TexParameterf(target, pname, param);
      };

  interface->fFunctions.fTexParameterfv = [trace_gl_fuctions](
                                              GrGLenum target, GrGLenum pname,
                                              const GrGLfloat* params) {
    trace_gl_fuctions->TexParameterfv(target, pname, params);
  };

  interface->fFunctions.fTexParameteri =
      [trace_gl_fuctions](GrGLenum target, GrGLenum pname, GrGLint param) {
        trace_gl_fuctions->TexParameteri(target, pname, param);
      };

  interface->fFunctions.fTexParameteriv = [trace_gl_fuctions](
                                              GrGLenum target, GrGLenum pname,
                                              const GrGLint* params) {
    trace_gl_fuctions->TexParameteriv(target, pname, params);
  };

  interface->fFunctions.fTexStorage2D = [trace_gl_fuctions](
                                            GrGLenum target, GrGLsizei levels,
                                            GrGLenum internalformat,
                                            GrGLsizei width, GrGLsizei height) {
    trace_gl_fuctions->TexStorage2D(target, levels, internalformat, width,
                                    height);
  };

  interface->fFunctions.fDiscardFramebuffer = [trace_gl_fuctions](
                                                  GrGLenum target,
                                                  GrGLsizei numAttachments,
                                                  const GrGLenum* attachments) {
    trace_gl_fuctions->DiscardFramebuffer(target, numAttachments, attachments);
  };

  interface->fFunctions.fTestFence = [trace_gl_fuctions](GrGLuint fence) {
    return trace_gl_fuctions->TestFence(fence);
  };

  interface->fFunctions.fTexSubImage2D =
      [trace_gl_fuctions](GrGLenum target, GrGLint level, GrGLint xoffset,
                          GrGLint yoffset, GrGLsizei width, GrGLsizei height,
                          GrGLenum format, GrGLenum type,
                          const GrGLvoid* pixels) {
        trace_gl_fuctions->TexSubImage2D(target, level, xoffset, yoffset, width,
                                         height, format, type, pixels);
      };

  interface->fFunctions.fTextureBarrier = [trace_gl_fuctions]() {
    trace_gl_fuctions->TextureBarrier();
  };

  interface->fFunctions.fUniform1f = [trace_gl_fuctions](GrGLint location,
                                                         GrGLfloat v0) {
    trace_gl_fuctions->Uniform1f(location, v0);
  };

  interface->fFunctions.fUniform1i = [trace_gl_fuctions](GrGLint location,
                                                         GrGLint v0) {
    trace_gl_fuctions->Uniform1i(location, v0);
  };

  interface->fFunctions.fUniform1fv = [trace_gl_fuctions](GrGLint location,
                                                          GrGLsizei count,
                                                          const GrGLfloat* v) {
    trace_gl_fuctions->Uniform1fv(location, count, v);
  };

  interface->fFunctions.fUniform1iv =
      [trace_gl_fuctions](GrGLint location, GrGLsizei count, const GrGLint* v) {
        trace_gl_fuctions->Uniform1iv(location, count, v);
      };

  interface->fFunctions.fUniform2f =
      [trace_gl_fuctions](GrGLint location, GrGLfloat v0, GrGLfloat v1) {
        trace_gl_fuctions->Uniform2f(location, v0, v1);
      };

  interface->fFunctions.fUniform2i =
      [trace_gl_fuctions](GrGLint location, GrGLint v0, GrGLint v1) {
        trace_gl_fuctions->Uniform2i(location, v0, v1);
      };

  interface->fFunctions.fUniform2fv = [trace_gl_fuctions](GrGLint location,
                                                          GrGLsizei count,
                                                          const GrGLfloat* v) {
    trace_gl_fuctions->Uniform2fv(location, count, v);
  };

  interface->fFunctions.fUniform2iv =
      [trace_gl_fuctions](GrGLint location, GrGLsizei count, const GrGLint* v) {
        trace_gl_fuctions->Uniform2iv(location, count, v);
      };

  interface->fFunctions.fUniform3f = [trace_gl_fuctions](
                                         GrGLint location, GrGLfloat v0,
                                         GrGLfloat v1, GrGLfloat v2) {
    trace_gl_fuctions->Uniform3f(location, v0, v1, v2);
  };

  interface->fFunctions.fUniform3i = [trace_gl_fuctions](GrGLint location,
                                                         GrGLint v0, GrGLint v1,
                                                         GrGLint v2) {
    trace_gl_fuctions->Uniform3i(location, v0, v1, v2);
  };

  interface->fFunctions.fUniform3fv = [trace_gl_fuctions](GrGLint location,
                                                          GrGLsizei count,
                                                          const GrGLfloat* v) {
    trace_gl_fuctions->Uniform3fv(location, count, v);
  };

  interface->fFunctions.fUniform3iv =
      [trace_gl_fuctions](GrGLint location, GrGLsizei count, const GrGLint* v) {
        trace_gl_fuctions->Uniform3iv(location, count, v);
      };

  interface->fFunctions.fUniform4f =
      [trace_gl_fuctions](GrGLint location, GrGLfloat v0, GrGLfloat v1,
                          GrGLfloat v2, GrGLfloat v3) {
        trace_gl_fuctions->Uniform4f(location, v0, v1, v2, v3);
      };

  interface->fFunctions.fUniform4i = [trace_gl_fuctions](
                                         GrGLint location, GrGLint v0,
                                         GrGLint v1, GrGLint v2, GrGLint v3) {
    trace_gl_fuctions->Uniform4i(location, v0, v1, v2, v3);
  };

  interface->fFunctions.fUniform4fv = [trace_gl_fuctions](GrGLint location,
                                                          GrGLsizei count,
                                                          const GrGLfloat* v) {
    trace_gl_fuctions->Uniform4fv(location, count, v);
  };

  interface->fFunctions.fUniform4iv =
      [trace_gl_fuctions](GrGLint location, GrGLsizei count, const GrGLint* v) {
        trace_gl_fuctions->Uniform4iv(location, count, v);
      };

  interface->fFunctions.fUniformMatrix2fv =
      [trace_gl_fuctions](GrGLint location, GrGLsizei count,
                          GrGLboolean transpose, const GrGLfloat* value) {
        trace_gl_fuctions->UniformMatrix2fv(location, count, transpose, value);
      };

  interface->fFunctions.fUniformMatrix3fv =
      [trace_gl_fuctions](GrGLint location, GrGLsizei count,
                          GrGLboolean transpose, const GrGLfloat* value) {
        trace_gl_fuctions->UniformMatrix3fv(location, count, transpose, value);
      };

  interface->fFunctions.fUniformMatrix4fv =
      [trace_gl_fuctions](GrGLint location, GrGLsizei count,
                          GrGLboolean transpose, const GrGLfloat* value) {
        trace_gl_fuctions->UniformMatrix4fv(location, count, transpose, value);
      };

  interface->fFunctions.fUnmapBuffer = [trace_gl_fuctions](GrGLenum target) {
    return trace_gl_fuctions->UnmapBuffer(target);
  };

  interface->fFunctions.fUnmapBufferSubData =
      [trace_gl_fuctions](const GrGLvoid* mem) {
        trace_gl_fuctions->UnmapBufferSubData(mem);
      };

  interface->fFunctions.fUnmapTexSubImage2D =
      [trace_gl_fuctions](const GrGLvoid* mem) {
        trace_gl_fuctions->UnmapTexSubImage2D(mem);
      };

  interface->fFunctions.fUseProgram = [trace_gl_fuctions](GrGLuint program) {
    trace_gl_fuctions->UseProgram(program);
  };

  interface->fFunctions.fVertexAttrib1f =
      [trace_gl_fuctions](GrGLuint indx, const GrGLfloat value) {
        trace_gl_fuctions->VertexAttrib1f(indx, value);
      };

  interface->fFunctions.fVertexAttrib2fv =
      [trace_gl_fuctions](GrGLuint indx, const GrGLfloat* values) {
        trace_gl_fuctions->VertexAttrib2fv(indx, values);
      };

  interface->fFunctions.fVertexAttrib3fv =
      [trace_gl_fuctions](GrGLuint indx, const GrGLfloat* values) {
        trace_gl_fuctions->VertexAttrib3fv(indx, values);
      };

  interface->fFunctions.fVertexAttrib4fv =
      [trace_gl_fuctions](GrGLuint indx, const GrGLfloat* values) {
        trace_gl_fuctions->VertexAttrib4fv(indx, values);
      };

  interface->fFunctions.fVertexAttribDivisor =
      [trace_gl_fuctions](GrGLuint index, GrGLuint divisor) {
        trace_gl_fuctions->VertexAttribDivisor(index, divisor);
      };

  interface->fFunctions.fVertexAttribIPointer =
      [trace_gl_fuctions](GrGLuint indx, GrGLint size, GrGLenum type,
                          GrGLsizei stride, const GrGLvoid* ptr) {
        trace_gl_fuctions->VertexAttribIPointer(indx, size, type, stride, ptr);
      };

  interface->fFunctions.fVertexAttribPointer =
      [trace_gl_fuctions](GrGLuint indx, GrGLint size, GrGLenum type,
                          GrGLboolean normalized, GrGLsizei stride,
                          const GrGLvoid* ptr) {
        trace_gl_fuctions->VertexAttribPointer(indx, size, type, normalized,
                                               stride, ptr);
      };

  interface->fFunctions.fViewport = [trace_gl_fuctions](GrGLint x, GrGLint y,
                                                        GrGLsizei width,
                                                        GrGLsizei height) {
    trace_gl_fuctions->Viewport(x, y, width, height);
  };

  /* EXT_base_instance */
  interface->fFunctions.fDrawArraysInstancedBaseInstance =
      [trace_gl_fuctions](GrGLenum mode, GrGLint first, GrGLsizei count,
                          GrGLsizei instancecount, GrGLuint baseinstance) {
        trace_gl_fuctions->DrawArraysInstancedBaseInstance(
            mode, first, count, instancecount, baseinstance);
      };

  interface->fFunctions.fDrawElementsInstancedBaseVertexBaseInstance =
      [trace_gl_fuctions](GrGLenum mode, GrGLsizei count, GrGLenum type,
                          const void* indices, GrGLsizei instancecount,
                          GrGLint basevertex, GrGLuint baseinstance) {
        trace_gl_fuctions->DrawElementsInstancedBaseVertexBaseInstance(
            mode, count, type, indices, instancecount, basevertex,
            baseinstance);
      };

  /* EXT_multi_draw_indirect */
  interface->fFunctions.fMultiDrawArraysIndirect =
      [trace_gl_fuctions](GrGLenum mode, const GrGLvoid* indirect,
                          GrGLsizei drawcount, GrGLsizei stride) {
        trace_gl_fuctions->MultiDrawArraysIndirect(mode, indirect, drawcount,
                                                   stride);
      };

  interface->fFunctions.fMultiDrawElementsIndirect =
      [trace_gl_fuctions](GrGLenum mode, GrGLenum type,
                          const GrGLvoid* indirect, GrGLsizei drawcount,
                          GrGLsizei stride) {
        trace_gl_fuctions->MultiDrawElementsIndirect(mode, type, indirect,
                                                     drawcount, stride);
      };

  /* ANGLE_base_vertex_base_instance */
  interface->fFunctions.fMultiDrawArraysInstancedBaseInstance =
      [trace_gl_fuctions](
          GrGLenum mode, const GrGLint* firsts, const GrGLsizei* counts,
          const GrGLsizei* instanceCounts, const GrGLuint* baseInstances,
          const GrGLsizei drawcount) {
        trace_gl_fuctions->MultiDrawArraysInstancedBaseInstance(
            mode, firsts, counts, instanceCounts, baseInstances, drawcount);
      };

  interface->fFunctions.fMultiDrawElementsInstancedBaseVertexBaseInstance =
      [trace_gl_fuctions](
          GrGLenum mode, const GrGLint* counts, GrGLenum type,
          const GrGLvoid* const* indices, const GrGLsizei* instanceCounts,
          const GrGLint* baseVertices, const GrGLuint* baseInstances,
          const GrGLsizei drawcount) {
        trace_gl_fuctions->MultiDrawElementsInstancedBaseVertexBaseInstance(
            mode, counts, type, indices, instanceCounts, baseVertices,
            baseInstances, drawcount);
      };

  /* ARB_sync */
  interface->fFunctions.fFenceSync = [trace_gl_fuctions](GrGLenum condition,
                                                         GrGLbitfield flags) {
    return trace_gl_fuctions->FenceSync(condition, flags);
  };

  interface->fFunctions.fIsSync = [trace_gl_fuctions](GrGLsync sync) {
    return trace_gl_fuctions->IsSync(sync);
  };

  interface->fFunctions.fClientWaitSync = [trace_gl_fuctions](
                                              GrGLsync sync, GrGLbitfield flags,
                                              GrGLuint64 timeout) {
    return trace_gl_fuctions->ClientWaitSync(sync, flags, timeout);
  };

  interface->fFunctions.fWaitSync = [trace_gl_fuctions](GrGLsync sync,
                                                        GrGLbitfield flags,
                                                        GrGLuint64 timeout) {
    trace_gl_fuctions->WaitSync(sync, flags, timeout);
  };

  interface->fFunctions.fDeleteSync = [trace_gl_fuctions](GrGLsync sync) {
    trace_gl_fuctions->DeleteSync(sync);
  };

  /* ARB_internalformat_query */
  interface->fFunctions.fGetInternalformativ =
      [trace_gl_fuctions](GrGLenum target, GrGLenum internalformat,
                          GrGLenum pname, GrGLsizei bufSize, GrGLint* params) {
        trace_gl_fuctions->GetInternalformativ(target, internalformat, pname,
                                               bufSize, params);
      };

  /* KHR_debug */
  interface->fFunctions.fDebugMessageControl =
      [trace_gl_fuctions](GrGLenum source, GrGLenum type, GrGLenum severity,
                          GrGLsizei count, const GrGLuint* ids,
                          GrGLboolean enabled) {
        trace_gl_fuctions->DebugMessageControl(source, type, severity, count,
                                               ids, enabled);
      };

  interface->fFunctions.fDebugMessageInsert =
      [trace_gl_fuctions](GrGLenum source, GrGLenum type, GrGLuint id,
                          GrGLenum severity, GrGLsizei length,
                          const GrGLchar* buf) {
        trace_gl_fuctions->DebugMessageInsert(source, type, id, severity,
                                              length, buf);
      };

  interface->fFunctions.fDebugMessageCallback =
      [trace_gl_fuctions](GRGLDEBUGPROC callback, const GrGLvoid* userParam) {
        trace_gl_fuctions->DebugMessageCallback(callback, userParam);
      };

  interface->fFunctions.fGetDebugMessageLog =
      [trace_gl_fuctions](GrGLuint count, GrGLsizei bufSize, GrGLenum* sources,
                          GrGLenum* types, GrGLuint* ids, GrGLenum* severities,
                          GrGLsizei* lengths, GrGLchar* messageLog) {
        return trace_gl_fuctions->GetDebugMessageLog(count, bufSize, sources,
                                                     types, ids, severities,
                                                     lengths, messageLog);
      };

  interface->fFunctions.fPushDebugGroup =
      [trace_gl_fuctions](GrGLenum source, GrGLuint id, GrGLsizei length,
                          const GrGLchar* message) {
        trace_gl_fuctions->PushDebugGroup(source, id, length, message);
      };

  interface->fFunctions.fPopDebugGroup = [trace_gl_fuctions]() {
    trace_gl_fuctions->PopDebugGroup();
  };

  interface->fFunctions.fObjectLabel =
      [trace_gl_fuctions](GrGLenum identifier, GrGLuint name, GrGLsizei length,
                          const GrGLchar* label) {
        trace_gl_fuctions->ObjectLabel(identifier, name, length, label);
      };

  /** EXT_window_rectangles */
  interface->fFunctions.fWindowRectangles =
      [trace_gl_fuctions](GrGLenum mode, GrGLsizei count, const GrGLint box[]) {
        trace_gl_fuctions->WindowRectangles(mode, count, box);
      };

  /** GL_QCOM_tiled_rendering */
  interface->fFunctions.fStartTiling =
      [trace_gl_fuctions](GrGLuint x, GrGLuint y, GrGLuint width,
                          GrGLuint height, GrGLbitfield preserveMask) {
        trace_gl_fuctions->StartTiling(x, y, width, height, preserveMask);
      };

  interface->fFunctions.fEndTiling =
      [trace_gl_fuctions](GrGLbitfield preserveMask) {
        trace_gl_fuctions->EndTiling(preserveMask);
      };
}  // NOLINT

TraceGlFuctions::TraceGlFuctions(GrGLInterface* interface)
    : real_functions_(interface->fFunctions),
      fbo_changed_(0),
      current_texture_size_(0),
      total_tex_upload_size_(0) {}

GrGLvoid TraceGlFuctions::ActiveTexture(GrGLenum texture) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fActiveTexture(texture);
}

GrGLvoid TraceGlFuctions::AttachShader(GrGLuint program, GrGLuint shader) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fAttachShader(program, shader);
}

GrGLvoid TraceGlFuctions::BeginQuery(GrGLenum target, GrGLuint id) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fBeginQuery(target, id);
}

GrGLvoid TraceGlFuctions::BindAttribLocation(GrGLuint program, GrGLuint index,
                                             const char* name) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fBindAttribLocation(program, index, name);
}

GrGLvoid TraceGlFuctions::BindBuffer(GrGLenum target, GrGLuint buffer) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fBindBuffer(target, buffer);
}

GrGLvoid TraceGlFuctions::BindFramebuffer(GrGLenum target,
                                          GrGLuint framebuffer) {
  TRACE_EVENT("gpu", __FUNCTION__);
  TRACE_COUNTER("gpu", __FUNCTION__, ++fbo_changed_);
  real_functions_.fBindFramebuffer(target, framebuffer);
}

GrGLvoid TraceGlFuctions::BindRenderbuffer(GrGLenum target,
                                           GrGLuint renderbuffer) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fBindRenderbuffer(target, renderbuffer);
}

GrGLvoid TraceGlFuctions::BindTexture(GrGLenum target, GrGLuint texture) {
  TRACE_EVENT("gpu", __FUNCTION__);
  target_to_texture_[target] = texture;
  real_functions_.fBindTexture(target, texture);
}

GrGLvoid TraceGlFuctions::BindFragDataLocation(GrGLuint program,
                                               GrGLuint color_number,
                                               const GrGLchar* name) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fBindFragDataLocation(program, color_number, name);
}

GrGLvoid TraceGlFuctions::BindFragDataLocationIndexed(GrGLuint program,
                                                      GrGLuint color_number,
                                                      GrGLuint index,
                                                      const GrGLchar* name) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fBindFragDataLocationIndexed(program, color_number, index,
                                               name);
}

GrGLvoid TraceGlFuctions::BindSampler(GrGLuint unit, GrGLuint sampler) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fBindSampler(unit, sampler);
}

GrGLvoid TraceGlFuctions::BindVertexArray(GrGLuint array) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fBindVertexArray(array);
}

GrGLvoid TraceGlFuctions::BlendBarrier() {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fBlendBarrier();
}

GrGLvoid TraceGlFuctions::BlendColor(GrGLclampf red, GrGLclampf green,
                                     GrGLclampf blue, GrGLclampf alpha) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fBlendColor(red, green, blue, alpha);
}

GrGLvoid TraceGlFuctions::BlendEquation(GrGLenum mode) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fBlendEquation(mode);
}

GrGLvoid TraceGlFuctions::BlendFunc(GrGLenum sfactor, GrGLenum dfactor) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fBlendFunc(sfactor, dfactor);
}

GrGLvoid TraceGlFuctions::BlitFramebuffer(GrGLint srcX0, GrGLint srcY0,
                                          GrGLint srcX1, GrGLint srcY1,
                                          GrGLint dstX0, GrGLint dstY0,
                                          GrGLint dstX1, GrGLint dstY1,
                                          GrGLbitfield mask, GrGLenum filter) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0,
                                   dstX1, dstY1, mask, filter);
}

GrGLvoid TraceGlFuctions::BufferData(GrGLenum target, GrGLsizeiptr size,
                                     const GrGLvoid* data, GrGLenum usage) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fBufferData(target, size, data, usage);
}

GrGLvoid TraceGlFuctions::BufferSubData(GrGLenum target, GrGLintptr offset,
                                        GrGLsizeiptr size,
                                        const GrGLvoid* data) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fBufferSubData(target, offset, size, data);
}

GrGLenum TraceGlFuctions::CheckFramebufferStatus(GrGLenum target) {
  TRACE_EVENT("gpu", __FUNCTION__);
  return real_functions_.fCheckFramebufferStatus(target);
}

GrGLvoid TraceGlFuctions::Clear(GrGLbitfield mask) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fClear(mask);
}

GrGLvoid TraceGlFuctions::ClearColor(GrGLclampf red, GrGLclampf green,
                                     GrGLclampf blue, GrGLclampf alpha) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fClearColor(red, green, blue, alpha);
}

GrGLvoid TraceGlFuctions::ClearStencil(GrGLint s) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fClearStencil(s);
}

GrGLvoid TraceGlFuctions::ClearTexImage(GrGLuint texture, GrGLint level,
                                        GrGLenum format, GrGLenum type,
                                        const GrGLvoid* data) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fClearTexImage(texture, level, format, type, data);
}

GrGLvoid TraceGlFuctions::ClearTexSubImage(GrGLuint texture, GrGLint level,
                                           GrGLint xoffset, GrGLint yoffset,
                                           GrGLint zoffset, GrGLsizei width,
                                           GrGLsizei height, GrGLsizei depth,
                                           GrGLenum format, GrGLenum type,
                                           const GrGLvoid* data) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fClearTexSubImage(texture, level, xoffset, yoffset, zoffset,
                                    width, height, depth, format, type, data);
}

GrGLvoid TraceGlFuctions::ColorMask(GrGLboolean red, GrGLboolean green,
                                    GrGLboolean blue, GrGLboolean alpha) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fColorMask(red, green, blue, alpha);
}

GrGLvoid TraceGlFuctions::CompileShader(GrGLuint shader) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fCompileShader(shader);
}

GrGLvoid TraceGlFuctions::CompressedTexImage2D(GrGLenum target, GrGLint level,
                                               GrGLenum internalformat,
                                               GrGLsizei width,
                                               GrGLsizei height, GrGLint border,
                                               GrGLsizei imageSize,
                                               const GrGLvoid* data) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fCompressedTexImage2D(target, level, internalformat, width,
                                        height, border, imageSize, data);
}

GrGLvoid TraceGlFuctions::CompressedTexSubImage2D(
    GrGLenum target, GrGLint level, GrGLint xoffset, GrGLint yoffset,
    GrGLsizei width, GrGLsizei height, GrGLenum format, GrGLsizei imageSize,
    const GrGLvoid* data) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fCompressedTexSubImage2D(
      target, level, xoffset, yoffset, width, height, format, imageSize, data);
}

GrGLvoid TraceGlFuctions::CopyTexSubImage2D(GrGLenum target, GrGLint level,
                                            GrGLint xoffset, GrGLint yoffset,
                                            GrGLint x, GrGLint y,
                                            GrGLsizei width, GrGLsizei height) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fCopyTexSubImage2D(target, level, xoffset, yoffset, x, y,
                                     width, height);
}

GrGLuint TraceGlFuctions::CreateProgram() {
  TRACE_EVENT("gpu", __FUNCTION__);
  return real_functions_.fCreateProgram();
}

GrGLuint TraceGlFuctions::CreateShader(GrGLenum type) {
  TRACE_EVENT("gpu", __FUNCTION__);
  return real_functions_.fCreateShader(type);
}

GrGLvoid TraceGlFuctions::CullFace(GrGLenum mode) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fCullFace(mode);
}

GrGLvoid TraceGlFuctions::DeleteBuffers(GrGLsizei n, const GrGLuint* buffers) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fDeleteBuffers(n, buffers);
}

GrGLvoid TraceGlFuctions::DeleteFences(GrGLsizei n, const GrGLuint* fences) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fDeleteFences(n, fences);
}

GrGLvoid TraceGlFuctions::DeleteFramebuffers(GrGLsizei n,
                                             const GrGLuint* framebuffers) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fDeleteFramebuffers(n, framebuffers);
}

GrGLvoid TraceGlFuctions::DeleteProgram(GrGLuint program) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fDeleteProgram(program);
}

GrGLvoid TraceGlFuctions::DeleteQueries(GrGLsizei n, const GrGLuint* ids) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fDeleteQueries(n, ids);
}

GrGLvoid TraceGlFuctions::DeleteRenderbuffers(GrGLsizei n,
                                              const GrGLuint* renderbuffers) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fDeleteRenderbuffers(n, renderbuffers);
}

GrGLvoid TraceGlFuctions::DeleteSamplers(GrGLsizei count,
                                         const GrGLuint* samplers) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fDeleteSamplers(count, samplers);
}

GrGLvoid TraceGlFuctions::DeleteShader(GrGLuint shader) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fDeleteShader(shader);
}

GrGLvoid TraceGlFuctions::DeleteTextures(GrGLsizei n,
                                         const GrGLuint* textures) {
  TRACE_EVENT("gpu", __FUNCTION__);
  for (GrGLsizei i = 0; i < n; i++) {
    auto it = texture_infos_.find(textures[i]);
    if (it != texture_infos_.end()) {
      for (auto& info : it->second) {
        if (info.inited) {
          current_texture_size_ -= info.size;
        }
      }
      texture_infos_.erase(it);
    }
  }
  LogTexUsage();
  real_functions_.fDeleteTextures(n, textures);
}

GrGLvoid TraceGlFuctions::DeleteVertexArrays(GrGLsizei n,
                                             const GrGLuint* arrays) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fDeleteVertexArrays(n, arrays);
}

GrGLvoid TraceGlFuctions::DepthMask(GrGLboolean flag) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fDepthMask(flag);
}

GrGLvoid TraceGlFuctions::Disable(GrGLenum cap) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fDisable(cap);
}

GrGLvoid TraceGlFuctions::DisableVertexAttribArray(GrGLuint index) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fDisableVertexAttribArray(index);
}

GrGLvoid TraceGlFuctions::DrawArrays(GrGLenum mode, GrGLint first,
                                     GrGLsizei count) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fDrawArrays(mode, first, count);
}

GrGLvoid TraceGlFuctions::DrawArraysInstanced(GrGLenum mode, GrGLint first,
                                              GrGLsizei count,
                                              GrGLsizei primcount) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fDrawArraysInstanced(mode, first, count, primcount);
}

GrGLvoid TraceGlFuctions::DrawArraysIndirect(GrGLenum mode,
                                             const GrGLvoid* indirect) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fDrawArraysIndirect(mode, indirect);
}

GrGLvoid TraceGlFuctions::DrawBuffer(GrGLenum mode) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fDrawBuffer(mode);
}

GrGLvoid TraceGlFuctions::DrawBuffers(GrGLsizei n, const GrGLenum* bufs) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fDrawBuffers(n, bufs);
}

GrGLvoid TraceGlFuctions::DrawElements(GrGLenum mode, GrGLsizei count,
                                       GrGLenum type, const GrGLvoid* indices) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fDrawElements(mode, count, type, indices);
}

GrGLvoid TraceGlFuctions::DrawElementsInstanced(GrGLenum mode, GrGLsizei count,
                                                GrGLenum type,
                                                const GrGLvoid* indices,
                                                GrGLsizei primcount) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fDrawElementsInstanced(mode, count, type, indices, primcount);
}

GrGLvoid TraceGlFuctions::DrawElementsIndirect(GrGLenum mode, GrGLenum type,
                                               const GrGLvoid* indirect) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fDrawElementsIndirect(mode, type, indirect);
}

GrGLvoid TraceGlFuctions::DrawRangeElements(GrGLenum mode, GrGLuint start,
                                            GrGLuint end, GrGLsizei count,
                                            GrGLenum type,
                                            const GrGLvoid* indices) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fDrawRangeElements(mode, start, end, count, type, indices);
}

GrGLvoid TraceGlFuctions::Enable(GrGLenum cap) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fEnable(cap);
}

GrGLvoid TraceGlFuctions::EnableVertexAttribArray(GrGLuint index) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fEnableVertexAttribArray(index);
}

GrGLvoid TraceGlFuctions::EndQuery(GrGLenum target) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fEndQuery(target);
}

GrGLvoid TraceGlFuctions::Finish() {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fFinish();
}

GrGLvoid TraceGlFuctions::FinishFence(GrGLuint fence) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fFinishFence(fence);
}

GrGLvoid TraceGlFuctions::Flush() {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fFlush();
}

GrGLvoid TraceGlFuctions::FlushMappedBufferRange(GrGLenum target,
                                                 GrGLintptr offset,
                                                 GrGLsizeiptr length) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fFlushMappedBufferRange(target, offset, length);
}

GrGLvoid TraceGlFuctions::FramebufferRenderbuffer(GrGLenum target,
                                                  GrGLenum attachment,
                                                  GrGLenum renderbuffertarget,
                                                  GrGLuint renderbuffer) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fFramebufferRenderbuffer(target, attachment,
                                           renderbuffertarget, renderbuffer);
}

GrGLvoid TraceGlFuctions::FramebufferTexture2D(GrGLenum target,
                                               GrGLenum attachment,
                                               GrGLenum textarget,
                                               GrGLuint texture,
                                               GrGLint level) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fFramebufferTexture2D(target, attachment, textarget, texture,
                                        level);
}

GrGLvoid TraceGlFuctions::FramebufferTexture2DMultisample(
    GrGLenum target, GrGLenum attachment, GrGLenum textarget, GrGLuint texture,
    GrGLint level, GrGLsizei samples) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fFramebufferTexture2DMultisample(
      target, attachment, textarget, texture, level, samples);
}

GrGLvoid TraceGlFuctions::FrontFace(GrGLenum mode) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fFrontFace(mode);
}

GrGLvoid TraceGlFuctions::GenBuffers(GrGLsizei n, GrGLuint* buffers) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fGenBuffers(n, buffers);
}

GrGLvoid TraceGlFuctions::GenFences(GrGLsizei n, GrGLuint* fences) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fGenFences(n, fences);
}

GrGLvoid TraceGlFuctions::GenFramebuffers(GrGLsizei n, GrGLuint* framebuffers) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fGenFramebuffers(n, framebuffers);
}

GrGLvoid TraceGlFuctions::GenerateMipmap(GrGLenum target) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fGenerateMipmap(target);
}

GrGLvoid TraceGlFuctions::GenQueries(GrGLsizei n, GrGLuint* ids) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fGenQueries(n, ids);
}

GrGLvoid TraceGlFuctions::GenRenderbuffers(GrGLsizei n,
                                           GrGLuint* renderbuffers) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fGenRenderbuffers(n, renderbuffers);
}

GrGLvoid TraceGlFuctions::GenSamplers(GrGLsizei count, GrGLuint* samplers) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fGenSamplers(count, samplers);
}

GrGLvoid TraceGlFuctions::GenTextures(GrGLsizei n, GrGLuint* textures) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fGenTextures(n, textures);
}

GrGLvoid TraceGlFuctions::GenVertexArrays(GrGLsizei n, GrGLuint* arrays) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fGenVertexArrays(n, arrays);
}

GrGLvoid TraceGlFuctions::GetBufferParameteriv(GrGLenum target, GrGLenum pname,
                                               GrGLint* params) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fGetBufferParameteriv(target, pname, params);
}

GrGLenum TraceGlFuctions::GetError() {
  TRACE_EVENT("gpu", __FUNCTION__);
  return real_functions_.fGetError();
}

GrGLvoid TraceGlFuctions::GetFramebufferAttachmentParameteriv(
    GrGLenum target, GrGLenum attachment, GrGLenum pname, GrGLint* params) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fGetFramebufferAttachmentParameteriv(target, attachment,
                                                       pname, params);
}

GrGLvoid TraceGlFuctions::GetIntegerv(GrGLenum pname, GrGLint* params) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fGetIntegerv(pname, params);
}

GrGLvoid TraceGlFuctions::GetMultisamplefv(GrGLenum pname, GrGLuint index,
                                           GrGLfloat* val) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fGetMultisamplefv(pname, index, val);
}

GrGLvoid TraceGlFuctions::GetProgramBinary(GrGLuint program, GrGLsizei bufsize,
                                           GrGLsizei* length,
                                           GrGLenum* binaryFormat,
                                           void* binary) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fGetProgramBinary(program, bufsize, length, binaryFormat,
                                    binary);
}

GrGLvoid TraceGlFuctions::GetProgramInfoLog(GrGLuint program, GrGLsizei bufsize,
                                            GrGLsizei* length, char* infolog) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fGetProgramInfoLog(program, bufsize, length, infolog);
}

GrGLvoid TraceGlFuctions::GetProgramiv(GrGLuint program, GrGLenum pname,
                                       GrGLint* params) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fGetProgramiv(program, pname, params);
}

GrGLvoid TraceGlFuctions::GetQueryiv(GrGLenum GLtarget, GrGLenum pname,
                                     GrGLint* params) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fGetQueryiv(GLtarget, pname, params);
}

GrGLvoid TraceGlFuctions::GetQueryObjecti64v(GrGLuint id, GrGLenum pname,
                                             GrGLint64* params) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fGetQueryObjecti64v(id, pname, params);
}

GrGLvoid TraceGlFuctions::GetQueryObjectiv(GrGLuint id, GrGLenum pname,
                                           GrGLint* params) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fGetQueryObjectiv(id, pname, params);
}

GrGLvoid TraceGlFuctions::GetQueryObjectui64v(GrGLuint id, GrGLenum pname,
                                              GrGLuint64* params) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fGetQueryObjectui64v(id, pname, params);
}

GrGLvoid TraceGlFuctions::GetQueryObjectuiv(GrGLuint id, GrGLenum pname,
                                            GrGLuint* params) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fGetQueryObjectuiv(id, pname, params);
}

GrGLvoid TraceGlFuctions::GetRenderbufferParameteriv(GrGLenum target,
                                                     GrGLenum pname,
                                                     GrGLint* params) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fGetRenderbufferParameteriv(target, pname, params);
}

GrGLvoid TraceGlFuctions::GetShaderInfoLog(GrGLuint shader, GrGLsizei bufsize,
                                           GrGLsizei* length, char* infolog) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fGetShaderInfoLog(shader, bufsize, length, infolog);
}

GrGLvoid TraceGlFuctions::GetShaderiv(GrGLuint shader, GrGLenum pname,
                                      GrGLint* params) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fGetShaderiv(shader, pname, params);
}

GrGLvoid TraceGlFuctions::GetShaderPrecisionFormat(GrGLenum shadertype,
                                                   GrGLenum precisiontype,
                                                   GrGLint* range,
                                                   GrGLint* precision) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fGetShaderPrecisionFormat(shadertype, precisiontype, range,
                                            precision);
}

const GrGLubyte* TraceGlFuctions::GetString(GrGLenum name) {
  TRACE_EVENT("gpu", __FUNCTION__);
  return real_functions_.fGetString(name);
}

const GrGLubyte* TraceGlFuctions::GetStringi(GrGLenum name, GrGLuint index) {
  TRACE_EVENT("gpu", __FUNCTION__);
  return real_functions_.fGetStringi(name, index);
}

GrGLvoid TraceGlFuctions::GetTexLevelParameteriv(GrGLenum target, GrGLint level,
                                                 GrGLenum pname,
                                                 GrGLint* params) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fGetTexLevelParameteriv(target, level, pname, params);
}

GrGLint TraceGlFuctions::GetUniformLocation(GrGLuint program,
                                            const char* name) {
  TRACE_EVENT("gpu", __FUNCTION__);
  return real_functions_.fGetUniformLocation(program, name);
}

GrGLvoid TraceGlFuctions::InsertEventMarker(GrGLsizei length,
                                            const char* marker) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fInsertEventMarker(length, marker);
}

GrGLvoid TraceGlFuctions::InvalidateBufferData(GrGLuint buffer) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fInvalidateBufferData(buffer);
}

GrGLvoid TraceGlFuctions::InvalidateBufferSubData(GrGLuint buffer,
                                                  GrGLintptr offset,
                                                  GrGLsizeiptr length) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fInvalidateBufferSubData(buffer, offset, length);
}

GrGLvoid TraceGlFuctions::InvalidateFramebuffer(GrGLenum target,
                                                GrGLsizei numAttachments,
                                                const GrGLenum* attachments) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fInvalidateFramebuffer(target, numAttachments, attachments);
}

GrGLvoid TraceGlFuctions::InvalidateSubFramebuffer(
    GrGLenum target, GrGLsizei numAttachments, const GrGLenum* attachments,
    GrGLint x, GrGLint y, GrGLsizei width, GrGLsizei height) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fInvalidateSubFramebuffer(target, numAttachments, attachments,
                                            x, y, width, height);
}

GrGLvoid TraceGlFuctions::InvalidateTexImage(GrGLuint texture, GrGLint level) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fInvalidateTexImage(texture, level);
}

GrGLvoid TraceGlFuctions::InvalidateTexSubImage(
    GrGLuint texture, GrGLint level, GrGLint xoffset, GrGLint yoffset,
    GrGLint zoffset, GrGLsizei width, GrGLsizei height, GrGLsizei depth) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fInvalidateTexSubImage(texture, level, xoffset, yoffset,
                                         zoffset, width, height, depth);
}

GrGLboolean TraceGlFuctions::IsTexture(GrGLuint texture) {
  TRACE_EVENT("gpu", __FUNCTION__);
  return real_functions_.fIsTexture(texture);
}

GrGLvoid TraceGlFuctions::LineWidth(GrGLfloat width) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fLineWidth(width);
}

GrGLvoid TraceGlFuctions::LinkProgram(GrGLuint program) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fLinkProgram(program);
}

GrGLvoid* TraceGlFuctions::MapBuffer(GrGLenum target, GrGLenum access) {
  TRACE_EVENT("gpu", __FUNCTION__);
  return real_functions_.fMapBuffer(target, access);
}

GrGLvoid* TraceGlFuctions::MapBufferRange(GrGLenum target, GrGLintptr offset,
                                          GrGLsizeiptr length,
                                          GrGLbitfield access) {
  TRACE_EVENT("gpu", __FUNCTION__);
  return real_functions_.fMapBufferRange(target, offset, length, access);
}

GrGLvoid* TraceGlFuctions::MapBufferSubData(GrGLuint target, GrGLintptr offset,
                                            GrGLsizeiptr size,
                                            GrGLenum access) {
  TRACE_EVENT("gpu", __FUNCTION__);
  return real_functions_.fMapBufferSubData(target, offset, size, access);
}

GrGLvoid* TraceGlFuctions::MapTexSubImage2D(GrGLenum target, GrGLint level,
                                            GrGLint xoffset, GrGLint yoffset,
                                            GrGLsizei width, GrGLsizei height,
                                            GrGLenum format, GrGLenum type,
                                            GrGLenum access) {
  TRACE_EVENT("gpu", __FUNCTION__);
  return real_functions_.fMapTexSubImage2D(target, level, xoffset, yoffset,
                                           width, height, format, type, access);
}

GrGLvoid* TraceGlFuctions::MemoryBarrier(GrGLbitfield barriers) {
  TRACE_EVENT("gpu", __FUNCTION__);
  return real_functions_.fMemoryBarrier(barriers);
}

GrGLvoid TraceGlFuctions::PatchParameteri(GrGLenum pname, GrGLint value) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fPatchParameteri(pname, value);
}

GrGLvoid TraceGlFuctions::PixelStorei(GrGLenum pname, GrGLint param) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fPixelStorei(pname, param);
}

GrGLvoid TraceGlFuctions::PolygonMode(GrGLenum face, GrGLenum mode) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fPolygonMode(face, mode);
}

GrGLvoid TraceGlFuctions::PopGroupMarker() {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fPopGroupMarker();
}

GrGLvoid TraceGlFuctions::ProgramBinary(GrGLuint program, GrGLenum binaryFormat,
                                        void* binary, GrGLsizei length) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fProgramBinary(program, binaryFormat, binary, length);
}

GrGLvoid TraceGlFuctions::ProgramParameteri(GrGLuint program, GrGLenum pname,
                                            GrGLint value) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fProgramParameteri(program, pname, value);
}

GrGLvoid TraceGlFuctions::PushGroupMarker(GrGLsizei length,
                                          const char* marker) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fPushGroupMarker(length, marker);
}

GrGLvoid TraceGlFuctions::QueryCounter(GrGLuint id, GrGLenum target) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fQueryCounter(id, target);
}

GrGLvoid TraceGlFuctions::ReadBuffer(GrGLenum src) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fReadBuffer(src);
}

GrGLvoid TraceGlFuctions::ReadPixels(GrGLint x, GrGLint y, GrGLsizei width,
                                     GrGLsizei height, GrGLenum format,
                                     GrGLenum type, GrGLvoid* pixels) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fReadPixels(x, y, width, height, format, type, pixels);
}

GrGLvoid TraceGlFuctions::RenderbufferStorage(GrGLenum target,
                                              GrGLenum internalformat,
                                              GrGLsizei width,
                                              GrGLsizei height) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fRenderbufferStorage(target, internalformat, width, height);
}

GrGLvoid TraceGlFuctions::RenderbufferStorageMultisample(
    GrGLenum target, GrGLsizei samples, GrGLenum internalformat,
    GrGLsizei width, GrGLsizei height) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fRenderbufferStorageMultisample(
      target, samples, internalformat, width, height);
}

GrGLvoid TraceGlFuctions::ResolveMultisampleFramebuffer() {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fResolveMultisampleFramebuffer();
}

GrGLvoid TraceGlFuctions::SamplerParameteri(GrGLuint sampler, GrGLenum pname,
                                            GrGLint params) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fSamplerParameteri(sampler, pname, params);
}

GrGLvoid TraceGlFuctions::SamplerParameteriv(GrGLuint sampler, GrGLenum pname,
                                             const GrGLint* params) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fSamplerParameteriv(sampler, pname, params);
}

GrGLvoid TraceGlFuctions::Scissor(GrGLint x, GrGLint y, GrGLsizei width,
                                  GrGLsizei height) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fScissor(x, y, width, height);
}

// GL_CHROMIUM_bind_uniform_location
GrGLvoid TraceGlFuctions::BindUniformLocation(GrGLuint program,
                                              GrGLint location,
                                              const char* name) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fBindUniformLocation(program, location, name);
}

GrGLvoid TraceGlFuctions::SetFence(GrGLuint fence, GrGLenum condition) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fSetFence(fence, condition);
}

GrGLvoid TraceGlFuctions::ShaderSource(GrGLuint shader, GrGLsizei count,
                                       const char* const* str,
                                       const GrGLint* length) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fShaderSource(shader, count, str, length);
}

GrGLvoid TraceGlFuctions::StencilFunc(GrGLenum func, GrGLint ref,
                                      GrGLuint mask) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fStencilFunc(func, ref, mask);
}

GrGLvoid TraceGlFuctions::StencilFuncSeparate(GrGLenum face, GrGLenum func,
                                              GrGLint ref, GrGLuint mask) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fStencilFuncSeparate(face, func, ref, mask);
}

GrGLvoid TraceGlFuctions::StencilMask(GrGLuint mask) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fStencilMask(mask);
}

GrGLvoid TraceGlFuctions::StencilMaskSeparate(GrGLenum face, GrGLuint mask) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fStencilMaskSeparate(face, mask);
}

GrGLvoid TraceGlFuctions::StencilOp(GrGLenum fail, GrGLenum zfail,
                                    GrGLenum zpass) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fStencilOp(fail, zfail, zpass);
}

GrGLvoid TraceGlFuctions::StencilOpSeparate(GrGLenum face, GrGLenum fail,
                                            GrGLenum zfail, GrGLenum zpass) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fStencilOpSeparate(face, fail, zfail, zpass);
}

GrGLvoid TraceGlFuctions::TexBuffer(GrGLenum target, GrGLenum internalformat,
                                    GrGLuint buffer) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fTexBuffer(target, internalformat, buffer);
}

GrGLvoid TraceGlFuctions::TexBufferRange(GrGLenum target,
                                         GrGLenum internalformat,
                                         GrGLuint buffer, GrGLintptr offset,
                                         GrGLsizeiptr size) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fTexBufferRange(target, internalformat, buffer, offset, size);
}

GrGLvoid TraceGlFuctions::TexImage2D(GrGLenum target, GrGLint level,
                                     GrGLint internalformat, GrGLsizei width,
                                     GrGLsizei height, GrGLint border,
                                     GrGLenum format, GrGLenum type,
                                     const GrGLvoid* pixels) {
  TRACE_EVENT("gpu", __FUNCTION__);
  if (auto texture_info =
          SetTextureInfo(target, level, internalformat, width, height)) {
    LogTexUsage();
    if (pixels != nullptr) {
      total_tex_upload_size_ += texture_info->size;
      LogTexUploadSize();
    }
  }
  real_functions_.fTexImage2D(target, level, internalformat, width, height,
                              border, format, type, pixels);
}

GrGLvoid TraceGlFuctions::TexParameterf(GrGLenum target, GrGLenum pname,
                                        GrGLfloat param) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fTexParameterf(target, pname, param);
}

GrGLvoid TraceGlFuctions::TexParameterfv(GrGLenum target, GrGLenum pname,
                                         const GrGLfloat* params) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fTexParameterfv(target, pname, params);
}

GrGLvoid TraceGlFuctions::TexParameteri(GrGLenum target, GrGLenum pname,
                                        GrGLint param) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fTexParameteri(target, pname, param);
}

GrGLvoid TraceGlFuctions::TexParameteriv(GrGLenum target, GrGLenum pname,
                                         const GrGLint* params) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fTexParameteriv(target, pname, params);
}

GrGLvoid TraceGlFuctions::TexStorage2D(GrGLenum target, GrGLsizei levels,
                                       GrGLenum internalformat, GrGLsizei width,
                                       GrGLsizei height) {
  TRACE_EVENT("gpu", __FUNCTION__);
  for (GrGLsizei i = 0; i < levels; i++) {
    SetTextureInfo(target, i, internalformat, std::max(1, width >> i),
                   std::max(1, height >> i));
  }
  LogTexUsage();

  real_functions_.fTexStorage2D(target, levels, internalformat, width, height);
}

GrGLvoid TraceGlFuctions::DiscardFramebuffer(GrGLenum target,
                                             GrGLsizei numAttachments,
                                             const GrGLenum* attachments) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fDiscardFramebuffer(target, numAttachments, attachments);
}

GrGLboolean TraceGlFuctions::TestFence(GrGLuint fence) {
  TRACE_EVENT("gpu", __FUNCTION__);
  return real_functions_.fTestFence(fence);
}

GrGLvoid TraceGlFuctions::TexSubImage2D(GrGLenum target, GrGLint level,
                                        GrGLint xoffset, GrGLint yoffset,
                                        GrGLsizei width, GrGLsizei height,
                                        GrGLenum format, GrGLenum type,
                                        const GrGLvoid* pixels) {
  TRACE_EVENT("gpu", __FUNCTION__);
  if (auto texture_info = GetTextureInfo(target, level)) {
    total_tex_upload_size_ +=
        CalculateTexSize(target, texture_info->format, width, height);
    LogTexUploadSize();
  }
  real_functions_.fTexSubImage2D(target, level, xoffset, yoffset, width, height,
                                 format, type, pixels);
}

GrGLvoid TraceGlFuctions::TextureBarrier() {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fTextureBarrier();
}

GrGLvoid TraceGlFuctions::Uniform1f(GrGLint location, GrGLfloat v0) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fUniform1f(location, v0);
}

GrGLvoid TraceGlFuctions::Uniform1i(GrGLint location, GrGLint v0) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fUniform1i(location, v0);
}

GrGLvoid TraceGlFuctions::Uniform1fv(GrGLint location, GrGLsizei count,
                                     const GrGLfloat* v) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fUniform1fv(location, count, v);
}

GrGLvoid TraceGlFuctions::Uniform1iv(GrGLint location, GrGLsizei count,
                                     const GrGLint* v) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fUniform1iv(location, count, v);
}

GrGLvoid TraceGlFuctions::Uniform2f(GrGLint location, GrGLfloat v0,
                                    GrGLfloat v1) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fUniform2f(location, v0, v1);
}

GrGLvoid TraceGlFuctions::Uniform2i(GrGLint location, GrGLint v0, GrGLint v1) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fUniform2i(location, v0, v1);
}

GrGLvoid TraceGlFuctions::Uniform2fv(GrGLint location, GrGLsizei count,
                                     const GrGLfloat* v) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fUniform2fv(location, count, v);
}

GrGLvoid TraceGlFuctions::Uniform2iv(GrGLint location, GrGLsizei count,
                                     const GrGLint* v) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fUniform2iv(location, count, v);
}

GrGLvoid TraceGlFuctions::Uniform3f(GrGLint location, GrGLfloat v0,
                                    GrGLfloat v1, GrGLfloat v2) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fUniform3f(location, v0, v1, v2);
}

GrGLvoid TraceGlFuctions::Uniform3i(GrGLint location, GrGLint v0, GrGLint v1,
                                    GrGLint v2) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fUniform3i(location, v0, v1, v2);
}

GrGLvoid TraceGlFuctions::Uniform3fv(GrGLint location, GrGLsizei count,
                                     const GrGLfloat* v) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fUniform3fv(location, count, v);
}

GrGLvoid TraceGlFuctions::Uniform3iv(GrGLint location, GrGLsizei count,
                                     const GrGLint* v) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fUniform3iv(location, count, v);
}

GrGLvoid TraceGlFuctions::Uniform4f(GrGLint location, GrGLfloat v0,
                                    GrGLfloat v1, GrGLfloat v2, GrGLfloat v3) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fUniform4f(location, v0, v1, v2, v3);
}

GrGLvoid TraceGlFuctions::Uniform4i(GrGLint location, GrGLint v0, GrGLint v1,
                                    GrGLint v2, GrGLint v3) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fUniform4i(location, v0, v1, v2, v3);
}

GrGLvoid TraceGlFuctions::Uniform4fv(GrGLint location, GrGLsizei count,
                                     const GrGLfloat* v) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fUniform4fv(location, count, v);
}

GrGLvoid TraceGlFuctions::Uniform4iv(GrGLint location, GrGLsizei count,
                                     const GrGLint* v) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fUniform4iv(location, count, v);
}

GrGLvoid TraceGlFuctions::UniformMatrix2fv(GrGLint location, GrGLsizei count,
                                           GrGLboolean transpose,
                                           const GrGLfloat* value) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fUniformMatrix2fv(location, count, transpose, value);
}

GrGLvoid TraceGlFuctions::UniformMatrix3fv(GrGLint location, GrGLsizei count,
                                           GrGLboolean transpose,
                                           const GrGLfloat* value) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fUniformMatrix3fv(location, count, transpose, value);
}

GrGLvoid TraceGlFuctions::UniformMatrix4fv(GrGLint location, GrGLsizei count,
                                           GrGLboolean transpose,
                                           const GrGLfloat* value) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fUniformMatrix4fv(location, count, transpose, value);
}

GrGLboolean TraceGlFuctions::UnmapBuffer(GrGLenum target) {
  TRACE_EVENT("gpu", __FUNCTION__);
  return real_functions_.fUnmapBuffer(target);
}

GrGLvoid TraceGlFuctions::UnmapBufferSubData(const GrGLvoid* mem) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fUnmapBufferSubData(mem);
}

GrGLvoid TraceGlFuctions::UnmapTexSubImage2D(const GrGLvoid* mem) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fUnmapTexSubImage2D(mem);
}

GrGLvoid TraceGlFuctions::UseProgram(GrGLuint program) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fUseProgram(program);
}

GrGLvoid TraceGlFuctions::VertexAttrib1f(GrGLuint indx, const GrGLfloat value) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fVertexAttrib1f(indx, value);
}

GrGLvoid TraceGlFuctions::VertexAttrib2fv(GrGLuint indx,
                                          const GrGLfloat* values) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fVertexAttrib2fv(indx, values);
}

GrGLvoid TraceGlFuctions::VertexAttrib3fv(GrGLuint indx,
                                          const GrGLfloat* values) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fVertexAttrib3fv(indx, values);
}

GrGLvoid TraceGlFuctions::VertexAttrib4fv(GrGLuint indx,
                                          const GrGLfloat* values) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fVertexAttrib4fv(indx, values);
}

GrGLvoid TraceGlFuctions::VertexAttribDivisor(GrGLuint index,
                                              GrGLuint divisor) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fVertexAttribDivisor(index, divisor);
}

GrGLvoid TraceGlFuctions::VertexAttribIPointer(GrGLuint indx, GrGLint size,
                                               GrGLenum type, GrGLsizei stride,
                                               const GrGLvoid* ptr) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fVertexAttribIPointer(indx, size, type, stride, ptr);
}

GrGLvoid TraceGlFuctions::VertexAttribPointer(GrGLuint indx, GrGLint size,
                                              GrGLenum type,
                                              GrGLboolean normalized,
                                              GrGLsizei stride,
                                              const GrGLvoid* ptr) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fVertexAttribPointer(indx, size, type, normalized, stride,
                                       ptr);
}

GrGLvoid TraceGlFuctions::Viewport(GrGLint x, GrGLint y, GrGLsizei width,
                                   GrGLsizei height) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fViewport(x, y, width, height);
}

/* EXT_base_instance */
GrGLvoid TraceGlFuctions::DrawArraysInstancedBaseInstance(
    GrGLenum mode, GrGLint first, GrGLsizei count, GrGLsizei instancecount,
    GrGLuint baseinstance) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fDrawArraysInstancedBaseInstance(mode, first, count,
                                                   instancecount, baseinstance);
}

GrGLvoid TraceGlFuctions::DrawElementsInstancedBaseVertexBaseInstance(
    GrGLenum mode, GrGLsizei count, GrGLenum type, const void* indices,
    GrGLsizei instancecount, GrGLint basevertex, GrGLuint baseinstance) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fDrawElementsInstancedBaseVertexBaseInstance(
      mode, count, type, indices, instancecount, basevertex, baseinstance);
}

/* EXT_multi_draw_indirect */
GrGLvoid TraceGlFuctions::MultiDrawArraysIndirect(GrGLenum mode,
                                                  const GrGLvoid* indirect,
                                                  GrGLsizei drawcount,
                                                  GrGLsizei stride) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fMultiDrawArraysIndirect(mode, indirect, drawcount, stride);
}

GrGLvoid TraceGlFuctions::MultiDrawElementsIndirect(GrGLenum mode,
                                                    GrGLenum type,
                                                    const GrGLvoid* indirect,
                                                    GrGLsizei drawcount,
                                                    GrGLsizei stride) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fMultiDrawElementsIndirect(mode, type, indirect, drawcount,
                                             stride);
}

/* ANGLE_base_vertex_base_instance */
GrGLvoid TraceGlFuctions::MultiDrawArraysInstancedBaseInstance(
    GrGLenum mode, const GrGLint* firsts, const GrGLsizei* counts,
    const GrGLsizei* instanceCounts, const GrGLuint* baseInstances,
    const GrGLsizei drawcount) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fMultiDrawArraysInstancedBaseInstance(
      mode, firsts, counts, instanceCounts, baseInstances, drawcount);
}

GrGLvoid TraceGlFuctions::MultiDrawElementsInstancedBaseVertexBaseInstance(
    GrGLenum mode, const GrGLint* counts, GrGLenum type,
    const GrGLvoid* const* indices, const GrGLsizei* instanceCounts,
    const GrGLint* baseVertices, const GrGLuint* baseInstances,
    const GrGLsizei drawcount) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fMultiDrawElementsInstancedBaseVertexBaseInstance(
      mode, counts, type, indices, instanceCounts, baseVertices, baseInstances,
      drawcount);
}

/* ARB_sync */
GrGLsync TraceGlFuctions::FenceSync(GrGLenum condition, GrGLbitfield flags) {
  TRACE_EVENT("gpu", __FUNCTION__);
  return real_functions_.fFenceSync(condition, flags);
}

GrGLboolean TraceGlFuctions::IsSync(GrGLsync sync) {
  TRACE_EVENT("gpu", __FUNCTION__);
  return real_functions_.fIsSync(sync);
}

GrGLenum TraceGlFuctions::ClientWaitSync(GrGLsync sync, GrGLbitfield flags,
                                         GrGLuint64 timeout) {
  TRACE_EVENT("gpu", __FUNCTION__);
  return real_functions_.fClientWaitSync(sync, flags, timeout);
}

GrGLvoid TraceGlFuctions::WaitSync(GrGLsync sync, GrGLbitfield flags,
                                   GrGLuint64 timeout) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fWaitSync(sync, flags, timeout);
}

GrGLvoid TraceGlFuctions::DeleteSync(GrGLsync sync) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fDeleteSync(sync);
}

/* ARB_internalformat_query */
GrGLvoid TraceGlFuctions::GetInternalformativ(GrGLenum target,
                                              GrGLenum internalformat,
                                              GrGLenum pname, GrGLsizei bufSize,
                                              GrGLint* params) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fGetInternalformativ(target, internalformat, pname, bufSize,
                                       params);
}

/* KHR_debug */
GrGLvoid TraceGlFuctions::DebugMessageControl(GrGLenum source, GrGLenum type,
                                              GrGLenum severity,
                                              GrGLsizei count,
                                              const GrGLuint* ids,
                                              GrGLboolean enabled) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fDebugMessageControl(source, type, severity, count, ids,
                                       enabled);
}

GrGLvoid TraceGlFuctions::DebugMessageInsert(GrGLenum source, GrGLenum type,
                                             GrGLuint id, GrGLenum severity,
                                             GrGLsizei length,
                                             const GrGLchar* buf) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fDebugMessageInsert(source, type, id, severity, length, buf);
}

GrGLvoid TraceGlFuctions::DebugMessageCallback(GRGLDEBUGPROC callback,
                                               const GrGLvoid* userParam) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fDebugMessageCallback(callback, userParam);
}

GrGLuint TraceGlFuctions::GetDebugMessageLog(GrGLuint count, GrGLsizei bufSize,
                                             GrGLenum* sources, GrGLenum* types,
                                             GrGLuint* ids,
                                             GrGLenum* severities,
                                             GrGLsizei* lengths,
                                             GrGLchar* messageLog) {
  TRACE_EVENT("gpu", __FUNCTION__);
  return real_functions_.fGetDebugMessageLog(
      count, bufSize, sources, types, ids, severities, lengths, messageLog);
}

GrGLvoid TraceGlFuctions::PushDebugGroup(GrGLenum source, GrGLuint id,
                                         GrGLsizei length,
                                         const GrGLchar* message) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fPushDebugGroup(source, id, length, message);
}

GrGLvoid TraceGlFuctions::PopDebugGroup() {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fPopDebugGroup();
}

GrGLvoid TraceGlFuctions::ObjectLabel(GrGLenum identifier, GrGLuint name,
                                      GrGLsizei length, const GrGLchar* label) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fObjectLabel(identifier, name, length, label);
}

/** EXT_window_rectangles */
GrGLvoid TraceGlFuctions::WindowRectangles(GrGLenum mode, GrGLsizei count,
                                           const GrGLint box[]) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fWindowRectangles(mode, count, box);
}

/** GL_QCOM_tiled_rendering */
GrGLvoid TraceGlFuctions::StartTiling(GrGLuint x, GrGLuint y, GrGLuint width,
                                      GrGLuint height,
                                      GrGLbitfield preserveMask) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fStartTiling(x, y, width, height, preserveMask);
}

GrGLvoid TraceGlFuctions::EndTiling(GrGLbitfield preserveMask) {
  TRACE_EVENT("gpu", __FUNCTION__);
  real_functions_.fEndTiling(preserveMask);
}

TraceGlFuctions::TextureInfo* TraceGlFuctions::GetTextureInfo(GrGLenum target,
                                                              GrGLint level) {
  auto target_to_texture_it = target_to_texture_.find(target);
  if (target_to_texture_it == target_to_texture_.end()) {
    return {};
  }
  GrGLuint id = target_to_texture_it->second;
  auto& texture_info_levels = texture_infos_[id];
  if (texture_info_levels.size() <= static_cast<size_t>(level)) {
    texture_info_levels.resize(level + 1);
  }
  return &texture_info_levels[level];
}

TraceGlFuctions::TextureInfo* TraceGlFuctions::SetTextureInfo(
    GrGLenum target, GrGLint level, GrGLint internalformat, GrGLsizei width,
    GrGLsizei height) {
  auto* info = GetTextureInfo(target, level);
  if (!info) {
    return nullptr;
  }

  if (info->inited) {
    current_texture_size_ -= info->size;
  }
  info->inited = true;
  info->width = width;
  info->height = height;
  info->format = internalformat;
  info->size = CalculateTexSize(target, internalformat, width, height);
  current_texture_size_ += info->size;

  return info;
}

void TraceGlFuctions::LogTexUsage() {
  TRACE_COUNTER("gpu", "TextureUsage", current_texture_size_);
}

void TraceGlFuctions::LogTexUploadSize() {
  TRACE_COUNTER("gpu", "TextureUploadSize", total_tex_upload_size_);
}

}  // namespace clay
