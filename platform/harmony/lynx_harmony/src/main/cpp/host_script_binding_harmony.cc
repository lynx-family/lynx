// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/host_script_binding_harmony.h"

#include <atomic>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/platform/harmony/napi_util.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/shell/host_script/runtime/host_script_session.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_runtime_wrapper.h"

namespace lynx {
namespace harmony {
namespace {
using shell::HostScriptSession;

napi_value String(napi_env env, const std::string& text) {
  napi_value result;
  napi_create_string_utf8(env, text.data(), text.size(), &result);
  return result;
}

// N-API references below belong exclusively to the platform UI environment.
// Only copied C++ values and the atomic admission flags cross threads.
struct PlatformState {
  napi_env env;
  napi_ref target = nullptr;
  std::atomic<bool> active{true};
  std::atomic<bool> view_active{false};
  fml::RefPtr<fml::TaskRunner> runner;
  std::weak_ptr<HostScriptSession> session;

  void Call(const char* method, std::vector<std::string> strings,
            std::vector<uint8_t> bytes = {}) {
    if (!active || !target) return;
    base::NapiHandleScope scope(env);
    napi_value receiver = nullptr;
    napi_get_reference_value(env, target, &receiver);
    if (!receiver) return;
    napi_value function;
    if (napi_get_named_property(env, receiver, method, &function) != napi_ok)
      return;
    napi_value args[2];
    napi_create_array_with_length(env, strings.size(), &args[0]);
    for (size_t i = 0; i < strings.size(); ++i) {
      napi_set_element(env, args[0], i, String(env, strings[i]));
    }
    void* buffer = nullptr;
    napi_create_arraybuffer(env, bytes.size(), &buffer, &args[1]);
    if (!bytes.empty()) std::memcpy(buffer, bytes.data(), bytes.size());
    napi_value result;
    napi_call_function(env, receiver, function, 2, args, &result);
    bool pending = false;
    napi_is_exception_pending(env, &pending);
    if (pending) {
      napi_get_and_clear_last_exception(env, &result);
      if (auto owner = session.lock()) {
        owner->Notify("error", -1, "Host Script platform operation failed");
      }
    }
  }

  void Dispose() {
    active = false;
    view_active = false;
    if (target) {
      napi_delete_reference(env, target);
      target = nullptr;
    }
  }
};

class ViewProxy final : public shell::LynxViewRefProxy {
 public:
  explicit ViewProxy(std::shared_ptr<PlatformState> state)
      : state_(std::move(state)) {}
  bool LoadTemplate(shell::LynxViewRefLoadTemplateRequest request) override {
    return Post({request.has_template ? "loadTemplate" : "loadURL", request.url,
                 request.initial_data_json, request.global_props_json,
                 request.processor_name, request.read_only ? "true" : "false"},
                std::move(request.template_data));
  }
  bool LoadSSR(shell::LynxViewRefSsrRequest) override { return false; }
  bool HydrateSSR(shell::LynxViewRefSsrRequest) override { return false; }
  bool UpdateMetaData(
      shell::LynxViewRefUpdateMetaDataRequest request) override {
    return Post(
        {"updateMetaData", request.data_json, request.global_props_json});
  }
  bool ReloadTemplate(
      shell::LynxViewRefReloadTemplateRequest request) override {
    return Post(
        {"reloadTemplate", request.data_json, request.global_props_json});
  }
  bool SetGlobalProps(std::string json) override {
    return Post({"setGlobalProps", std::move(json)});
  }
  bool SendGlobalEvent(shell::LynxViewRefGlobalEventRequest request) override {
    return Post({"sendGlobalEvent", request.name, request.params_json});
  }
  void Invalidate() override { state_->view_active = false; }

 private:
  bool Post(std::vector<std::string> args, std::vector<uint8_t> bytes = {}) {
    if (!state_->active || !state_->view_active) return false;
    state_->runner->PostTask([state = state_, args = std::move(args),
                              bytes = std::move(bytes)]() mutable {
      if (state->active && state->view_active) {
        state->Call("dispatch", std::move(args), std::move(bytes));
      }
    });
    return true;
  }
  std::shared_ptr<PlatformState> state_;
};

class SessionListener final : public runtime::RuntimeLifecycleListenerDelegate {
 public:
  SessionListener(std::shared_ptr<HostScriptSession> session,
                  std::shared_ptr<PlatformState> state)
      : RuntimeLifecycleListenerDelegate(DelegateType::PART),
        session_(std::move(session)),
        state_(std::move(state)) {}
  void OnRuntimeCreate(std::shared_ptr<runtime::IVSyncObserver>) override {}
  void OnRuntimeInit(int64_t) override {}
  void OnAppEnterForeground() override {}
  void OnAppEnterBackground() override {}
  void OnRuntimeAttach(void* env, const char*) override {
    if (state_->active &&
        !session_->Attach(reinterpret_cast<napi_env_primjs>(env))) {
      auto state = state_;
      state->runner->PostTask([state] {
        state->Call("entryResult",
                    {"ERROR", "Cannot attach Host Script bindings"});
      });
    }
  }
  void OnRuntimeDetach() override { session_->Detach(); }

 private:
  std::shared_ptr<HostScriptSession> session_;
  std::shared_ptr<PlatformState> state_;
};

struct Binding {
  std::shared_ptr<PlatformState> state;
  std::shared_ptr<HostScriptSession> session;
  void Dispose() {
    session->InvalidateView();
    state->Dispose();
  }
  static Binding* Get(napi_env env, napi_callback_info info,
                      size_t* argc = nullptr, napi_value* args = nullptr) {
    napi_value self;
    napi_get_cb_info(env, info, argc, args, &self, nullptr);
    Binding* binding = nullptr;
    napi_unwrap(env, self, reinterpret_cast<void**>(&binding));
    return binding;
  }
  static napi_value New(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2], self;
    napi_get_cb_info(env, info, &argc, args, &self, nullptr);
    LynxRuntimeWrapper* runtime = nullptr;
    if (argc != 2 ||
        napi_unwrap(env, args[0], reinterpret_cast<void**>(&runtime)) !=
            napi_ok ||
        !runtime) {
      napi_throw_error(env, "INVALID_STATE",
                       "A live background runtime is required");
      return nullptr;
    }
    auto state = std::make_shared<PlatformState>();
    state->env = env;
    state->runner = base::UIThread::GetRunner();
    napi_create_reference(env, args[1], 0, &state->target);
    auto session = HostScriptSession::Create(
        [state](const std::string& status, const std::string& message) {
          state->runner->PostTask([state, status, message] {
            state->Call("entryResult", {status, message});
          });
        },
        false);
    state->session = session;
    auto* binding = new Binding{state, session};
    napi_wrap(
        env, self, binding,
        [](napi_env, void* data, void*) {
          auto* binding = static_cast<Binding*>(data);
          binding->Dispose();
          delete binding;
        },
        nullptr, nullptr);
    runtime->AddLifecycleListener(
        std::make_unique<SessionListener>(session, state));
    return self;
  }
  static napi_value Bind(napi_env env, napi_callback_info info) {
    auto* binding = Get(env, info);
    bool accepted = false;
    if (binding && binding->state->active) {
      binding->state->view_active = true;
      accepted = binding->session->BindView(
          std::make_unique<ViewProxy>(binding->state));
      if (!accepted) binding->state->view_active = false;
    }
    napi_value result;
    napi_get_boolean(env, accepted, &result);
    return result;
  }
  static napi_value Notify(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value args[3];
    auto* binding = Get(env, info, &argc, args);
    if (binding && binding->state->active && argc == 3) {
      binding->session->Notify(base::NapiUtil::ConvertToString(env, args[0]),
                               base::NapiUtil::ConvertToInt32(env, args[1]),
                               base::NapiUtil::ConvertToString(env, args[2]));
    }
    return nullptr;
  }
  static napi_value Destroy(napi_env env, napi_callback_info info) {
    if (auto* binding = Get(env, info)) binding->Dispose();
    return nullptr;
  }
};
}  // namespace

void InitHostScriptBinding(napi_env env, napi_value exports) {
  napi_property_descriptor properties[] = {
      {"bind", nullptr, Binding::Bind, nullptr, nullptr, nullptr, napi_default,
       nullptr},
      {"notify", nullptr, Binding::Notify, nullptr, nullptr, nullptr,
       napi_default, nullptr},
      {"destroy", nullptr, Binding::Destroy, nullptr, nullptr, nullptr,
       napi_default, nullptr}};
  napi_value constructor;
  napi_define_class(
      env, "LynxHostScriptBinding", NAPI_AUTO_LENGTH, Binding::New, nullptr,
      sizeof(properties) / sizeof(properties[0]), properties, &constructor);
  napi_set_named_property(env, exports, "LynxHostScriptBinding", constructor);
}

}  // namespace harmony
}  // namespace lynx
