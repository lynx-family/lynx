// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/runtime/host_script_api.h"

#include "core/shell/host_script/runtime/host_script_module.h"
#include "core/shell/host_script/runtime/host_script_session.h"

namespace lynx {
namespace shell {
namespace {

constexpr char kNativeBridgeName[] = "__lynxHostScriptNative";
constexpr char kHostScriptApi[] = R"HOST_SCRIPT(
(() => {
  const native = globalThis.__lynxHostScriptNative;
  const error = (code, message) =>
    Object.defineProperty(new Error(message), 'code', {
      value: code,
      enumerable: true,
    });
  const options = value => {
    if (!value || typeof value !== 'object' || Array.isArray(value))
      throw error('INVALID_ARGUMENT', 'Options must be an object');
    return value;
  };
  const json = (value, array = false) => {
    if (value === undefined && !array) return undefined;
    try {
      if (!value || typeof value !== 'object' || Array.isArray(value) !== array)
        throw new Error();
      const result = JSON.stringify(value);
      if (result === undefined) throw new Error();
      return result;
    } catch {
      throw error(
        'INVALID_ARGUMENT',
        array
          ? 'Event params must be a JSON-serializable array'
          : 'Page data must be a JSON-serializable object'
      );
    }
  };
  let ready;
  const lynxView = Object.freeze({
    get available() {
      return native.hasCurrent();
    },
    get ready() {
      if (ready === undefined)
        ready = Promise.resolve().then(() => native.waitForCurrent());
      return ready;
    },
    loadURL(url, value = {}) {
      return native.loadURL(
        url,
        json(options(value).data),
        json(value.globalProps)
      );
    },
    loadTemplate(value) {
      options(value);
      return native.loadTemplate(
        value.template,
        value.url,
        json(value.initialData),
        json(value.globalProps),
        value.processor,
        value.readOnly
      );
    },
    loadSSR(value) {
      options(value);
      return native.loadSSR(
        value.data,
        value.url,
        json(value.initialData)
      );
    },
    hydrateSSR(value) {
      options(value);
      return native.hydrateSSR(
        value.data,
        value.url,
        json(value.initialData)
      );
    },
    updateMetaData(meta) {
      if (
        !meta ||
        typeof meta !== 'object' ||
        Array.isArray(meta) ||
        (meta.updateData === undefined && meta.globalProps === undefined)
      )
        throw error('INVALID_ARGUMENT', 'Provide updateData or globalProps');
      return native.updateMetaData(
        json(meta.updateData),
        json(meta.globalProps)
      );
    },
    setGlobalProps(props) {
      const encoded = json(props);
      if (encoded === undefined)
        throw error('INVALID_ARGUMENT', 'Global props must be an object');
      return native.setGlobalProps(encoded);
    },
    reloadTemplate(value = {}) {
      options(value);
      return native.reloadTemplate(
        json(value.data),
        json(value.globalProps)
      );
    },
    sendGlobalEvent(name, ...args) {
      return native.sendGlobalEvent(name, json(args, true));
    },
    on(event, listener) {
      if (typeof listener !== 'function')
        throw error(
          'INVALID_ARGUMENT',
          'on expects an event name and listener'
        );
      native.on(event, listener);
      return this;
    },
    off(event, listener) {
      if (typeof listener !== 'function')
        throw error(
          'INVALID_ARGUMENT',
          'off expects an event name and listener'
        );
      if (native.hasCurrent()) native.off(event, listener);
      return this;
    },
  });
  const context = Object.freeze({ lynxView });
  let defined = false;
  const defineHostScript = setup => {
    if (defined || typeof setup !== 'function') {
      const message = defined
        ? 'defineHostScript can only be called once'
        : 'defineHostScript expects a setup function';
      native.reportEntryResult('INVALID_ENTRY', message);
      throw error('INVALID_ENTRY', message);
    }
    defined = true;
    native.reportEntryResult('REGISTERED');
    const failed = reason =>
      native.reportEntryResult(
        'ERROR',
        reason instanceof Error ? reason.message || reason.name : String(reason)
      );
    try {
      Promise.resolve(setup(context)).then(
        () => native.reportEntryResult('READY'),
        failed
      );
    } catch (reason) {
      failed(reason);
    }
  };
  Object.defineProperty(globalThis, 'defineHostScript', {
    value: defineHostScript,
    writable: false,
    configurable: true,
  });
})();
)HOST_SCRIPT";

}  // namespace

bool InstallHostScriptApi(Napi::Env env,
                          const std::shared_ptr<HostScriptSession>& session) {
  auto native = Napi::Object::New(env);
  HostScriptModule(session).Populate(native);
  env.Global().Set(kNativeBridgeName, native);
  env.RunScript(kHostScriptApi, "lynx://host-script-api.js");
  env.Global().Delete(kNativeBridgeName);
  return !env.IsExceptionPending();
}

void UninstallHostScriptApi(Napi::Env env) {
  env.Global().Delete("defineHostScript");
  env.Global().Delete(kNativeBridgeName);
}

}  // namespace shell
}  // namespace lynx
