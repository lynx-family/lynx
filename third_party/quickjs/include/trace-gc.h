// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SRC_GC_TRACE_GC_H_
#define SRC_GC_TRACE_GC_H_
#include <string>
#ifdef __cplusplus
extern "C" {
#endif
#include "quickjs/include/quickjs.h"
#ifdef __cplusplus
}
#endif

struct LEPUSRuntime;

#ifdef USE_PRIMJS_NAPI
#include "primjs_napi_defines.h"
#endif
typedef struct napi_env__ *napi_env;
typedef struct napi_value__ *napi_value;
class NAPIHandleScope;
typedef void napi_func(napi_env env, NAPIHandleScope *scope);

enum HandleType {
  HANDLE_TYPE_UNDEFINED,
  HANDLE_TYPE_HEAP_OBJ,
  HANDLE_TYPE_DIR_HEAP_OBJ,
  HANDLE_TYPE_LEPUS_VALUE,
  HANDLE_TYPE_CSTRING,
  HANDLE_TYPE_LEPUS_TOKEN,
  HANDLE_TYPE_BC_READER_STATE,
  HANDLE_TYPE_VALUE_BUFFER
};

typedef struct {
  void *ptr;
  HandleType type;
} HeapStruct;

class PtrHandles {
 public:
  PtrHandles(LEPUSRuntime *rt);
  ~PtrHandles();
  // handles
  void PushHandle(void *ptr, HandleType type);
  void ResetHandle(void *val, HandleType type);
  void PushLEPUSValueArrayHandle(LEPUSValue *array, int size,
                                 bool need_init = true);
  // special type
  void PushLEPUSAtom(uint32_t atom);
  void PushLEPUSPropertyDescriptor(LEPUSPropertyDescriptor *desc);
  void PushLEPUSValuePtr(LEPUSValue val);
  // tools
  HeapStruct *GetHandles() const { return handles; }
  void SetHeapObjIdx(int idx) { handle_idx = idx; }
  int GetHeapObjIdx() const { return handle_idx; }

 private:
  int handle_idx;
  int handle_size;
  HeapStruct *handles;
  LEPUSRuntime *rt_;
  void InitialHandles();
  void ResizeHandles();
};

class CheckTools {
 public:
  CheckTools();
  ~CheckTools();
  bool PushTid(int tid);
  bool IsValidTid(int tid);

 private:
  int tid_idx;
  int tid_size;
  int *tids;
};

class HandleScope {
 public:
  HandleScope(LEPUSRuntime *rt);
  HandleScope(LEPUSContext *ctx);
  HandleScope(LEPUSContext *ctx, void *ptr, HandleType type);
  ~HandleScope();
  void PushHandle(void *ptr, HandleType type);
  void PushLEPUSAtom(JSAtom atom);
  void PushLEPUSValueArrayHandle(LEPUSValue *array, int size,
                                 bool need_init = true);
  void ResetHandle(void *ptr, HandleType type);
  void PushLEPUSPropertyDescriptor(LEPUSPropertyDescriptor *desc);

 private:
  PtrHandles *ptr_handles;
  int handle_prev_idx;
};

class NAPIHandleScope {
 public:
  NAPIHandleScope(napi_env env, LEPUSContext *ctx, napi_func *func = nullptr);
  explicit NAPIHandleScope(LEPUSContext *ctx)
      : env_(nullptr),
        ctx_(ctx),
        handle_tail_(nullptr),
        reset_napi_env(nullptr) {
    is_gc = ctx_ == nullptr ? false : LEPUS_IsGCMode(ctx_);
    if (is_gc) {
      prev_ = reinterpret_cast<NAPIHandleScope *>(GetNapiScope(ctx_));
      SetNapiScope(ctx_, this);
    }
  }

  inline ~NAPIHandleScope() {
    Handle *curr = handle_tail_;
    while (curr) {
      Handle *temp = curr;
      if (!is_gc) {
        LEPUS_FreeValue(ctx_, curr->value);
      }
      curr = curr->prev;
      delete temp;
    }
    if (is_gc) {
      SetNapiScope(ctx_, prev_);
    } else {
      reset_napi_env(env_, prev_);
    }
  }

  NAPIHandleScope(const NAPIHandleScope &) = delete;
  void operator=(const NAPIHandleScope &) = delete;

  napi_value Escape(napi_value v);

  napi_value CreateHandle(LEPUSValue v);
  struct Handle {
    LEPUSValue value;
    Handle *prev;
  };
  Handle *GetHandle() { return handle_tail_; }
  NAPIHandleScope *GetPrevScope() { return prev_; }

  napi_env env_;
  LEPUSContext *ctx_;
  bool is_gc;
  NAPIHandleScope *prev_;
  Handle *handle_tail_;
  napi_func *reset_napi_env;
};

class GCObserver {
 public:
  virtual ~GCObserver() = default;
  virtual void OnGC(std::string mem_info) = 0;
};
#ifdef USE_PRIMJS_NAPI
#include "primjs_napi_undefs.h"
#endif

void HeapObjStore(LEPUSContext *ctx, void *fieldAddr, LEPUSValue value);
void HeapObjStore(LEPUSContext *ctx, void *fieldAddr, void *value);
void WriteBarrierNoStore(LEPUSRuntime *rt, void *value);
void WriteBarrierNoStore(LEPUSContext *ctx, LEPUSValue value);
void WriteBarrierNoStore(LEPUSContext *ctx, void *value);
void HeapObjStoreNoCtx(void *fieldAddr, LEPUSValue value);
void HeapObjStoreNoCtx(void *fieldAddr, void *value);

#endif  // SRC_GC_TRACE_GC_H_
