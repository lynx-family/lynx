// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/resource/lynx_resource_loader_harmony.h"

#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/fml/message_loop.h"
#include "base/include/log/logging.h"
#include "base/include/platform/harmony/napi_util.h"
#include "base/include/timer/time_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_trace_event_def.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/resource/lynx_info_reporter_helper_harmony.h"

namespace {

constexpr int32_t LocalErrorCode = -1;
static constexpr const char* LocalErrorMsg = "Get provider error";

bool ReadFileToMemory(const char* file_path,
                      std::unique_ptr<uint8_t[]>& data_ptr, size_t& data_size) {
  std::ifstream file(file_path, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    LOGE("Failed to open file: " << file_path);
    return false;
  }

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  data_ptr = std::make_unique<uint8_t[]>(size);
  if (!data_ptr) {
    LOGE("Failed to allocate memory.");
    file.close();
    return false;
  }

  if (!file.read(reinterpret_cast<char*>(data_ptr.get()), size)) {
    LOGE("Failed to read file.");
    data_ptr.reset();
    file.close();
    return false;
  }

  data_size = static_cast<size_t>(size);
  file.close();
  return true;
}

std::vector<uint8_t> LoadJSSource(const std::string& url) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LOAD_JS_SOURCE, "url", url);
  static const std::string FILE_SCHEME = "file://";
  static const std::string ASSETS_SCHEME = "assets://";
  std::vector<uint8_t> result;
  std::unique_ptr<uint8_t[]> data;
  size_t len = 0;
  bool success = false;
  if (url.length() > ASSETS_SCHEME.length() && url.find(ASSETS_SCHEME) == 0) {
    RawFile* raw_file = OH_ResourceManager_OpenRawFile(
        lynx::harmony::LynxResourceLoaderHarmony::resource_manager,
        url.substr(ASSETS_SCHEME.length()).c_str());
    len = OH_ResourceManager_GetRawFileSize(raw_file);
    data = std::make_unique<uint8_t[]>(len);
    success = OH_ResourceManager_ReadRawFile(raw_file, data.get(), len) > 0;
    OH_ResourceManager_CloseRawFile(raw_file);
  } else if (url.length() > FILE_SCHEME.length() &&
             url.find(FILE_SCHEME) == 0) {
    success =
        ReadFileToMemory(url.substr(FILE_SCHEME.length()).c_str(), data, len);
  }

  if (success) {
    uint8_t* byte_ptr = reinterpret_cast<uint8_t*>(data.get());
    result = std::vector<uint8_t>(byte_ptr, byte_ptr + len);
  } else {
    LOGE("ReadRawFile failed, url: " << url);
  }
  return result;
}
}  // namespace

namespace lynx {
namespace harmony {

NativeResourceManager* LynxResourceLoaderHarmony::resource_manager = nullptr;

LynxResourceLoaderHarmony::LynxResourceLoaderHarmony(napi_env env,
                                                     napi_value providers)
    : env_(env) {
  napi_create_reference(env, providers, 0, &providers_ref_);
  // ref generic fetcher;
  napi_value generic_fetcher;
  napi_get_element(env, providers, 0, &generic_fetcher);
  napi_create_reference(env, generic_fetcher, 0, &generic_fetcher_);

  // ref media fetcher;
  napi_value media_fetcher;
  napi_get_element(env, providers, 1, &media_fetcher);
  napi_create_reference(env, media_fetcher, 0, &media_fetcher_);

  // ref template fetcher;
  napi_value template_fetcher;
  napi_get_element(env, providers, 2, &template_fetcher);
  napi_create_reference(env, template_fetcher, 0, &template_fetcher_);
}

LynxResourceLoaderHarmony::~LynxResourceLoaderHarmony() {}

void LynxResourceLoaderHarmony::LoadResource(
    const pub::LynxResourceRequest& request,
    base::MoveOnlyClosure<void, pub::LynxResourceResponse&> callback) {
  pub::ResourceLoadTiming timing;

  if (request.type == pub::LynxResourceType::kAssets) {
    pub::LynxResourceResponse response{.data = LoadJSSource(request.url)};
    callback(response);
    return;
  }
  if (!ui_task_runner_) {
    pub::LynxResourceResponse response{
        .err_code = LocalErrorCode,
        .err_msg = std::string(
            "The ui task runner of LynxResourceLoaderHarmony is nil")};
    callback(response);
    return;
  }

  std::unordered_map<std::string, std::string> params;
  params.emplace("type", std::to_string(static_cast<int32_t>(request.type)));

  timing.request_internal_prepare_finish = base::CurrentTimeMicroseconds();
  PostTaskOnUIThread([weak_this = weak_from_this(),
                      url = std::move(request.url), params = std::move(params),
                      callback = std::move(callback),
                      resource_type = request.type,
                      timing = std::move(timing)]() mutable {
    timing.request_prepare_to_call_fetcher = base::CurrentTimeMicroseconds();

    auto shared_this =
        std::static_pointer_cast<LynxResourceLoaderHarmony>(weak_this.lock());
    if (!shared_this) {
      pub::LynxResourceResponse response{.err_code = LocalErrorCode,
                                         .err_msg = LocalErrorMsg};
      callback(response);
      return;
    }
    napi_env env = shared_this->env_;
    base::NapiHandleScope scope(env);
    napi_value resource_fetcher = nullptr;
    if (resource_type == pub::LynxResourceType::kLazyBundle) {
      if (shared_this->template_fetcher_) {
        resource_fetcher = base::NapiUtil::GetReferenceNapiValue(
            env, shared_this->template_fetcher_);
      }
    } else {
      if (shared_this->generic_fetcher_) {
        resource_fetcher = base::NapiUtil::GetReferenceNapiValue(
            env, shared_this->generic_fetcher_);
      }
    }

    if (!resource_fetcher) {
      pub::LynxResourceResponse response{.err_code = LocalErrorCode,
                                         .err_msg = LocalErrorMsg};
      callback(response);
      return;
    }

    napi_value call_args[2];
    napi_create_object(env, &call_args[0]);
    BuildResourceRequestValue(env, call_args[0], url,
                              static_cast<int32_t>(resource_type));

    napi_value params_key;
    napi_create_string_utf8(env, "params", NAPI_AUTO_LENGTH, &params_key);
    napi_set_property(env, call_args[0], params_key,
                      base::NapiUtil::CreateMap(env, params));

    napi_valuetype type;
    napi_typeof(env, resource_fetcher, &type);
    if (type == napi_undefined) {
      LOGE("Can not find provider of type:"
           << static_cast<int32_t>(resource_type) << ", url:" << url);
      pub::LynxResourceResponse response{.err_code = LocalErrorCode,
                                         .err_msg = LocalErrorMsg};
      callback(response);
      return;
    }

    // TODO(chenyouhui): store callback_handler if we need support
    // multi-callback
    timing.request_send_to_fetcher = base::CurrentTimeMicroseconds();
    auto* callback_handler =
        new CallbackHandler(std::move(callback), std::move(timing), url);
    napi_status status = napi_ok;
    if (resource_type == pub::LynxResourceType::kLazyBundle) {
      napi_create_function(env, "callback", 9,
                           CallbackHandler::HandleTemplateRequestCallback,
                           callback_handler, &call_args[1]);
      status = base::NapiUtil::InvokeJsMethod(
          env, resource_fetcher, "fetchTemplateAndVerify", 2, call_args);
    } else {
      napi_create_function(env, "callback", 9,
                           CallbackHandler::HandleRequestCallback,
                           callback_handler, &call_args[1]);
      status = base::NapiUtil::InvokeJsMethod(env, resource_fetcher,
                                              "fetchResource", 2, call_args);
    }
    if (status != napi_ok) {
      pub::LynxResourceResponse response{.err_code = LocalErrorCode,
                                         .err_msg = LocalErrorMsg};
      callback_handler->callback_(response);
      delete callback_handler;
      LOGE("LoadResource failed, url: " << url);
    }
  });
}

void LynxResourceLoaderHarmony::LoadResourcePath(
    const pub::LynxResourceRequest& request,
    base::MoveOnlyClosure<void, pub::LynxPathResponse&> path_callback) {
  pub::ResourceLoadTiming timing;

  if (!ui_task_runner_) {
    pub::LynxPathResponse response{
        .err_code = LocalErrorCode,
        .err_msg = "The ui task runner of LynxResourceLoaderHarmony is nil"};
    path_callback(response);
    return;
  }

  timing.request_internal_prepare_finish = base::CurrentTimeMicroseconds();

  PostTaskOnUIThread([weak_this = weak_from_this(),
                      url = std::move(request.url), resourceType = request.type,
                      callback = std::move(path_callback),
                      timing = std::move(timing)]() mutable {
    timing.request_prepare_to_call_fetcher = base::CurrentTimeMicroseconds();
    auto shared_this =
        std::static_pointer_cast<LynxResourceLoaderHarmony>(weak_this.lock());
    if (!shared_this) {
      pub::LynxPathResponse response{.err_code = LocalErrorCode,
                                     .err_msg = LocalErrorMsg};
      callback(response);
      return;
    }
    napi_env env = shared_this->env_;
    base::NapiHandleScope scope(env);
    napi_value generic_fetcher = nullptr;
    if (shared_this->generic_fetcher_) {
      generic_fetcher = base::NapiUtil::GetReferenceNapiValue(
          env, shared_this->generic_fetcher_);
    }
    if (!generic_fetcher) {
      pub::LynxPathResponse response{.err_code = LocalErrorCode,
                                     .err_msg = LocalErrorMsg};
      callback(response);
      return;
    }

    napi_value call_args[2];
    napi_create_object(env, &call_args[0]);
    BuildResourceRequestValue(env, call_args[0], url,
                              static_cast<uint32_t>(resourceType));

    napi_valuetype type;
    napi_typeof(env, generic_fetcher, &type);
    if (type == napi_undefined) {
      LOGE("Can not get GenericResourceProvider;");
      pub::LynxPathResponse response{.err_code = LocalErrorCode,
                                     .err_msg = LocalErrorMsg};
      callback(response);
      return;
    }

    timing.request_send_to_fetcher = base::CurrentTimeMicroseconds();

    auto* callback_handler =
        new CallbackHandler(std::move(callback), std::move(timing), url);
    napi_create_function(env, "callback", NAPI_AUTO_LENGTH,
                         CallbackHandler::HandlePathRequestCallback,
                         callback_handler, &call_args[1]);
    napi_status status = base::NapiUtil::InvokeJsMethod(
        env, generic_fetcher, "fetchResourcePath", 2, call_args);
    if (status != napi_ok) {
      pub::LynxResourceResponse response{.err_code = LocalErrorCode,
                                         .err_msg = LocalErrorMsg};
      callback_handler->callback_(response);
      delete callback_handler;
      LOGE("Call fetchResourcePath failed. url: " << url);
    };
  });
}

void LynxResourceLoaderHarmony::LoadStream(
    const pub::LynxResourceRequest& request,
    const std::shared_ptr<pub::LynxStreamDelegate>& stream_delegate) {
  if (!ui_task_runner_) {
    stream_delegate->OnError(
        "The ui task runner of LynxResourceLoaderHarmony is nil.");
    return;
  }

  PostTaskOnUIThread([weak_this = weak_from_this(),
                      url = std::move(request.url), resourceType = request.type,
                      stream_delegate = stream_delegate]() mutable {
    auto shared_this =
        std::static_pointer_cast<LynxResourceLoaderHarmony>(weak_this.lock());
    if (!shared_this) {
      stream_delegate->OnError(LocalErrorMsg);
      return;
    }

    napi_env env = shared_this->env_;
    base::NapiHandleScope scope(env);
    napi_value generic_fetcher = nullptr;
    if (shared_this->generic_fetcher_) {
      generic_fetcher = base::NapiUtil::GetReferenceNapiValue(
          env, shared_this->generic_fetcher_);
    }
    if (!generic_fetcher) {
      stream_delegate->OnError(LocalErrorMsg);
      return;
    }

    napi_value call_args[2];
    napi_create_object(env, &call_args[0]);
    BuildResourceRequestValue(env, call_args[0], url,
                              static_cast<uint32_t>(resourceType));

    napi_valuetype type;
    napi_typeof(env, generic_fetcher, &type);
    if (type == napi_undefined) {
      LOGE("Can not get GenericResourceProvider.");
      stream_delegate->OnError(LocalErrorMsg);
      return;
    }
    napi_create_object(env, &call_args[1]);
    BuildStreamResourceDelegate(env, call_args[1], url, stream_delegate);
    napi_status status = base::NapiUtil::InvokeJsMethod(
        env, generic_fetcher, "fetchStream", 2, call_args);
    if (status != napi_ok) {
      stream_delegate->OnError(LocalErrorMsg);
      return;
    }
  });
}

void LynxResourceLoaderHarmony::BuildResourceRequestValue(
    napi_env env, napi_value& request, const std::string& url, int32_t type) {
  napi_value url_key, url_value;
  napi_value type_key, type_value;
  napi_create_string_latin1(env, "url", NAPI_AUTO_LENGTH, &url_key);
  napi_create_string_utf8(env, url.c_str(), NAPI_AUTO_LENGTH, &url_value);
  napi_set_property(env, request, url_key, url_value);
  napi_create_string_latin1(env, "type", NAPI_AUTO_LENGTH, &type_key);
  napi_create_int32(env, type, &type_value);
  napi_set_property(env, request, type_key, type_value);
}

void LynxResourceLoaderHarmony::BuildStreamResourceDelegate(
    napi_env env, napi_value& delegate, const std::string& url,
    const std::shared_ptr<pub::LynxStreamDelegate>& stream_delegate) {
  napi_value on_start_key, on_data_key, on_end_key, on_error_key;
  napi_value on_start_value, on_data_value, on_end_value, on_error_value;
  napi_create_string_utf8(env, "onStart", NAPI_AUTO_LENGTH, &on_start_key);
  napi_create_string_utf8(env, "onData", NAPI_AUTO_LENGTH, &on_data_key);
  napi_create_string_utf8(env, "onEnd", NAPI_AUTO_LENGTH, &on_end_key);
  napi_create_string_utf8(env, "onError", NAPI_AUTO_LENGTH, &on_error_key);

  auto* stream_handler = new CallbackHandler(stream_delegate, url);
  napi_create_function(env, "onStart", NAPI_AUTO_LENGTH,
                       CallbackHandler::HandleStreamOnStartCallback,
                       stream_handler, &on_start_value);
  napi_set_property(env, delegate, on_start_key, on_start_value);

  napi_create_function(env, "onData", NAPI_AUTO_LENGTH,
                       CallbackHandler::HandleStreamOnDataCallback,
                       stream_handler, &on_data_value);
  napi_set_property(env, delegate, on_data_key, on_data_value);

  napi_create_function(env, "onEnd", NAPI_AUTO_LENGTH,
                       CallbackHandler::HandleStreamOnEndCallback,
                       stream_handler, &on_end_value);
  napi_set_property(env, delegate, on_end_key, on_end_value);

  napi_create_function(env, "onError", NAPI_AUTO_LENGTH,
                       CallbackHandler::HandleStreamOnErrorCallback,
                       stream_handler, &on_error_value);
  napi_set_property(env, delegate, on_error_key, on_error_value);

  napi_wrap(
      env, delegate, stream_handler,
      [](napi_env env, void* obj, void* data) {
        if (auto* stream_handler = reinterpret_cast<CallbackHandler*>(obj);
            stream_handler != nullptr) {
          LOGI("stream_handle destroy!");
          delete stream_handler;
        }
      },
      nullptr, nullptr);
}

void LynxResourceLoaderHarmony::DeleteRef() {
  napi_delete_reference(env_, providers_ref_);
  providers_ref_ = nullptr;

  napi_delete_reference(env_, generic_fetcher_);
  generic_fetcher_ = nullptr;

  napi_delete_reference(env_, media_fetcher_);
  media_fetcher_ = nullptr;

  napi_delete_reference(env_, template_fetcher_);
  template_fetcher_ = nullptr;
}

napi_value LynxResourceLoaderHarmony::CallbackHandler::HandleRequestCallback(
    napi_env env, napi_callback_info info) {
  auto receive_response_time_point = base::CurrentTimeMicroseconds();

  napi_value js_this;
  size_t argc = 2;
  napi_value argv[2];
  void* callback_ptr;
  napi_get_cb_info(env, info, &argc, argv, &js_this, &callback_ptr);

  CallbackHandler* callback_handler =
      reinterpret_cast<CallbackHandler*>(callback_ptr);

  callback_handler->timing_.response_received_from_fetcher =
      receive_response_time_point;

  napi_valuetype error_type;
  napi_typeof(env, argv[0], &error_type);
  pub::LynxResourceResponse response;
  response.timing = std::move(callback_handler->timing_);
  if (error_type == napi_valuetype::napi_object) {
    response.err_code = -1;
    response.err_msg = "error response";
    response.timing.response_trigger_callback = base::CurrentTimeMicroseconds();
    callback_handler->callback_(response);
  } else {
    base::NapiUtil::ConvertToArrayBuffer(env, argv[1], response.data);
    response.timing.response_trigger_callback = base::CurrentTimeMicroseconds();
    callback_handler->callback_(response);
    LynxInfoReporterHelper::GetInstance().ReportTemplateInfo(
        callback_handler->url_);
  }
  delete callback_handler;
  return js_this;
}

napi_value
LynxResourceLoaderHarmony::CallbackHandler::HandleTemplateRequestCallback(
    napi_env env, napi_callback_info info) {
  auto receive_response_time_point = base::CurrentTimeMicroseconds();

  napi_value js_this;
  size_t argc = 2;
  napi_value argv[2];
  void* callback_ptr;
  napi_get_cb_info(env, info, &argc, argv, &js_this, &callback_ptr);

  CallbackHandler* callback_handler =
      reinterpret_cast<CallbackHandler*>(callback_ptr);

  callback_handler->timing_.response_received_from_fetcher =
      receive_response_time_point;

  pub::LynxResourceResponse response;
  response.timing = std::move(callback_handler->timing_);
  napi_value code_value;
  napi_get_named_property(env, argv[0], "code", &code_value);
  int32_t err_code = base::NapiUtil::ConvertToInt32(env, code_value);

  if (err_code != lynx::error::E_SUCCESS) {
    napi_value message_value;
    napi_get_named_property(env, argv[0], "data", &message_value);
    std::string err_msg = base::NapiUtil::ConvertToString(env, message_value);
    if (err_code == lynx::error::E_APP_BUNDLE_VERIFY_INVALID_SIGNATURE) {
      response.err_code = err_code;
      response.err_msg = std::move(err_msg);
    } else {
      response.err_code = -1;
      response.err_msg = "error response";
    }
    response.timing.response_trigger_callback = base::CurrentTimeMicroseconds();
    callback_handler->callback_(response);
  } else {
    napi_value result_binary = nullptr;
    napi_get_named_property(env, argv[1], "binary", &result_binary);
    base::NapiUtil::ConvertToArrayBuffer(env, result_binary, response.data);
    response.timing.response_trigger_callback = base::CurrentTimeMicroseconds();
    callback_handler->callback_(response);
    LynxInfoReporterHelper::GetInstance().ReportTemplateInfo(
        callback_handler->url_);
  }
  delete callback_handler;
  return js_this;
}

napi_value
LynxResourceLoaderHarmony::CallbackHandler::HandlePathRequestCallback(
    napi_env env, napi_callback_info info) {
  auto receive_response_time_point = base::CurrentTimeMicroseconds();

  napi_value js_this;
  size_t argc = 2;
  napi_value argv[2];
  void* callback_ptr;
  napi_get_cb_info(env, info, &argc, argv, &js_this, &callback_ptr);
  CallbackHandler* callback_handler =
      reinterpret_cast<CallbackHandler*>(callback_ptr);
  callback_handler->timing_.response_received_from_fetcher =
      receive_response_time_point;

  napi_valuetype error_type;
  napi_typeof(env, argv[0], &error_type);
  pub::LynxPathResponse response;
  response.timing = std::move(callback_handler->timing_);
  if (error_type == napi_valuetype::napi_object) {
    napi_value code;
    napi_value message;
    napi_get_named_property(env, argv[0], "code", &code);
    napi_get_named_property(env, argv[0], "message", &message);
    response.err_code = base::NapiUtil::ConvertToInt32(env, code);
    response.err_msg = base::NapiUtil::ConvertToString(env, message);
    response.timing.response_trigger_callback = base::CurrentTimeMicroseconds();
    callback_handler->path_callback_(response);
    delete callback_handler;
  } else {
    response.path = base::NapiUtil::ConvertToString(env, argv[1]);
    response.timing.response_trigger_callback = base::CurrentTimeMicroseconds();
    callback_handler->path_callback_(response);
    delete callback_handler;
  }
  return js_this;
}

napi_value
LynxResourceLoaderHarmony::CallbackHandler::HandleStreamOnStartCallback(
    napi_env env, napi_callback_info info) {
  napi_value js_this;
  size_t argc = 1;
  napi_value argv[1];
  void* callback_ptr;
  napi_get_cb_info(env, info, &argc, argv, &js_this, &callback_ptr);
  CallbackHandler* callback_handler =
      reinterpret_cast<CallbackHandler*>(callback_ptr);

  napi_valuetype type;
  napi_typeof(env, argv[0], &type);
  if (type == napi_valuetype::napi_number) {
    uint32_t content_offset = base::NapiUtil::ConvertToUInt32(env, argv[0]);
    callback_handler->stream_delegate_->OnStart(content_offset);
  }
  return nullptr;
}

napi_value
LynxResourceLoaderHarmony::CallbackHandler::HandleStreamOnDataCallback(
    napi_env env, napi_callback_info info) {
  napi_value js_this;
  napi_value result;
  size_t argc = 3;
  napi_value argv[3];
  void* callback_ptr;
  napi_get_undefined(env, &result);
  napi_get_cb_info(env, info, &argc, argv, &js_this, &callback_ptr);
  CallbackHandler* callback_handler =
      reinterpret_cast<CallbackHandler*>(callback_ptr);
  std::vector<uint8_t> data;
  base::NapiUtil::ConvertToArrayBuffer(env, argv[0], data);
  callback_handler->stream_delegate_->OnData(std::move(data));
  return result;
}

napi_value
LynxResourceLoaderHarmony::CallbackHandler::HandleStreamOnEndCallback(
    napi_env env, napi_callback_info info) {
  napi_value js_this;
  size_t argc = 0;
  napi_value argv[0];
  void* callback_ptr;
  napi_get_cb_info(env, info, &argc, argv, &js_this, &callback_ptr);
  CallbackHandler* callback_handler =
      reinterpret_cast<CallbackHandler*>(callback_ptr);
  callback_handler->stream_delegate_->OnEnd();
  return nullptr;
}

napi_value
LynxResourceLoaderHarmony::CallbackHandler::HandleStreamOnErrorCallback(
    napi_env env, napi_callback_info info) {
  napi_value js_this;
  size_t argc = 1;
  napi_value argv[1];
  void* callback_ptr;
  napi_get_cb_info(env, info, &argc, argv, &js_this, &callback_ptr);
  CallbackHandler* callback_handler =
      reinterpret_cast<CallbackHandler*>(callback_ptr);

  napi_valuetype type;
  napi_typeof(env, argv[0], &type);
  if (type == napi_valuetype::napi_string) {
    std::string error_msg = base::NapiUtil::ConvertToString(env, argv[0]);
    callback_handler->stream_delegate_->OnError(std::move(error_msg));
  }
  return nullptr;
}

// private
void LynxResourceLoaderHarmony::PostTaskOnUIThread(base::closure task) {
  if (ui_task_runner_) {
    ui_task_runner_->PostTask(std::move(task));
  } else {
    LOGE("Unable to switch to Main thread");
  }
}

bool LynxResourceLoaderHarmony::IsLocalResource(const std::string& url) {
  base::NapiHandleScope scope(env_);
  napi_value media_fetcher =
      base::NapiUtil::GetReferenceNapiValue(env_, media_fetcher_);
  if (!media_fetcher) {
    return false;
  }

  napi_value call_args[1];
  napi_create_object(env_, &call_args[0]);

  napi_create_string_utf8(env_, url.c_str(), NAPI_AUTO_LENGTH, &call_args[0]);

  napi_value result = nullptr;
  napi_status status = base::NapiUtil::InvokeJsMethod(
      env_, media_fetcher, "isLocalResource", 1, call_args, &result);
  if (status != napi_ok) {
    LOGE("InvokeJsMethod isLocalResource failed:" << status);
    return false;
  }
  int res = -1;
  if (result) {
    napi_get_value_int32(env_, result, &res);
  }
  return res == 0 ? true : false;
}

std::string LynxResourceLoaderHarmony::ShouldRedirectUrl(
    const pub::LynxResourceRequest& request) {
  base::NapiHandleScope scope(env_);

  napi_value media_fetcher =
      base::NapiUtil::GetReferenceNapiValue(env_, media_fetcher_);
  if (!media_fetcher) {
    return request.url;
  }

  napi_value call_args[1];
  napi_create_object(env_, &call_args[0]);
  BuildResourceRequestValue(env_, call_args[0], request.url,
                            static_cast<uint32_t>(request.type));

  std::string url = request.url;
  napi_value result = nullptr;
  napi_status status = base::NapiUtil::InvokeJsMethod(
      env_, media_fetcher, "shouldRedirectUrl", 1, call_args, &result);

  if (status != napi_ok) {
    LOGE("InvokeJsMethod shouldRedirectUrl failed:" << status);
    return url;
  }
  if (result) {
    url = base::NapiUtil::ConvertToString(env_, result);
  }
  return url;
}

}  // namespace harmony
}  // namespace lynx
