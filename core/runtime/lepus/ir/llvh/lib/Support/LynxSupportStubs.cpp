// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// NOTE:
// This repository vendors a minimal llvh subset for Lepus IR.
// Some header-only algorithms (e.g. dominator tree utilities) reference
// llvh::raw_ostream and llvh::Debug symbols, but we don't build the full LLVM
// Support library. Provide a minimal set of definitions to satisfy linking.

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "core/runtime/lepus/ir/llvh/include/llvh/Support/Debug.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/Support/raw_ostream.h"

namespace llvh {

#undef isCurrentDebugType
#undef setCurrentDebugType
#undef setLLVHCurrentDebugTypes

// ---- Debug.h symbols ----

bool DebugFlag = false;
bool VerifyDomInfo = false;
bool VerifyLoopInfo = false;
bool VerifyMemorySSA = false;
bool EnableDebugBuffering = false;

static const char* gDebugType = nullptr;

bool isCurrentDebugType(const char* Type) {
  if (!Type || !gDebugType) {
    return true;
  }
  return std::strcmp(Type, gDebugType) == 0;
}

void setCurrentDebugType(const char* Type) { gDebugType = Type; }

void setLLVHCurrentDebugTypes(const char** /*Types*/, unsigned /*Count*/) {
  // Minimal behavior: accept any type.
  gDebugType = nullptr;
}

raw_ostream& dbgs() {
  // Route debug output to a null sink.
  return nulls();
}

// ---- raw_ostream core ----

raw_ostream::~raw_ostream() {
  flush();
  if (BufferMode == InternalBuffer && OutBufStart) {
    delete[] OutBufStart;
  }
  OutBufStart = OutBufEnd = OutBufCur = nullptr;
}

void raw_ostream::anchor() {}
void raw_pwrite_stream::anchor() {}

size_t raw_ostream::preferred_buffer_size() const { return 4096; }

void raw_ostream::SetBufferAndMode(char* BufferStart, size_t Size,
                                   BufferKind Mode) {
  if (BufferMode == InternalBuffer && OutBufStart) {
    delete[] OutBufStart;
  }
  BufferMode = Mode;
  OutBufStart = BufferStart;
  OutBufCur = BufferStart;
  OutBufEnd = BufferStart ? BufferStart + Size : nullptr;
}

void raw_ostream::SetBuffered() {
  if (BufferMode == Unbuffered) {
    BufferMode = InternalBuffer;
  }
  if (!OutBufStart && BufferMode == InternalBuffer) {
    const size_t Size = preferred_buffer_size();
    SetBufferAndMode(new char[Size], Size, InternalBuffer);
  }
}

void raw_ostream::copy_to_buffer(const char* Ptr, size_t Size) {
  std::memcpy(OutBufCur, Ptr, Size);
  OutBufCur += Size;
}

void raw_ostream::flush_nonempty() {
  const size_t Size = static_cast<size_t>(OutBufCur - OutBufStart);
  if (Size) {
    write_impl(OutBufStart, Size);
    OutBufCur = OutBufStart;
  }
}

raw_ostream& raw_ostream::write(unsigned char C) {
  const char Ch = static_cast<char>(C);
  return write(&Ch, 1);
}

raw_ostream& raw_ostream::write(const char* Ptr, size_t Size) {
  if (Size == 0) {
    return *this;
  }

  if (BufferMode != Unbuffered && OutBufStart == nullptr) {
    const size_t BufSize = preferred_buffer_size();
    SetBufferAndMode(new char[BufSize], BufSize, InternalBuffer);
  }

  if (BufferMode == Unbuffered || OutBufStart == nullptr) {
    write_impl(Ptr, Size);
    return *this;
  }

  const size_t Available = static_cast<size_t>(OutBufEnd - OutBufCur);
  if (Size <= Available) {
    copy_to_buffer(Ptr, Size);
    return *this;
  }

  flush();

  if (Size > static_cast<size_t>(OutBufEnd - OutBufStart)) {
    write_impl(Ptr, Size);
    return *this;
  }

  copy_to_buffer(Ptr, Size);
  return *this;
}

raw_ostream& raw_ostream::indent(unsigned NumSpaces) {
  static constexpr char kSpaces[] =
      "                                                                ";
  while (NumSpaces) {
    const unsigned Chunk = std::min<unsigned>(
        NumSpaces, static_cast<unsigned>(sizeof(kSpaces) - 1));
    write(kSpaces, Chunk);
    NumSpaces -= Chunk;
  }
  return *this;
}

raw_ostream& raw_ostream::operator<<(unsigned long N) {
  char Buf[32];
  const int Len = std::snprintf(Buf, sizeof(Buf), "%lu", N);
  if (Len > 0) {
    write(Buf, static_cast<size_t>(Len));
  }
  return *this;
}

raw_ostream& raw_ostream::operator<<(long N) {
  char Buf[32];
  const int Len = std::snprintf(Buf, sizeof(Buf), "%ld", N);
  if (Len > 0) {
    write(Buf, static_cast<size_t>(Len));
  }
  return *this;
}

raw_ostream& raw_ostream::operator<<(unsigned long long N) {
  char Buf[32];
  const int Len = std::snprintf(Buf, sizeof(Buf), "%llu", N);
  if (Len > 0) {
    write(Buf, static_cast<size_t>(Len));
  }
  return *this;
}

raw_ostream& raw_ostream::operator<<(long long N) {
  char Buf[32];
  const int Len = std::snprintf(Buf, sizeof(Buf), "%lld", N);
  if (Len > 0) {
    write(Buf, static_cast<size_t>(Len));
  }
  return *this;
}

raw_ostream& raw_ostream::operator<<(const void* P) {
  char Buf[32];
  const int Len = std::snprintf(Buf, sizeof(Buf), "%p", P);
  if (Len > 0) {
    write(Buf, static_cast<size_t>(Len));
  }
  return *this;
}

raw_ostream& raw_ostream::operator<<(double N) {
  char Buf[64];
  const int Len = std::snprintf(Buf, sizeof(Buf), "%g", N);
  if (Len > 0) {
    write(Buf, static_cast<size_t>(Len));
  }
  return *this;
}

// ---- Stream sinks/adaptors ----

void raw_string_ostream::write_impl(const char* Ptr, size_t Size) {
  OS.append(Ptr, Size);
}

raw_string_ostream::~raw_string_ostream() { flush(); }

void raw_svector_ostream::write_impl(const char* Ptr, size_t Size) {
  OS.append(Ptr, Ptr + Size);
}

void raw_svector_ostream::pwrite_impl(const char* Ptr, size_t Size,
                                      uint64_t Offset) {
  if (Offset + Size > OS.size()) {
    return;
  }
  std::memcpy(OS.data() + Offset, Ptr, Size);
}

uint64_t raw_svector_ostream::current_pos() const { return OS.size(); }

void raw_null_ostream::write_impl(const char* /*Ptr*/, size_t /*Size*/) {}

void raw_null_ostream::pwrite_impl(const char* /*Ptr*/, size_t /*Size*/,
                                   uint64_t /*Offset*/) {}

uint64_t raw_null_ostream::current_pos() const { return 0; }

raw_null_ostream::~raw_null_ostream() { flush(); }

raw_ostream& nulls() {
  static raw_null_ostream S;
  return S;
}

raw_ostream& outs() { return nulls(); }
raw_ostream& errs() { return nulls(); }

}  // namespace llvh
