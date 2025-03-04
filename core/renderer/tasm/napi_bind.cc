// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#if defined(ENABLE_NAPI)
#include "core/template_bundle/template_codec/binary_encoder/encoder.h"
#include "napi.h"
#include "third_party/aes/aes.h"

class TASMAddon : public Napi::Addon<TASMAddon> {
 public:
  TASMAddon(Napi::Env env, Napi::Object exports) {
    // In the constructor we declare the functions the add-on makes available
    // to JavaScript.
    DefineAddon(
        exports,
        {
            InstanceMethod("encode", &TASMAddon::EncodeNapi, napi_enumerable),
            InstanceMethod("qjsCheck", &TASMAddon::quickjsCheckNapi,
                           napi_enumerable),
            InstanceMethod("encode_ssr", &TASMAddon::EncodeSSRNapi,
                           napi_enumerable),
            InstanceMethod("encrypt", &TASMAddon::EncryptNapi, napi_enumerable),
            InstanceMethod("decrypt", &TASMAddon::DecryptNapi, napi_enumerable),
        });
  }

 private:
  Napi::Value DecryptNapi(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    std::string options = info[0].As<Napi::String>();
    std::string res = AES().DecryptECB(options);
    return Napi::String::New(env, res);
  }

  Napi::Value EncodeNapi(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    std::string options = info[0].As<Napi::String>();
    auto res = lynx::tasm::encode(options.c_str());
    Napi::Object obj = Napi::Object::New(env);
    Napi::Buffer<uint8_t> buffer =
        Napi::Buffer<uint8_t>::New(env, res.buffer.size());
    auto bufferData = buffer.Data();
    for (size_t i = 0; i < res.buffer.size(); i++) {
      bufferData[i] = static_cast<uint8_t>(res.buffer[i]);
    }
    obj.Set("status", res.status);
    obj.Set("buffer", buffer);
    obj.Set("lepus_code", res.lepus_code);
    obj.Set("lepus_debug", res.lepus_debug);
    obj.Set("error_msg", res.error_msg);
    obj.Set("section_size", res.section_size);
    return obj;
  }

  Napi::Value quickjsCheckNapi(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    std::string options = info[0].As<Napi::String>();
    auto res = lynx::tasm::quickjsCheck(options.c_str());
    return Napi::String::New(env, res);
  }

  Napi::Value EncodeSSRNapi(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    napi_status status;
    uint8_t* buffer_ptr;
    size_t buffer_length;
    Napi::Object obj = Napi::Object::New(env);
    status = napi_get_buffer_info(
        env, info[0], reinterpret_cast<void**>(&buffer_ptr), &buffer_length);
    if (status != napi_ok) {
      obj.Set("status",
              static_cast<int32_t>(lynx::tasm::EncodeSSRError::ERR_BUF));
      obj.Set("error_msg", "Not Valid Buffer");
      return obj;
    }
    std::string data = info[1].As<Napi::String>();
    auto res = lynx::tasm::encode_ssr(buffer_ptr, buffer_length, data);
    Napi::Buffer<uint8_t> buffer =
        Napi::Buffer<uint8_t>::New(env, res.buffer.size());
    auto bufferData = buffer.Data();
    for (size_t i = 0; i < res.buffer.size(); i++) {
      bufferData[i] = static_cast<uint8_t>(res.buffer[i]);
    }
    obj.Set("status", res.status);
    obj.Set("buffer", buffer);
    obj.Set("error_msg", res.error_msg);
    return obj;
  }

  Napi::Value EncryptNapi(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    std::string options = info[0].As<Napi::String>();
    std::string res = AES().EncryptECB(options);
    return Napi::String::New(env, res);
  }
};

// The macro announces that instances of the class `TASMAddon` will be
// created for each instance of the add-on that must be loaded into Node.js.
NAPI_MODULE_INIT() {
  return Napi::RegisterModule(env, exports, &TASMAddon::Init);
}

#endif
