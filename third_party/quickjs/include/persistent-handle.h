
#ifndef INCLUDE_PERSISTENT_HANDLE_H_
#define INCLUDE_PERSISTENT_HANDLE_H_
extern "C" {
#include "quickjs.h"
}
#include "global-handles.h"
#ifdef ENABLE_GC_DEBUG_TOOLS
#define DCHECK2(condition) \
  if (!(condition)) abort();
#if defined(ANDROID) || defined(__ANDROID__)
#include <android/log.h>
#endif
#else
#define DCHECK2(condition) ((void)0)
#endif

const int kApiSystemPointerSize = sizeof(void*);
static const int kNodeClassIdOffset = 1 * kApiSystemPointerSize;

class PersistentBase {
 public:
  /**
   * If non-empty, destroy the underlying storage cell
   * IsEmpty() will return true after this call.
   */
  inline void Reset(LEPUSRuntime* runtime);
  inline void Reset(LEPUSContext* ctx);

  /**
   * If non-empty, destroy the underlying storage cell
   * and create a new one with the contents of other if other is non empty
   */
  inline void Reset(LEPUSRuntime* runtime, LEPUSValue other,
                    bool is_weak = false);

  inline void Reset(LEPUSContext* ctx, LEPUSValue other, bool is_weak = false);

  inline bool IsEmpty() const { return val_ == nullptr; }
  inline void Empty() { val_ = nullptr; }

  inline LEPUSValue Get() const {
    if (val_) return *val_;
    return LEPUS_UNDEFINED;
  }

  inline bool IsWeak() const;

  PersistentBase(const PersistentBase& other) = delete;
  void operator=(const PersistentBase&) = delete;

  explicit inline PersistentBase(LEPUSValue* val) : val_(val) {}
  inline static LEPUSValue* New(LEPUSRuntime* runtime, LEPUSValue that,
                                bool is_weak);

  LEPUSValue* val_;
};

class GCPersistent : public PersistentBase {
 public:
  /**
   * A GCPersistent with no storage cell.
   */
  inline GCPersistent() : PersistentBase(nullptr) {}
  /**
   * Construct a GCPersistent from a Local.
   * When the Local is non-empty, a new storage cell is created
   * pointing to the same object, and no flags are set.
   */
  inline GCPersistent(LEPUSRuntime* runtime, LEPUSValue that,
                      bool is_weak = false)
      : PersistentBase(PersistentBase::New(runtime, that, is_weak)) {}

  inline GCPersistent(LEPUSContext* ctx, LEPUSValue that, bool is_weak = false)
      : PersistentBase(
            PersistentBase::New(LEPUS_GetRuntime(ctx), that, is_weak)) {}

  /**
   * The copy constructors and assignment operator create a GCPersistent
   * exactly as the GCPersistent constructor, but the Copy function from the
   * traits class is called, allowing the setting of flags based on the
   * copied GCPersistent.
   */
  GCPersistent(const GCPersistent& that) = delete;

  GCPersistent& operator=(const GCPersistent& that) = delete;

  /**
   * The destructor will dispose the GCPersistent based on the
   * kResetInDestructor flags in the traits class.  Since not calling dispose
   * can result in a memory leak, it is recommended to always set this flag.
   */
  inline ~GCPersistent() {
    // if (false) this->Reset();
  }
  void SetWeak(LEPUSRuntime* runtime) { SetWeakState(runtime, this->val_); }

 private:
  friend class Utils;
  friend class GCPersistent;
  inline LEPUSValue* operator*() const { return this->val_; }
};

class WASMGCPersistent : public PersistentBase {
 public:
  inline WASMGCPersistent()
      : PersistentBase(nullptr), rt(nullptr), val(LEPUS_UNDEFINED) {}
  inline WASMGCPersistent(LEPUSValue that)
      : PersistentBase(nullptr), rt(nullptr), val(that) {}

  inline WASMGCPersistent(LEPUSRuntime* runtime, LEPUSValue that,
                          bool is_weak = false)
      : PersistentBase(PersistentBase::New(runtime, that, is_weak)),
        rt(runtime),
        val(that) {}

  inline WASMGCPersistent(LEPUSContext* ctx, LEPUSValue that,
                          bool is_weak = false)
      : PersistentBase(
            PersistentBase::New(LEPUS_GetRuntime(ctx), that, is_weak)),
        rt(LEPUS_GetRuntime(ctx)),
        val(that) {}

  WASMGCPersistent(const WASMGCPersistent& that)
      : PersistentBase(PersistentBase::New(that.GetRT(), that.Get(), false)) {
    rt = that.GetRT();
    val = that.Get();
  }

  WASMGCPersistent& operator=(const WASMGCPersistent& that) {
    if (val_)
      *val_ = that.Get();
    else {
      val_ = PersistentBase::New(that.GetRT(), that.Get(), false);
    }
    rt = that.GetRT();
    val = that.Get();
    return *this;
  }
  void Reset(LEPUSContext* ctx, LEPUSValue value) {
    PersistentBase::Reset(LEPUS_GetRuntime(ctx), value, false);
    rt = LEPUS_GetRuntime(ctx);
    val = value;
  }

  LEPUSValue Get() const {
    if (val_) {
      return *val_;
    } else {
      return val;
    }
  }

  LEPUSValue* GetPtr() const {
    if (val_) {
      return val_;
    } else {
      return const_cast<LEPUSValue*>(&val);
    }
  }

  virtual inline ~WASMGCPersistent() { PersistentBase::Reset(rt); }
  LEPUSRuntime* GetRT() const { return rt; }

 private:
  inline LEPUSValue* operator*() const { return this->val_; }
  LEPUSRuntime* rt;
  LEPUSValue val;
};

LEPUSValue* PersistentBase::New(LEPUSRuntime* runtime, LEPUSValue that,
                                bool is_weak) {
  if (!runtime || LEPUS_IsUndefined(that)) return nullptr;
  return GlobalizeReference(runtime, that, is_weak);
}

bool PersistentBase::IsWeak() const {
  *(int*)(0xdead) = 0;
  return false;
}

void PersistentBase::Reset(LEPUSRuntime* runtime) {
  if (this->IsEmpty() || !runtime) return;
  DisposeGlobal(runtime, this->val_);
  val_ = nullptr;
}

void PersistentBase::Reset(LEPUSContext* ctx) { Reset(LEPUS_GetRuntime(ctx)); }

/**
 * If non-empty, destroy the underlying storage cell
 * and create a new one with the contents of other if other is non empty
 */

void PersistentBase::Reset(LEPUSRuntime* runtime, LEPUSValue other,
                           bool is_weak) {
  Reset(runtime);
  if (LEPUS_IsUndefined(other)) return;
  this->val_ = New(runtime, other, is_weak);
}

void PersistentBase::Reset(LEPUSContext* ctx, LEPUSValue other, bool is_weak) {
  Reset(LEPUS_GetRuntime(ctx), other, is_weak);
}

class QJSValueValueAllocator {
 public:
  static void* New(LEPUSRuntime* runtime) {
    return AllocateQJSValueValue(runtime);
  }
  static void Delete(LEPUSRuntime* runtime, void* instance) {
    FreeQJSValueValue(runtime, (LEPUSValue*)instance);
  }
};

#endif  // INCLUDE_PERSISTENT_HANDLE_H_
