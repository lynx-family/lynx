#ifndef SRC_NAPI_ENV_JSC_H_
#define SRC_NAPI_ENV_JSC_H_

#include <JavaScriptCore/JavaScriptCore.h>

#include "js_native_api.h"

EXTERN_C_START

NAPI_EXTERN void napi_attach_jsc(napi_env env, JSGlobalContextRef global_ctx);

NAPI_EXTERN void napi_detach_jsc(napi_env env);

NAPI_EXTERN JSGlobalContextRef napi_get_env_context_jsc(napi_env env);

NAPI_EXTERN JSValueRef napi_js_value_to_jsc_value(napi_env env,
                                                  napi_value value);

NAPI_EXTERN napi_value napi_jsc_value_to_js_value(napi_env env,
                                                  JSValueRef value);

EXTERN_C_END

#endif
