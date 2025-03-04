#ifndef TRACE_GC_H
#define TRACE_GC_H
#ifdef __cplusplus
extern "C" {
#endif
#include "quickjs.h"
#ifdef __cplusplus
}
#endif

struct LEPUSRuntime;

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
  explicit NAPIHandleScope(napi_env env, LEPUSContext *ctx,
                           napi_func *func = nullptr);
  explicit NAPIHandleScope(LEPUSContext *ctx)
      : env_(nullptr),
        ctx_(ctx),
        handle_tail_(nullptr),
        reset_napi_env(nullptr) {
    is_gc = ctx_ == nullptr ? false : LEPUS_IsGCMode(ctx_);
    if (is_gc) {
      prev_ = (NAPIHandleScope *)GetNapiScope(ctx_);
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


#endif /* TRACE_GC_H */
