// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/napi/napi_environment.h"

#include "third_party/binding/napi/shim/shim_napi.h"

namespace lynx {
namespace piper {

namespace {
const uint64_t kEnvClassID = reinterpret_cast<uint64_t>(&kEnvClassID);
}

// static
NapiEnvironment* NapiEnvironment::From(Napi::Env env) {
  return env.GetInstanceData<NapiEnvironment>(kEnvClassID);
}

NapiEnvironment::NapiEnvironment(std::unique_ptr<Delegate> delegate)
    : delegate_(std::move(delegate)) {
  DCHECK(delegate_);
}

NapiEnvironment::~NapiEnvironment() { Detach(); }

void NapiEnvironment::Attach() {
  if (attached_) return;
  attached_ = true;

  DCHECK(proxy_);
  proxy_->Attach();
  proxy_->SetupLoader();
  proxy_->SetUncaughtExceptionHandler();

  Napi::Env env = proxy_->Env();
  env.SetInstanceData(kEnvClassID, this, nullptr, nullptr);
  delegate_->OnAttach(env);
}

void NapiEnvironment::Detach() {
  if (!attached_) return;
  attached_ = false;

  DCHECK(proxy_);
  Napi::Env env = proxy_->Env();
  delegate_->OnDetach(env);
  proxy_->RemoveLoader();

  proxy_->Detach();
}

void NapiEnvironment::RegisterModule(const std::string& name,
                                     std::unique_ptr<Module> module) {
  delegate_->RegisterModule(name, std::move(module));
}

NapiEnvironment::Module* NapiEnvironment::GetModule(const std::string& name) {
  return delegate_->GetModule(name);
}

}  // namespace piper
}  // namespace lynx
