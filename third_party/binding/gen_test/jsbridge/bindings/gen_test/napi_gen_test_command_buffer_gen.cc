// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// This file has been auto-generated from the Jinja2 template
// third_party/binding/idl-codegen/templates/napi_command_buffer.cc.tmpl
// by the script code_generator_napi.py.
// DO NOT MODIFY!

// clang-format off
#include "jsbridge/bindings/gen_test/napi_gen_test_command_buffer.h"

#include "base/include/auto_reset.h"
#include "jsbridge/bindings/gen_test/napi_async_object.h"
#include "jsbridge/bindings/gen_test/napi_test_context.h"
#include "third_party/binding/gen_test/test_async_object.h"
#include "third_party/binding/gen_test/test_context.h"

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace gen_test {

// TODO(yuyifei): Return value wrapper and remove |env| parameter.
Napi::Value NapiGenTestCommandBuffer::RunBuffer(uint32_t* buffer, uint32_t length, Napi::Env env) {
  Napi::Value rv;
  uint32_t current = 1u;
  while (current < length) {
    struct __attribute__ ((__packed__)) MethodHeader {
      uint32_t method;
      uint32_t object_id;
    };
    MethodHeader& header = ReadBuffer<MethodHeader>(buffer, current);
    bool type_safe = false;
    union ObjectPointer {
      TestContext* test_context;
      // Currently assumes single inheritance only.
      ImplBase* orphan;
    } pointer;
    if (NapiTestContext* bridge = FromId<NapiTestContext>(header.object_id)) {
      pointer.test_context = bridge->ToImplUnsafe();
      type_safe = true;
    }
    if (auto it = orphans_.find(header.object_id); it != orphans_.end()) {
      DCHECK(!type_safe);
      pointer.orphan = it->second.get();
      type_safe = true;
    }
    if (!type_safe) {
      LOGE("Method " << header.method << " called with invalid object: " << header.object_id);
    }
    current += 2;
    switch (header.method) {
      case 1: { // TestContext.voidFromVoid()
        struct __attribute__ ((__packed__)) VoidFromVoidCommand {
        };
        if (type_safe) {
          pointer.test_context->VoidFromVoid();
        }
        break;
      }
      case 2: { // TestContext.voidFromString()
        struct __attribute__ ((__packed__)) VoidFromStringCommand {
          uint32_t sStringLengthGen;
        };
        VoidFromStringCommand& command = ReadBuffer<VoidFromStringCommand>(buffer, current);
        current += 1;
        std::string s = binding::Utf16LeToUtf8((char16_t*)(buffer + current), command.sStringLengthGen);
        current += (command.sStringLengthGen + 1) / 2;
        if (type_safe) {
          pointer.test_context->VoidFromString(std::move(s));
        }
        break;
      }
      case 3: { // TestContext.voidFromStringArray()
        struct __attribute__ ((__packed__)) VoidFromStringArrayCommand {
          uint32_t saArrayLengthGen;
        };
        VoidFromStringArrayCommand& command = ReadBuffer<VoidFromStringArrayCommand>(buffer, current);
        current += 1;
        std::vector<std::string> sa;
        for (uint32_t i = 0; i < command.saArrayLengthGen; ++i) {
          uint32_t str_len = ReadBuffer<uint32_t>(buffer, current);
          sa.push_back(binding::Utf16LeToUtf8((char16_t*)(buffer + current + 1), str_len));
          current += (str_len + 3) / 2;
        }
        if (type_safe) {
          pointer.test_context->VoidFromStringArray(std::move(sa));
        }
        break;
      }
      case 4: { // TestContext.voidFromTypedArray()
        struct __attribute__ ((__packed__)) VoidFromTypedArrayCommand {
          uint32_t faArrayLengthGen;
        };
        VoidFromTypedArrayCommand& command = ReadBuffer<VoidFromTypedArrayCommand>(buffer, current);
        current += 1;
        SharedVector<float> fa((float*)buffer + current, command.faArrayLengthGen);
        current += command.faArrayLengthGen;
        if (type_safe) {
          pointer.test_context->VoidFromTypedArray(fa);
        }
        break;
      }
      case 5: { // TestContext.voidFromArrayBuffer()
        struct __attribute__ ((__packed__)) VoidFromArrayBufferCommand {
          uint32_t abByteLengthGen;
        };
        VoidFromArrayBufferCommand& command = ReadBuffer<VoidFromArrayBufferCommand>(buffer, current);
        current += 1;
        if (type_safe) {
          pointer.test_context->VoidFromArrayBuffer(buffer + current, command.abByteLengthGen);
        }
        current += (command.abByteLengthGen + 3) / 4;
        break;
      }
      case 6: { // TestContext.voidFromArrayBufferView()
        struct __attribute__ ((__packed__)) VoidFromArrayBufferViewCommand {
          uint32_t abvViewTypeGen;
          uint32_t abvByteLengthGen;
        };
        VoidFromArrayBufferViewCommand& command = ReadBuffer<VoidFromArrayBufferViewCommand>(buffer, current);
        current += 2;
        ArrayBufferView abv(buffer + current, (ArrayBufferView::ViewType)command.abvViewTypeGen, command.abvByteLengthGen);
        if (type_safe) {
          pointer.test_context->VoidFromArrayBufferView(abv);
        }
        current += (command.abvByteLengthGen + 3) / 4;
        break;
      }
      case 7: { // TestContext.voidFromNullableArrayBufferView()
        struct __attribute__ ((__packed__)) VoidFromNullableArrayBufferViewCommand {
          uint32_t abvViewTypeGen;
          uint32_t abvByteLengthGen;
        };
        VoidFromNullableArrayBufferViewCommand& command = ReadBuffer<VoidFromNullableArrayBufferViewCommand>(buffer, current);
        current += 2;
        ArrayBufferView abv(buffer + current, (ArrayBufferView::ViewType)command.abvViewTypeGen, command.abvByteLengthGen);
        if (type_safe) {
          pointer.test_context->VoidFromNullableArrayBufferView(abv);
        }
        current += (command.abvByteLengthGen + 3) / 4;
        break;
      }
      case 8: { // TestContext.createAsyncObject()
        struct __attribute__ ((__packed__)) CreateAsyncObjectCommand {
          uint32_t id;
        };
        CreateAsyncObjectCommand& command = ReadBuffer<CreateAsyncObjectCommand>(buffer, current);
        current += 1;
        if (type_safe) {
          auto&& result = pointer.test_context->CreateAsyncObject();
          FromAsyncId(command.id)->Init(std::unique_ptr<ImplBase>(result));
        }
        break;
      }
      case 9: { // TestContext.asyncForAsyncObject()
        struct __attribute__ ((__packed__)) AsyncForAsyncObjectCommand {
          uint32_t taoId;
        };
        AsyncForAsyncObjectCommand& command = ReadBuffer<AsyncForAsyncObjectCommand>(buffer, current);
        current += 1;
        TestAsyncObject* tao = nullptr;
        if (auto* async = FromAsyncId(command.taoId)) {
          tao = async->ToImplUnsafe<TestAsyncObject>();
        }
        if (!tao) {
          LogTypeError("AsyncForAsyncObject", "tao", command.taoId);
          type_safe = false;
        }
        if (type_safe) {
          pointer.test_context->AsyncForAsyncObject(tao);
        }
        break;
      }
      case 10: { // TestContext.asyncForNullableAsyncObject()
        struct __attribute__ ((__packed__)) AsyncForNullableAsyncObjectCommand {
          uint32_t taoId;
        };
        AsyncForNullableAsyncObjectCommand& command = ReadBuffer<AsyncForNullableAsyncObjectCommand>(buffer, current);
        current += 1;
        TestAsyncObject* tao = nullptr;
        if (auto* async = FromAsyncId(command.taoId)) {
          tao = async->ToImplUnsafe<TestAsyncObject>();
        }
        if (type_safe) {
          pointer.test_context->AsyncForNullableAsyncObject(tao);
        }
        break;
      }
      case 11: { // TestContext.syncForAsyncObject()
        struct __attribute__ ((__packed__)) SyncForAsyncObjectCommand {
          uint32_t taoId;
        };
        SyncForAsyncObjectCommand& command = ReadBuffer<SyncForAsyncObjectCommand>(buffer, current);
        current += 1;
        TestAsyncObject* tao = nullptr;
        if (auto* async = FromAsyncId(command.taoId)) {
          tao = async->ToImplUnsafe<TestAsyncObject>();
        }
        if (!tao) {
          LogTypeError("SyncForAsyncObject", "tao", command.taoId);
          type_safe = false;
        }
        if (type_safe) {
          auto&& result = pointer.test_context->SyncForAsyncObject(tao);
          rv = Napi::String::New(env, result);
          // Sync calls should be at the end of buffer.
          DCHECK(current == length);
          if (current != length) {
            Napi::Error::New(env, "Command out of order. Are you using native Promise instead of Lynx mock on iOS < 15?").ThrowAsJavaScriptException();
            return Napi::Value();
          }
        }
        break;
      }
      default:{
        LOGE("=================== gen_test Error Report ===================");
        LOGE("Unexpected gen_test command! Current len: " << length << ", pos: " << current << ", method: " << header.method << ", object_id: " << header.object_id << ", " << sizeof(int*));
        std::stringstream start_content;
        for (uint32_t i = 0; i < std::min(128u, current); ++i) {
          start_content << "[" << i << "]" << ReadBuffer<uint32_t>(buffer, i);
        }
        LOGE("Content at gen_test buffer start: " << start_content.str());
        std::stringstream current_content;
        for (uint32_t i = std::max(0, (int)current - 64); i < std::min(length, current + 64); ++i) {
          current_content << "[" << i << "]" << ReadBuffer<uint32_t>(buffer, i);
        }
        LOGE("Content around gen_test current: " << current_content.str());
        LOGE("==========================================================");
        NOTREACHED();
        break;
      }
    }
  }
  // We should reach the end of buffer after a flush.
  DCHECK(current == length);
  return rv;
}

// static
template <typename T>
T* NapiGenTestCommandBuffer::FromId(uint32_t id) {
  if (auto it = ObjectRegistry().find(id); it != ObjectRegistry().end()) {
    if (T::IsInstance(it->second)) {
      return static_cast<T*>(it->second);
    }
  }
  return nullptr;
}

}  // namespace gen_test
}  // namespace lynx
