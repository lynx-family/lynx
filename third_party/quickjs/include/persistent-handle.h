// Copyright 2009 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef THIRD_PARTY_QUICKJS_INCLUDE_PERSISTENT_HANDLE_H_
#define THIRD_PARTY_QUICKJS_INCLUDE_PERSISTENT_HANDLE_H_
#include "global-handles.h"
#include "quickjs.h"

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
    if (val_) {
      *val_ = that.Get();
    } else {
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

  virtual inline ~WASMGCPersistent() {
    if (rt) PersistentBase::Reset(rt);
  }
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
  *reinterpret_cast<int*>(0xdead) = 0;
  return false;
}

void PersistentBase::Reset(LEPUSRuntime* runtime) {
  if (this->IsEmpty()) return;
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
    FreeQJSValueValue(runtime, reinterpret_cast<LEPUSValue*>(instance));
  }
};

#endif  // THIRD_PARTY_QUICKJS_INCLUDE_PERSISTENT_HANDLE_H_
