// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/lepus/renderer_functions.h"

#include <assert.h>

#include <deque>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/debug/lynx_assert.h"
#include "base/include/log/logging.h"
#include "base/include/string/string_number_convert.h"
#include "base/include/string/string_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/css/css_style_sheet_manager.h"
#include "core/renderer/css/css_utils.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/block_element.h"
#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/renderer/dom/fiber/for_element.h"
#include "core/renderer/dom/fiber/if_element.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/dom/fiber/none_element.h"
#include "core/renderer/dom/fiber/page_element.h"
#include "core/renderer/dom/fiber/raw_text_element.h"
#include "core/renderer/dom/fiber/scroll_element.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fiber/tree_resolver.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/dom/fiber/wrapper_element.h"
#include "core/renderer/dom/list_component_info.h"
#include "core/renderer/dom/selector/fiber_element_selector.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/dom/vdom/radon/radon_diff_list_node.h"
#include "core/renderer/dom/vdom/radon/radon_diff_list_node2.h"
#include "core/renderer/dom/vdom/radon/radon_factory.h"
#include "core/renderer/dom/vdom/radon/radon_lazy_component.h"
#include "core/renderer/dom/vdom/radon/radon_list_base.h"
#include "core/renderer/dom/vdom/radon/radon_node.h"
#include "core/renderer/dom/vdom/radon/radon_page.h"
#include "core/renderer/events/events.h"
#include "core/renderer/events/gesture.h"
#include "core/renderer/signal/computation.h"
#include "core/renderer/signal/lynx_signal.h"
#include "core/renderer/signal/memo.h"
#include "core/renderer/signal/scope.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/utils/base/base_def.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/renderer/utils/base/tasm_utils.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/renderer/utils/value_utils.h"
#include "core/resource/lazy_bundle/lazy_bundle_utils.h"
#include "core/runtime/bindings/common/event/message_event.h"
#include "core/runtime/bindings/common/event/runtime_constants.h"
#include "core/runtime/bindings/lepus/event/lepus_event_listener.h"
#include "core/runtime/bindings/lepus/renderer.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/builtin.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/tasks/lepus_callback_manager.h"
#include "core/runtime/vm/lepus/tasks/lepus_raf_manager.h"
#include "core/shared_data/white_board_delegate.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "third_party/modp_b64/modp_b64.h"

#if ENABLE_AIR
#include "core/renderer/dom/air/air_element/air_block_element.h"
#include "core/renderer/dom/air/air_element/air_component_element.h"
#include "core/renderer/dom/air/air_element/air_element.h"
#include "core/renderer/dom/air/air_element/air_for_element.h"
#include "core/renderer/dom/air/air_element/air_if_element.h"
#include "core/renderer/dom/air/air_element/air_page_element.h"
#endif

#if defined(OS_WIN)
#ifdef SetProp
#undef SetProp
#endif  // SetProp
#endif  // OS_WIN

namespace lynx {
namespace tasm {

namespace {

template <class... Args>
lepus::Value RenderFatal(lepus::Context* ctx, const char* fmt, Args&&... a) {
  auto err_msg =
      std::string("\nerror code: ")
          .append(std::to_string(error::E_MTS_RENDERER_FUNCTION_FATAL))
          .append("\nerror message: ")
          .append(lynx::base::FormatString(fmt, std::forward<Args>(a)...));
  bool should_abort = tasm::LynxEnv::GetInstance().IsDevToolComponentAttach() &&
                      !tasm::LynxEnv::GetInstance().IsLogBoxEnabled();
  return ctx->ReportFatalError(err_msg, should_abort,
                               error::E_MTS_RENDERER_FUNCTION_FATAL);
}

template <class... Args>
void RenderWarning(const char* fmt, Args&&... a) {
  auto error = lynx::base::LynxError(error::E_MTS_RENDERER_FUNCTION_ERROR, fmt,
                                     std::forward<Args>(a)...);
  lynx::base::ErrorStorage::GetInstance().SetError(std::move(error));
}

lepus::Value ElementAPIFatal(lepus::Context* ctx, const std::string& msg) {
  auto err_msg = std::string("\nerror code: ")
                     .append(std::to_string(error::E_ELEMENT_API_ERROR))
                     .append("\nerror message: ")
                     .append(msg);
  bool should_abort = tasm::LynxEnv::GetInstance().IsDevToolComponentAttach() &&
                      !tasm::LynxEnv::GetInstance().IsLogBoxEnabled();
  return ctx->ReportFatalError(err_msg, should_abort,
                               error::E_ELEMENT_API_ERROR);
}

template <class... Args>
void ElementAPIError(const char* fmt, Args&&... a) {
  auto msg = lynx::base::FormatString(fmt, std::forward<Args>(a)...);
  auto error = base::LynxError(error::E_ELEMENT_API_ERROR, msg);
  base::ErrorStorage::GetInstance().SetError(std::move(error));
}

lepus::Value GetSystemInfoFromTasm(TemplateAssembler* tasm) {
  auto config = tasm->page_proxy()->GetConfig();
  return GenerateSystemInfo(&config);
}

}  // namespace

#define RENDERER_FUNCTION_CC(name)                          \
  lepus::Value RendererFunctions::name(lepus::Context* ctx, \
                                       lepus::Value* argv, int argc)

#define CONVERT_ARG(name, index) lepus::Value* name = argv + index;

#define CHECK_ARGC_EQ(name, count)                                    \
  do {                                                                \
    if (argc != (count)) {                                            \
      return RenderFatal(ctx, #name " param size should be " #count); \
    }                                                                 \
  } while (0)

#define CONVERT_ARG_AND_CHECK(name, index, Type, FunName)                     \
  lepus::Value* name = argv + (index);                                        \
  do {                                                                        \
    if (!name->Is##Type()) {                                                  \
      return RenderFatal(ctx, #FunName " param " #index " should be " #Type); \
    }                                                                         \
  } while (0)

#define CHECK_ARGC_GE(name, now_argc)                                    \
  do {                                                                   \
    if (argc < (now_argc)) {                                             \
      return RenderFatal(ctx, #name " param size should >= " #now_argc); \
    }                                                                    \
  } while (0)

#define ARGC() argc

#define CONVERT(v) (*v)

#define DCONVERT(v) ((v)->ToLepusValue())

#define RETURN(v) return (v)
#define RETURN_UNDEFINED() return lepus::Value();

#define LEPUS_CONTEXT() ctx
#define GET_TASM_POINTER() \
  static_cast<TemplateAssembler*>(LEPUS_CONTEXT()->GetDelegate())

#define CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(name, index, Type, FunName)      \
  CONVERT_ARG(name, index);                                                    \
  if (GET_TASM_POINTER()                                                       \
          ->GetPageConfig()                                                    \
          ->GetEnableElementAPITypeCheckThrowWarning()) {                      \
    do {                                                                       \
      if (!name->Is##Type()) {                                                 \
        ElementAPIError(#FunName " param " #index " should be " #Type);        \
        RETURN_UNDEFINED();                                                    \
      }                                                                        \
    } while (0);                                                               \
  } else {                                                                     \
    do {                                                                       \
      if (!name->Is##Type()) {                                                 \
        return ElementAPIFatal(ctx,                                            \
                               #FunName " param " #index " should be " #Type); \
      }                                                                        \
    } while (0);                                                               \
  }

#define CHECK_ILLEGAL_ATTRIBUTE_CONFIG(name, FunName)                 \
  if (name->IsAsyncResolveInvoked()) {                                \
    ElementAPIError(#name " already trigger async resolve, " #FunName \
                          " will be aborted");                        \
    RETURN_UNDEFINED();                                               \
  }

#define GET_IMPL_ID_AND_KEY(id, index_id, key, index_key, FuncName) \
  CONVERT_ARG_AND_CHECK(arg_id, index_id, Number, FuncName);        \
  CONVERT_ARG_AND_CHECK(arg_key, index_key, Number, FuncName);      \
  id = static_cast<int32_t>(arg_id->Number());                      \
  key = static_cast<uint64_t>(arg_key->Number());

/* Use this macro when fiber element is created. For example:
 ```
 auto& manager = self->page_proxy()->element_manager();
 auto element = manager->CreateFiberNode(arg0->String());

 ON_NODE_CREATE(element);
 RETURN(lepus::Value(element));
 ```
 */
#define ON_NODE_CREATE(node)                                   \
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "Devtool::ON_NODE_CREATE"); \
  EXEC_EXPR_FOR_INSPECTOR(GET_TASM_POINTER()                   \
                              ->page_proxy()                   \
                              ->element_manager()              \
                              ->PrepareNodeForInspector(node.get());)

/* Use this macro when air element is created */
#define ON_AIR_NODE_CREATED(node)                                           \
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "Devtool::ON_AIR_NODE_CREATED");         \
  auto* page =                                                              \
      GET_TASM_POINTER() -> page_proxy() -> element_manager() -> AirRoot(); \
  bool enableAsync = page->EnableAsyncCalc();                               \
  if (enableAsync && node) {                                                \
    node->SetEnableAsyncCalc(enableAsync);                                  \
    page->AppendLastElement();                                              \
    page->RecordFirstScreenElement(node);                                   \
  }

/* Use this macro when fiber element is modified, including its attributes,
 inline styles, classes, id and so on. For example:
 ```
 auto element =
 fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
 element->SetAttribute(arg1->String(), *arg2);

 ON_NODE_MODIFIED(element);
 ```
 */
#define ON_NODE_MODIFIED(node)                                   \
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "Devtool::ON_NODE_MODIFIED"); \
  EXEC_EXPR_FOR_INSPECTOR(GET_TASM_POINTER()                     \
                              ->page_proxy()                     \
                              ->element_manager()                \
                              ->OnElementNodeSetForInspector(node.get());)

/* Use this macro when fiber element is added to another fiber element. For
 example:
 ```
 auto parent =
 fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted()); auto child
 = fml::static_ref_ptr_cast<FiberElement>(arg1->RefCounted());
 parent->InsertNode(child);

 ON_NODE_ADDED(child);
 RETURN(lepus::Value(child));
 ```
 !!! Ensure that ON_NODE_ADDED is called after InsertNode !!!
 When calling ON_NODE_ADDED and ON_NODE_REMOVED, the relevant functions of the
 DevTool SDK will be invoked. In DevTool, it depends on parent and node to
 obtain the corresponding information to ensure that the DOM tree can be
 displayed correctly. Therefore, we need to ensure that ON_NODE_REMOVED is
 called before RemoveNode, and ON_NODE_ADDED is called after InsertNode. If
 ON_NODE_REMOVED is called after RemoveNode, the parent of the node is empty at
 this time, and DevTool cannot get the corresponding parent and accurate
 information. The same goes for calling ON_NODE_ADDED before InsertNode.
 */
#define ON_NODE_ADDED(node)                                                  \
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "Devtool::ON_NODE_ADDED");                \
  EXEC_EXPR_FOR_INSPECTOR(GET_TASM_POINTER()                                 \
                              ->page_proxy()                                 \
                              ->element_manager()                            \
                              ->CheckAndProcessSlotForInspector(node.get()); \
                          GET_TASM_POINTER()                                 \
                              ->page_proxy()                                 \
                              ->element_manager()                            \
                              ->OnElementNodeAddedForInspector(node.get());)

/* Use this macro when fiber element is removed from the parent. For example:
 ```
 auto parent =
 fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted()); auto child
 = fml::static_ref_ptr_cast<FiberElement>(arg1->RefCounted());

 ON_NODE_REMOVED(child);
 parent->RemoveNode(child);

 RETURN(lepus::Value(child));
 ```
 !!! Ensure that ON_NODE_REMOVED is called before RemoveNode !!!
 When calling ON_NODE_ADDED and ON_NODE_REMOVED, the relevant functions of the
 DevTool SDK will be invoked. In DevTool, it depends on parent and node to
 obtain the corresponding information to ensure that the DOM tree can be
 displayed correctly. Therefore, we need to ensure that ON_NODE_REMOVED is
 called before RemoveNode, and ON_NODE_ADDED is called after InsertNode. If
 ON_NODE_REMOVED is called after RemoveNode, the parent of the node is empty at
 this time, and DevTool cannot get the corresponding parent and accurate
 information. The same goes for calling ON_NODE_ADDED before InsertNode.
 */
#define ON_NODE_REMOVED(node)                                 \
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "Devtool::ON_NODE_ADDED"); \
  EXEC_EXPR_FOR_INSPECTOR(GET_TASM_POINTER()                  \
                              ->page_proxy()                  \
                              ->element_manager()             \
                              ->OnElementNodeRemovedForInspector(node.get());)

void UpdateComponentConfig(TemplateAssembler* tasm, RadonComponent* component) {
  component->UpdateSystemInfo(GetSystemInfoFromTasm(tasm));
}

RadonComponent* GetRadonComponent(lepus::Context* context, lepus::Value* arg) {
  auto* tasm = static_cast<TemplateAssembler*>(context->GetDelegate());
  if (tasm->page_proxy()->HasRadonPage()) {
    RadonBase* base = reinterpret_cast<RadonBase*>(arg->CPoint());
    if (base->IsRadonPage() || base->IsRadonComponent()) {
      return static_cast<RadonComponent*>(base);
    }
  }
  return nullptr;
}

void InnerThemeReplaceParams(lepus::Context* ctx, std::string& retStr,
                             lepus::Value* argv, int argc,
                             int paramStartIndex) {
  const int params_size = argc;
  int startPos = 0;
  do {
    const char* head = retStr.c_str();
    const char* start = strchr(head + startPos, '{');
    if (start == nullptr) break;
    const char* cur = start + 1;
    int index = 0;
    while (*cur >= '0' && *cur <= '9') {
      index = index * 10 + (*cur - '0');
      ++cur;
    }
    if (*cur != '}' || index < 0 || index >= params_size - paramStartIndex) {
      startPos = static_cast<int>(cur + 1 - head);
      continue;
    }

    CONVERT_ARG(param, paramStartIndex + index);
    std::ostringstream s;
    CONVERT(param).PrintValue(s, true);
    std::string lepusStr = s.str();
    const std::string newStr = retStr.substr(0, start - head) + lepusStr;
    startPos = static_cast<int>(newStr.size());
    const int endPos = static_cast<int>(cur + 1 - head);
    retStr = newStr + retStr.substr(endPos, retStr.size() - endPos);
  } while (static_cast<size_t>(startPos) < retStr.size());
}

lepus::Value InnerTranslateResourceForTheme(lepus::Context* ctx,
                                            lepus::Value* argv, int argc,
                                            const char* keyIn) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "InnerTranslateResourceForTheme");
  const long params_size = argc;
  int res_start_index = 0;
  DCHECK(argc >= 1);
  if (argv->IsCPointer()) {
    DCHECK(argc >= 2);
    // ignore first cpointer param for TemplateAssembler
    res_start_index = 1;
  }
  auto* self = GET_TASM_POINTER();
  CONVERT_ARG(res_id, res_start_index);
  std::string ret = "";
  if (self && res_id->IsString()) {
    const auto& res_id_str = CONVERT(res_id).StdString();
    if (!res_id_str.empty()) {
      int param_start_index = res_start_index + 1;
      std::string key;
      if (keyIn && *keyIn) {
        key = keyIn;
      } else if (params_size > param_start_index) {
        ++param_start_index;
        CONVERT_ARG(theme_key, (res_start_index + 1));
        if (theme_key->IsString()) {
          key = theme_key->StdString();
        }
      }
      ret = self->TranslateResourceForTheme(res_id_str, key);
      if (param_start_index < params_size && !ret.empty()) {
        InnerThemeReplaceParams(ctx, ret, argv, argc, param_start_index);
      }
    }
  }
  return lepus::Value(std::move(ret));
}

GestureDetector InnerCreateGestureDetector(double gesture_id,
                                           double gesture_type,
                                           lepus::Value* callback_config,
                                           lepus::Value* relation_map_value,
                                           lepus::Context* ctx) {
  // Extract the "callbacks" property from the input "callbacksConfigs"
  // argument.
  BASE_STATIC_STRING_DECL(kCallbacks, "callbacks");
  BASE_STATIC_STRING_DECL(kConfig, "config");
  auto callbacks = callback_config->GetProperty(kCallbacks);
  auto config = callback_config->GetProperty(kConfig);

  // Initialize a vector to store gesture callbacks with their names and
  // functions.
  std::vector<GestureCallback> gesture_callback_vector{};

  // Iterate over each item in the "callbacks" array and extract the name and
  // callback function.
  ForEachLepusValue(callbacks, [&gesture_callback_vector, ctx](
                                   const lepus::Value& index,
                                   const lepus::Value& value) {
    // Define constants for property names in the "value" object.
    BASE_STATIC_STRING_DECL(kName, "name");
    BASE_STATIC_STRING_DECL(kCallback, "callback");

    // Extract the "name" and "callback" properties from the current item.
    const lepus::Value& name = value.GetProperty(kName);
    const lepus::Value& callback = value.GetProperty(kCallback);

    // Check if the "name" property is a string. If not, log a warning.
    if (!name.IsString()) {
      LOGW("CreateGestureDetector' "
           << value.Number()
           << " parameter must contain type, and type must be string.");
      return;
    }

    // Check if the "callback" property is a callable function. If not, log a
    // warning.
    if (!(callback.IsCallable() || callback.IsObject())) {
      LOGW("CreateGestureDetector' " << value.Number()
                                     << " parameter must contain callback, and "
                                        "callback must be callable or object.");
      return;
    }

    if (callback.IsCallable()) {
      // Add the extracted gesture callback (name and function) to the vector.
      gesture_callback_vector.emplace_back(name.String(), lepus::Value(),
                                           callback);
    } else if (callback.IsObject()) {
      gesture_callback_vector.emplace_back(name.String(), callback, ctx);
    }
  });
  // Extract "simultaneous", "waitFor", "continueWith" properties from the
  // "relationMap" argument.
  auto simultaneous_value =
      relation_map_value->GetProperty(BASE_STATIC_STRING(kGestureSimultaneous));
  auto wait_for_value =
      relation_map_value->GetProperty(BASE_STATIC_STRING(kGestureWaitFor));
  auto continue_with_value =
      relation_map_value->GetProperty(BASE_STATIC_STRING(kGestureContinueWith));

  // Initialize vectors to store the gesture relation map (simultaneous,
  // wait_for and continue_with) values.
  std::vector<uint32_t> simultaneous_vector{};
  std::vector<uint32_t> wait_for_vector{};
  std::vector<uint32_t> continue_with_vector{};

  // Iterate over "simultaneous" property and extract its numeric values.
  ForEachLepusValue(simultaneous_value,
                    [&simultaneous_vector](const lepus::Value& index,
                                           const lepus::Value& value) {
                      if (value.IsNumber()) {
                        simultaneous_vector.emplace_back(value.Number());
                      }
                    });

  // Iterate over "waitFor" property and extract its numeric values.
  ForEachLepusValue(
      wait_for_value,
      [&wait_for_vector](const lepus::Value& index, const lepus::Value& value) {
        if (value.IsNumber()) {
          wait_for_vector.emplace_back(value.Number());
        }
      });

  // Iterate over "continueWith" property and extract its numeric values.
  ForEachLepusValue(continue_with_value,
                    [&continue_with_vector](const lepus::Value& index,
                                            const lepus::Value& value) {
                      if (value.IsNumber()) {
                        continue_with_vector.emplace_back(value.Number());
                      }
                    });

  // Create a map to store the gesture relation information (simultaneous and
  // wait_for).
  std::unordered_map<std::string, std::vector<uint32_t>> relation_map;

  // Populate the map with the extracted vectors.
  relation_map.emplace(kGestureSimultaneous, simultaneous_vector);
  relation_map.emplace(kGestureWaitFor, wait_for_vector);
  relation_map.emplace(kGestureContinueWith, continue_with_vector);

  // Create a GestureDetector object using the extracted data.
  GestureDetector detector(gesture_id, static_cast<GestureType>((gesture_type)),
                           gesture_callback_vector, relation_map, config);

  return detector;
}

std::optional<lepus::Value> SetAirElement(lepus::Context* ctx,
                                          AirElement* element,
                                          lepus::Value* argv[], int argc) {
#if ENABLE_AIR

#define CONVERT_ARG_POINTER(name, index) lepus::Value* name = argv[index];

#define CONVERT_ARG_POINTER_AND_CHECK(name, index, Type, FunName)             \
  lepus::Value* name = argv[index];                                           \
  do {                                                                        \
    if (!name->Is##Type()) {                                                  \
      return RenderFatal(ctx, #FunName " param " #index " should be " #Type); \
    }                                                                         \
  } while (0)

  CHECK_ARGC_GE(SetAirElement, 5);
  auto* self = GET_TASM_POINTER();
  auto& manager = self->page_proxy()->element_manager();
  bool enable_async_calc = manager->AirRoot()->EnableAsyncCalc();
  // parent
  CONVERT_ARG_POINTER(arg0, 0);
  if (arg0->IsRefCounted()) {
    auto parent =
        fml::static_ref_ptr_cast<AirLepusRef>(arg0->RefCounted())->Get();
    parent->InsertNode(element);
  } else if (arg0->IsNumber()) {
    // In the new proposal about Lepus Tree, the parameter `parent` is only a
    // number which represents the unique id of parent element.
    auto parent =
        manager->air_node_manager()->Get(static_cast<int>(arg0->Number()));
    if (parent) {
      parent->InsertNode(element);
    }
  }
  // style
  CONVERT_ARG_POINTER(arg1, 1);
  if (arg1->IsObject()) {
    tasm::ForEachLepusValue(*arg1, [&element, enable_async_calc](
                                       const lepus::Value& key,
                                       const lepus::Value& value) {
      CSSPropertyID id = static_cast<CSSPropertyID>(std::stoi(key.StdString()));
      if (CSSProperty::IsPropertyValid(id)) {
        element->SetInlineStyle(id, value, !enable_async_calc);
      }
    });
  } else if (arg1->IsString()) {
    lepus::Value new_argv[] = {
        lepus::Value(AirLepusRef::Create(
            manager->air_node_manager()->Get(element->impl_id()))),
        *arg1};
    RendererFunctions::AirSetInlineStyles(ctx, new_argv, 2);
  }
  CONVERT_ARG_POINTER(arg2, 2);
  // attribute
  if (arg2->IsObject()) {
    tasm::ForEachLepusValue(
        *arg2, [&element, enable_async_calc](const lepus::Value& key,
                                             const lepus::Value& value) {
          element->SetAttribute(key.String(), value, !enable_async_calc);
        });
  }
  CONVERT_ARG_POINTER(arg3, 3);
  // class
  if (arg3->IsString()) {
    element->SetClasses(*arg3);
  }
  CONVERT_ARG_POINTER(arg4, 4);
  // id
  if (arg4->IsString()) {
    element->SetIdSelector(*arg4);
  }

  if (argc >= 7) {
    // In the new proposal about Lepus Tree, event and dataset are also
    // provided in the create operation.
    CONVERT_ARG_POINTER_AND_CHECK(arg5, 5, Object, SetAirElement);
    CONVERT_ARG_POINTER_AND_CHECK(arg6, 6, Object, SetAirElement);
    const auto& event_type =
        arg5->GetProperty(BASE_STATIC_STRING(AirElement::kAirLepusEventType));
    const auto& event_name =
        arg5->GetProperty(BASE_STATIC_STRING(AirElement::kAirLepusEventName));
    const auto& event_callback = arg5->GetProperty(
        BASE_STATIC_STRING(AirElement::kAirLepusEventCallback));
    if (event_type.IsString() && event_name.IsString() &&
        event_callback.IsString()) {
      auto type = event_type.String();
      auto name = event_name.String();
      auto callback = event_callback.String();
      element->SetEventHandler(name, element->SetEvent(type, name, callback));
    }
    tasm::ForEachLepusValue(
        *arg6, [&element](const lepus::Value& key, const lepus::Value& value) {
          element->SetDataSet(key.String(), value);
        });
  }

#undef CONVERT_ARG_POINTER
#undef CONVERT_ARG_POINTER_AND_CHECK
#endif
  return std::nullopt;
}

void UpdateAirElement(lepus::Context* ctx, const lepus::Value& lepus_element,
                      bool need_flush) {
#if ENABLE_AIR
  auto* self = GET_TASM_POINTER();
  auto& manager = self->page_proxy()->element_manager();
  auto impl_id = static_cast<int>(
      lepus_element
          .GetProperty(BASE_STATIC_STRING(AirElement::kAirLepusUniqueId))
          .Number());
  auto bits = static_cast<int>(
      lepus_element
          .GetProperty(BASE_STATIC_STRING(AirElement::kAirLepusContentBits))
          .Number());
  auto type = static_cast<int>(
      lepus_element.GetProperty(BASE_STATIC_STRING(AirElement::kAirLepusType))
          .Number());
  auto element = manager->air_node_manager()->Get(impl_id);
  if (!element) {
    return;
  }
  // update tt:if and tt:for
  switch (type) {
    case kAirIf: {
      auto index = static_cast<int32_t>(
          lepus_element
              .GetProperty(BASE_STATIC_STRING(AirElement::kAirLepusIfIndex))
              .Number());
      static_cast<AirIfElement*>(element.get())->UpdateIfIndex(index);
      break;
    }
    case kAirFor: {
      auto count = static_cast<uint32_t>(
          lepus_element
              .GetProperty(BASE_STATIC_STRING(AirElement::kAirLepusForCount))
              .Number());
      static_cast<AirForElement*>(element.get())->UpdateChildrenCount(count);
      break;
    }
    default: {
      break;
    }
  }
  // The value of bits is updated in lepus, each bit of bits indicates which
  // content has been updated.
  // 1. if bits & 00000001, inline styles is updated in lepus;
  // 2. if bits & 00000010, attributes are updated in lepus;
  // 3. if bits & 00000100, classes are updated in lepus;
  // 4. if bits & 00001000, id selector is updated in lepus;
  // 5. if bits & 00010000, event is updated in lepus;
  // 6. if bits & 00100000, dataset is updated in lepus.
  // update inline styles
  if (bits & (1 << 0)) {
    const auto& inline_styles = lepus_element.GetProperty(
        BASE_STATIC_STRING(AirElement::kAirLepusInlineStyle));
    lepus::Value new_argv[] = {lepus::Value(AirLepusRef::Create(element)),
                               inline_styles};
    RendererFunctions::AirSetInlineStyles(ctx, new_argv, 2);
  }
  // update attributes
  if (bits & (1 << 1)) {
    const auto& attrs = lepus_element.GetProperty(
        BASE_STATIC_STRING(AirElement::kAirLepusAttrs));
    tasm::ForEachLepusValue(attrs, [&element](const lepus::Value& attr_key,
                                              const lepus::Value& attr_value) {
      element->SetAttribute(attr_key.String(), attr_value);
    });
  }
  // update classes
  if (bits & (1 << 2)) {
    const auto& classes = lepus_element.GetProperty(
        BASE_STATIC_STRING(AirElement::kAirLepusClasses));
    element->SetClasses(classes);
  }
  // update id selector
  if (bits & (1 << 3)) {
    const auto& id = lepus_element.GetProperty(
        BASE_STATIC_STRING(AirElement::kAirLepusIdSelector));
    element->SetIdSelector(id);
  }
  // update event
  if (bits & (1 << 4)) {
    const auto& event = lepus_element.GetProperty(
        BASE_STATIC_STRING(AirElement::kAirLepusEvent));
    const auto& event_type =
        event.GetProperty(BASE_STATIC_STRING(AirElement::kAirLepusEventType));
    const auto& event_name =
        event.GetProperty(BASE_STATIC_STRING(AirElement::kAirLepusEventName));
    const auto& event_callback = event.GetProperty(
        BASE_STATIC_STRING(AirElement::kAirLepusEventCallback));
    if (event_type.IsString() && event_name.IsString() &&
        event_callback.IsString()) {
      auto type = event_type.String();
      auto name = event_name.String();
      auto callback = event_callback.String();
      element->SetEventHandler(name, element->SetEvent(type, name, callback));
    }
  }
  // update dataset
  if (bits & (1 << 5)) {
    const auto& dataset = lepus_element.GetProperty(
        BASE_STATIC_STRING(AirElement::kAirLepusDataset));
    tasm::ForEachLepusValue(
        dataset, [&element](const lepus::Value& data_key,
                            const lepus::Value& data_value) {
          element->SetDataSet(data_key.String(), data_value);
        });
  }
  if (need_flush) {
    element->FlushProps();
  }
#endif
}

void CreateAirElement(lepus::Context* ctx, const lepus::Value& lepus_element) {
#if ENABLE_AIR
  // Create air element according to the property of lepus element.
  const auto& lepus_id =
      lepus_element.GetProperty(BASE_STATIC_STRING(AirElement::kAirLepusId));
  const auto& impl_id = lepus_element.GetProperty(
      BASE_STATIC_STRING(AirElement::kAirLepusUniqueId));
  const auto& lepus_key =
      lepus_element.GetProperty(BASE_STATIC_STRING(AirElement::kAirLepusKey));
  const auto& parent = lepus_element.GetProperty(
      BASE_STATIC_STRING(AirElement::kAirLepusParent));
  const auto& type = static_cast<int>(
      lepus_element.GetProperty(BASE_STATIC_STRING(AirElement::kAirLepusType))
          .Number());
  lepus::Value element_ref;
  switch (type) {
    case kAirComponent: {
      const auto& name = lepus_element.GetProperty(
          BASE_STATIC_STRING(AirElement::kAirLepusComponentName));
      const auto& path = lepus_element.GetProperty(
          BASE_STATIC_STRING(AirElement::kAirLepusComponentPath));
      const auto& tid = lepus_element.GetProperty(
          BASE_STATIC_STRING(AirElement::kAirLepusComponentTid));
      const auto& inline_styles = lepus_element.GetProperty(
          BASE_STATIC_STRING(AirElement::kAirLepusInlineStyle));
      const auto& attrs = lepus_element.GetProperty(
          BASE_STATIC_STRING(AirElement::kAirLepusAttrs));
      const auto& classes = lepus_element.GetProperty(
          BASE_STATIC_STRING(AirElement::kAirLepusClasses));
      const auto& id = lepus_element.GetProperty(
          BASE_STATIC_STRING(AirElement::kAirLepusIdSelector));
      const auto& event = lepus_element.GetProperty(
          BASE_STATIC_STRING(AirElement::kAirLepusEvent));
      const auto& dataset = lepus_element.GetProperty(
          BASE_STATIC_STRING(AirElement::kAirLepusDataset));
      lepus::Value new_argv[] = {
          tid,           name,  path,    lepus_id, impl_id, lepus_key, parent,
          inline_styles, attrs, classes, id,       event,   dataset};
      element_ref = RendererFunctions::AirCreateComponent(ctx, new_argv, 13);
      break;
    }
    case kAirIf: {
      const auto& index = lepus_element.GetProperty(
          BASE_STATIC_STRING(AirElement::kAirLepusIfIndex));
      lepus::Value new_argv[] = {lepus_id, impl_id, lepus_key, parent, index};
      RendererFunctions::AirCreateIf(ctx, new_argv, 5);
      break;
    }
    case kAirFor: {
      const auto& child_count = lepus_element.GetProperty(
          BASE_STATIC_STRING(AirElement::kAirLepusForCount));
      lepus::Value new_argv[] = {lepus_id, impl_id, lepus_key, parent,
                                 child_count};
      RendererFunctions::AirCreateFor(ctx, new_argv, 5);
      break;
    }
    case kAirBlock: {
      lepus::Value new_argv[] = {lepus_id, impl_id, lepus_key, parent};
      RendererFunctions::AirCreateBlock(ctx, new_argv, 4);
      break;
    }
    case kAirRawText: {
      const auto& attrs = lepus_element.GetProperty(
          BASE_STATIC_STRING(AirElement::kAirLepusAttrs));
      lepus::Value new_argv[] = {lepus_id, attrs, parent, impl_id, lepus_key};
      element_ref = RendererFunctions::AirCreateRawText(ctx, new_argv, 5);
      break;
    }
    case kAirNormal: {
      const auto& tag = lepus_element.GetProperty(
          BASE_STATIC_STRING(AirElement::kAirLepusTag));
      const auto& use_opt = lepus_element.GetProperty(
          BASE_STATIC_STRING(AirElement::kAirLepusUseOpt));
      const auto& inline_styles = lepus_element.GetProperty(
          BASE_STATIC_STRING(AirElement::kAirLepusInlineStyle));
      const auto& attrs = lepus_element.GetProperty(
          BASE_STATIC_STRING(AirElement::kAirLepusAttrs));
      const auto& classes = lepus_element.GetProperty(
          BASE_STATIC_STRING(AirElement::kAirLepusClasses));
      const auto& id = lepus_element.GetProperty(
          BASE_STATIC_STRING(AirElement::kAirLepusIdSelector));
      const auto& event = lepus_element.GetProperty(
          BASE_STATIC_STRING(AirElement::kAirLepusEvent));
      const auto& dataset = lepus_element.GetProperty(
          BASE_STATIC_STRING(AirElement::kAirLepusDataset));
      lepus::Value new_argv[] = {tag,     lepus_id,  use_opt, inline_styles,
                                 attrs,   classes,   id,      parent,
                                 impl_id, lepus_key, event,   dataset};
      element_ref = RendererFunctions::AirCreateElement(ctx, new_argv, 12);
      break;
    }
    default: {
      break;
    }
  }
  auto* page = GET_TASM_POINTER()->page_proxy()->element_manager()->AirRoot();
  if (!element_ref.IsEmpty() && page->EnableAsyncCalc()) {
    auto* element =
        fml::static_ref_ptr_cast<AirLepusRef>(element_ref.RefCounted())->Get();
    page->RecordFirstScreenElement(element);
    page->AppendLastElement();
  }
#endif
}

/* Lepus Lynx API BEGIN */
RENDERER_FUNCTION_CC(GetTextInfo) {
  if (argc < 2) {
    LEPUS_CONTEXT()->ReportFatalError(
        "lynx.getTextInfo's parameter must >= 2!!", false,
        error::E_MTS_RENDERER_FUNCTION_FATAL);
    RETURN_UNDEFINED();
  }
  CONVERT_ARG(content, 0);
  if (!content->IsString()) {
    LEPUS_CONTEXT()->ReportFatalError(
        "lynx.getTextInfo's first parameter must be string!!", false,
        error::E_MTS_RENDERER_FUNCTION_FATAL);
    RETURN_UNDEFINED();
  }
  CONVERT_ARG(options, 1);
  if (!options->IsObject()) {
    LEPUS_CONTEXT()->ReportFatalError(
        "lynx.getTextInfo's second parameter must be object!!", false,
        error::E_MTS_RENDERER_FUNCTION_FATAL);
    RETURN_UNDEFINED();
  }
  // TODO(songshourui.null): get current LynxView's pixelRatio
  options->SetProperty(BASE_STATIC_STRING(kPixelRatio),
                       lepus::Value(Config::pixelRatio()));
  auto text_info =
      GET_TASM_POINTER()
          ->page_proxy()
          ->element_manager()
          ->painting_context()
          ->impl()
          ->GetTextInfo(content->StdString(), pub::ValueImplLepus(*options));
  return pub::ValueUtils::ConvertValueToLepusValue(*text_info);
}

RENDERER_FUNCTION_CC(SetSessionStorageItem) {
  CHECK_ARGC_EQ(SetSessionStorageItem, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, String, SetSessionStorageItem);
  CONVERT_ARG_AND_CHECK(arg1, 1, Object, SetSessionStorageItem);
  auto* tasm = GET_TASM_POINTER();
  auto white_board_delegate = tasm->GetWhiteBoardDelegate();
  if (white_board_delegate) {
    white_board_delegate->SetSessionStorageItem(arg0->StdString(),
                                                arg1->ToLepusValue());
  }
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(GetSessionStorageItem) {
  CHECK_ARGC_EQ(GetSessionStorageItem, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, String, GetSessionStorageItem);
  auto* tasm = GET_TASM_POINTER();
  auto white_board_delegate = tasm->GetWhiteBoardDelegate();
  if (white_board_delegate) {
    return white_board_delegate->GetSessionStorageItem(arg0->StdString());
  }
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(GetDevTool) {
  auto* tasm = GET_TASM_POINTER();
  if (tasm == nullptr) {
    RETURN_UNDEFINED();
  }

  RETURN(tasm->GetContextProxy(runtime::ContextProxy::Type::kDevTool)
             ->GetBinding(LEPUS_CONTEXT()));
}

RENDERER_FUNCTION_CC(GetJSContext) {
  auto* tasm = GET_TASM_POINTER();
  if (tasm == nullptr) {
    RETURN_UNDEFINED();
  }

  RETURN(tasm->GetContextProxy(runtime::ContextProxy::Type::kJSContext)
             ->GetBinding(LEPUS_CONTEXT()));
}

RENDERER_FUNCTION_CC(GetCoreContext) {
  auto* tasm = GET_TASM_POINTER();
  if (tasm == nullptr) {
    RETURN_UNDEFINED();
  }

  RETURN(tasm->GetContextProxy(runtime::ContextProxy::Type::kCoreContext)
             ->GetBinding(LEPUS_CONTEXT()));
}

RENDERER_FUNCTION_CC(GetUIContext) {
  auto* tasm = GET_TASM_POINTER();
  if (tasm == nullptr) {
    RETURN_UNDEFINED();
  }

  RETURN(tasm->GetContextProxy(runtime::ContextProxy::Type::kUIContext)
             ->GetBinding(LEPUS_CONTEXT()));
}

RENDERER_FUNCTION_CC(GetCustomSectionSync) {
  CHECK_ARGC_GE(GetCustomSectionSync, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, String, GetCustomSectionSync);

  auto* tasm = GET_TASM_POINTER();
  if (tasm) {
    RETURN(tasm->GetCustomSection(arg0->StdString()));
  }

  RETURN_UNDEFINED();
}

/* Lepus Lynx API END */

/* ContextProxy API BEGIN */
RENDERER_FUNCTION_CC(RuntimeAddEventListener) {
  CHECK_ARGC_GE(RuntimeAddEventListener, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, String, RuntimeAddEventListener);
  CONVERT_ARG_AND_CHECK(arg1, 1, Callable, RuntimeAddEventListener);

  ContextProxyInLepus* context_proxy =
      ContextProxyInLepus::GetContextProxyFromLepusValue(
          LEPUS_CONTEXT()->GetCurrentThis(argv, argc - 1));
  if (context_proxy == nullptr) {
    RenderFatal(LEPUS_CONTEXT(),
                "DispatchEvent failed since the context_proxy is nullptr!");
    RETURN_UNDEFINED();
  }

  context_proxy->AddEventListener(
      arg0->StdString(),
      std::make_unique<LepusClosureEventListener>(LEPUS_CONTEXT(), *arg1));

  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(RuntimeRemoveEventListener) {
  CHECK_ARGC_GE(RuntimeRemoveEventListener, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, String, RuntimeRemoveEventListener);
  CONVERT_ARG_AND_CHECK(arg1, 1, Callable, RuntimeRemoveEventListener);

  ContextProxyInLepus* context_proxy =
      ContextProxyInLepus::GetContextProxyFromLepusValue(
          LEPUS_CONTEXT()->GetCurrentThis(argv, argc - 1));
  if (context_proxy == nullptr) {
    RenderFatal(LEPUS_CONTEXT(),
                "DispatchEvent failed since the context_proxy is nullptr!");
    RETURN_UNDEFINED();
  }

  context_proxy->RemoveEventListener(
      arg0->StdString(),
      std::make_unique<LepusClosureEventListener>(LEPUS_CONTEXT(), *arg1));

  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(PostMessage) {
  CHECK_ARGC_GE(PostMessage, 1);
  CONVERT_ARG(arg0, 0);

  ContextProxyInLepus* context_proxy =
      ContextProxyInLepus::GetContextProxyFromLepusValue(
          LEPUS_CONTEXT()->GetCurrentThis(argv, argc - 1));
  if (context_proxy == nullptr) {
    RenderFatal(LEPUS_CONTEXT(),
                "DispatchEvent failed since the context_proxy is nullptr!");
    RETURN_UNDEFINED();
  }

  context_proxy->PostMessage(*arg0);
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(DispatchEvent) {
  CHECK_ARGC_GE(DispatchEvent, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, Object, DispatchEvent);

  ContextProxyInLepus* context_proxy =
      ContextProxyInLepus::GetContextProxyFromLepusValue(
          LEPUS_CONTEXT()->GetCurrentThis(argv, argc - 1));
  if (context_proxy == nullptr) {
    RenderFatal(LEPUS_CONTEXT(),
                "DispatchEvent failed since the context_proxy is nullptr!");
    RETURN_UNDEFINED();
  }

  auto kType = BASE_STATIC_STRING(runtime::kType);
  if (!arg0->Contains(kType) || !arg0->GetProperty(kType).IsString()) {
    RenderFatal(LEPUS_CONTEXT(),
                "DispatchEvent failed since the arg0 must contain type and the "
                "value must be string!");
    RETURN_UNDEFINED();
  }

  if (!arg0->Contains(BASE_STATIC_STRING(runtime::kData))) {
    RenderFatal(LEPUS_CONTEXT(),
                "DispatchEvent failed since the arg0 must contain data!");
    RETURN_UNDEFINED();
  }

  runtime::MessageEvent event = context_proxy->CreateMessageEvent(*arg0);
  context_proxy->DispatchEvent(event);

  RETURN_UNDEFINED();
}

void ModifyStyleSheetByIdHelper(
    TemplateAssembler* tasm, const std::string& entry_name, int32_t id,
    std::unique_ptr<SharedCSSFragment> style_sheet) {
  const auto& style_sheet_manager = tasm->style_sheet_manager(entry_name);
  if (!style_sheet) {
    style_sheet_manager->RemoveSharedCSSFragment(id);
  } else {
    // SharedCSSFragment has id_ field so no id is required
    style_sheet_manager->ReplaceSharedCSSFragment(std::move(style_sheet));
  }

  // After replacement/deletion above the CSSFragment held by FiberElement will
  // become a wild ptr, need clear all style_sheet of entire tree
  // so that replaced CSSFragment can be re-obtained
  auto root =
      static_cast<FiberElement*>(tasm->page_proxy()->element_manager()->root());
  if (root) {
    root->ApplyFunctionRecursive([](auto element) {
      element->ResetStyleSheet();
      element->MarkStyleDirty(false);
    });
  }
}

RENDERER_FUNCTION_CC(ReplaceStyleSheetByIdWithBase64) {
  auto* self = GET_TASM_POINTER();
  // parameter size = 2
  // [0] Number -> css id
  // [1] String -> base64-encoded css fragment binary
  // [2] String|Undefined -> optional, entry_name

  CHECK_ARGC_GE(ReplaceStyleSheetByIdWithBase64, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, Number, ReplaceStyleSheetByIdWithBase64);
  CONVERT_ARG_AND_CHECK(arg1, 1, String, ReplaceStyleSheetByIdWithBase64);
  std::string entry_name = DEFAULT_ENTRY_NAME;
  if ((argc > 3 && LEPUS_CONTEXT()->IsVMContext()) ||
      (argc > 2 && !LEPUS_CONTEXT()->IsVMContext())) {
    CONVERT_ARG_AND_CHECK(arg2, 2, String, ReplaceStyleSheetByIdWithBase64);
    entry_name = arg2->StdString();
  }

  auto base64_buffer = arg1->StdString();
  modp_b64_decode(base64_buffer);
  auto input_stream = std::make_unique<lepus::ByteArrayInputStream>(
      std::vector<uint8_t>(base64_buffer.begin(), base64_buffer.end()));
  auto reader = TemplateBinaryReader(self, nullptr, std::move(input_stream));

  const auto id = static_cast<int32_t>(arg0->Number());

  reader.Decode();
  if (!reader.template_bundle()
           .GetCSSStyleManager()
           ->IsSharedCSSFragmentDecoded(id)) {
    reader.DecodeCSSFragmentByIdInRender(id);
  }

  // CSSStyleSheetManager hold by this reader is never shared
  // so we can manipulate it without lock
  auto fragment_handle = reader.template_bundle()
                             .GetCSSStyleManager()
                             ->GetCSSFragmentMap()
                             ->extract(id);
  if (fragment_handle.empty()) {
    RenderFatal(LEPUS_CONTEXT(),
                "css fragment with specific id is not provided by this buffer");
    RETURN_UNDEFINED();
  }

  ModifyStyleSheetByIdHelper(self, entry_name,
                             static_cast<int32_t>(arg0->Number()),
                             std::move(fragment_handle.mapped()));

  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(RemoveStyleSheetById) {
  auto* self = GET_TASM_POINTER();
  // parameter size = 1
  // [0] Number -> css id
  // [1] String|Undefined -> optional, entry_name

  CHECK_ARGC_GE(RemoveStyleSheetById, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, Number, RemoveStyleSheetById);
  std::string entry_name = DEFAULT_ENTRY_NAME;
  if ((argc > 2 && LEPUS_CONTEXT()->IsVMContext()) ||
      (argc > 1 && !LEPUS_CONTEXT()->IsVMContext())) {
    CONVERT_ARG_AND_CHECK(arg1, 1, String, RemoveStyleSheetById);
    entry_name = arg1->StdString();
  }

  ModifyStyleSheetByIdHelper(self, entry_name,
                             static_cast<int32_t>(arg0->Number()), nullptr);

  RETURN_UNDEFINED();
}
/* ContextProxy API END */

RENDERER_FUNCTION_CC(IndexOf) {
  CHECK_ARGC_EQ(IndexOf, 2);
  CONVERT_ARG(obj, 0);
  CONVERT_ARG_AND_CHECK(idx, 1, Number, IndexOf);

  int index = idx->Number();
  RETURN(obj->GetProperty(index));
}

RENDERER_FUNCTION_CC(GetLength) {
  CHECK_ARGC_EQ(GetLength, 1);
  CONVERT_ARG(value, 0);
  int len = value->GetLength();
  RETURN(lepus::Value(len));
}

RENDERER_FUNCTION_CC(SetValueToMap) {
  CHECK_ARGC_EQ(SetValueToMap, 3);
  CONVERT_ARG_AND_CHECK(obj, 0, Object, SetValueToMap);
  CONVERT_ARG_AND_CHECK(key, 1, String, SetValueToMap);
  CONVERT_ARG(value, 2);
  obj->SetProperty(key->String(), *value);
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AttachPage) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AttachPage");
  LOGI("AttachPage" << ctx);
  CHECK_ARGC_EQ(AttachPage, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, AttachPage);
  CONVERT_ARG_AND_CHECK(arg1, 1, CPointer, AttachPage);

  auto* self = GET_TASM_POINTER();
  RadonBase* base = reinterpret_cast<RadonBase*>(arg1->CPoint());
  if (!base->IsRadonPage()) {
    RETURN_UNDEFINED();
  }
  RadonPage* root = static_cast<RadonPage*>(base);
  self->page_proxy()->SetRadonPage(root);
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(CreateVirtualNode) {
  CHECK_ARGC_GE(CreateVirtualNode, 1);
  CONVERT_ARG_AND_CHECK(name_val, 0, String, CreateVirtualNode);
  auto tag_name = name_val->String();
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CreateVirtualNode", "tagName",
              tag_name.str());
  RadonNodeIndexType eid = kRadonInvalidNodeIndex;
  if (argc > 1) {
    CONVERT_ARG_AND_CHECK(eid_val, 1, Number, CreateVirtualNode);
    eid = static_cast<RadonNodeIndexType>(eid_val->Number());
  }
  auto* tasm = GET_TASM_POINTER();
  auto* node = new lynx::tasm::RadonNode(tasm->page_proxy(), tag_name, eid);
  RETURN(lepus::Value(static_cast<RadonBase*>(node)));
}

RENDERER_FUNCTION_CC(CreateVirtualPage) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CreateVirtualPage");

  // notify devtool page is updated
  EXEC_EXPR_FOR_INSPECTOR(
      GET_TASM_POINTER()->page_proxy()->element_manager()->OnDocumentUpdated());

  CHECK_ARGC_EQ(CreateVirtualPage, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, Number, CreateVirtualPage);
  CONVERT_ARG_AND_CHECK(arg1, 1, CPointer, CreateVirtualPage);

  int tid = static_cast<int>(arg0->Number());
  auto* self = GET_TASM_POINTER();
  auto it = self->page_moulds().find(tid);
  DCHECK(it != self->page_moulds().end());
  PageMould* pm = it->second.get();
  auto entry = self->FindTemplateEntry(tasm::DEFAULT_ENTRY_NAME);
  if (entry) {
    self->page_proxy()->SetRemoveCSSScopeEnabled(
        entry->compile_options().enable_remove_css_scope_);
  }

  bool keep_page_data = self->page_proxy()->GetEnableSavePageData();
  RadonPage* page = new lynx::tasm::RadonPage(
      self->page_proxy(), tid, nullptr,
      self->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME), pm, LEPUS_CONTEXT());
  if (page && !keep_page_data) {
    page->DeriveFromMould(pm);
  }
  page->SetGetDerivedStateFromPropsProcessor(
      self->GetProcessorWithName(REACT_PRE_PROCESS_LIFECYCLE));
  if (self->GetPageDSL() == PackageInstanceDSL::REACT) {
    page->SetDSL(PackageInstanceDSL::REACT);
    page->SetGetDerivedStateFromErrorProcessor(
        self->GetProcessorWithName(REACT_ERROR_PROCESS_LIFECYCLE));
  }
  page->SetScreenMetricsOverrider(
      self->GetProcessorWithName(SCREEN_METRICS_OVERRIDER));
  page->SetEnableSavePageData(keep_page_data);
  page->SetShouldComponentUpdateProcessor(
      self->GetProcessorWithName(REACT_SHOULD_COMPONENT_UPDATE));

  bool enable_check_data_when_update_page =
      self->page_proxy()->GetEnableCheckDataWhenUpdatePage();
  page->SetEnableCheckDataWhenUpdatePage(enable_check_data_when_update_page);

  RETURN(lepus::Value(page));
}

RENDERER_FUNCTION_CC(CreateVirtualComponent) {
  CHECK_ARGC_GE(CreateVirtualComponent, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, Number, CreateVirtualComponent);
  CONVERT_ARG_AND_CHECK(arg1, 1, CPointer, CreateVirtualComponent);
  std::string component_name = "";
  if (ARGC() > 2) {
    CONVERT_ARG_AND_CHECK(arg2, 2, String, CreateVirtualComponent);
    component_name = arg2->StdString();
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CreateVirtualComponent", "componentName",
              component_name);
  int tid = static_cast<int>(arg0->Number());
  int component_instance_id = 0;
  if (ARGC() > 4) {
    CONVERT_ARG_AND_CHECK(arg4, 4, Number, CreateVirtualComponent);
    component_instance_id = static_cast<int>(arg4->Number());
  }

  lepus::Context* context = ctx;
  auto* self = GET_TASM_POINTER();
  auto cm_pair = self->FindComponentMould(context->name(), component_name, tid);
  ComponentMould* mould = cm_pair.first;
  auto& entry_name = cm_pair.second;

  auto component = new RadonComponent(self->page_proxy(), tid, nullptr,
                                      self->style_sheet_manager(entry_name),
                                      mould, context, component_instance_id);
  component->SetEntryName(entry_name);
  component->SetDSL(self->GetPageConfig()->GetDSL());
  if (ARGC() > 2) {
    component->SetName(std::move(component_name));
  }
  if (ARGC() > 3) {
    CONVERT_ARG_AND_CHECK(arg3, 3, String, CreateVirtualComponent);
    component->SetPath(arg3->String());
  } else {
    component->SetPath(mould->path());
  }

  auto globalProps = self->GetGlobalProps();
  if (!globalProps.IsNil()) {
    component->UpdateGlobalProps(globalProps);
  }

  if (component->GetDSL() == PackageInstanceDSL::REACT) {
    component->SetGetDerivedStateFromErrorProcessor(
        self->GetComponentProcessorWithName(component->path().str(),
                                            REACT_ERROR_PROCESS_LIFECYCLE,
                                            LEPUS_CONTEXT()->name()));
  }

  component->SetGetDerivedStateFromPropsProcessor(
      self->GetComponentProcessorWithName(component->path().str(),
                                          REACT_PRE_PROCESS_LIFECYCLE,
                                          LEPUS_CONTEXT()->name()));

  component->SetShouldComponentUpdateProcessor(
      self->GetComponentProcessorWithName(component->path().str(),
                                          REACT_SHOULD_COMPONENT_UPDATE,
                                          LEPUS_CONTEXT()->name()));
  UpdateComponentConfig(self, component);
  auto* base = static_cast<RadonBase*>(static_cast<RadonComponent*>(component));
  RETURN(lepus::Value(base));
}

RENDERER_FUNCTION_CC(AppendChild) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AppendChild");
  CHECK_ARGC_EQ(AppendChild, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, AppendChild);
  CONVERT_ARG_AND_CHECK(arg1, 1, CPointer, AppendChild);

  RadonBase* parent = reinterpret_cast<RadonBase*>(arg0->CPoint());
  RadonBase* child = reinterpret_cast<RadonBase*>(arg1->CPoint());
  parent->AddChild(std::unique_ptr<RadonBase>(child));
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AppendSubTree) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AppendSubTree");
  CHECK_ARGC_EQ(AppendChild, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, AppendSubTree);
  CONVERT_ARG_AND_CHECK(arg1, 1, CPointer, AppendSubTree);

  RadonBase* parent = reinterpret_cast<RadonBase*>(arg0->CPoint());
  RadonBase* sub_tree = reinterpret_cast<RadonBase*>(arg1->CPoint());
  parent->AddSubTree(std::unique_ptr<RadonBase>(sub_tree));
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(CloneSubTree) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CloneSubTree");
  CHECK_ARGC_EQ(CloneSubTree, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, CloneSubTree);

  auto* to_be_copied = reinterpret_cast<RadonBase*>(arg0->CPoint());
  auto* new_node = radon_factory::CopyRadonDiffSubTree(*to_be_copied);
  RETURN(lepus::Value(new_node));
}

RENDERER_FUNCTION_CC(SetAttributeTo) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "SetAttributeTo");
  CHECK_ARGC_EQ(SetAttributeTo, 3);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, SetAttributeTo);
  CONVERT_ARG_AND_CHECK(arg1, 1, String, SetAttributeTo);
  CONVERT_ARG(arg2, 2);
  auto key = arg1->String();
  lepus::Value value = CONVERT(arg2);
  auto* base = reinterpret_cast<RadonBase*>(arg0->CPoint());
  base->SetLynxKey(key, value);
  if (base->IsRadonNode()) {
    auto* node = static_cast<RadonNode*>(base);
    node->SetDynamicAttribute(key, value);
  }
  //  TODO: Handle UpdateContextData for radon-diff
  //  constexpr char kContextPrefix[] = "context-";
  //  constexpr size_t kContextPrefixSize = 8;
  //  std::string key_str = key.Get()->c_str();
  //
  //  if (base::BeginsWith(key_str, kContextPrefix)) {
  //    size_t len = key_str.length();
  //    std::string context_key =
  //        key_str.substr(kContextPrefixSize, len - kContextPrefixSize);
  //    if (node->IsVirtualPlug()) {
  //      ((VirtualPlug*)node)->UpdateContextData(context_key, value);
  //    } else {
  //      if (!node->IsVirtualComponent()) {
  //        node = node->component();
  //      }
  //      ((VirtualComponent*)node)->UpdateContextData(context_key, value);
  //    }
  //  } else {
  //    node->SetDynamicAttribute(key, value);
  //  }
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(SetContextData) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "SetContextData");
  //  TODO: Handle SetContextData for radon-diff
  //  CHECK_ARGC_EQ(SetContextData, 3);
  //  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, SetContextData);
  //  CONVERT_ARG_AND_CHECK(arg1, 1, String, SetContextData);
  //  CONVERT_ARG(arg2, 2);
  //
  //  VirtualNode* node = reinterpret_cast<VirtualNode*>(arg0->CPoint());
  //  auto key = arg1->String();
  //  lepus::Value value = CONVERT(arg2);
  //
  //  if (node->IsVirtualPlug()) {
  //    ((VirtualPlug*)node)->UpdateContextData(key, value);
  //  } else {
  //    if (!node->IsVirtualComponent()) {
  //      node = node->component();
  //    }
  //    ((VirtualComponent*)node)->UpdateContextData(key, value);
  //  }
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(SetStaticStyleTo2) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "SetStaticStyleTo2");
  CHECK_ARGC_EQ(SetStaticStyleTo2, 4);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, SetStaticStyleTo2);
  CONVERT_ARG_AND_CHECK(arg1, 1, Number, SetStaticStyleTo2);
  CONVERT_ARG_AND_CHECK(arg2, 2, Number, SetStaticStyleTo2);
  CONVERT_ARG(arg3, 3);

  CSSPropertyID id =
      static_cast<CSSPropertyID>(static_cast<int>(arg1->Number()));
  if (CSSProperty::IsPropertyValid(id)) {
    CSSValuePattern pattern =
        static_cast<CSSValuePattern>(static_cast<int>(arg2->Number()));
    auto value = CONVERT(arg3);
    reinterpret_cast<RadonNode*>(arg0->CPoint())
        ->SetStaticInlineStyle(id, tasm::CSSValue(value, pattern));
  } else {
    LynxWarning(false, error::E_CSS_UNKNOWN_PROPERTY,
                "Unknown css id: " + std::to_string(id));
  }
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(SetScriptEventTo) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "SetScriptEventTo");

  if (LEPUS_CONTEXT()->IsLepusContext()) {
    LOGI("SetScriptEventTo failed since context is lepus context.");
    RETURN_UNDEFINED();
  }

  DCHECK(ARGC() >= 4);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, SetScriptEventTo);
  CONVERT_ARG_AND_CHECK(arg1, 1, String, SetScriptEventTo);
  CONVERT_ARG_AND_CHECK(arg2, 2, String, SetScriptEventTo);
  CONVERT_ARG(arg3, 3);
  CONVERT_ARG(arg4, 4);

  auto type = arg1->String();
  auto name = arg2->String();

  reinterpret_cast<RadonNode*>(arg0->CPoint())
      ->SetLepusEvent(type, name, *arg3, *arg4);
  RETURN_UNDEFINED();
}

std::unique_ptr<ListComponentInfo> ComponentInfoFromContext(lepus::Context* ctx,
                                                            lepus::Value* argv,
                                                            int argc) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ComponentInfoFromContext");
  CONVERT_ARG(name, 1);
  CONVERT_ARG(data, 2);
  CONVERT_ARG(props, 3);
  CONVERT_ARG(ids, 4);
  CONVERT_ARG(style, 5);
  CONVERT_ARG(clazz, 6);
  CONVERT_ARG(event, 7);
  CONVERT_ARG(dataset, 8);

  lepus::Value comp_type = lepus::Value();
  if (ARGC() > 9) {
    CONVERT_ARG(arg9, 9);
    comp_type = *arg9;
  } else {
    static constexpr const char kDefault[] = "default";
    comp_type.SetString(BASE_STATIC_STRING(kDefault));
  }

  return std::make_unique<ListComponentInfo>(
      name->StdString(), LEPUS_CONTEXT()->name(), CONVERT(data), CONVERT(props),
      CONVERT(ids), CONVERT(style), CONVERT(clazz), CONVERT(event),
      CONVERT(dataset), CONVERT(&comp_type));
}

RENDERER_FUNCTION_CC(AppendListComponentInfo) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AppendListComponentInfo");
  CHECK_ARGC_GE(AppendListComponentInfo, 9);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, AppendListComponentInfo);
  auto radon_list =
      static_cast<RadonListBase*>(reinterpret_cast<RadonBase*>(arg0->CPoint()));
  ListNode* list = static_cast<ListNode*>(radon_list);

  auto info = ComponentInfoFromContext(ctx, argv, argc);
  list->AppendComponentInfo(std::move(info));
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(CreateVirtualPlug) {
  CHECK_ARGC_EQ(CreateVirtualPlug, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, String, CreateVirtualPlug);
  auto tag_name = arg0->String();
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CreateVirtualPlug", "tagName",
              tag_name.str());
  auto* plug = new RadonPlug(tag_name, nullptr);
  RETURN(lepus::Value(static_cast<RadonBase*>(plug)));
}

RENDERER_FUNCTION_CC(CreateVirtualPlugWithComponent) {
  CHECK_ARGC_EQ(CreateVirtualPlugWithComponent, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, String, CreateVirtualPlugWithComponent);
  CONVERT_ARG_AND_CHECK(arg1, 1, CPointer, CreateVirtualPlugWithComponent);
  auto tag_name = arg0->String();
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CreateVirtualPlugWithComponent", "tagName",
              tag_name.str());
  RadonComponent* comp = reinterpret_cast<RadonComponent*>(arg1->CPoint());
  auto* plug = new RadonPlug(tag_name, nullptr);
  plug->SetComponent(comp);
  RETURN(lepus::Value(static_cast<RadonBase*>(plug)));
}

RENDERER_FUNCTION_CC(MarkComponentHasRenderer) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "MarkComponentHasRenderer");
  CHECK_ARGC_EQ(MarkComponentHasRenderer, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, MarkComponentHasRenderer);
  // TODO(radon): radon diff support.
  // RadonComponent* component =
  //          reinterpret_cast<RadonComponent*>(arg0->CPoint());
  // component->MarkHasRendered();
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(SetStaticAttrTo) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "SetStaticAttrTo");
  CHECK_ARGC_EQ(SetStaticAttrTo, 3);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, SetStaticAttrTo);
  CONVERT_ARG_AND_CHECK(arg1, 1, String, SetStaticAttrTo);
  CONVERT_ARG(arg2, 2);
  auto key = arg1->String();
  auto value = CONVERT(arg2);
  auto* base = reinterpret_cast<RadonBase*>(arg0->CPoint());
  base->SetLynxKey(key, value);
  reinterpret_cast<RadonNode*>(arg0->CPoint())
      ->SetStaticAttribute(key, std::move(value));
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(SetStyleTo) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "SetStyleTo");
  CHECK_ARGC_EQ(SetStyleTo, 3);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, SetStyleTo);
  CONVERT_ARG(arg1, 1);
  CONVERT_ARG(arg2, 2);

  if (!arg1->IsString() && !arg1->IsNumber()) {
    RenderFatal(ctx, "SetStyleTo Params1 type error:%d",
                static_cast<int>(arg1->Type()));
  }

  CSSPropertyID id;
  if (arg1->IsString()) {
    id = CSSProperty::GetPropertyID(arg1->String());
  } else {
    id = static_cast<CSSPropertyID>(static_cast<int>(arg1->Number()));
  }
  if (!arg2->IsString()) {
    ElementAPIError("SetStyleTo %s Params2 type error",
                    CSSProperty::GetPropertyName(id).c_str());
    RETURN_UNDEFINED();
  }
  auto* tasm = GET_TASM_POINTER();
  auto value = arg2->String();
  if (CSSProperty::IsPropertyValid(id) && !value.empty()) {
    reinterpret_cast<RadonNode*>(arg0->CPoint())
        ->SetInlineStyle(id, std::move(value),
                         tasm->GetPageConfig()->GetCSSParserConfigs());
  }
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(SetDynamicStyleTo) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "SetDynamicStyleTo");
  CHECK_ARGC_EQ(SetDynamicStyleTo, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, SetDynamicStyleTo);
  CONVERT_ARG(arg1, 1);

  if (!arg1->IsString()) {
    RETURN_UNDEFINED();
  }
  auto style_value = arg1->ToString();
  auto splits = base::SplitStringByCharsOrderly<':', ';'>(style_value);
  auto* tasm = GET_TASM_POINTER();

  // Preview style pairs and calculate inline style map capacity.
  struct PredecodePair {
    CSSPropertyID id;
    std::string value;

    struct TraitID {
      static inline CSSPropertyID GetPropertyID(const PredecodePair& input) {
        return input.id;
      }
    };
  };
  size_t valid_count = 0;
  const auto count = splits.size() / 2;
  PredecodePair decode_values[count];
  for (size_t i = 0; i < count; ++i) {
    auto& pair = decode_values[valid_count];
    pair.id = CSSProperty::GetPropertyID(base::TrimString(splits[i * 2]));
    if (CSSProperty::IsPropertyValid(pair.id)) {
      pair.value = base::TrimString(splits[i * 2 + 1]);
      if (pair.value.length() > 0) {
        valid_count++;
      }
    }
  }

  auto radon_node = reinterpret_cast<RadonNode*>(arg0->CPoint());
  radon_node->PresetInlineStyleMapCapacity(
      CSSProperty::GetTotalParsedStyleCountFromArray(decode_values,
                                                     valid_count));
  const auto& parser_configs = tasm->GetPageConfig()->GetCSSParserConfigs();
  for (size_t i = 0; i < valid_count; ++i) {
    radon_node->SetInlineStyle(decode_values[i].id,
                               base::String(std::move(decode_values[i].value)),
                               parser_configs);
  }
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(SetStaticStyleTo) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "SetStaticStyleTo");
  CHECK_ARGC_EQ(SetStaticStyleTo, 3);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, SetStaticStyleTo);
  CONVERT_ARG(arg1, 1);
  CONVERT_ARG(arg2, 2);
  if (!arg2->IsString()) {
    RETURN_UNDEFINED();
  }
  CSSPropertyID id;
  if (arg1->IsString()) {
    id = CSSProperty::GetPropertyID(arg1->String());
  } else {
    id = static_cast<CSSPropertyID>(static_cast<int>(arg1->Number()));
  }
  auto value = arg2->String();
  auto* tasm = GET_TASM_POINTER();
  if (CSSProperty::IsPropertyValid(id) && !value.empty()) {
    reinterpret_cast<RadonNode*>(arg0->CPoint())
        ->SetStaticInlineStyle(id, std::move(value),
                               tasm->GetPageConfig()->GetCSSParserConfigs());
  }
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(SetDataSetTo) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "SetDataSetTo");
  CHECK_ARGC_EQ(SetDataSetTo, 3);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, SetDataSetTo);
  CONVERT_ARG_AND_CHECK(arg1, 1, String, SetDataSetTo);
  CONVERT_ARG(arg2, 2);

  auto key = arg1->String();
  auto value = CONVERT(arg2);
  reinterpret_cast<RadonNode*>(arg0->CPoint())->SetDataSet(key, value);
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(SetStaticEventTo) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "SetStaticEventTo");
  CHECK_ARGC_EQ(SetDataSetTo, 4);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, SetStaticEventTo);
  CONVERT_ARG_AND_CHECK(arg1, 1, String, SetStaticEventTo);
  CONVERT_ARG_AND_CHECK(arg2, 2, String, SetStaticEventTo);
  CONVERT_ARG_AND_CHECK(arg3, 3, String, SetStaticEventTo);

  auto type = arg1->String();
  auto name = arg2->String();
  auto value = arg3->String();
  reinterpret_cast<RadonNode*>(arg0->CPoint())
      ->SetStaticEvent(type, name, value);
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(SetClassTo) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "SetClassTo");
  CHECK_ARGC_EQ(SetClassTo, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, SetClassTo);
  CONVERT_ARG(arg1, 1);
  if (!arg1->IsString()) {
    RETURN_UNDEFINED();
  }

  auto clazz = arg1->String();
  if (clazz.empty()) RETURN_UNDEFINED();

  auto* radon_node = reinterpret_cast<RadonNode*>(arg0->CPoint());

  // Split and trimmed
  base::SplitString(clazz.str(), ' ', true,
                    [radon_node](const char* s, size_t length, int index) {
                      radon_node->SetClass(base::String(s, length));
                      return true;
                    });

  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(SetStaticClassTo) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "SetStaticClassTo");
  CHECK_ARGC_EQ(SetStaticClassTo, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, SetStaticClassTo);
  CONVERT_ARG_AND_CHECK(arg1, 1, String, SetStaticClassTo);

  auto clazz = arg1->String();
  if (!clazz.empty()) {
    reinterpret_cast<RadonNode*>(arg0->CPoint())->SetStaticClass(clazz);
  }
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(SetId) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "SetId");
  CHECK_ARGC_EQ(SetId, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, SetId);
  CONVERT_ARG(arg1, 1);

  // if arg1 is not a String, it will return empty string
  auto id = arg1->String();
  if (!id.empty()) {
    reinterpret_cast<RadonNode*>(arg0->CPoint())->SetIdSelector(id);
  }
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(UpdateComponentInfo) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "UpdateComponentInfo");
  CHECK_ARGC_EQ(UpdateComponentInfo, 4);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, UpdateComponentInfo);
  CONVERT_ARG_AND_CHECK(arg1, 1, String, UpdateComponentInfo);
  // CONVERT_ARG_AND_CHECK(arg2, 2, Array, UpdateComponentInfo);
  CONVERT_ARG(arg2, 2);
  CONVERT_ARG_AND_CHECK(arg3, 3, String, UpdateComponentInfo);
  auto* component_info_storage = GetRadonComponent(ctx, arg0);
  lepus::Value slot1 = DCONVERT(arg2);
  lepus::Value slot2 = DCONVERT(arg3);
  if (!slot1.IsArrayOrJSArray()) {
    RenderFatal(ctx, "UpdateComponentInfo: arg2 should be array");
  }

  if (component_info_storage) {
    auto key = arg1->String();
    component_info_storage->GetComponentInfoMap(ctx->name())
        .SetProperty(key, slot1);
    component_info_storage->GetComponentPathMap(ctx->name())
        .SetProperty(key, slot2);
  }
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(GetComponentInfo) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "GetComponentInfo");
  CHECK_ARGC_EQ(GetComponentInfo, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, GetComponentInfo);
  auto* component_info_storage = GetRadonComponent(ctx, arg0);
  if (!component_info_storage) {
    RETURN_UNDEFINED();
  }
  lepus::Value ret(component_info_storage->GetComponentInfoMap(ctx->name()));
  RETURN(ret);
}

RENDERER_FUNCTION_CC(CreateSlot) {
  CHECK_ARGC_EQ(CreateSlot, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, String, CreateSlot);

  auto tag_name = arg0->String();
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CreateSlot", "SlotName", tag_name.str());
  auto* slot = new RadonSlot(tag_name);
  RETURN(lepus::Value(slot));
}

RENDERER_FUNCTION_CC(SetProp) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "SetProp");
  CHECK_ARGC_EQ(SetProp, 3);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, SetProp);
  CONVERT_ARG_AND_CHECK(arg1, 1, String, SetProp);
  CONVERT_ARG(arg2, 2);

  base::String key = arg1->String();
  auto* component = GetRadonComponent(LEPUS_CONTEXT(), arg0);
  if (!component) {
    RETURN_UNDEFINED();
  }
  auto* tasm = GET_TASM_POINTER();
  // lynx-key and removeComponentElement shouldn't be a property.
  // So if lynx-key has been setted successfully, we shouldn't SetProperties
  // then.
  if (!component->SetSpecialComponentAttribute(key, *arg2)) {
    component->SetProperties(key, *arg2,
                             tasm->GetPageConfig()->GetStrictPropType());
  }
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(SetData) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "SetData");
  CHECK_ARGC_EQ(SetData, 3);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, SetData);
  CONVERT_ARG_AND_CHECK(arg1, 1, String, SetData);
  CONVERT_ARG(arg2, 2);
  auto* component = GetRadonComponent(LEPUS_CONTEXT(), arg0);
  if (component) {
    component->SetData(arg1->String(), *arg2);
  }
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AppendVirtualPlugToComponent) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AppendVirtualPlugToComponent");
  CHECK_ARGC_EQ(AppendVirtualPlugToComponent, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, AppendVirtualPlugToComponent);
  CONVERT_ARG_AND_CHECK(arg1, 1, CPointer, AppendVirtualPlugToComponent);
  auto* base = reinterpret_cast<RadonBase*>(arg0->CPoint());
  auto* component = static_cast<RadonComponent*>(base);
  base = reinterpret_cast<RadonBase*>(arg1->CPoint());
  auto* plug = static_cast<RadonPlug*>(base);
  plug->radon_component_ = component;
  component->AddRadonPlug(plug->plug_name(), std::unique_ptr<RadonBase>{plug});
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AddVirtualPlugToComponent) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AddVirtualPlugToComponent");
  CHECK_ARGC_EQ(AddVirtualPlugToComponent, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, AddVirtualPlugToComponent);
  CONVERT_ARG_AND_CHECK(arg1, 1, CPointer, AddVirtualPlugToComponent);

  auto* base = reinterpret_cast<RadonBase*>(arg0->CPoint());
  auto* component = static_cast<RadonComponent*>(base);
  base = reinterpret_cast<RadonBase*>(arg1->CPoint());
  auto* plug = static_cast<RadonPlug*>(base);
  plug->SetAttachedComponent(component);
  component->AddRadonPlug(plug->plug_name(), std::unique_ptr<RadonBase>{plug});
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AddFallbackToDynamicComponent) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AddFallbackToDynamicComponent");
  CHECK_ARGC_EQ(AddFallbackToDynamicComponent, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, AddFallbackToDynamicComponent);
  CONVERT_ARG_AND_CHECK(arg1, 1, CPointer, AddFallbackToDynamicComponent);

  auto* base = reinterpret_cast<RadonBase*>(arg0->CPoint());
  auto* component = static_cast<RadonLazyComponent*>(base);
  base = reinterpret_cast<RadonBase*>(arg1->CPoint());
  auto* plug = static_cast<RadonPlug*>(base);
  plug->SetAttachedComponent(component);
  component->AddFallback(std::unique_ptr<RadonPlug>{plug});
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(GetComponentData) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "GetComponentData");
  CHECK_ARGC_EQ(GetComponentData, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, GetComponentData);
  auto* component = GetRadonComponent(LEPUS_CONTEXT(), arg0);
  if (component) {
    RETURN(lepus::Value(component->GetData()));
  }
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(GetComponentProps) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "GetComponentProps");
  CHECK_ARGC_EQ(GetComponentProps, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, GetComponentProps);
  auto* component = GetRadonComponent(LEPUS_CONTEXT(), arg0);
  if (component) {
    RETURN(lepus::Value(component->GetProperties()));
  }
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(GetComponentContextData) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "GetComponentContextData");
  CHECK_ARGC_EQ(GetComponentContextData, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, GetComponentContextData);
  CONVERT_ARG_AND_CHECK(arg1, 1, String, GetComponentContextData);
  // TODO: Handle GetComponentContextData

  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(CreateComponentByName) {
  CHECK_ARGC_EQ(CreateComponentByName, 3);
  CONVERT_ARG_AND_CHECK(arg0, 0, String, CreateComponentByName);
  CONVERT_ARG_AND_CHECK(arg1, 1, CPointer, CreateComponentByName);
  CONVERT_ARG_AND_CHECK(arg2, 2, Number, CreateComponentByName);
  const std::string& component_name = arg0->StdString();
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CreateComponentByName", "componentName",
              component_name);
  int component_instance_id = static_cast<int>(arg2->Number());
  lepus::Context* context = LEPUS_CONTEXT();
  auto* self = GET_TASM_POINTER();
  auto iter = self->component_name_to_id(context->name()).find(component_name);
  DCHECK(iter != self->component_name_to_id(context->name()).end());
  int tid = iter->second;

  auto cm_pair = self->FindComponentMould(context->name(), component_name, tid);
  ComponentMould* mould = cm_pair.first;
  auto& entry_name = cm_pair.second;

  auto component = new RadonComponent(self->page_proxy(), tid, nullptr,
                                      self->style_sheet_manager(entry_name),
                                      mould, context, component_instance_id);
  component->SetEntryName(entry_name);
  component->SetDSL(self->GetPageConfig()->GetDSL());
  component->SetName(arg0->String());
  component->SetPath(mould->path());

  auto globalProps = self->GetGlobalProps();
  if (!globalProps.IsNil()) {
    component->UpdateGlobalProps(globalProps);
  }

  if (component->GetDSL() == PackageInstanceDSL::REACT) {
    component->SetGetDerivedStateFromErrorProcessor(
        self->GetComponentProcessorWithName(component->path().str(),
                                            REACT_ERROR_PROCESS_LIFECYCLE,
                                            LEPUS_CONTEXT()->name()));
  }

  component->SetGetDerivedStateFromPropsProcessor(
      self->GetComponentProcessorWithName(component->path().str(),
                                          REACT_PRE_PROCESS_LIFECYCLE,
                                          LEPUS_CONTEXT()->name()));
  component->SetShouldComponentUpdateProcessor(
      self->GetComponentProcessorWithName(component->path().str(),
                                          REACT_SHOULD_COMPONENT_UPDATE,
                                          LEPUS_CONTEXT()->name()));

  UpdateComponentConfig(self, component);
  auto* base = static_cast<RadonBase*>(static_cast<RadonComponent*>(component));
  RETURN(lepus::Value(base));
}

RENDERER_FUNCTION_CC(CreateDynamicVirtualComponent) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CreateDynamicVirtualComponent");
  // Get Params
  CHECK_ARGC_GE(CreateDynamicVirtualComponent, 4);
  CONVERT_ARG_AND_CHECK(arg0, 0, Number, CreateDynamicVirtualComponent);
  CONVERT_ARG_AND_CHECK(arg1, 1, CPointer, CreateDynamicVirtualComponent);
  CONVERT_ARG(arg2, 2);
  CONVERT_ARG_AND_CHECK(arg3, 3, Number, CreateDynamicVirtualComponent);

  int component_instance_id = static_cast<int>(arg3->Number());
  int tid = static_cast<int>(arg0->Number());
  auto* self = GET_TASM_POINTER();
  lepus::Context* context = LEPUS_CONTEXT();

  std::unique_ptr<RadonLazyComponent> comp = nullptr;
  std::shared_ptr<TemplateEntry> entry = nullptr;

  if (arg2->IsUndefined()) {
    // arg2 is undefined, means that, it is a js component, its entry name will
    // be determined by js.
    comp = std::make_unique<RadonLazyComponent>(self, "", self->page_proxy(),
                                                tid, component_instance_id);
    comp->MarkJSComponent();
  } else if (arg2->IsString()) {
    auto entry_name = arg2->String();
    const auto& url = self->GetTargetUrl(context->name(), entry_name.str());
    if (!url.empty()) {
      comp = std::make_unique<RadonLazyComponent>(self, url, self->page_proxy(),
                                                  tid, component_instance_id);
      comp->SetName(std::move(entry_name));
      entry = self->RequireTemplateEntry(comp.get(), url);
    }
  }

  if (!comp) {
    ElementAPIError(
        "The \"is\" property of dynamic component must be a "
        "non-empty string or undefined.");
    RETURN_UNDEFINED()
  }

  comp->SetDSL(self->GetPageConfig()->GetDSL());

  if (entry != nullptr) {
    auto cm_it = entry->lazy_bundle_moulds().find(0);
    if (cm_it != entry->lazy_bundle_moulds().end()) {
      DynamicComponentMould* cm = cm_it->second.get();
      auto context = entry->GetVm().get();
      comp->InitLazyComponent(nullptr, entry->GetStyleSheetManager(), cm,
                              context);
      comp->SetGlobalProps(self->GetGlobalProps());
      comp->SetPath(cm->path());
      if (comp->GetDSL() == PackageInstanceDSL::REACT) {
        comp->SetGetDerivedStateFromErrorProcessor(
            self->GetComponentProcessorWithName(comp->path().str(),
                                                REACT_ERROR_PROCESS_LIFECYCLE,
                                                context->name()));
      }

      comp->SetGetDerivedStateFromPropsProcessor(
          self->GetComponentProcessorWithName(comp->path().str(),
                                              REACT_PRE_PROCESS_LIFECYCLE,
                                              context->name()));
      comp->SetShouldComponentUpdateProcessor(
          self->GetComponentProcessorWithName(comp->path().str(),
                                              REACT_SHOULD_COMPONENT_UPDATE,
                                              context->name()));
    } else {
      // something wrong with dynamic component template.js, maybe
      // engineVersion has not been setted.
      ElementAPIError(
          "CreateDynamicVirtualComponent Failed, loadComponent Failed.");
      RETURN_UNDEFINED();
    }
  }

  auto* base = static_cast<RadonBase*>(comp.release());
  RETURN(lepus::Value(base));
}

RENDERER_FUNCTION_CC(ProcessComponentData) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ProcessComponentData");
  CHECK_ARGC_EQ(ProcessComponentData, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, ProcessComponentData);
  RadonComponent* component = reinterpret_cast<RadonComponent*>(arg0->CPoint());
  component->PreRenderForRadonComponent();
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(SetEventTo) {
  // TODO
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(RenderDynamicComponent) {
  CONVERT_ARG(arg0, 0);
  // arg0 must be a non-empty string
  if (!arg0->IsString()) {
    RETURN_UNDEFINED()
  }

  const auto& arg0_str = arg0->StdString();
  if (arg0_str.empty()) {
    RETURN_UNDEFINED();
  }

  const std::string& entry_name = arg0_str;

  CONVERT_ARG_AND_CHECK(arg2, 2, CPointer, RenderDynamicComponent);
  RadonLazyComponent* component = static_cast<RadonLazyComponent*>(
      reinterpret_cast<RadonBase*>(arg2->CPoint()));

  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LazyBundle::RenderEntrance",
              [entry_name](lynx::perfetto::EventContext ctx) {
                auto* debug = ctx.event()->add_debug_annotations();
                debug->set_name("entry_name");
                debug->set_string_value(entry_name);
              });

  if (component->IsEmpty()) {
    // For radon diff, component may be empty,
    // that means target context has not been load,
    // check this case before rendering
    RETURN_UNDEFINED()
  }

  CHECK_ARGC_EQ(RenderDynamicComponent, 6);

  CONVERT_ARG_AND_CHECK(arg1, 1, CPointer, RenderDynamicComponent);
  CONVERT_ARG(arg3, 3);
  CONVERT_ARG(arg4, 4);
  CONVERT_ARG(arg5, 5);

  auto* self = GET_TASM_POINTER();

  lepus::Context* context = LEPUS_CONTEXT();
  std::string url = self->GetTargetUrl(context->name(), entry_name);
  lepus::Context* target_context = self->context(url);
  BASE_STATIC_STRING_DECL(kRenderEntranceDynamicComponent,
                          "$renderEntranceDynamicComponent");
  target_context->Call(kRenderEntranceDynamicComponent, *arg2, *arg3, *arg4,
                       *arg5);
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(RegisterDataProcessor) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RegisterDataProcessor");
  DCHECK(ARGC() >= 2);
  DCHECK(ARGC() <= 4);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, RegisterDataProcessor);
  CONVERT_ARG_AND_CHECK(arg1, 1, Callable, RegisterDataProcessor);

  auto* self = GET_TASM_POINTER();
  if (ARGC() == 2) {
    // Default preprocessor
    self->SetDefaultProcessor(*arg1);
  } else if (ARGC() == 3) {
    CONVERT_ARG_AND_CHECK(arg2, 2, String, RegisterDataProcessor);
    self->SetProcessorWithName(*arg1, arg2->StdString());
  } else if (ARGC() == 4) {  // component 'getDerived'
    CONVERT_ARG_AND_CHECK(arg2, 2, String, RegisterDataProcessor);
    CONVERT_ARG_AND_CHECK(arg3, 3, String, RegisterDataProcessor);
    const auto& name = arg2->StdString();
    const auto& component_path = arg3->StdString();
    self->SetComponentProcessorWithName(*arg1, name, component_path,
                                        LEPUS_CONTEXT()->name());
  }
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AddEventListener) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AddEventListener");
  DCHECK(ARGC() == 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, String, RegisterDataProcessor);
  CONVERT_ARG_AND_CHECK(arg1, 1, Callable, RegisterDataProcessor);
  auto* tasm = GET_TASM_POINTER();
  tasm->SetLepusEventListener(arg0->StdString(), *arg1);
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(ReFlushPage) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ReFlushPage)");
  auto* tasm = GET_TASM_POINTER();
  tasm->ReFlushPage();
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(SetComponent) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "SetComponent");
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, SetComponent);
  CONVERT_ARG_AND_CHECK(arg1, 1, CPointer, SetComponent);

  auto* node =
      static_cast<RadonNode*>(reinterpret_cast<RadonBase*>(arg0->CPoint()));
  auto* component = static_cast<RadonComponent*>(
      reinterpret_cast<RadonBase*>(arg1->CPoint()));

  if (node != nullptr && component != nullptr) {
    node->SetComponent(component);
  }

  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(RegisterElementWorklet) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RegisterElementWorklet");

  if (LEPUS_CONTEXT()->IsLepusContext()) {
    LOGI("RegisterElementWorklet failed since context is lepus context.");
    RETURN_UNDEFINED();
  }

  // parameter size = 3
  // [0]  worklet Instance -> JSValue
  // [1]  worklet Module Name -> String
  // [2]  component Reference -> CPointer

  DCHECK(ARGC() >= 3);
  CONVERT_ARG_AND_CHECK(arg0, 0, Object, RegisterElementWorklet);
  CONVERT_ARG_AND_CHECK(arg1, 1, String, RegisterElementWorklet);
  CONVERT_ARG_AND_CHECK(arg2, 2, CPointer, RegisterElementWorklet);

  auto* base = reinterpret_cast<RadonBase*>(arg2->CPoint());
  auto* component = static_cast<RadonComponent*>(base);
  component->InsertWorklet(arg1->StdString(), *arg0);
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(CreateVirtualListNode) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CreateVirtualListNode");
  CHECK_ARGC_EQ(CreateVirtualListNode, 3);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, CreateVirtualListNode);
  CONVERT_ARG_AND_CHECK(arg1, 1, Number, CreateVirtualListNode);
  auto* self = GET_TASM_POINTER();
  lepus::Context* context = LEPUS_CONTEXT();
  uint32_t eid = static_cast<uint32_t>(arg1->Number());
  auto page_proxy = self->page_proxy();
  auto& manager = page_proxy->element_manager();
  if (manager->GetListNewArchitecture() ||
      manager->GetEnableNativeListFromShell() ||
      manager->GetEnableNativeListFromPageConfig()) {
    auto* list = new RadonDiffListNode2(context, page_proxy, self, eid);
    RETURN(lepus::Value(static_cast<RadonBase*>(list)));
  } else {
    auto* list = new RadonDiffListNode(context, page_proxy, self, eid);
    RETURN(lepus::Value(static_cast<RadonBase*>(list)));
  }
}

RENDERER_FUNCTION_CC(ThemedTranslation) {
  lepus::Value val = InnerTranslateResourceForTheme(ctx, argv, argc, nullptr);
  RETURN(val);
}

RENDERER_FUNCTION_CC(ThemedTranslationLegacy) {
  // FIXME: this function if to solve old version lynx had some mistaken when
  // register _sysTheme and _GetLazyLoadCount, if remove this function some
  // template compile with old version cli may not be able to use the theme
  // function
  // clang-format on
  CHECK_ARGC_GE(GetLazyLoadCount, 2);
  CONVERT_ARG(arg1, 1);
  if (arg1->IsString()) {
    return ThemedTranslation(ctx, argv, argc);
  }
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(ThemedLanguageTranslation) {
  lepus::Value val =
      InnerTranslateResourceForTheme(ctx, argv, argc, "language");
  RETURN(lepus::Value(val));
}

RENDERER_FUNCTION_CC(I18nResourceTranslation) {
  CHECK_ARGC_EQ(I18nResourceTranslation, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, Object, I18nResourceTranslation);
  auto* self = GET_TASM_POINTER();
  BASE_STATIC_STRING_DECL(kLocale, "locale");
  BASE_STATIC_STRING_DECL(kChannel, "channel");
  BASE_STATIC_STRING_DECL(kFallbackUrl, "fallback_url");
  lepus::Value locale = arg0->GetProperty(kLocale);
  lepus::Value channel = arg0->GetProperty(kChannel);
  lepus::Value fallback_url = arg0->GetProperty(kFallbackUrl);
  RETURN(self->GetI18nResources(locale, channel, fallback_url));
}

RENDERER_FUNCTION_CC(GetGlobalProps) {
  auto* self = GET_TASM_POINTER();
  auto global_props = self->GetGlobalProps();
  RETURN(global_props);
}

RENDERER_FUNCTION_CC(GetSystemInfo) {
  RETURN(GetSystemInfoFromTasm(GET_TASM_POINTER()));
}

RENDERER_FUNCTION_CC(HandleExceptionInLepus) {
  CHECK_ARGC_EQ(HandleExceptionInLepus, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, HandleExceptionInLepus);
  CONVERT_ARG_AND_CHECK(arg1, 1, Object, HandleExceptionInLepus);
  BASE_STATIC_STRING_DECL(kMessage, "message");
  lepus::Value msg = arg1->GetProperty(kMessage);
  LOGE("HandleExceptionInLepus: " << msg);
  auto* component = reinterpret_cast<RadonComponent*>(arg0->CPoint());
  auto* errorComponent =
      static_cast<RadonComponent*>(component->GetErrorBoundary());
  if (errorComponent != nullptr) {
    errorComponent->SetRenderError(*arg1);
  }
  RETURN_UNDEFINED();
}

// attach optimize information for i18n resource
RENDERER_FUNCTION_CC(FilterI18nResource) {
  CHECK_ARGC_EQ(FilterI18nResource, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, Object, FilterI18nResource);
  auto* self = GET_TASM_POINTER();
  BASE_STATIC_STRING_DECL(kChannel, "channel");
  BASE_STATIC_STRING_DECL(kLocale, "locale");
  BASE_STATIC_STRING_DECL(kReserveKeys, "reserveKeys");
  lepus::Value channel = arg0->GetProperty(kChannel);
  lepus::Value locale = arg0->GetProperty(kLocale);
  lepus::Value reserve_keys = arg0->GetProperty(kReserveKeys);
  self->FilterI18nResource(channel, locale, reserve_keys);
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(MarkPageElement) {
  CHECK_ARGC_EQ(MarkPageElement, 0);
  LOGI("MarkPageElement");
  auto* self = GET_TASM_POINTER();
  self->page_proxy()->SetPageElementEnabled(true);
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(SendGlobalEvent) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "SendGlobalEvent");
  auto* tasm = GET_TASM_POINTER();
  CHECK_ARGC_EQ(SendGlobalEvent, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, String, SendGlobalEvent);
  lepus::Value* arg1 = argv + 1;
  tasm->SendGlobalEvent(arg0->StdString(), *arg1);
  RETURN_UNDEFINED();
}

/* Element API BEGIN */
RENDERER_FUNCTION_CC(FiberCreateElement) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberCreateElement");
  // parameter size >= 2
  // [0] String -> element's tag
  // [1] Number -> parent component/page's unique id
  // [2] Object|Undefined -> optional info, not used now
  CHECK_ARGC_GE(FiberCreateElement, 2);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, String, FiberCreateElement);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, Number, FiberCreateElement);

  auto& manager = GET_TASM_POINTER()->page_proxy()->element_manager();
  auto element = manager->CreateFiberNode(arg0->String());
  element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(arg1->Number()));

  if (argc > 2) {
    CONVERT_ARG(arg2, 2);
    const auto& nid = arg2->GetProperty(BASE_STATIC_STRING(kNodeIndex));
    if (nid.IsNumber()) {
      element->SetNodeIndex(nid.Number());
    }
  }

  ON_NODE_CREATE(element);
  RETURN(lepus::Value(std::move(element)));
}

RENDERER_FUNCTION_CC(FiberCreatePage) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberCreatePage");

  // notify devtool page is updated.
  EXEC_EXPR_FOR_INSPECTOR(
      GET_TASM_POINTER()->page_proxy()->element_manager()->OnDocumentUpdated());

  // parameter size >= 2
  // [0] String -> componentID
  // [1] Number -> component/page's css fragment id
  // [2] Object|Undefined -> optional info, not used now
  CHECK_ARGC_GE(FiberCreatePage, 2);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, String, FiberCreatePage);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, Number, FiberCreatePage);

  auto* self = GET_TASM_POINTER();
  auto& manager = self->page_proxy()->element_manager();
  auto page = manager->CreateFiberPage(arg0->String(),
                                       static_cast<int32_t>(arg1->Number()));
  page->set_style_sheet_manager(
      self->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME));

  ON_NODE_CREATE(page);
  ON_NODE_ADDED(page);
  RETURN(lepus::Value(std::move(page)));
}

// __GetPageElement does not require any parameters and returns the current
// PageElement. If there is no PageElement, it returns null.
RENDERER_FUNCTION_CC(FiberGetPageElement) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberGetPageElement");
  auto* self = GET_TASM_POINTER();
  auto& manager = self->page_proxy()->element_manager();

  auto* root = manager->GetPageElement();
  if (root == nullptr) {
    RETURN_UNDEFINED();
  }
  RETURN(lepus::Value(fml::RefPtr<PageElement>(manager->GetPageElement())));
}

RENDERER_FUNCTION_CC(FiberCreateComponent) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberCreateComponent");
  // parameter size >= 6
  // [0] Number -> parent component/page's unique id
  // [1] String -> self's componentID
  // [2] Number -> component/page's css fragment id
  // [3] String -> entry name
  // [4] String -> component name
  // [5] String -> component path
  // [6] Object -> component config, not used now
  // [7] Object|Undefined -> optional info, not used now
  CHECK_ARGC_GE(FiberCreateComponent, 6);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, Number, FiberCreateComponent);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, String, FiberCreateComponent);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg2, 2, Number, FiberCreateComponent);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg3, 3, String, FiberCreateComponent);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg4, 4, String, FiberCreateComponent);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg5, 5, String, FiberCreateComponent);
  CONVERT_ARG(arg6, 6);

  auto* self = GET_TASM_POINTER();
  auto& manager = self->page_proxy()->element_manager();
  const auto& parent_component_unique_id = static_cast<int64_t>(arg0->Number());
  auto component_id = arg1->String();
  auto css_id = static_cast<int32_t>(arg2->Number());
  const auto& entry_name = arg3->StdString();
  auto name = arg4->String();
  auto path = arg5->String();

  const std::string& entry_name_str =
      entry_name.empty() ? tasm::DEFAULT_ENTRY_NAME : entry_name;

  auto component_element = manager->CreateFiberComponent(
      component_id, css_id, base::String(entry_name_str), name, path);
  component_element->SetParentComponentUniqueIdForFiber(
      parent_component_unique_id);
  component_element->set_style_sheet_manager(
      self->style_sheet_manager(entry_name_str));

  if (argc >= 7 && arg6->IsObject()) {
    if (arg6->GetProperty(BASE_STATIC_STRING(kRemoveComponentElement))
            .IsTrue()) {
      component_element->MarkAsWrapperComponent();
    }
    component_element->SetConfig(arg6->ToLepusValue());
  }

  ON_NODE_CREATE(component_element);
  RETURN(lepus::Value(std::move(component_element)));
}

RENDERER_FUNCTION_CC(FiberCreateView) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberCreateView");
  // parameter size >= 1
  // [0] Number -> parent component/page's unique id
  // [1] Object|Undefined -> optional info, not used now
  CHECK_ARGC_GE(FiberCreateView, 1);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, Number, FiberCreateView);

  auto& manager = GET_TASM_POINTER()->page_proxy()->element_manager();

  auto element = manager->CreateFiberView();
  element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(arg0->Number()));

  if (argc > 1) {
    CONVERT_ARG(arg1, 1);
    const auto& nid = arg1->GetProperty(BASE_STATIC_STRING(kNodeIndex));
    if (nid.IsNumber()) {
      element->SetNodeIndex(nid.Number());
    }
  }

  ON_NODE_CREATE(element);
  RETURN(lepus::Value(std::move(element)));
}

RENDERER_FUNCTION_CC(FiberCreateList) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberCreateList");
  // parameter size >= 3
  // [0] Number -> parent component/page's unique id
  // [1] Function -> componentAtIndex callback
  // [2] Function -> enqueueComponent callback
  // [3] Object|Undefined -> optional info, not used now
  // [4] Function -> componentAtIndexes callback
  CHECK_ARGC_GE(FiberCreateList, 3);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(parent_component_unique_id, 0, Number,
                                        FiberCreateList);
  CONVERT_ARG(component_at_index, 1);
  CONVERT_ARG(enqueue_component, 2);

  BASE_STATIC_STRING_DECL(kTag, "tag");

  static constexpr const char kList[] = "list";
  base::String tag = BASE_STATIC_STRING(kList);

  if (argc > 3) {
    CONVERT_ARG(arg3, 3);
    const auto& custom_tag = arg3->GetProperty(kTag);
    if (custom_tag.IsString()) {
      tag = custom_tag.String();
    }
  }

  lepus::Value component_at_indexes;
  if (argc > 4) {
    CONVERT_ARG(arg4, 4);
    component_at_indexes = *arg4;
  }

  auto* self = GET_TASM_POINTER();
  auto& manager = self->page_proxy()->element_manager();
  auto element = manager->CreateFiberList(
      self, tag, *component_at_index, *enqueue_component, component_at_indexes);
  element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(parent_component_unique_id->Number()));

  ON_NODE_CREATE(element);
  RETURN(lepus::Value(std::move(element)));
}

RENDERER_FUNCTION_CC(FiberCreateScrollView) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberCreateScrollView");
  // parameter size >= 1
  // [0] Number -> parent component/page's unique id
  // [1] Object|Undefined -> optional info, not used now
  CHECK_ARGC_GE(FiberCreateScrollView, 1);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, Number, FiberCreateScrollView);

  BASE_STATIC_STRING_DECL(kTag, "tag");

  static constexpr const char kScrollView[] = "scroll-view";
  base::String tag = BASE_STATIC_STRING(kScrollView);

  auto& manager = GET_TASM_POINTER()->page_proxy()->element_manager();

  uint32_t node_index = 0;
  if (argc > 1) {
    CONVERT_ARG(arg1, 1);
    const auto& custom_tag = arg1->GetProperty(kTag);
    if (custom_tag.IsString()) {
      tag = custom_tag.String();
    }
    const auto& nid = arg1->GetProperty(BASE_STATIC_STRING(kNodeIndex));
    if (nid.IsNumber()) {
      node_index = nid.Number();
    }
  }
  auto element = manager->CreateFiberScrollView(tag);

  element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(arg0->Number()));
  element->SetNodeIndex(node_index);

  ON_NODE_CREATE(element);
  RETURN(lepus::Value(std::move(element)));
}

RENDERER_FUNCTION_CC(FiberCreateText) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberCreateText");
  // parameter size >= 1
  // [0] Number -> parent component/page's unique id
  // [1] Object|Undefined -> optional info, not used now
  CHECK_ARGC_GE(FiberCreateText, 1);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, Number, FiberCreateText);

  BASE_STATIC_STRING_DECL(kTag, "tag");

  base::String tag = BASE_STATIC_STRING(kElementTextTag);

  auto& manager = GET_TASM_POINTER()->page_proxy()->element_manager();

  uint32_t node_index = 0;
  if (argc > 1) {
    CONVERT_ARG(arg1, 1);
    const auto& custom_tag = arg1->GetProperty(kTag);
    if (custom_tag.IsString()) {
      tag = custom_tag.String();
    }
    const auto& nid = arg1->GetProperty(BASE_STATIC_STRING(kNodeIndex));
    if (nid.IsNumber()) {
      node_index = nid.Number();
    }
  }
  auto element = manager->CreateFiberText(tag);
  element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(arg0->Number()));
  element->SetNodeIndex(node_index);

  ON_NODE_CREATE(element);
  RETURN(lepus::Value(std::move(element)));
}

RENDERER_FUNCTION_CC(FiberCreateImage) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberCreateImage");
  // parameter size >= 1
  // [0] Number -> parent component/page's unique id
  // [1] Object|Undefined -> optional info, not used now
  CHECK_ARGC_GE(FiberCreateImage, 1);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, Number, FiberCreateImage);

  BASE_STATIC_STRING_DECL(kTag, "tag");

  base::String tag = BASE_STATIC_STRING(kElementImageTag);

  auto& manager = GET_TASM_POINTER()->page_proxy()->element_manager();

  uint32_t node_index = 0;
  if (argc > 1) {
    CONVERT_ARG(arg1, 1);
    const auto& custom_tag = arg1->GetProperty(kTag);
    if (custom_tag.IsString()) {
      tag = custom_tag.String();
    }
    const auto& nid = arg1->GetProperty(BASE_STATIC_STRING(kNodeIndex));
    if (nid.IsNumber()) {
      node_index = nid.Number();
    }
  }
  auto element = manager->CreateFiberImage(tag);
  element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(arg0->Number()));
  element->SetNodeIndex(node_index);

  ON_NODE_CREATE(element);
  RETURN(lepus::Value(std::move(element)));
}

RENDERER_FUNCTION_CC(FiberCreateRawText) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberCreateRawText");
  // parameter size >= 1
  // [0] String -> raw text's content
  // [1] Object|Undefined -> optional info, not used now
  CHECK_ARGC_GE(FiberCreateRawText, 1);
  CONVERT_ARG(content, 0);

  auto& manager = GET_TASM_POINTER()->page_proxy()->element_manager();

  auto element = manager->CreateFiberRawText();
  element->SetText(content->ToLepusValue());

  ON_NODE_CREATE(element);
  RETURN(lepus::Value(std::move(element)));
}

RENDERER_FUNCTION_CC(FiberCreateIf) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberCreateIf");
  // parameter size >= 1
  // [0] Number -> parent component/page's unique id
  // [1] Object|Undefined -> optional info, not used now
  CHECK_ARGC_GE(FiberCreateIf, 1);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, Number, FiberCreateIf);
  auto& manager = GET_TASM_POINTER()->page_proxy()->element_manager();

  BASE_STATIC_STRING_DECL(tag, "if");
  auto element = fml::AdoptRef<IfElement>(new IfElement(manager.get(), tag));
  element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(arg0->Number()));

  ON_NODE_CREATE(element);
  RETURN(lepus::Value(std::move(element)));
}

RENDERER_FUNCTION_CC(FiberCreateFor) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberCreateFor");
  // parameter size >= 1
  // [0] Number -> parent component/page's unique id
  // [1] Object|Undefined -> optional info, not used now
  CHECK_ARGC_GE(FiberCreateFor, 1);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, Number, FiberCreateFor);

  auto& manager = GET_TASM_POINTER()->page_proxy()->element_manager();

  BASE_STATIC_STRING_DECL(tag, "for");
  auto element = fml::AdoptRef<ForElement>(new ForElement(manager.get(), tag));
  element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(arg0->Number()));

  ON_NODE_CREATE(element);
  RETURN(lepus::Value(std::move(element)));
}

RENDERER_FUNCTION_CC(FiberCreateBlock) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberCreateBlock");
  // parameter size >= 1
  // [0] Number -> parent component/page's unique id
  // [1] Object|Undefined -> optional info, not used now
  CHECK_ARGC_GE(FiberCreateBlock, 1);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, Number, FiberCreateBlock);

  auto& manager = GET_TASM_POINTER()->page_proxy()->element_manager();

  BASE_STATIC_STRING_DECL(tag, "block");
  auto element =
      fml::AdoptRef<BlockElement>(new BlockElement(manager.get(), tag));
  element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(arg0->Number()));

  ON_NODE_CREATE(element);
  RETURN(lepus::Value(std::move(element)));
}

RENDERER_FUNCTION_CC(FiberAddConfig) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberAddConfig");
  // parameter size = 3
  // [0] RefCounted -> element
  // [1] String -> key
  // [2] any -> value
  CHECK_ARGC_GE(FiberAddConfig, 3);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted, FiberAddConfig);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, String, FiberAddConfig);
  CONVERT_ARG(arg2, 2);

  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  element->AddConfig(arg1->String(), arg2->ToLepusValue());

  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberSetConfig) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberSetConfig");
  // parameter size = 2
  // [0] RefCounted -> element
  // [1] any -> value
  CHECK_ARGC_GE(FiberSetConfig, 2);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted, FiberSetConfig);
  CONVERT_ARG(arg1, 1);

  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  if (arg1->IsObject()) {
    element->SetConfig(arg1->ToLepusValue());
  }

  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberCreateNonElement) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberCreateNonElement");
  // parameter size >= 1
  // [0] Number -> parent component/page's unique id
  // [1] Object|Undefined -> optional info, not used now
  CHECK_ARGC_GE(FiberCreateImage, 1);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, Number, FiberCreateNonElement);

  auto& manager = GET_TASM_POINTER()->page_proxy()->element_manager();

  auto element = manager->CreateFiberNoneElement();
  element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(arg0->Number()));

  ON_NODE_CREATE(element);
  RETURN(lepus::Value(std::move(element)));
}

RENDERER_FUNCTION_CC(FiberCreateWrapperElement) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberCreateWrapperElement");
  // parameter size >= 1
  // [0] Number -> parent component/page's unique id
  // [1] Object|Undefined -> optional info, not used now
  CHECK_ARGC_GE(FiberCreateImage, 1);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, Number,
                                        FiberCreateWrapperElement);

  auto& manager = GET_TASM_POINTER()->page_proxy()->element_manager();

  auto element = manager->CreateFiberWrapperElement();
  element->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(arg0->Number()));

  if (argc > 1) {
    CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, Object,
                                          FiberCreateWrapperElement);
    BASE_STATIC_STRING_DECL(kType, "type");
    const auto& wrapper_type = arg1->GetProperty(kType);
    if (wrapper_type.IsNumber()) {
      element->SetWrapperType(
          static_cast<WrapperElement::Type>(wrapper_type.Number()));
    }
  }

  ON_NODE_CREATE(element);
  RETURN(lepus::Value(std::move(element)));
}

RENDERER_FUNCTION_CC(FiberAppendElement) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberAppendElement");
  // parameter size = 2
  // [0] RefCounted -> parent element
  // [1] RefCounted -> child element
  CHECK_ARGC_GE(FiberAppendElement, 2);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted,
                                        FiberAppendElement);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, RefCounted,
                                        FiberAppendElement);
  auto parent = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  auto child = fml::static_ref_ptr_cast<FiberElement>(arg1->RefCounted());
  parent->InsertNode(child);

  ON_NODE_ADDED(child);
  RETURN(lepus::Value(std::move(child)));
}

RENDERER_FUNCTION_CC(FiberRemoveElement) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberRemoveElement");
  // parameter size = 2
  // [0] RefCounted -> parent element
  // [1] RefCounted -> child element
  CHECK_ARGC_GE(FiberRemoveElement, 2);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted,
                                        FiberRemoveElement);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, RefCounted,
                                        FiberRemoveElement);
  auto parent = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  auto child = fml::static_ref_ptr_cast<FiberElement>(arg1->RefCounted());

  // make sure to notify DevTool child removed before RemoveNode
  ON_NODE_REMOVED(child);

  parent->RemoveNode(child);

  RETURN(lepus::Value(std::move(child)));
}

RENDERER_FUNCTION_CC(FiberInsertElementBefore) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberInsertElementBefore");
  // parameter size = 3
  // [0] RefCounted -> parent element
  // [1] RefCounted -> child element
  // [2] RefCounted|Number|null|Undefined -> ref element
  CHECK_ARGC_GE(FiberInsertElementBefore, 3);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted,
                                        FiberInsertElementBefore);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, RefCounted,
                                        FiberInsertElementBefore);
  CONVERT_ARG(arg2, 2)
  auto parent = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  auto child = fml::static_ref_ptr_cast<FiberElement>(arg1->RefCounted());
  if (arg2->IsRefCounted()) {
    auto ref = fml::static_ref_ptr_cast<FiberElement>(arg2->RefCounted());
    parent->InsertNodeBefore(child, ref);
  } else {
    parent->InsertNode(child);
  }

  ON_NODE_ADDED(child);
  RETURN(lepus::Value(std::move(child)));
}

RENDERER_FUNCTION_CC(FiberFirstElement) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberFirstElement");
  // parameter size = 1
  // [0] RefCounted -> parent element
  CHECK_ARGC_GE(FiberFirstElement, 1);
  CONVERT_ARG(arg0, 0);
  if (!arg0->IsRefCounted()) {
    RETURN_UNDEFINED();
  }

  constexpr const static int32_t kFirstElementIndex = 0;

  auto parent = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  auto child =
      static_cast<FiberElement*>(parent->GetChildAt(kFirstElementIndex));
  if (child == nullptr) {
    RETURN_UNDEFINED();
  }

  RETURN(lepus::Value(fml::RefPtr<FiberElement>(child)));
}

RENDERER_FUNCTION_CC(FiberLastElement) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberLastElement");
  // parameter size = 1
  // [0] RefCounted -> parent element
  CHECK_ARGC_GE(FiberLastElement, 1);
  CONVERT_ARG(arg0, 0);
  if (!arg0->IsRefCounted()) {
    RETURN_UNDEFINED();
  }

  auto parent = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  if (parent->GetChildCount() == 0) {
    RETURN_UNDEFINED();
  }
  auto child = static_cast<FiberElement*>(
      parent->GetChildAt(parent->GetChildCount() - 1));
  if (child == nullptr) {
    RETURN_UNDEFINED();
  }
  RETURN(lepus::Value(fml::RefPtr<FiberElement>(child)));
}

RENDERER_FUNCTION_CC(FiberNextElement) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberNextElement");
  // parameter size = 1
  // [0] RefCounted -> element
  CHECK_ARGC_GE(FiberNextElement, 1);
  CONVERT_ARG(arg0, 0);
  if (!arg0->IsRefCounted()) {
    RETURN_UNDEFINED();
  }

  auto* element =
      fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted()).get();
  auto* parent = static_cast<FiberElement*>(element->parent());

  if (parent == nullptr) {
    RETURN_UNDEFINED()
  }

  FiberElement* next = static_cast<FiberElement*>(element->next_sibling());
  if (next == nullptr) {
    RETURN_UNDEFINED();
  }

  RETURN(lepus::Value(fml::RefPtr<FiberElement>(next)));
}

RENDERER_FUNCTION_CC(FiberAsyncResolveElement) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberAsyncResolveElement");
  // parameter size = 1
  // [0] RefCounted -> element to be async resolved
  // [return] undefined
  CHECK_ARGC_EQ(FiberAsyncResolveElement, 1);
  CONVERT_ARG(arg0, 0);
  if (!arg0->IsRefCounted()) {
    RETURN_UNDEFINED();
  }

  auto* element =
      fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted()).get();
  CHECK_ILLEGAL_ATTRIBUTE_CONFIG(element, FiberAsyncResolveElement);
  element->AsyncResolveProperty();

  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberReplaceElement) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberReplaceElement");
  // parameter size = 2
  // [0] RefCounted -> new element
  // [1] RefCounted -> old element
  // [return] undefined
  CHECK_ARGC_GE(FiberReplaceElement, 2);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted,
                                        FiberReplaceElement);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, RefCounted,
                                        FiberReplaceElement);

  auto new_element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  auto old_element = fml::static_ref_ptr_cast<FiberElement>(arg1->RefCounted());

  // if new element == old element, return
  if (new_element->impl_id() == old_element->impl_id()) {
    LOGI("FiberReplaceElement parameters are the same, return directly.");
    RETURN_UNDEFINED();
  }

  auto* parent = static_cast<FiberElement*>(old_element->parent());
  if (parent == nullptr) {
    LOGE("FiberReplaceElement failed since parent is null.");
    RETURN_UNDEFINED();
  }

  parent->InsertNodeBefore(new_element, old_element);

  ON_NODE_ADDED(new_element);
  ON_NODE_REMOVED(old_element);

  parent->RemoveNode(old_element);

  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberReplaceElements) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberReplaceElements");
  // parameter size = 3
  // [0] RefCounted -> parent
  // [0] RefCounted | Array | Null -> new element
  // [1] RefCounted | Array | Null -> old element
  // [return] undefined
  CHECK_ARGC_GE(FiberReplaceElements, 2);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted,
                                        FiberReplaceElements);
  CONVERT_ARG(arg1, 1);
  CONVERT_ARG(arg2, 2);

  // Get parent
  auto* parent =
      fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted()).get();
  if (parent == nullptr) {
    LOGE("FiberReplaceElements failed since parent is null.");
    RETURN_UNDEFINED();
  }

  // Get inserted elements.
  std::deque<fml::RefPtr<FiberElement>> inserted_elements{};
  if (arg1->IsRefCounted()) {
    inserted_elements.emplace_back(
        fml::static_ref_ptr_cast<FiberElement>(arg1->RefCounted()));
  } else if (arg1->IsArrayOrJSArray()) {
    tasm::ForEachLepusValue(
        *arg1, [&inserted_elements](const auto& index, const auto& value) {
          if (value.IsRefCounted()) {
            inserted_elements.emplace_back(
                fml::static_ref_ptr_cast<FiberElement>(value.RefCounted()));
          }
        });
  }

  // Get removed elements.
  std::deque<fml::RefPtr<FiberElement>> removed_elements{};
  if (arg2->IsRefCounted()) {
    removed_elements.emplace_back(
        fml::static_ref_ptr_cast<FiberElement>(arg2->RefCounted()));
  } else if (arg2->IsArrayOrJSArray()) {
    tasm::ForEachLepusValue(*arg2, [&removed_elements](const auto& index,
                                                       const auto& value) {
      if (value.IsRefCounted()) {
        removed_elements.emplace_back(
            fml::static_ref_ptr_cast<FiberElement>(value.RefCounted()).get());
      }
    });
  }

  // Perform a simple diff on the inserted_elements and removed_elements,
  // removing each element one by one until either inserted_elements
  // orremoved_elements are empty or the elements are not the same. Same applies
  // to the tail end.

  // need to determine the ref node: Get ref = remove.back.next_sibling
  // ref node is nullptr means to append to the end
  auto* last_old_element =
      !removed_elements.empty() ? removed_elements.back().get() : nullptr;
  auto* ref = last_old_element
                  ? static_cast<FiberElement*>(last_old_element->next_sibling())
                  : nullptr;

  EXEC_EXPR_FOR_INSPECTOR({
    for (const auto& child : removed_elements) {
      ON_NODE_REMOVED(child);
    }
  });

  parent->ReplaceElements(inserted_elements, removed_elements, ref);

  EXEC_EXPR_FOR_INSPECTOR({
    for (const auto& child : inserted_elements) {
      ON_NODE_ADDED(child);
    }
  });
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberSwapElement) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberSwapElement");
  // parameter size = 2
  // [0] RefCounted -> left element
  // [1] RefCounted -> right element
  // [return] undefined
  CHECK_ARGC_GE(FiberSwapElement, 2);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted, FiberSwapElement);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, RefCounted, FiberSwapElement);

  auto left_element =
      fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  auto right_element =
      fml::static_ref_ptr_cast<FiberElement>(arg1->RefCounted());

  auto* left_parent = static_cast<FiberElement*>(left_element->parent());
  if (left_parent == nullptr) {
    LOGE("FiberSwapElement failed since left parent is null.");
    RETURN_UNDEFINED();
  }

  auto* right_parent = static_cast<FiberElement*>(right_element->parent());
  if (right_parent == nullptr) {
    LOGE("FiberSwapElement failed since right parent is null.");
    RETURN_UNDEFINED();
  }

  auto left_index = left_parent->IndexOf(left_element.get());
  auto right_index = right_parent->IndexOf(right_element.get());

  ON_NODE_REMOVED(left_element);
  left_parent->RemoveNode(left_element);

  ON_NODE_REMOVED(right_element);
  right_parent->RemoveNode(right_element);

  // TODO(linxs): opt this logic.
  if (right_index < left_index) {
    right_parent->InsertNode(left_element, right_index);
    left_parent->InsertNode(right_element, left_index);
  } else {
    left_parent->InsertNode(right_element, left_index);
    right_parent->InsertNode(left_element, right_index);
  }

  ON_NODE_ADDED(left_element);
  ON_NODE_ADDED(right_element);
  RETURN_UNDEFINED();
}

// This function accepts only one parameter, the 0th is the element. The return
// value is the element's parent.
RENDERER_FUNCTION_CC(FiberGetParent) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberGetParent");
  // parameter size = 1
  // [0] RefCounted -> element
  CHECK_ARGC_GE(FiberGetParent, 1);
  CONVERT_ARG(arg0, 0);
  if (!arg0->IsRefCounted()) {
    RETURN_UNDEFINED();
  }

  auto child = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  auto parent = static_cast<FiberElement*>(child->parent());
  if (parent == nullptr) {
    RETURN_UNDEFINED();
  }

  RETURN(lepus::Value(fml::RefPtr<FiberElement>(parent)));
}

RENDERER_FUNCTION_CC(FiberGetChildren) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberGetChildren");
  // parameter size = 1
  // [0] RefCounted -> parent element
  CHECK_ARGC_GE(FiberGetChildren, 1);
  CONVERT_ARG(arg0, 0);
  if (!arg0->IsRefCounted()) {
    RETURN_UNDEFINED();
  }

  auto parent = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());

  auto ary = lepus::CArray::Create();
  const auto& children = parent->children();
  for (const auto& c : children) {
    ary->emplace_back(c);
  }
  RETURN(lepus::Value(std::move(ary)));
}

RENDERER_FUNCTION_CC(FiberIsTemplateElement) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberIsTemplateElement");
  // parameter size = 1
  // [0] RefCounted -> element
  CHECK_ARGC_GE(FiberIsTemplateElement, 1);
  CONVERT_ARG(arg0, 0);
  if (!arg0->IsRefCounted()) {
    RETURN(lepus::Value(false));
  }
  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  RETURN(lepus::Value(element->IsTemplateElement()));
}

RENDERER_FUNCTION_CC(FiberIsPartElement) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberIsPartElement");
  // parameter size = 1
  // [0] RefCounted -> element
  CHECK_ARGC_GE(FiberIsPartElement, 1);
  CONVERT_ARG(arg0, 0);
  if (!arg0->IsRefCounted()) {
    RETURN(lepus::Value(false));
  }
  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  RETURN(lepus::Value(element->IsPartElement()));
}

RENDERER_FUNCTION_CC(FiberMarkTemplateElement) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberMarkTemplateElement");
  // parameter size = 1
  // [0] RefCounted -> element
  CHECK_ARGC_GE(FiberMarkTemplateElement, 1);
  CONVERT_ARG(arg0, 0);
  if (arg0->IsRefCounted()) {
    auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
    element->MarkTemplateElement();
  }
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberMarkPartElement) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberMarkPartElement");
  // parameter size = 2
  // [0] RefCounted -> element
  // [1] string -> id
  CHECK_ARGC_GE(FiberMarkPartElement, 2);
  CONVERT_ARG(arg0, 0);
  CONVERT_ARG(arg1, 1);
  if (arg0->IsRefCounted() && arg1->IsString()) {
    auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
    element->MarkPartElement(arg1->String());
  }
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberGetTemplateParts) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberGetTemplateParts");
  // parameter size = 1
  // [0] RefCounted -> element
  CHECK_ARGC_GE(FiberGetTemplateParts, 1);
  CONVERT_ARG(arg0, 0);
  if (!arg0->IsRefCounted()) {
    RETURN_UNDEFINED();
  }
  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  if (!element->IsTemplateElement()) {
    RETURN_UNDEFINED();
  }
  auto parts_map = TreeResolver::GetTemplateParts(element);
  RETURN(lepus::Value(std::move(parts_map)));
}

RENDERER_FUNCTION_CC(FiberCloneElement) {
  // parameter size = 2
  // [0] RefCounted -> element
  // [1] Object | Undefined | Null -> options
  CHECK_ARGC_GE(FiberCloneElement, 1);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted, FiberCloneElement);

  TreeResolver::CloningDepth depth = TreeResolver::CloningDepth::kSingle;
  bool clone_resolved_props = true;
  if (argc > 1) {
    CONVERT_ARG(arg1, 1);
    if (arg1->IsObject()) {
      auto sDepth = BASE_STATIC_STRING(kDepth);
      auto sCloneResolvedProps = BASE_STATIC_STRING(kCloneResolvedProps);

      lepus::Value maybe_depth = arg1->GetProperty(sDepth);
      if (maybe_depth.IsNumber()) {
        depth = static_cast<TreeResolver::CloningDepth>(
            static_cast<uint32_t>(maybe_depth.Number()));
      }
      lepus::Value maybe_clone_resolved_props =
          arg1->GetProperty(sCloneResolvedProps);
      if (maybe_clone_resolved_props.IsBool()) {
        clone_resolved_props = maybe_clone_resolved_props.Bool();
      }
    }
  }

  auto* self = GET_TASM_POINTER();
  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());

  if (clone_resolved_props && element->flush_required() &&
      element->IsAttached()) {
    element->FlushActionsAsRoot();
  }

  const std::shared_ptr<CSSStyleSheetManager>& style_sheet_manager =
      self->style_sheet_manager(DEFAULT_ENTRY_NAME);
  return lepus::Value(TreeResolver::CloneElements(element, style_sheet_manager,
                                                  clone_resolved_props, depth));
}

RENDERER_FUNCTION_CC(FiberElementIsEqual) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElementIsEqual");
  // parameter size = 2
  // [0] RefCounted -> left element
  // [1] RefCounted -> right element
  CHECK_ARGC_GE(FiberElementIsEqual, 2);
  CONVERT_ARG(arg0, 0)
  CONVERT_ARG(arg1, 1)

  if (!arg0->RefCounted() || !arg1->RefCounted()) {
    return lepus::Value(false);
  }

  auto left = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted()).get();
  auto right = fml::static_ref_ptr_cast<FiberElement>(arg1->RefCounted()).get();
  RETURN(lepus::Value(left == right));
}

RENDERER_FUNCTION_CC(FiberGetElementUniqueID) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberGetElementUniqueID");
  // parameter size = 1
  // [0] RefCounted -> element
  CHECK_ARGC_GE(FiberGetElementUniqueID, 1);
  CONVERT_ARG(arg0, 0);
  int64_t unique_id = -1;
  if (arg0->IsRefCounted()) {
    auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
    unique_id = element->impl_id();
  }
  RETURN(lepus::Value(unique_id));
}

RENDERER_FUNCTION_CC(FiberGetTag) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberGetTag");
  // parameter size = 1
  // [0] RefCounted -> element
  CHECK_ARGC_GE(FiberGetTag, 1);
  CONVERT_ARG(arg0, 0);
  if (!arg0->IsRefCounted()) {
    RETURN_UNDEFINED();
  }

  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  RETURN(lepus::Value(element->GetTag()));
}

RENDERER_FUNCTION_CC(FiberSetAttribute) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberSetAttribute");
  // parameter size = 3
  // [0] RefCounted -> element
  // [1] String/Number -> key
  // [2] any -> value
  CHECK_ARGC_GE(FiberSetAttribute, 3);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted, FiberSetAttribute);
  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  CONVERT_ARG(arg1, 1);
  CONVERT_ARG(arg2, 2);
  uint32_t type = static_cast<uint32_t>(arg1->Number());
  if (type == 0) {
    auto string_type = arg1->StringView();
    if (string_type.empty()) {
      return RenderFatal(LEPUS_CONTEXT(), "bad type");
    }
    CHECK_ILLEGAL_ATTRIBUTE_CONFIG(element, FiberSetAttribute);
    element->SetAttribute(arg1->String(), arg2->ToLepusValue());
  } else {
    auto key = static_cast<ElementBuiltInAttributeEnum>(type);
    element->SetBuiltinAttribute(key, *arg2);
  }
  ON_NODE_MODIFIED(element);
  RETURN_UNDEFINED();
}

// __GetAttributeByName accepts two parameters, the first parameter is the
// element, and the second parameter is a string, which is the attribute key. It
// returns the value corresponding to this attribute key, if there is no
// corresponding attribute, it returns null.
RENDERER_FUNCTION_CC(FiberGetAttributeByName) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberGetAttributeByName");
  // parameter size = 2
  // [0] RefCounted -> element
  // [1] String -> key
  CHECK_ARGC_GE(FiberGetAttributeByName, 2);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted,
                                        FiberGetAttributeByName);
  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  CONVERT_ARG(arg1, 1);
  uint32_t type = static_cast<uint32_t>(arg1->Number());
  if (type == 0) {
    auto string_type = arg1->StringView();
    if (string_type.empty()) {
      return RenderFatal(LEPUS_CONTEXT(), "bad type");
    }
    const auto& attr_std_map = element->data_model()->attributes();
    auto iter = attr_std_map.find(arg1->String());
    if (iter == attr_std_map.end()) {
      RETURN_UNDEFINED();
    }
    RETURN(iter->second);
  } else {
    const auto& builtin_attr_map = element->builtin_attr_map();
    auto iter = builtin_attr_map.find(type);
    if (iter == builtin_attr_map.end()) {
      RETURN_UNDEFINED();
    }
    RETURN(iter->second);
  }
  RETURN_UNDEFINED();
}

// __GetAttributeNames accepts one parameter, which is the element. It returns
// an array, which are the attribute keys of the element. If there are no
// attributes, it returns an empty array.
RENDERER_FUNCTION_CC(FiberGetAttributeNames) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberGetAttributeNames");
  // parameter size = 1
  // [0] RefCounted -> element
  CHECK_ARGC_GE(FiberGetAttributeNames, 1);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted,
                                        FiberGetAttributeNames);

  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  auto ary = lepus::CArray::Create();

  const auto& attr_std_map = element->data_model()->attributes();
  for (const auto& pair : attr_std_map) {
    ary->emplace_back(pair.first);
  }

  const auto& builtin_attr_map = element->builtin_attr_map();
  for (const auto& pair : builtin_attr_map) {
    ary->emplace_back(pair.first);
  }

  RETURN(lepus::Value(std::move(ary)));
}

RENDERER_FUNCTION_CC(FiberGetAttributes) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberGetAttributes");
  // parameter size = 1
  // [0] RefCounted -> element
  CHECK_ARGC_GE(FiberGetAttributes, 1);
  CONVERT_ARG(arg0, 0);
  if (!arg0->IsRefCounted()) {
    RETURN_UNDEFINED();
  }

  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  const auto& attr_std_map = element->data_model()->attributes();

  lepus::Value res(lepus::Dictionary::Create());
  for (const auto& pair : attr_std_map) {
    res.SetProperty(pair.first, pair.second);
  }

  const auto& builtin_attr_map = element->builtin_attr_map();
  for (const auto& pair : builtin_attr_map) {
    res.SetProperty(base::String(std::to_string(pair.first)), pair.second);
  }

  RETURN(res);
}

RENDERER_FUNCTION_CC(FiberAddClass) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberAddClass");
  // parameter size = 2
  // [0] RefCounted -> element
  // [1] String -> class name
  CHECK_ARGC_GE(FiberAddClass, 2);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted, FiberAddClass);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, String, FiberAddClass);

  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  CHECK_ILLEGAL_ATTRIBUTE_CONFIG(element, FiberAddClass);
  element->OnClassChanged(element->classes(), {arg1->String()});
  element->SetClass(arg1->String());
  ON_NODE_MODIFIED(element);
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberSetClasses) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberSetClasses");
  // parameter size = 2
  // [0] RefCounted -> element
  // [1] String -> classes
  CHECK_ARGC_GE(FiberSetClasses, 2);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted, FiberSetClasses);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, String, FiberSetClasses);

  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  CHECK_ILLEGAL_ATTRIBUTE_CONFIG(element, FiberSetClasses);
  auto clazz = arg1->String();
  ClassList old_classes = element->ReleaseClasses();
  element->RemoveAllClass();
  if (clazz.empty()) {
    element->OnClassChanged(old_classes, {});
    ON_NODE_MODIFIED(element);
    RETURN_UNDEFINED();
  }

  if (clazz.str().find(" ") == std::string::npos) {
    element->SetClass(clazz);
    element->OnClassChanged(old_classes, {clazz});
    ON_NODE_MODIFIED(element);
    RETURN_UNDEFINED();
  }

  ClassList classes = SplitClasses(clazz.c_str(), clazz.length());
  if (classes.empty()) {
    element->OnClassChanged(old_classes, {});
    RETURN_UNDEFINED();
  }

  element->SetClasses(std::move(classes));

  element->OnClassChanged(old_classes, element->classes());
  ON_NODE_MODIFIED(element);

  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberGetClasses) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberGetClasses");
  // parameter size = 1
  // [0] RefCounted -> element
  CHECK_ARGC_GE(FiberGetClasses, 1);
  CONVERT_ARG(arg0, 0);
  if (!arg0->IsRefCounted()) {
    RETURN_UNDEFINED();
  }

  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  auto ary = lepus::CArray::Create();
  for (const auto& c : element->classes()) {
    ary->emplace_back(c);
  }
  RETURN(lepus::Value(std::move(ary)));
}

RENDERER_FUNCTION_CC(FiberAddInlineStyle) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberAddInlineStyle");
  // parameter size = 3
  // [0] RefCounted -> element
  // [1] Number | String -> css property id
  // [2] value -> style
  CHECK_ARGC_GE(FiberAddInlineStyle, 3);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted,
                                        FiberAddInlineStyle);
  CONVERT_ARG(arg1, 1);
  CONVERT_ARG(arg2, 2);

  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  CHECK_ILLEGAL_ATTRIBUTE_CONFIG(element, FiberAddInlineStyle);
  // If the arg1 is a string, then arg1->Number() will return 0, which is an
  // illegal CSS property id. And then, execute
  // CSSProperty::GetPropertyID(arg1->String()) to get the CSS property id.
  CSSPropertyID id = static_cast<CSSPropertyID>(arg1->Number());
  if (UNLIKELY(id == CSSPropertyID::kPropertyStart)) {
    id = CSSProperty::GetPropertyID(arg1->String());
  }
  element->SetStyle(id, arg2->ToLepusValue());

  ON_NODE_MODIFIED(element);
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberSetInlineStyles) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberSetInlineStyles");
  // parameter size = 2
  // [0] RefCounted -> element
  // [1] String -> inline-style
  CHECK_ARGC_GE(FiberSetInlineStyles, 2);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted,
                                        FiberSetInlineStyles);
  CONVERT_ARG(arg1, 1);
  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  CHECK_ILLEGAL_ATTRIBUTE_CONFIG(element, FiberSetInlineStyles);

  // Since FiberSetInlineStyles means clear the previous value and set the new
  // value, then, call RemoveAllInlineStyles before call fiber element's
  // SetStyle.
  element->RemoveAllInlineStyles();

  if (arg1->IsString()) {
    element->SetRawInlineStyles(arg1->ToLepusValue());
  } else if (arg1->IsObject()) {
    // TODO(linxs): opt this function, should diff first. Use
    tasm::ForEachLepusValue(
        *arg1, [&](const lepus::Value& key, const lepus::Value& value) {
          auto id = CSSProperty::GetPropertyID(
              base::CamelCaseToDashCase(key.StringView()));
          if (CSSProperty::IsPropertyValid(id)) {
            element->SetStyle(id, value.ToLepusValue());
          }
        });
  } else if (!arg1->IsEmpty()) {
    // If arg1 is not string, not obejct and not empty, should crash like
    // CONVERT_ARG_AND_CHECK
    RenderFatal(ctx,
                "FiberSetInlineStyles: params 1 should use String or Object");
  }

  ON_NODE_MODIFIED(element);
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberGetInlineStyles) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberGetInlineStyles");
  // parameter size = 1
  // [0] RefCounted -> element
  CHECK_ARGC_GE(FiberGetInlineStyles, 1);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted,
                                        FiberGetInlineStyles);

  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  RETURN(element->GetRawInlineStyles());
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberSetParsedStyles) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberSetParsedStyles");
  // parameter size >= 2
  // [0] RefCounted -> element
  // [1] String -> parsed styles' key
  // [2] Object | Undefined | Null -> options
  CHECK_ARGC_GE(FiberSetParsedStyles, 2);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted,
                                        FiberSetParsedStyles);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, String, FiberSetParsedStyles);
  CONVERT_ARG(arg2, 2);

  std::string entry_name = tasm::DEFAULT_ENTRY_NAME;
  if (arg2->IsObject()) {
    BASE_STATIC_STRING_DECL(kEntryName, "entryName");
    const auto& entry_name_prop = arg2->GetProperty(kEntryName);
    if (entry_name_prop.IsString()) {
      entry_name = entry_name_prop.ToString();
    }
  }
  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  CHECK_ILLEGAL_ATTRIBUTE_CONFIG(element, FiberSetParsedStyles);
  auto entry = GET_TASM_POINTER()->FindTemplateEntry(entry_name);
  element->SetParsedStyles(*(entry->GetParsedStyles(arg1->StdString())), *arg2);
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberGetComputedStyles) {
  // TODO(songshourui.null): impl this later
  RETURN_UNDEFINED();
}

// This function accepts four parameters, the 0th is the element, the 1st is the
// event name, the 2nd is the event type, and the 3rd is the event function.
// When func is undefined, delete the corresponding event; when it is string,
// overwrite the previous name and type and add the corresponding js event; when
// it is callable, overwrite the previous name and type and add the
// corresponding lepus event.
RENDERER_FUNCTION_CC(FiberAddEvent) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberAddEvent");
  // parameter size = 4
  // [0] RefCounted -> element
  // [1] String -> type
  // [2] String -> name
  // [3] String/Function -> function
  CHECK_ARGC_GE(FiberAddEvent, 4);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted, FiberAddEvent);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(type, 1, String, FiberAddEvent);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(name, 2, String, FiberAddEvent);
  CONVERT_ARG(callback, 3);

  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  CHECK_ILLEGAL_ATTRIBUTE_CONFIG(element, FiberAddEvent);
  if (callback->IsEmpty()) {
    // If callback is undefined, remove event.
    element->RemoveEvent(name->String(), type->String());
  } else if (callback->IsString()) {
    element->SetJSEventHandler(name->String(), type->String(),
                               callback->String());
  } else if (callback->IsCallable()) {
    element->SetLepusEventHandler(name->String(), type->String(),
                                  lepus::Value(), *callback);
  } else if (callback->IsObject()) {
    BASE_STATIC_STRING_DECL(kType, "type");
    BASE_STATIC_STRING_DECL(kValue, "value");
    const auto& obj_type = callback->GetProperty(kType).StdString();
    const auto& value = callback->GetProperty(kValue);

    if (obj_type == tasm::kWorklet) {
      // worklet event
      element->SetWorkletEventHandler(name->String(), type->String(), value,
                                      LEPUS_CONTEXT());
    }
  } else {
    LOGW(
        "FiberAddEvent's 3rd parameter must be undefined, null, string or "
        "callable.");
  }

  ON_NODE_MODIFIED(element);
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(CreateGestureDetector) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CreateGestureDetector");
  // parameter size = 5
  // [0] RefCounted -> element/vdom
  // [1] (long)id -> gesture id
  // [2] (int)type -> gesture type
  // [3] Array callback -> events : [{name, script, function}]
  // [4] Array map -> relation map : {{"simultaneous" : [id1,id2...]},
  // "waitFor" : [id1,id2...]}}
  // Note: The code assumes that these arguments are provided correctly and in
  // the expected order.

  if (LEPUS_CONTEXT()->IsLepusContext()) {
    // Check if the context is a lepus context, if so, return undefined and log
    // an info message.
    LOGI("CreateGestureDetector failed since context is lepus context.");
    RETURN_UNDEFINED();
  }

  // Check if the required number of arguments (5) is present for the function
  // call.
  DCHECK(ARGC() >= 5);

  // Convert and validate input arguments.
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, CPointer,
                                        CreateGestureDetector);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, Number, CreateGestureDetector);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg2, 2, Number, CreateGestureDetector);
  CONVERT_ARG(callbacksConfigs, 3);
  CONVERT_ARG(relationMap, 4);

  // Extract the "callbacks" property from the input "callbacksConfigs"
  // argument.
  BASE_STATIC_STRING_DECL(kCallbacks, "callbacks");
  auto callbacks = callbacksConfigs->GetProperty(kCallbacks);

  // Check if the extracted "callbacks" property is an array or a JavaScript
  // array. If not, return undefined and log an info message.
  if (!callbacks.IsArrayOrJSArray()) {
    LOGI("CreateGestureDetector failed since callbacks is not array.");
    RETURN_UNDEFINED();
  }

  // Create a GestureDetector object using the extracted data.
  GestureDetector detector = InnerCreateGestureDetector(
      (*arg1).Number(), (*arg2).Number(), callbacksConfigs, relationMap,
      LEPUS_CONTEXT());

  // Set the created GestureDetector object for the
  // specified gesture id.
  reinterpret_cast<RadonNode*>(arg0->CPoint())
      ->SetGestureDetector(static_cast<uint32_t>(arg1->Number()), detector);

  // Return undefined to indicate successful function execution.
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberSetGestureDetector) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberSetGestureDetector");
  // parameter size = 5
  // [0] RefCounted -> element/vdom
  // [1] (long)id -> gesture id
  // [2] (int)type -> gesture type
  // [3] Array callback -> events : [{name, script, function}]
  // [4] Array map -> relation map : {{"simultaneous" : [id1,id2...]},
  // "waitFor" : [id1,id2...]}}
  // Note: The code assumes that these arguments are provided correctly and in
  // the expected order.

  if (LEPUS_CONTEXT()->IsLepusContext()) {
    // Check if the context is a lepus context, if so, return undefined and log
    // an info message.
    LOGI("FiberSetGestureDetector failed since context is lepus context.");
    RETURN_UNDEFINED();
  }

  // Check if the required number of arguments (5) is present for the function
  // call.
  DCHECK(ARGC() >= 5);

  // Convert and validate input arguments.
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted,
                                        FiberSetGestureDetector);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, Number,
                                        FiberSetGestureDetector);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg2, 2, Number,
                                        FiberSetGestureDetector);
  CONVERT_ARG(callbacksConfigs, 3);
  CONVERT_ARG(relationMap, 4);

  // Extract the "callbacks" property from the input "callbacksConfigs"
  // argument.
  BASE_STATIC_STRING_DECL(kCallbacks, "callbacks");
  auto callbacks = callbacksConfigs->GetProperty(kCallbacks);

  // Check if the extracted "callbacks" property is an array or a JavaScript
  // array. If not, return undefined and log an info message.
  if (!callbacks.IsArrayOrJSArray()) {
    LOGI("FiberSetGestureDetector failed since callbacks is not array.");
    RETURN_UNDEFINED();
  }

  // Create a GestureDetector object using the extracted data.
  GestureDetector detector = InnerCreateGestureDetector(
      (*arg1).Number(), (*arg2).Number(), callbacksConfigs, relationMap,
      LEPUS_CONTEXT());

  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  CHECK_ILLEGAL_ATTRIBUTE_CONFIG(element, FiberSetGestureDetector);
  element->SetGestureDetector(static_cast<uint32_t>(arg1->Number()), detector);

  ON_NODE_MODIFIED(element);
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberRemoveGestureDetector) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberRemoveGestureDetector");
  // parameter size = 2
  // [0] RefCounted -> element
  // [1] (long)id -> gesture id
  //...

  if (LEPUS_CONTEXT()->IsLepusContext()) {
    // Check if the context is a lepus context, if so, return undefined and log
    // an info message.
    LOGI("FiberRemoveGestureDetector failed since context is lepus context.");
    RETURN_UNDEFINED();
  }

  CHECK_ARGC_GE(FiberRemoveGestureDetector, 2);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted,
                                        FiberRemoveGestureDetector);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, Number,
                                        FiberRemoveGestureDetector);

  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  CHECK_ILLEGAL_ATTRIBUTE_CONFIG(element, FiberRemoveGestureDetector);
  element->RemoveGestureDetector((*arg1).Number());

  ON_NODE_MODIFIED(element);
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberSetGestureState) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberRemoveGestureDetector");
  // parameter size = 2
  // [0] RefCounted -> element
  // [1] (long)id -> gesture id
  // [2] (int)state -> gesture state  ACTIVE - 1 FAIL - 2 END - 3

  CHECK_ARGC_GE(FiberSetGestureState, 3);

  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted,
                                        FiberSetGestureState);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, Number, FiberSetGestureState);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg2, 2, Number, FiberSetGestureState);

  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());

  element->SetGestureDetectorState(arg1->Number(), arg2->Number());

  ON_NODE_MODIFIED(element);
  RETURN_UNDEFINED();
}

// This method is used to handle whether Native Gesture allows internal elements
// to consume the gesture or lets elements outside of lynxView consume the
// gesture.
RENDERER_FUNCTION_CC(FiberConsumeGesture) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberConsumeGesture");
  // parameter size = 3
  // [0] RefCounted -> element
  // [1] (long)id -> gesture id
  // [2] (any)params -> func params { inner: boolean,
  // consume: boolean}

  CHECK_ARGC_GE(FiberConsumeGesture, 3);

  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted,
                                        FiberConsumeGesture);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, Number, FiberConsumeGesture);
  CONVERT_ARG(arg2, 2);
  if (!arg2->IsObject()) {
    LOGW(
        "FiberConsumeGesture parameter must contain type, and type must be "
        "object.");
    RETURN_UNDEFINED();
  }
  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  element->ConsumeGesture(arg1->Number(), arg2->ToLepusValue());

  ON_NODE_MODIFIED(element);
  RETURN_UNDEFINED();
}

// The function accepts two parameters, the 0th is element and the 1st is Array
// composited by evnet object, which must contain three keys: name, type, and
// function. When this function is executed, the element's all events will be
// deleted first, and then the array will be traversed to add corresponding
// events.
RENDERER_FUNCTION_CC(FiberSetEvents) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberSetEvents");
  // parameter size = 2
  // [0] RefCounted -> element
  // [1] Array -> events : [{name, type, function}]
  CHECK_ARGC_GE(FiberSetEvents, 2);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted, FiberSetEvents);
  CONVERT_ARG(callbacks, 1);

  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  CHECK_ILLEGAL_ATTRIBUTE_CONFIG(element, FiberSetEvents);
  element->RemoveAllEvents();

  if (!callbacks->IsArrayOrJSArray()) {
    ON_NODE_MODIFIED(element);
    RETURN_UNDEFINED();
  }

  ForEachLepusValue(
      *callbacks, [element, LEPUS_CONTEXT()](const lepus::Value& index,
                                             const lepus::Value& value) {
        BASE_STATIC_STRING_DECL(kName, "name");
        BASE_STATIC_STRING_DECL(kType, "type");
        BASE_STATIC_STRING_DECL(kFunction, "function");

        const auto& name = value.GetProperty(kName);
        const auto& type = value.GetProperty(kType);
        const auto& callback = value.GetProperty(kFunction);

        if (!name.IsString()) {
          LOGW("FiberSetEvents' "
               << value.Number()
               << " parameter must contain name, and name must be string.");
          return;
        }
        if (!type.IsString()) {
          LOGW("FiberSetEvents' "
               << value.Number()
               << " parameter must contain type, and type must be string.");
          return;
        }
        if (callback.IsString()) {
          element->SetJSEventHandler(name.String(), type.String(),
                                     callback.String());
        } else if (callback.IsCallable()) {
          element->SetLepusEventHandler(name.String(), type.String(),
                                        lepus::Value(), callback);
        } else if (callback.IsObject()) {
          BASE_STATIC_STRING_DECL(kValue, "value");
          const auto& obj_type = callback.GetProperty(kType).StdString();
          const auto& value = callback.GetProperty(kValue);
          if (obj_type == tasm::kWorklet) {
            // worklet event
            element->SetWorkletEventHandler(name.String(), type.String(), value,
                                            LEPUS_CONTEXT());
          }

        } else {
          LOGW("FiberSetEvents' " << value.Number()
                                  << " parameter must contain callback, and "
                                     "callback must be string or callable.");
        }
      });

  ON_NODE_MODIFIED(element);
  RETURN_UNDEFINED();
}

// The function takes three parameters, element, event name and event type. When
// element does not have any corresponding event binding, return lepus::Value().
// Otherwise return a event object, where event contains name, type, jsFunction,
// lepusFunction and piperEventContent. The event must contain name and type,
// and may contain only one of jsFunction, lepusFunction and piperEventContent.
RENDERER_FUNCTION_CC(FiberGetEvent) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberGetEvent");
  // parameter size >= 3
  // [0] RefCounted -> element
  // [1] String -> event name
  // [2] String -> event type
  CHECK_ARGC_GE(FiberGetEvent, 3);
  CONVERT_ARG(arg0, 0);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, String, FiberGetEvent);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg2, 2, String, FiberGetEvent);
  constexpr const static char* kGlobalBind = "global-bindEvent";

  if (!arg0->IsRefCounted()) {
    RETURN_UNDEFINED();
  }

  // Get element.
  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  // Get event type.
  const auto& type = arg2->StdString();

  // Get events according to the event type.
  const auto& events = type == kGlobalBind
                           ? element->data_model()->global_bind_events()
                           : element->data_model()->static_events();

  // Return lepus::Value() if not find the event with event name.
  auto iter = events.find(arg1->String());
  if (iter == events.end()) {
    RETURN_UNDEFINED();
  }
  // Return lepus::Value() if event type not the same as required type.
  if (iter->second->type().str() != type) {
    RETURN_UNDEFINED();
  }

  RETURN(iter->second->ToLepusValue());
}

// The function takes one parameter, element. When element does not have any
// event binding, return lepus::Value(). Otherwise return a
// Record<eventName:String, Array<event:Object>>, where event contains name,
// type, jsFunction, lepusFunction and piperEventContent. The event must contain
// name and type, and may contain only one of jsFunction, lepusFunction and
// piperEventContent.
RENDERER_FUNCTION_CC(FiberGetEvents) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberGetEvents");
  // parameter size = 1
  // [0] RefCounted -> element
  CHECK_ARGC_GE(FiberGetEvents, 1);
  CONVERT_ARG(arg0, 0);
  if (!arg0->IsRefCounted()) {
    RETURN_UNDEFINED();
  }

  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  const auto& event = element->data_model()->static_events();
  const auto& global_event = element->data_model()->global_bind_events();

  if (event.empty() && global_event.empty()) {
    RETURN_UNDEFINED();
  }

  const static auto& merge_event = [](const EventMap& event,
                                      lepus::Value& result) {
    for (const auto& e : event) {
      fml::RefPtr<lepus::CArray> ary;
      if (result.Contains(e.first)) {
        ary = result.GetProperty(e.first).Array();
      } else {
        ary = lepus::CArray::Create();
        result.SetProperty(e.first, lepus::Value(ary));
      }
      ary->emplace_back(e.second->ToLepusValue());
    }
  };
  lepus::Value result = lepus::Value(lepus::Dictionary::Create());

  merge_event(event, result);
  merge_event(global_event, result);

  RETURN(result);
}

RENDERER_FUNCTION_CC(FiberSetID) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberSetID");
  // parameter size = 2
  // [0] RefCounted -> element
  // [1] String|undefined -> id
  CHECK_ARGC_GE(FiberSetID, 2);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted, FiberSetID);
  CONVERT_ARG(arg1, 1);

  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  CHECK_ILLEGAL_ATTRIBUTE_CONFIG(element, FiberSetID);
  if (arg1->IsString()) {
    element->SetIdSelector(arg1->String());
  } else {
    element->SetIdSelector(base::String());
  }

  ON_NODE_MODIFIED(element);
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberGetID) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberGetID");
  // parameter size = 1
  // [0] RefCounted -> element
  CHECK_ARGC_GE(FiberGetID, 1);
  CONVERT_ARG(arg0, 0);
  if (!arg0->IsRefCounted()) {
    RETURN_UNDEFINED();
  }

  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  RETURN(lepus::Value(element->GetIdSelector()));
}

RENDERER_FUNCTION_CC(FiberAddDataset) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberAddDataset");
  // parameter size = 3
  // [0] RefCounted -> element
  // [1] String -> key
  // [2] any -> value
  CHECK_ARGC_GE(FiberAddDataset, 3);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted, FiberAddDataset);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, String, FiberAddDataset);
  CONVERT_ARG(arg2, 2);

  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  CHECK_ILLEGAL_ATTRIBUTE_CONFIG(element, FiberAddDataset);
  element->AddDataset(arg1->String(), arg2->ToLepusValue());

  ON_NODE_MODIFIED(element);
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberSetDataset) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberSetDataset");
  // parameter size = 2
  // [0] RefCounted -> element
  // [1] any -> dataset
  CHECK_ARGC_GE(FiberSetDataset, 2);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted, FiberSetDataset);
  CONVERT_ARG(arg1, 1);

  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  CHECK_ILLEGAL_ATTRIBUTE_CONFIG(element, FiberSetDataset);
  element->SetDataset(arg1->ToLepusValue());

  ON_NODE_MODIFIED(element);
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberGetDataset) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberGetDataset");
  // parameter size = 1
  // [0] RefCounted -> element
  CHECK_ARGC_GE(FiberGetDataset, 1);
  CONVERT_ARG(arg0, 0);
  if (!arg0->IsRefCounted()) {
    RETURN_UNDEFINED();
  }

  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  const auto& data_map = element->dataset();
  auto dict = lepus::Dictionary::Create();
  for (const auto& pair : data_map) {
    dict->SetValue(pair.first, pair.second);
  }
  RETURN(lepus::Value(std::move(dict)));
}

RENDERER_FUNCTION_CC(FiberGetDataByKey) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberGetDataByKey");
  // parameter size = 2
  // [0] RefCounted -> element
  // [1] String -> key
  CHECK_ARGC_GE(FiberGetDataByKey, 2);
  CONVERT_ARG(arg0, 0);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, String, FiberGetDataByKey);

  if (!arg0->IsRefCounted()) {
    RETURN_UNDEFINED();
  }

  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  const auto& data_map = element->dataset();

  auto iter = data_map.find(arg1->String());
  if (iter == data_map.end()) {
    RETURN_UNDEFINED();
  }

  RETURN(lepus::Value(iter->second));
}

RENDERER_FUNCTION_CC(FiberGetComponentID) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberGetComponentID");
  // parameter size = 1
  // [0] RefCounted -> component element
  CHECK_ARGC_GE(FiberGetComponentID, 1);
  CONVERT_ARG(arg0, 0);

  // If arg0 is not RefCounted, return lepus::Value()
  if (!arg0->IsRefCounted()) {
    RETURN_UNDEFINED()
  }

  // If element is not component, return lepus::Value()
  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  if (!element->is_component()) {
    RETURN_UNDEFINED()
  }

  auto component =
      fml::static_ref_ptr_cast<ComponentElement>(arg0->RefCounted());
  RETURN(lepus::Value(component->component_id()));
}

RENDERER_FUNCTION_CC(FiberUpdateComponentID) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberUpdateComponentID");
  // parameter size = 2
  // [0] RefCounted -> component element
  // [1] String -> component id
  CHECK_ARGC_GE(FiberUpdateComponentID, 2);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted,
                                        FiberUpdateComponentID);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, String,
                                        FiberUpdateComponentID);

  auto component =
      fml::static_ref_ptr_cast<ComponentElement>(arg0->RefCounted());
  component->set_component_id(arg1->String());
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberUpdateListCallbacks) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberUpdateListCallbacks");
  // parameter size >= 3
  // [0] RefCounted -> list element
  // [1] Function -> component_at_index callback
  // [2] Function -> enqueue_component callback
  // [3] Function -> component_at_indexes callback
  CHECK_ARGC_GE(FiberUpdateListCallbacks, 3);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted,
                                        FiberUpdateListCallbacks);
  CONVERT_ARG(arg1, 1);
  CONVERT_ARG(arg2, 2);
  lepus::Value component_at_indexes;
  if (argc > 3) {
    CONVERT_ARG(arg3, 3);
    component_at_indexes = *arg3;
  }
  auto list_element = fml::static_ref_ptr_cast<ListElement>(arg0->RefCounted());
  list_element->UpdateCallbacks(*arg1, *arg2, component_at_indexes);
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberSetCSSId) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberSetCSSId");
  auto* self = GET_TASM_POINTER();
  // parameter size = 2
  // [0] RefCounted|Array<RefCounted> -> element(s)
  // [1] Number -> css_id
  // [2] String|Undefined -> optional, entry_name

  CHECK_ARGC_GE(FiberSetCSSId, 2);
  CONVERT_ARG(arg0, 0);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, Number, FiberSetCSSId);
  std::string entry_name = tasm::DEFAULT_ENTRY_NAME;
  if (argc > 2) {
    CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg2, 2, String, FiberSetCSSId);
    entry_name = arg2->StdString();
  }

  std::shared_ptr<CSSStyleSheetManager> style_sheet_manager =
      self->style_sheet_manager(entry_name);

  auto looper = [style_sheet_manager, arg1, ctx](const lepus::Value& key,
                                                 const lepus::Value& value) {
    if (!value.IsRefCounted()) {
      RenderFatal(ctx,
                  "FiberSetCSSId params 0 type should use RefCounted or "
                  "array of RefCounted");
    }
    auto element = fml::static_ref_ptr_cast<FiberElement>(value.RefCounted());
    element->set_style_sheet_manager(style_sheet_manager);
    // For Lynx SDK's version < 2.17, when `ComponentElement` executes
    // `FiberSetCSSId`, it changes the `component_css_id_` of `ComponentElement`
    // instead of `css_id_`, which does not meet expectations. Since this API is
    // currently only in RL3.0, and RL3.0 does not depend on `ComponentElement`
    // before this, a break will be introduced in versions >= 2.17. After this
    // update, when `ComponentElement` executes `FiberSetCSSId`, it will change
    // the `css_id_` of `ComponentElement`.
    element->SetCSSID(static_cast<int32_t>(arg1->Number()));
  };

  if (arg0->IsArrayOrJSArray()) {
    tasm::ForEachLepusValue(*arg0, std::move(looper));
  } else {
    looper(lepus::Value(), *arg0);
  }
  RETURN_UNDEFINED();
}

// Timing related
// Generate a new pipelineOptions,
// which will contain a pipelineID as a unique identifier.
// This pipelineID can be used later for timing measurements.
// The generated pipelineOptions will need to be flushed
// by invoking FiberFlushElementTree.
RENDERER_FUNCTION_CC(GeneratePipelineOptions) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "GeneratePipelineOptions");
  RETURN(PipelineOptionsToLepusValue(PipelineOptions()));
}

// OnPipelineStart method needs to be called at the very
// beginning of the pipeline.
// Generally, the pipelineOptions generated by GeneratePipelineOptions
// can be immediately used to call OnPipelineStart.
RENDERER_FUNCTION_CC(OnPipelineStart) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "OnPipelineStart");
  // parameter size = 1
  // [0] String -> pipeline id
  // [1] String -> pipeline origin

  CHECK_ARGC_GE(OnPipelineStart, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, String, OnPipelineStart);

  const auto& pipeline_id = arg0->StdString();
  const auto us_timestamp = base::CurrentSystemTimeMicroseconds();
  std::string pipeline_origin;
  if (argc > 1) {
    CONVERT_ARG_AND_CHECK(arg1, 1, String, OnPipelineStart);
    pipeline_origin = arg1->StdString();
  }

  GET_TASM_POINTER()->GetDelegate().OnPipelineStart(
      pipeline_id, pipeline_origin, us_timestamp);
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(BindPipelineIDWithTimingFlag) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "BindPipelineIDWithTimingFlag");
  // parameter size = 2
  // [0] String -> pipeline id
  // [1] String -> timing flag

  CHECK_ARGC_EQ(BindPipelineIDWithTimingFlag, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, String, BindPipelineIDWithTimingFlag);
  CONVERT_ARG_AND_CHECK(arg1, 1, String, BindPipelineIDWithTimingFlag);
  const auto& pipeline_id = arg0->StdString();
  const auto& timing_flag = arg1->StdString();

  GET_TASM_POINTER()->GetDelegate().BindPipelineIDWithTimingFlag(pipeline_id,
                                                                 timing_flag);
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(MarkTiming) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "MarkTiming");
  // parameter size = 2
  // [0] String -> pipeline id
  // [1] String -> timing key
  CHECK_ARGC_EQ(MarkTiming, 2);

  CONVERT_ARG_AND_CHECK(arg0, 0, String, MarkTiming);
  CONVERT_ARG_AND_CHECK(arg1, 1, String, MarkTiming);
  const auto& pipeline_id = arg0->StdString();
  const auto& timing_key = arg1->StdString();

  tasm::TimingCollector::Scope<TemplateAssembler::Delegate> scope(
      &GET_TASM_POINTER()->GetDelegate(), pipeline_id);
  tasm::TimingCollector::Instance()->MarkFrameworkTiming(timing_key);
  RETURN_UNDEFINED();
}

// The addTimingListener will be a no-op implementation in the lepus runtime.
RENDERER_FUNCTION_CC(AddTimingListener) { RETURN_UNDEFINED(); }

RENDERER_FUNCTION_CC(FiberFlushElementTree) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberFlushElementTree");
  // parameter size >= 0
  // [0] RefCounted -> element, flush the tree with the element as the root node
  // [1] Object -> options

  // If argc >= 1, convert arg0 to element.
  FiberElement* element = nullptr;
  if (argc >= 1) {
    CONVERT_ARG(arg0, 0);
    if (arg0->IsRefCounted()) {
      element =
          fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted()).get();
    }
  }

  bool trigger_data_updated = false;
  // If argc >= 2, get PipelineOptions from arg1.
  // The options.triggerLayout's default value is true, set it to false if do
  // need call DispatchLayoutUpdates. The options.operationID's default value is
  // 0, if call __FiberFlushElementTree in componentAtIndex, please set
  // operationID to the value passed in componentAtIndex.
  tasm::PipelineOptions options;
  // TODO(kechenglong): get pipeline_id from lepus arg1
  auto top_pipeline_id = tasm::TimingCollector::Instance()->GetTopPipelineID();
  if (!top_pipeline_id.empty()) {
    options.pipeline_id = top_pipeline_id;
  }
  if (argc >= 2) {
    CONVERT_ARG(arg1, 1);
    if (arg1->IsObject()) {
      BASE_STATIC_STRING_DECL(kPipelineOptions, "pipelineOptions");
      if (arg1->Contains(kPipelineOptions) &&
          arg1->GetProperty(kPipelineOptions).IsObject()) {
        const auto& table = arg1->GetProperty(kPipelineOptions);
        options.pipeline_id =
            table.GetProperty(BASE_STATIC_STRING(kPipelineID)).StdString();
        options.pipeline_origin =
            table.GetProperty(BASE_STATIC_STRING(kPipelineOrigin)).StdString();
        options.need_timestamps =
            table.GetProperty(BASE_STATIC_STRING(kPipelineNeedTimestamps))
                .Bool();
      }

      BASE_STATIC_STRING_DECL(kTriggerLayout, "triggerLayout");
      if (arg1->Contains(kTriggerLayout)) {
        options.trigger_layout_ = arg1->GetProperty(kTriggerLayout).Bool();
      }

      BASE_STATIC_STRING_DECL(kOperationID, "operationID");
      if (arg1->Contains(kOperationID)) {
        options.operation_id =
            static_cast<int64_t>(arg1->GetProperty(kOperationID).Number());
      }

      // the elementID is used for the list on multi-thread mode
      BASE_STATIC_STRING_DECL(kElementID, "elementID");
      if (arg1->Contains(kElementID)) {
        options.list_comp_id_ =
            static_cast<int>(arg1->GetProperty(kElementID).Number());
      }

      BASE_STATIC_STRING_DECL(kOperationIDs, "operationIDs");
      if (arg1->Contains(kOperationIDs)) {
        const auto& operationIDs = arg1->GetProperty(kOperationIDs);
        if (operationIDs.IsArray()) {
          ForEachLepusValue(
              operationIDs,
              [&options](const lepus::Value& key, const lepus::Value& value) {
                if (value.IsNumber()) {
                  options.operation_ids_.emplace_back(
                      static_cast<int64_t>(value.Number()));
                }
              });
        }
      }

      BASE_STATIC_STRING_DECL(kElementIDs, "elementIDs");
      if (arg1->Contains(kElementIDs)) {
        const auto& elementIDs = arg1->GetProperty(kElementIDs);
        if (elementIDs.IsJSArray()) {
          ForEachLepusValue(elementIDs, [&options](const lepus::Value& key,
                                                   const lepus::Value& value) {
            if (value.IsNumber()) {
              options.list_item_ids_.emplace_back(
                  static_cast<int>(value.Number()));
            }
          });
        }
      }

      BASE_STATIC_STRING_DECL(kListID, "listID");
      if (arg1->Contains(kListID)) {
        options.list_id_ =
            static_cast<int>(arg1->GetProperty(kListID).Number());
      }

      // TODO(dingwang.wxx): Remove this logic by using timing api to record the
      // rendering time of list item in FE framework.
      static bool enable_report =
          LynxEnv::GetInstance().EnableReportListItemLifeStatistic();
      if (enable_report) {
        options.enable_report_list_item_life_statistic_ = true;
        BASE_STATIC_STRING_DECL(kListItemLifeOption, "listItemLifeOption");
        if (arg1->Contains(kListItemLifeOption)) {
          const auto& list_item_life_option =
              arg1->GetProperty(kListItemLifeOption);
          if (list_item_life_option.IsObject()) {
            BASE_STATIC_STRING_DECL(kStartRenderTime, "startRenderTime");
            BASE_STATIC_STRING_DECL(kEndRenderTime, "endRenderTime");
            if (list_item_life_option.Contains(kStartRenderTime) &&
                list_item_life_option.Contains(kEndRenderTime)) {
              options.list_item_life_option_
                  .start_render_time_ = static_cast<uint64_t>(
                  list_item_life_option.GetProperty(kStartRenderTime).Number());
              options.list_item_life_option_
                  .end_render_time_ = static_cast<uint64_t>(
                  list_item_life_option.GetProperty(kEndRenderTime).Number());
            }
          }
        }
      }

      BASE_STATIC_STRING_DECL(kTimingFlag, "__lynx_timing_flag");
      if (arg1->Contains(kTimingFlag)) {
        const auto& timing_flag = arg1->GetProperty(kTimingFlag).StdString();
        if (!timing_flag.empty()) {
          options.need_timestamps = true;
          GET_TASM_POINTER()->GetDelegate().BindPipelineIDWithTimingFlag(
              options.pipeline_id, timing_flag);
        }
      }

      BASE_STATIC_STRING_DECL(kReloadTemplate, "reloadTemplate");
      if (arg1->Contains(kReloadTemplate)) {
        options.is_reload_template =
            (arg1->GetProperty(kReloadTemplate).Bool());
        options.need_timestamps |= options.is_reload_template;
      }

      auto kNativeUpdateDataOrder_str =
          BASE_STATIC_STRING(kNativeUpdateDataOrder);
      if (arg1->Contains(kNativeUpdateDataOrder_str)) {
        options.native_update_data_order_ =
            (arg1->GetProperty(kNativeUpdateDataOrder_str).Number());
      }

      BASE_STATIC_STRING_DECL(kTriggerDataUpdated, "triggerDataUpdated");
      if (arg1->Contains(kTriggerDataUpdated)) {
        trigger_data_updated = arg1->GetProperty(kTriggerDataUpdated).Bool();
      }

      BASE_STATIC_STRING_DECL(kListReuseNotification, "listReuseNotification");
      if (arg1->Contains(kListReuseNotification)) {
        const auto& notification_value =
            arg1->GetProperty(kListReuseNotification);
        if (notification_value.IsObject()) {
          BASE_STATIC_STRING_DECL(kListElement, "listElement");
          BASE_STATIC_STRING_DECL(kItemKey, "itemKey");
          if (notification_value.Contains(kListElement) &&
              notification_value.Contains(kItemKey)) {
            const auto& list_value =
                notification_value.GetProperty(kListElement);
            const auto& item_key_value =
                notification_value.GetProperty(kItemKey);
            if (list_value.IsRefCounted() && item_key_value.IsString()) {
              ListElement* list_element =
                  fml::static_ref_ptr_cast<ListElement>(list_value.RefCounted())
                      .get();
              if (list_element != nullptr) {
                list_element->NotifyListReuseNode(
                    fml::RefPtr<FiberElement>(element),
                    item_key_value.String());
              }
            }
          }
        }
      }

      BASE_STATIC_STRING_DECL(kAsyncFlush, "asyncFlush");
      if (arg1->Contains(kAsyncFlush) &&
          arg1->GetProperty(kAsyncFlush).Bool()) {
        if (element) {
          element->AsyncResolveSubtreeProperty();
        }
        RETURN_UNDEFINED();
      }
    }
  }

  auto self = GET_TASM_POINTER();

  tasm::TimingCollector::Scope<TemplateAssembler::Delegate> scope(
      &self->GetDelegate(), options);
  if (options.is_reload_template) {
    tasm::TimingCollector::Instance()->Mark(tasm::timing::kMtsRenderEnd);
  }
  self->page_proxy()->element_manager()->OnPatchFinish(options, element);

  // Currently, only client updateData, client resetData, and JS root component
  // setData updates trigger the OnDataUpdated callback, and only when the page
  // has actually changed. Other data updates, such as client reloadTemplate and
  // JS child components setData, do not trigger OnDataUpdated. In order to
  // align with this logic, the timing of OnDataUpdated is moved to the end of
  // FiberFlushElementTree, and it is controlled by LepusRuntime through
  // triggerDataUpdated.
  if (trigger_data_updated) {
    self->GetDelegate().OnDataUpdated();
  }
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberOnLifecycleEvent) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberOnLifecycleEvent");
  // parameter size = 1
  // [0] Array -> component event info
  CHECK_ARGC_GE(FiberOnLifecycleEvent, 1);
  CONVERT_ARG(arg0, 0);
  // TODO(liyanbo): refact this use event api.
  GET_TASM_POINTER()->GetDelegate().OnLifecycleEvent(*arg0);
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberElementFromBinary) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberElementFromBinary");
  // parameter size >= 2
  // [0] String -> template id
  // [1] Number -> component id
  CHECK_ARGC_EQ(FiberElementFromBinary, 2);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, String,
                                        FiberElementFromBinary);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, Number,
                                        FiberElementFromBinary);

  const auto& self = GET_TASM_POINTER();
  const auto& entry = self->FindEntry(tasm::DEFAULT_ENTRY_NAME);
  const auto& info = entry->GetElementTemplateInfo(arg0->StdString());

  lepus::Value node_ary = TreeResolver::InitElementTree(
      TreeResolver::FromTemplateInfo(info), arg1->Int64(),
      self->page_proxy()->element_manager().get(),
      self->style_sheet_manager(DEFAULT_ENTRY_NAME));

  // Call manager->PrepareNodeForInspector to init inspector attr for the
  // element tree.
  EXEC_EXPR_FOR_INSPECTOR(
      auto* manager = self->page_proxy()->element_manager().get();
      if (manager->GetDevToolFlag() && manager->IsDomTreeEnabled()) {
        tasm::ForEachLepusValue(node_ary, [manager](const auto& index,
                                                    const auto& value) {
          std::function<void(FiberElement*)> prepare_node_f =
              [manager, &prepare_node_f](const auto& element) {
                manager->PrepareNodeForInspector(element);
                for (const auto& child : element->children()) {
                  prepare_node_f(child.get());
                }
              };
          prepare_node_f(
              fml::static_ref_ptr_cast<FiberElement>(value.RefCounted()).get());
        });
      });

  RETURN(node_ary);
}

RENDERER_FUNCTION_CC(FiberElementFromBinaryAsync) {
  // TODO(songshourui.null): impl this later
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberQueryComponent) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberQueryComponent");
  // supporting usage: QueryComponent(url, (result) => {});
  auto* tasm = GET_TASM_POINTER();
  CHECK_ARGC_GE(FiberQueryComponent, 1);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, String, FiberQueryComponent);
  const auto& url = arg0->StdString();
  lepus::Value callback;
  if (argc >= 2) {
    CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, Callable,
                                          FiberQueryComponent);
    callback = *arg1;
  }

  auto entry = tasm->RequireTemplateEntry(nullptr, url, callback);
  if (entry) {
    auto dictionary = lepus::Dictionary::Create();
    dictionary->SetValue(BASE_STATIC_STRING(lazy_bundle::kEvalResult),
                         entry->GetBinaryEvalResult());
    return lepus::Value(std::move(dictionary));
  }
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberQuerySelector) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberQuerySelector");
  CHECK_ARGC_GE(FiberQuerySelector, 3);
  CONVERT_ARG(arg0, 0);
  if (!arg0->IsRefCounted()) {
    RETURN_UNDEFINED();
  }
  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, String, FiberQuerySelector);
  NodeSelectOptions options(NodeSelectOptions::IdentifierType::CSS_SELECTOR,
                            arg1->StdString());
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg2, 2, Object, FiberQuerySelector);
  BASE_STATIC_STRING_DECL(kOnlyCurrentComponent, "onlyCurrentComponent");
  auto only_current_component = arg2->GetProperty(kOnlyCurrentComponent);
  options.only_current_component =
      only_current_component.IsBool() ? only_current_component.Bool() : true;
  auto result = tasm::FiberElementSelector::Select(element.get(), options);
  if (result.Success()) {
    RETURN(lepus::Value(fml::RefPtr<FiberElement>(result.GetOneNode())));
  }
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberUpdateComponentInfo) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberUpdateComponentInfo");
  // parameter size = 2
  // [0] RefCounted -> component element
  // [1] Object -> component info
  CHECK_ARGC_GE(FiberUpdateComponentInfo, 2);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted,
                                        FiberUpdateComponentID);
  CONVERT_ARG(arg1, 1);

  auto component =
      fml::static_ref_ptr_cast<ComponentElement>(arg0->RefCounted());
  if (!arg1->IsObject()) {
    LOGW("FiberUpdateComponentInfo failed since the input info is not object");
    RETURN_UNDEFINED();
  }

  tasm::ForEachLepusValue(*arg1, [&component](const auto& key,
                                              const auto& value) {
    constexpr const static char* kComponentID = "componentID";
    constexpr const static char* kComponentName = "name";
    constexpr const static char* kComponentPath = "path";
    constexpr const static char* kComponentEntry = "entry";
    constexpr const static char* kComponentCSSID = "cssID";

    const auto& key_str = key.StdString();

    if (key_str == kComponentID) {
      component->set_component_id(value.String());
    } else if (key_str == kComponentName) {
      component->set_component_name(value.String());
    } else if (key_str == kComponentPath) {
      component->set_component_path(value.String());
    } else if (key_str == kComponentEntry) {
      component->set_component_entry(value.String());
    } else if (key_str == kComponentCSSID) {
      // Currently, the `cssID` in `FiberUpdateComponentInfo` updates the
      // `component_css_id_` of `ComponentElement` rather than `css_id_`. In the
      // future, we will consider adding two new keys to update
      // `component_css_id_` and `css_id_` separately. The behavior
      // corresponding to `cssID` will not change to avoid causing a break.
      fml::static_ref_ptr_cast<ComponentElement>(component)->SetComponentCSSID(
          value.Number());
    }
  });
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberGetElementConfig) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberGetElementConfig");
  // parameter size = 1
  // [0] RefCounted -> element
  CHECK_ARGC_GE(FiberGetElementConfig, 1);
  CONVERT_ARG(arg0, 0);

  // If arg0 is not RefCounted, return lepus::Value()
  if (!arg0->IsRefCounted()) {
    RETURN_UNDEFINED()
  }

  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());

  RETURN(element->config());
}

RENDERER_FUNCTION_CC(FiberGetInlineStyle) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberGetInlineStyle");
  // parameter size = 2
  // [0] RefCounted -> element
  // [1] Number -> css property id
  CHECK_ARGC_GE(FiberGetInlineStyle, 2);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted,
                                        FiberGetInlineStyle);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, Number, FiberGetInlineStyle);

  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());

  const RawLepusStyleMap& inline_styles = element->GetCurrentRawInlineStyles();
  const auto iterator =
      inline_styles.find(static_cast<CSSPropertyID>(arg1->Number()));
  if (iterator != inline_styles.end()) {
    RETURN(iterator->second);
  }
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberQuerySelectorAll) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberQuerySelectorAll");
  CHECK_ARGC_GE(FiberQuerySelectorAll, 3);
  CONVERT_ARG(arg0, 0);
  if (!arg0->IsRefCounted()) {
    RETURN_UNDEFINED();
  }
  auto element = fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, String, FiberQuerySelectorAll);
  NodeSelectOptions options(NodeSelectOptions::IdentifierType::CSS_SELECTOR,
                            arg1->StdString());
  options.first_only = false;
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg2, 2, Object, FiberQuerySelector);
  BASE_STATIC_STRING_DECL(kOnlyCurrentComponent, "onlyCurrentComponent");
  auto only_current_component = arg2->GetProperty(kOnlyCurrentComponent);
  options.only_current_component =
      only_current_component.IsBool() ? only_current_component.Bool() : true;
  auto result = tasm::FiberElementSelector::Select(element.get(), options);

  auto ary = lepus::CArray::Create();
  for (const auto& c : result.nodes) {
    ary->emplace_back(fml::RefPtr<FiberElement>(c));
  }
  RETURN(lepus::Value(std::move(ary)));
}

RENDERER_FUNCTION_CC(FiberSetLepusInitData) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberSetLepusInitData");
  // parameter size >= 1
  // [0] Object -> lepus init data
  CHECK_ARGC_GE(FiberSetLepusInitData, 1);
  CONVERT_ARG(arg0, 0);

  auto tasm = GET_TASM_POINTER();
  if (tasm == nullptr) {
    RETURN_UNDEFINED();
  }
  const auto& entry = tasm->FindTemplateEntry(tasm::DEFAULT_ENTRY_NAME);
  if (entry == nullptr) {
    RETURN_UNDEFINED();
  }
  entry->SetLepusInitData(*arg0);
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberGetDiffData) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberGetDiffData");
  // parameter size >= 2
  // [0] Object -> old data, the current page data
  // [1] Object -> new data, incoming data of updating
  // [2] Object -> options, used as UpdatePageOption
  // Note: This function is used to find the diff keys between old data and new
  // data. The new data will be traversed and the changed keys in the old data
  // will be placed in 'diff_key_array'. The whole data after combining old and
  // new data will be placed in 'new_data' key.
  CHECK_ARGC_GE(FiberGetDiffData, 2);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, Object, FiberGetDiffData);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, Object, FiberGetDiffData);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg2, 2, Object, FiberGetDiffData);

  BASE_STATIC_STRING_DECL(kReloadTemplate, "reload_template");
  BASE_STATIC_STRING_DECL(kResetPageData, "reset_page_data");
  BASE_STATIC_STRING_DECL(kDiffKeyArray, "diff_key_array");
  BASE_STATIC_STRING_DECL(kUpdateNewData, "new_data");

  auto array = lepus::CArray::Create();
  // find removed keys and set corresponding data to undefined when resetData
  // or reloadTemplate
  auto* tasm = GET_TASM_POINTER();

  lepus::Value data = *arg0;
  lepus::Value newData = *arg1;
  if (tasm &&
      tasm->GetPageConfig()->GetEnableAirDetectRemovedKeysWhenUpdateData() &&
      (arg2->GetProperty(kReloadTemplate).Bool() ||
       arg2->GetProperty(kResetPageData).Bool())) {
    ForEachLepusValue(data,
                      [&data, &array, &newData](const lepus::Value& key,
                                                const lepus::Value& value) {
                        auto key_str = key.String();
                        if (!newData.Contains(key_str) && !value.IsEmpty()) {
                          if ((key_str.str() != kGlobalPropsKey) &&
                              (key_str.str() != kSystemInfo)) {
                            lepus::Value data_value = data.GetProperty(key_str);
                            data_value.SetUndefined();
                            array->push_back(key);
                            data.SetProperty(key_str, data_value);
                          }
                        }
                      });
  }

  // find diff key
  tasm::ForEachLepusValue(newData, [&data, &array](const lepus::Value& key,
                                                   const lepus::Value& value) {
    auto key_str = key.String();
    lepus::Value ret = data.GetProperty(key_str);
    if (!ret.IsEmpty()) {
      if (CheckTableShadowUpdated(ret, value) ||
          value.GetLength() != ret.GetLength()) {
        array->push_back(key);
        data.SetProperty(key_str, value);
      }
    } else {
      array->push_back(key);
      data.SetProperty(key_str, value);
    }
  });

  lepus::Value result = lepus::Value(lepus::Dictionary::Create());
  result.SetProperty(kDiffKeyArray, lepus::Value(std::move(array)));
  result.SetProperty(kUpdateNewData, data);

  RETURN(lepus::Value(result));
}

RENDERER_FUNCTION_CC(FiberGetElementByUniqueID) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberGetElementByUniqueID");
  // parameter size >= 1
  // [0] Number -> element uniqueId
  CHECK_ARGC_GE(FiberGetElementByUniqueID, 1);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, Number,
                                        FiberGetElementByUniqueID);

  int32_t uniqueId = static_cast<int32_t>(arg0->Int64());
  auto& manager = GET_TASM_POINTER()->page_proxy()->element_manager();
  auto element =
      static_cast<FiberElement*>(manager->node_manager()->Get(uniqueId));

  if (element == nullptr) {
    RETURN_UNDEFINED();
  }

  RETURN(lepus::Value(fml::RefPtr<FiberElement>(element)));
}

RENDERER_FUNCTION_CC(FiberUpdateIfNodeIndex) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberUpdateIfNodeIndex");
  // parameter size >= 2
  // [0] RefCounted -> element
  // [1] Number -> if index
  CHECK_ARGC_GE(FiberUpdateIfNodeIndex, 2);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted,
                                        FiberUpdateIfNodeIndex);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, Number,
                                        FiberUpdateIfNodeIndex);

  auto element = fml::static_ref_ptr_cast<IfElement>(arg0->RefCounted());
  int32_t index = static_cast<int32_t>(arg1->Int64());
  if (element->is_if()) {
    element->UpdateIfIndex(index);
  }

  RETURN(lepus::Value());
}

RENDERER_FUNCTION_CC(FiberUpdateForChildCount) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberUpdateForChildCount");
  // parameter size >= 2
  // [0] Object -> origin data
  // [1] Number -> for child count
  CHECK_ARGC_GE(FiberUpdateForChildCount, 2);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg0, 0, RefCounted,
                                        FiberUpdateForChildCount);
  CONVERT_ARG_AND_CHECK_FOR_ELEMENT_API(arg1, 1, Number,
                                        FiberUpdateForChildCount);

  auto element = fml::static_ref_ptr_cast<ForElement>(arg0->RefCounted());
  if (element->is_for()) {
    uint32_t count = static_cast<uint32_t>(arg1->Number());
    element->UpdateChildrenCount(count);
  }

  RETURN(lepus::Value());
}

RENDERER_FUNCTION_CC(LoadLepusChunk) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LoadLepusChunk");
  // parameter size == 2
  // [0] String -> path of lepus chunk
  // [1] Object -> options
  // return -> boolean
  CHECK_ARGC_GE(LoadLepusChunk, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, String, LoadLepusChunk);
  CONVERT_ARG_AND_CHECK(arg1, 1, Object, LoadLepusChunk);

  auto* tasm = GET_TASM_POINTER();

  if (tasm == nullptr) {
    RETURN(lepus::Value(false));
  }

  std::string entryName = tasm::DEFAULT_ENTRY_NAME;

  BASE_STATIC_STRING_DECL(kLazyBundleEntry, "dynamicComponentEntry");
  auto template_entry_val = arg1->GetProperty(kLazyBundleEntry);

  if (template_entry_val.IsString()) {
    entryName = template_entry_val.ToString();
  }

  const auto& entry = tasm->FindTemplateEntry(entryName);
  if (entry == nullptr) {
    RETURN(lepus::Value(false));
  }

  bool is_success = entry->LoadLepusChunk(arg0->ToString(), std::move(*arg1));

  RETURN(lepus::Value(is_success));
}

RENDERER_FUNCTION_CC(FiberCreateElementWithProperties) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberCreateElementWithProperties");
  // parameter description
  CONVERT_ARG(arg0, 0);
  ElementBuiltInTagEnum enum_tag =
      static_cast<ElementBuiltInTagEnum>(arg0->Number());
  if (enum_tag == 0) {
    auto string_type = arg0->String();
    if (string_type.empty()) {
      RenderWarning("Bad builtin string type");
      RETURN_UNDEFINED();
    }
    enum_tag = ElementProperty::ConvertStringTagToEnumTag(string_type);
  }
  if (enum_tag == ElementBuiltInTagEnum::ELEMENT_EMPTY) {
    RETURN_UNDEFINED()
  }
  auto* tasm_pointer = GET_TASM_POINTER();
  auto& manager = tasm_pointer->page_proxy()->element_manager();
  auto element = manager->CreateFiberElement(enum_tag);
  if (element->is_component() || element->is_page()) {
    element->set_style_sheet_manager(
        tasm_pointer->style_sheet_manager(DEFAULT_ENTRY_NAME));
  }
  if (element->is_list()) {
    static_cast<ListElement*>(element.get())->set_tasm(tasm_pointer);
  }

  // properties array
  CONVERT_ARG(arg1, 1);
  if (!arg1->IsArrayOrJSArray()) {
    RenderWarning("args[1] is param_bundle, need array.");
    RETURN_UNDEFINED();
  }
  if (arg1->Array()->size() != 7) {
    // [0] String -> id
    // [1] String -> tag
    // [2] String -> class
    // [3] Array -> event
    // [4] Object -> style
    // [5] Object -> attribute
    // [6] Object  -> builtin attribute
    if (arg1->GetProperty(0).IsString()) {
      element->SetIdSelector(arg1->GetProperty(0).String());
    } else {
      RenderWarning("param_bundle[0] is id, need a String.");
      RETURN_UNDEFINED();
    }
    if (!arg1->GetProperty(1).IsString()) {
      RenderWarning("param_bundle[1] is tag, need a String.");
      RETURN_UNDEFINED();
    }
    if (arg1->GetProperty(2).IsString()) {
      element->OnClassChanged(element->classes(),
                              {arg1->GetProperty(2).String()});
      element->SetClass(arg1->String());
    } else {
      RenderWarning("param_bundle[2] is class, need a String.");
      RETURN_UNDEFINED();
    }
    if (arg1->GetProperty(3).IsArrayOrJSArray()) {
      auto callbacks = arg1->GetProperty(3);
      element->RemoveAllEvents();

      ForEachLepusValue(callbacks, [element, LEPUS_CONTEXT()](
                                       const lepus::Value& index,
                                       const lepus::Value& value) {
        BASE_STATIC_STRING_DECL(kName, "name");
        BASE_STATIC_STRING_DECL(kType, "type");
        BASE_STATIC_STRING_DECL(kFunction, "function");

        const auto& name = value.GetProperty(kName);
        const auto& type = value.GetProperty(kType);
        const auto& callback = value.GetProperty(kFunction);

        if (!name.IsString()) {
          LOGW("FiberSetEvents' "
               << value.Number()
               << " parameter must contain name, and name must be string.");
        }
        if (!type.IsString()) {
          LOGW("FiberSetEvents' "
               << value.Number()
               << " parameter must contain type, and type must be string.");
        }
        if (callback.IsString()) {
          element->SetJSEventHandler(name.String(), type.String(),
                                     callback.String());
        } else if (callback.IsCallable()) {
          element->SetLepusEventHandler(name.String(), type.String(),
                                        lepus::Value(), callback);
        } else if (callback.IsObject()) {
          BASE_STATIC_STRING_DECL(kValue, "value");

          const auto& obj_type = callback.GetProperty(kType).String().str();
          const auto& value = callback.GetProperty(kValue);
          if (obj_type == tasm::kWorklet) {
            // worklet event
            element->SetWorkletEventHandler(name.String(), type.String(), value,
                                            LEPUS_CONTEXT());
          }

        } else {
          LOGW("FiberSetEvents' " << value.Number()
                                  << " parameter must contain callback, and "
                                     "callback must be string or callable.");
        }
      });
    } else {
      RenderWarning("param_bundle[3] is event, need an Array.");
    }

    if (arg1->GetProperty(4).IsObject()) {
      tasm::ForEachLepusValue(
          arg1->GetProperty(4),
          [&element](const lepus::Value& key, const lepus::Value& value) {
            auto id = CSSProperty::GetPropertyID(
                base::CamelCaseToDashCase(key.String().str()));
            if (CSSProperty::IsPropertyValid(id)) {
              element->SetStyle(id, value);
            }
          });
    } else if (arg1->GetProperty(4).IsString()) {
      // string style TBD.
    } else {
      RenderWarning("param_bundle[4] is style, need an Object or an Array.");
      RETURN_UNDEFINED();
    }

    if (arg1->GetProperty(5).IsObject()) {
      tasm::ForEachLepusValue(
          arg1->GetProperty(5),
          [&element](const lepus::Value& key, const lepus::Value& value) {
            if (key.IsString()) {
              element->SetAttribute(key.String(), value);
            }
          });
    } else {
      RenderWarning("param_bundle[5] is attribute, need an Object.");
      RETURN_UNDEFINED();
    }

    if (arg1->GetProperty(6).IsObject()) {
      tasm::ForEachLepusValue(
          arg1->GetProperty(6),
          [&element](const lepus::Value& key, const lepus::Value& value) {
            if (key.IsNumber()) {
              element->SetBuiltinAttribute(
                  static_cast<ElementBuiltInAttributeEnum>(key.Number()),
                  value);
            }
          });
    } else {
      RenderWarning("param_bundle[6] is builtin attribute, need an Object.");
      RETURN_UNDEFINED();
    }
  }
  CONVERT_ARG(arg2, 2);
  if (!arg2->IsObject()) {
    RenderWarning("args[2] is options, need object.");
    RETURN_UNDEFINED();
  }

  ON_NODE_CREATE(element);
  RETURN(lepus::Value(std::move(element)));
}

RENDERER_FUNCTION_CC(FiberCreateSignal) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberCreateSignal");
  CHECK_ARGC_GE(FiberCreateSignal, 1);
  CONVERT_ARG(arg0, 0);  // init value

  auto signal = fml::MakeRefCounted<Signal>(
      GET_TASM_POINTER()->GetSignalContext(), *arg0);

  RETURN(lepus::Value(signal));
}

RENDERER_FUNCTION_CC(FiberWriteSignal) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberWriteSignal");
  CHECK_ARGC_GE(FiberCreateSignal, 1);
  CONVERT_ARG(arg0, 0);  // signal or signal array
  CONVERT_ARG(arg1, 1);  // value or value array

  if (!arg0->IsRefCounted() && !arg0->IsArrayOrJSArray()) {
    RenderWarning(
        "FiberWriteSignal failed since arg0 is not signal or signal array.");
    RETURN_UNDEFINED();
  }

  GET_TASM_POINTER()->GetSignalContext()->RunUpdates([arg0, arg1]() {
    if (arg0->IsRefCounted() &&
        arg0->RefCounted()->GetRefType() == lepus::RefType::kSignal) {
      auto signal = fml::static_ref_ptr_cast<Signal>(arg0->RefCounted());
      signal->SetValue(*arg1);
    } else if (arg0->IsArrayOrJSArray() && arg1->IsArrayOrJSArray()) {
      int32_t index = 0;

      tasm::ForEachLepusValue(*arg0, [&arg1, &index](const auto& key,
                                                     const auto& value) {
        if (value.IsRefCounted() &&
            value.RefCounted()->GetRefType() == lepus::RefType::kSignal) {
          auto signal = fml::static_ref_ptr_cast<Signal>(value.RefCounted());
          signal->SetValue(arg1->GetProperty(index));
        } else {
          RenderWarning(
              "FiberWriteSignal failed since %d of arg0 is not signal.", index);
        }
        index++;
      });
    } else {
      RenderWarning(
          "FiberWriteSignal failed since arg0 is not signal or signal array.");
    }
  });

  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberReadSignal) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberReadSignal");
  CHECK_ARGC_GE(FiberCreateSignal, 1);
  CONVERT_ARG(arg0, 0);  // signal or memo

  if (!arg0->IsRefCounted()) {
    RenderWarning("FiberReadSignal failed since arg0 is not signal or memo.");
    RETURN_UNDEFINED();
  }

  if (arg0->RefCounted()->GetRefType() != lepus::RefType::kSignal &&
      arg0->RefCounted()->GetRefType() != lepus::RefType::kMemo) {
    RenderWarning("FiberReadSignal failed since arg0 is not signal or memo.");
    RETURN_UNDEFINED();
  }

  RETURN(fml::static_ref_ptr_cast<Signal>(arg0->RefCounted())->GetValue());
}

RENDERER_FUNCTION_CC(FiberCreateComputation) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberCreateComputation");
  CHECK_ARGC_GE(FiberCreateComputation, 2);
  CONVERT_ARG(arg0, 0);  // block
  CONVERT_ARG(arg1, 1);  // init value
  CONVERT_ARG(arg2, 2);  // bool value

  auto computation = fml::MakeRefCounted<Computation>(
      GET_TASM_POINTER()->GetSignalContext(), LEPUS_CONTEXT(), *arg0, *arg1,
      arg2->IsTrue(), nullptr);

  RETURN(lepus::Value(computation));
}

RENDERER_FUNCTION_CC(FiberCreateMemo) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberCreateMemo");
  CHECK_ARGC_GE(FiberCreateComputation, 2);
  CONVERT_ARG(arg0, 0);  // block
  CONVERT_ARG(arg1, 1);  // init value

  auto memo = fml::MakeRefCounted<Memo>(GET_TASM_POINTER()->GetSignalContext(),
                                        LEPUS_CONTEXT(), *arg0, *arg1);

  RETURN(lepus::Value(memo));
}

RENDERER_FUNCTION_CC(FiberUnTrack) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberUnTrack");
  CHECK_ARGC_GE(FiberUnTrack, 1);
  CONVERT_ARG(arg0, 0);  // block

  GET_TASM_POINTER()->GetSignalContext()->MarkUnTrack(true);

  auto value = LEPUS_CONTEXT()->CallClosure(*arg0);

  GET_TASM_POINTER()->GetSignalContext()->MarkUnTrack(false);

  RETURN(value);
}

RENDERER_FUNCTION_CC(FiberCreateScope) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberCreateScope");
  CHECK_ARGC_GE(FiberCreateScope, 1);
  CONVERT_ARG(arg0, 0);  // block

  auto scope = fml::MakeRefCounted<Scope>(
      GET_TASM_POINTER()->GetSignalContext(), LEPUS_CONTEXT(), *arg0);
  RETURN(scope->ObtainResult());
}

RENDERER_FUNCTION_CC(FiberGetScope) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberGetScope");

  auto scope = GET_TASM_POINTER()->GetSignalContext()->GetTopScope();

  if (scope == nullptr) {
    RETURN_UNDEFINED();
  }

  RETURN(lepus::Value(fml::RefPtr<BaseScope>(scope)));
}

RENDERER_FUNCTION_CC(FiberCleanUp) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberCleanUp");
  CHECK_ARGC_GE(FiberCleanUp, 1);
  CONVERT_ARG(arg0, 0);  // scope

  if (!arg0->IsRefCounted()) {
    RenderWarning(
        "FiberCleanUp failed since arg0 is not scope/computation/memo.");
    RETURN_UNDEFINED();
  }

  auto ref_counted = arg0->RefCounted();

  switch (ref_counted->GetRefType()) {
    case lepus::RefType::kScope:
      fml::static_ref_ptr_cast<Scope>(arg0->RefCounted())->CleanUp();
      break;
    case lepus::RefType::kComputation:
      fml::static_ref_ptr_cast<Computation>(arg0->RefCounted())->CleanUp();
      break;
    case lepus::RefType::kMemo:
      fml::static_ref_ptr_cast<Memo>(arg0->RefCounted())->CleanUp();
      break;
    default:
      LOGW("FiberCleanUp's first arg can not be cleaned up.");
  }

  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(FiberOnCleanUp) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FiberOnCleanUp");
  CHECK_ARGC_GE(FiberOnCleanUp, 2);
  CONVERT_ARG(arg0, 0);  // scope
  CONVERT_ARG(arg1, 1);  // block

  if (!arg0->IsRefCounted()) {
    RenderWarning(
        "FiberCleanUp failed since arg0 is not scope/computation/memo.");
    RETURN_UNDEFINED();
  }

  if (!arg1->IsCallable()) {
    RenderWarning(
        "FiberCleanUp failed since arg0 is not scope/computation/memo.");
    RETURN_UNDEFINED();
  }

  auto ref_counted = arg0->RefCounted();

  switch (ref_counted->GetRefType()) {
    case lepus::RefType::kScope:
      fml::static_ref_ptr_cast<Scope>(arg0->RefCounted())->OnCleanUp(*arg1);
      break;
    case lepus::RefType::kComputation:
      fml::static_ref_ptr_cast<Computation>(arg0->RefCounted())
          ->OnCleanUp(*arg1);
      break;
    case lepus::RefType::kMemo:
      fml::static_ref_ptr_cast<Memo>(arg0->RefCounted())->OnCleanUp(*arg1);
      break;
    default:
      LOGW("FiberCleanUp's first arg can not be cleaned up.");
  }

  RETURN_UNDEFINED();
}

/* Element API END */

RENDERER_FUNCTION_CC(SetSourceMapRelease) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "SetSourceMapRelease");
  CHECK_ARGC_EQ(SendGlobalEvent, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, Object, SetSourceMapRelease);
  LEPUS_CONTEXT()->SetSourceMapRelease(*arg0);
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(ReportError) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ReportError");
  if (LEPUS_CONTEXT()->IsLepusNGContext()) {
    CHECK_ARGC_GE(ReportError, 1);

    CONVERT_ARG(arg0, 0);
    BASE_STATIC_STRING_DECL(kMessage, "message");
    BASE_STATIC_STRING_DECL(kStack, "stack");
    std::string error_message;
    std::string error_stack;
    if (arg0->IsObject()) {
      error_message = arg0->GetProperty(kMessage).ToString();
      error_stack = arg0->GetProperty(kStack).ToString();
    } else {
      error_message = arg0->ToString();
    }

    int error_code = error::E_MTS_RUNTIME_ERROR;
    int error_level = static_cast<int>(base::LynxErrorLevel::Error);
    if (argc >= 2) {
      CONVERT_ARG_AND_CHECK(arg1, 1, Object, ReportError);
      BASE_STATIC_STRING_DECL(kErrorCode, "errorCode");
      BASE_STATIC_STRING_DECL(kErrorLevel, "errorLevel");
      // compat BTS lynx.reportError.
      BASE_STATIC_STRING_DECL(kLevel, "level");
      BASE_STATIC_STRING_DECL(kWarning, "warning");
      if (arg1->Contains(kErrorCode)) {
        error_code = static_cast<int>(arg1->GetProperty(kErrorCode).Number());
      }
      if (arg1->Contains(kErrorLevel)) {
        error_level = static_cast<int>(arg1->GetProperty(kErrorLevel).Number());
      } else if (arg1->Contains(kLevel)) {
        error_level = arg1->GetProperty(kLevel).String() == kWarning
                          ? static_cast<int>(base::LynxErrorLevel::Warn)
                          : static_cast<int>(base::LynxErrorLevel::Error);
      }
      if (error_level < static_cast<int>(base::LynxErrorLevel::Error) ||
          error_level > static_cast<int>(base::LynxErrorLevel::Warn)) {
        error_level = static_cast<int>(base::LynxErrorLevel::Error);
      }
    }
    LEPUS_CONTEXT()->ReportErrorWithMsg(std::move(error_message),
                                        std::move(error_stack), error_code,
                                        error_level);
  }
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(LynxAddReporterCustomInfo) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "lynx.AddReporterCustomInfo");
  if (LEPUS_CONTEXT()->IsLepusNGContext()) {
    CHECK_ARGC_GE(LynxAddReporterCustomInfo, 1);

    CONVERT_ARG(arg0, 0);
    std::unordered_map<std::string, std::string> info;
    if (arg0->IsObject()) {
      tasm::ForEachLepusValue(
          *arg0, [&info](const lepus::Value& key, const lepus::Value& value) {
            if (key.IsString() && value.IsString()) {
              info[key.StdString()] = value.StdString();
            }
          });
    }
    if (!info.empty()) {
      LEPUS_CONTEXT()->AddReporterCustomInfo(info);
    }
  }
  RETURN_UNDEFINED();
}

/* AirElement API BEGIN */
RENDERER_FUNCTION_CC(AirCreateElement) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirCreateElement");
  // parameter size >= 2
  // [0] String -> element's tag
  // [1] Number -> element's lepus_id
  // [2] Bool -> use_opt
  // [3] Object -> style
  // [4] Object -> attribute
  // [5] String -> class
  // [6] String -> id
  // [7] Refcounted ->parent
  // [8] Number -> impl
  // [9] Number -> lepus_key
  // [10] Number -> event_type
  // [11] String -> event_name
  CHECK_ARGC_GE(AirCreateElement, 3);
  CONVERT_ARG_AND_CHECK(arg0, 0, String, AirCreateElement);
  CONVERT_ARG_AND_CHECK(arg1, 1, Number, AirCreateElement);
  CONVERT_ARG_AND_CHECK(arg2, 2, Bool, AirCreateElement);
  auto tag = arg0->String();
  const auto& lepus_id = static_cast<int32_t>(arg1->Number());
  auto& manager = GET_TASM_POINTER()->page_proxy()->element_manager();
  int32_t impl_id = -1;
  uint64_t key = 0;
  if (argc >= 10) {
    GET_IMPL_ID_AND_KEY(impl_id, 8, key, 9, AirCreateElement);
  }
  key = key > 0 ? key : manager->AirRoot()->GetKeyForCreatedElement(lepus_id);

  fml::RefPtr<AirLepusRef> elementRef =
      manager->CreateAirNode(tag, lepus_id, impl_id, key);
  AirElement* element = elementRef->Get();
  ON_AIR_NODE_CREATED(element);
  // While create_opt is on, attributes,styles,class,id,parent will be
  // compressed to one command.
  // for example:
  // image_node = __AirCreateElement(image, 2, true, {30: '75rpx'}, {src:
  // test_url}, product-cover, product_cover, $parent); Static inline style,
  // attribute, class, id would be converted to arg3, arg4, arg5, arg6 ,
  // respectively.
  bool create_opt = arg2->Bool();
  if (create_opt) {
    CONVERT_ARG(arg3, 3);
    CONVERT_ARG(arg4, 4);
    CONVERT_ARG(arg5, 5);
    CONVERT_ARG(arg6, 6);
    CONVERT_ARG(arg7, 7);
    // Should be compatible with previous template.
    // In the new function SetAirElement, the initial parameter is designated as
    // the "parent", while the following parameters contain distinct data bases
    // on different templates. This strategy is implemented to sustain a
    // systematic arrangement of parameters when additional ones are included in
    // the future.
    // The fundamental order of parameters is (parent, styles, attributes,
    // classes, id, event, dataset).
    if (argc >= 12) {
      CONVERT_ARG_AND_CHECK(arg10, 10, Object, AirCreateElement);
      CONVERT_ARG_AND_CHECK(arg11, 11, Object, AirCreateElement);
      lepus::Value* new_argv[] = {arg7, arg3, arg4, arg5, arg6, arg10, arg11};
      if (auto exception = SetAirElement(ctx, element, new_argv, 7)) {
        return *exception;
      }
    } else {
      lepus::Value* new_argv[] = {arg7, arg3, arg4, arg5, arg6};
      if (auto exception = SetAirElement(ctx, element, new_argv, 5)) {
        return *exception;
      }
    }
  }
  RETURN(lepus::Value(std::move(elementRef)));
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirGetElement) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirGetElement");
  CHECK_ARGC_GE(AirGetElement, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, String, AirGetElement);
  CONVERT_ARG_AND_CHECK(arg1, 1, Number, AirGetElement);

  auto tag = arg0->String();
  const auto& lepus_id = static_cast<int32_t>(arg1->Number());

  auto* self = GET_TASM_POINTER();
  auto& manager = self->page_proxy()->element_manager();

  auto result = manager->GetAirNode(tag, lepus_id);
  if (result) {
    RETURN(lepus::Value(std::move(result)));
  }
  RETURN_UNDEFINED();
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirCreatePage) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirCreatePage");
  // parameter size >= 2
  // [0] String -> componentID
  // [1] Number -> component/page's lepus id
  CHECK_ARGC_GE(AirCreatePage, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, String, AirCreatePage);
  CONVERT_ARG_AND_CHECK(arg1, 1, Number, AirCreatePage);

  constexpr static const char* kCard = "card";

  auto* self = GET_TASM_POINTER();
  auto& manager = self->page_proxy()->element_manager();
  auto page = manager->CreateAirPage(arg1->Int32());
  const auto& entry = self->FindEntry(tasm::DEFAULT_ENTRY_NAME);
  page->SetContext(self->context(tasm::DEFAULT_ENTRY_NAME));
  page->SetRadon(entry->compile_options().radon_mode_ ==
                 CompileOptionRadonMode::RADON_MODE_RADON);
  page->SetParsedStyles(entry->GetComponentParsedStyles(kCard));

  int tid = (int)arg1->Number();
  auto it = self->page_moulds().find(tid);
  PageMould* pm = it->second.get();
  page->DeriveFromMould(pm);
  if (argc >= 4) {
    CONVERT_ARG_AND_CHECK(arg2, 2, Bool, AirCreatePage);
    CONVERT_ARG_AND_CHECK(arg3, 3, Number, AirCreatePage);
    page->SetEnableAsyncCalc(arg2->Bool());
    page->InitFirstScreenList(arg3->Number());
  }
  RETURN(lepus::Value(
      AirLepusRef::Create(manager->air_node_manager()->Get(page->impl_id()))));
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirCreateComponent) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirCreateComponent");
  CHECK_ARGC_GE(AirCreateComponent, 4);
  CONVERT_ARG_AND_CHECK(arg0, 0, Number, AirCreateComponent);
  CONVERT_ARG_AND_CHECK(arg1, 1, String, AirCreateComponent);
  CONVERT_ARG_AND_CHECK(arg2, 2, String, AirCreateComponent);
  CONVERT_ARG_AND_CHECK(arg3, 3, Number, AirCreateComponent);
  int32_t lepus_id = static_cast<int32_t>(arg3->Number());
  int tid = (int)arg0->Number();

  int32_t impl_id = -1;
  uint64_t key = 0;
  if (argc >= 6) {
    GET_IMPL_ID_AND_KEY(impl_id, 4, key, 5, AirCreateComponent);
  }

  auto* self = GET_TASM_POINTER();
  tasm::ElementManager* element_manager =
      self->page_proxy()->element_manager().get();

  lepus::Context* context = LEPUS_CONTEXT();
  auto cm_it = self->component_moulds(context->name()).find(tid);
  DCHECK(cm_it != self->component_moulds(context->name()).end());
  ComponentMould* cm = cm_it->second.get();

  std::shared_ptr<AirComponentElement> component =
      std::make_shared<AirComponentElement>(element_manager, tid, lepus_id,
                                            impl_id, LEPUS_CONTEXT());
  ON_AIR_NODE_CREATED(component.get());
  component->DeriveFromMould(cm);
  auto res = AirLepusRef::Create(component);
  key = key > 0 ? key
                : element_manager->AirRoot()->GetKeyForCreatedElement(lepus_id);
  element_manager->air_node_manager()->Record(component->impl_id(), component);
  element_manager->air_node_manager()->RecordForLepusId(component->GetLepusId(),
                                                        key, res);

  if (argc >= 7) {
    // In the new proposal about Lepus Tree, the unique id of parent element is
    // provided. This is to accomplish the insert operation in the create
    // function to reduce the number of render function calls.
    CONVERT_ARG(arg6, 6);
    if (argc >= 13) {
      lepus::Value* new_argv[7];
      for (int i = 6; i < 13; i++) {
        new_argv[i - 6] = &argv[i];
      }
      if (auto exception = SetAirElement(
              ctx, static_cast<AirElement*>(component.get()), new_argv, 7)) {
        return *exception;
      }
    } else {
      if (arg6->IsNumber()) {
        auto parent = element_manager->air_node_manager()->Get(
            static_cast<int>(arg6->Number()));
        if (parent) {
          parent->InsertNode(component.get());
        }
      }
    }
  }

  component->SetName(arg1->String());
  component->SetPath(arg2->String());

  const auto& entry = self->FindEntry(tasm::DEFAULT_ENTRY_NAME);
  component->SetParsedStyles(
      entry->GetComponentParsedStyles(arg2->StdString()));
  RETURN(lepus::Value(std::move(res)));
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirCreateBlock) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirCreateBlock");
  // parameter size >= 1
  // [1] Number -> air element's lepus id
  CHECK_ARGC_GE(AirCreateBlock, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, Number, AirCreateBlock);
  const auto& lepus_id = static_cast<int32_t>(arg0->Number());

  int32_t impl_id = -1;
  uint64_t key = 0;
  if (argc >= 3) {
    GET_IMPL_ID_AND_KEY(impl_id, 1, key, 2, AirCreateBlock);
  }

  auto* self = GET_TASM_POINTER();
  auto& manager = self->page_proxy()->element_manager();

  std::shared_ptr<AirBlockElement> block =
      std::make_shared<AirBlockElement>(manager.get(), lepus_id, impl_id);
  auto res = AirLepusRef::Create(block);
  key = key > 0 ? key : manager->AirRoot()->GetKeyForCreatedElement(lepus_id);
  manager->air_node_manager()->Record(block->impl_id(), block);
  manager->air_node_manager()->RecordForLepusId(block->GetLepusId(), key, res);

  if (argc >= 4) {
    // In the new proposal about Lepus Tree, the unique id of parent element is
    // provided to accomplish the create and insert operation in one render
    // function.
    CONVERT_ARG(arg3, 3);
    if (arg3->IsNumber()) {
      auto parent =
          manager->air_node_manager()->Get(static_cast<int>(arg3->Number()));
      if (parent) {
        parent->InsertNode(block.get());
      }
    }
  }

  RETURN(lepus::Value(std::move(res)));
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirCreateIf) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirCreateIf");
  // parameter size >= 1
  // [1] Number -> air element's lepus id
  CHECK_ARGC_GE(AirCreateIf, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, Number, AirCreateIf);
  const auto& lepus_id = static_cast<int32_t>(arg0->Number());

  int32_t impl_id = -1;
  uint64_t key = 0;
  if (argc >= 3) {
    GET_IMPL_ID_AND_KEY(impl_id, 1, key, 2, AirCreateIf);
  }

  auto* self = GET_TASM_POINTER();
  auto& manager = self->page_proxy()->element_manager();

  std::shared_ptr<AirIfElement> if_element =
      std::make_shared<AirIfElement>(manager.get(), lepus_id, impl_id);
  auto res = AirLepusRef::Create(if_element);
  key = key > 0 ? key : manager->AirRoot()->GetKeyForCreatedElement(lepus_id);
  manager->air_node_manager()->Record(if_element->impl_id(), if_element);
  manager->air_node_manager()->RecordForLepusId(if_element->GetLepusId(), key,
                                                res);

  if (argc >= 5) {
    // In the new proposal about Lepus Tree, the unique id of parent element and
    // active branch index of tt:if are also provided.
    CONVERT_ARG(arg3, 3);
    if (arg3->IsNumber()) {
      auto parent =
          manager->air_node_manager()->Get(static_cast<int>(arg3->Number()));
      if (parent) {
        parent->InsertNode(if_element.get());
      }
    }
    CONVERT_ARG_AND_CHECK(arg4, 4, Number, AirCreateIf);
    if_element->UpdateIfIndex(static_cast<int32_t>(arg4->Number()));
  }

  RETURN(lepus::Value(std::move(res)));
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirCreateRadonIf) { RETURN_UNDEFINED(); }

RENDERER_FUNCTION_CC(AirCreateFor) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirCreateFor");
  // parameter size >= 1
  // [1] Number -> air element's lepus id
  CHECK_ARGC_GE(AirCreateFor, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, Number, AirCreateFor);
  const auto& lepus_id = static_cast<int32_t>(arg0->Number());

  int32_t impl_id = -1;
  uint64_t key = 0;
  if (argc >= 3) {
    GET_IMPL_ID_AND_KEY(impl_id, 1, key, 2, AirCreateFor);
  }

  auto* self = GET_TASM_POINTER();
  auto& manager = self->page_proxy()->element_manager();

  std::shared_ptr<AirForElement> for_element =
      std::make_shared<AirForElement>(manager.get(), lepus_id, impl_id);
  auto res = AirLepusRef::Create(for_element);
  key = key > 0 ? key : manager->AirRoot()->GetKeyForCreatedElement(lepus_id);
  manager->air_node_manager()->Record(for_element->impl_id(), for_element);
  manager->air_node_manager()->RecordForLepusId(for_element->GetLepusId(), key,
                                                res);

  if (argc >= 5) {
    // In the new proposal about Lepus Tree, the unique id of parent element and
    // child element count of tt:for are also provided.
    CONVERT_ARG(arg3, 3);
    if (arg3->IsNumber()) {
      auto parent =
          manager->air_node_manager()->Get(static_cast<int>(arg3->Number()));
      if (parent) {
        parent->InsertNode(for_element.get());
      }
    }
    CONVERT_ARG_AND_CHECK(arg4, 4, Number, AirCreateFor);
    for_element->UpdateChildrenCount(static_cast<uint32_t>(arg4->Number()));
  }

  RETURN(lepus::Value(std::move(res)));
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirCreatePlug) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirCreatePlug");
  // TODO(liuli) support plug and slot later
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirCreateSlot) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirCreateSlot");
  // TODO(liuli) support plug and slot later
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirAppendElement) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirAppendElement");
  // parameter size = 2
  // [0] ptr -> parent element
  // [1] ptr -> child element
  CHECK_ARGC_EQ(AirAppendElement, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, RefCounted, AirAppendElement);
  CONVERT_ARG_AND_CHECK(arg1, 1, RefCounted, AirAppendElement);
  auto parent =
      fml::static_ref_ptr_cast<AirLepusRef>(arg0->RefCounted())->Get();
  auto child = fml::static_ref_ptr_cast<AirLepusRef>(arg1->RefCounted())->Get();
  if (child->parent() != nullptr) {
    RETURN_UNDEFINED();
  }
  parent->InsertNode(child);
  RETURN(lepus::Value(child));
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirRemoveElement) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirRemoveElement");
  // parameter size = 2
  // [0] ptr -> parent element
  // [1] ptr -> child element
  CHECK_ARGC_EQ(AirRemoveElement, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, RefCounted, AirRemoveElement);
  CONVERT_ARG_AND_CHECK(arg1, 1, RefCounted, AirRemoveElement);
  auto parent =
      fml::static_ref_ptr_cast<AirLepusRef>(arg0->RefCounted())->Get();
  auto child = fml::static_ref_ptr_cast<AirLepusRef>(arg1->RefCounted())->Get();
  parent->RemoveNode(child);
  RETURN(lepus::Value(child));
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirInsertElementBefore) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirInsertElementBefore");
  // parameter size = 3
  // [0] ptr -> parent element
  // [1] ptr -> child element
  // [2] ptr|null|Undefined -> ref element
  CHECK_ARGC_EQ(AirInsertElementBefore, 3);
  CONVERT_ARG_AND_CHECK(arg0, 0, RefCounted, AirInsertElementBefore);
  CONVERT_ARG_AND_CHECK(arg1, 1, RefCounted, AirInsertElementBefore);
  CONVERT_ARG(arg2, 2)
  auto parent =
      fml::static_ref_ptr_cast<AirLepusRef>(arg0->RefCounted())->Get();
  auto child = fml::static_ref_ptr_cast<AirLepusRef>(arg1->RefCounted())->Get();
  if (arg2 && arg2->RefCounted()) {
    auto ref = fml::static_ref_ptr_cast<AirLepusRef>(arg2->RefCounted())->Get();
    parent->InsertNodeBefore(child, ref);
  } else {
    parent->InsertNode(child);
  }
  RETURN(lepus::Value(child));
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirGetElementUniqueID) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirGetElementUniqueID");
  // parameter size = 1
  // [0] RefCounted -> element
  CHECK_ARGC_EQ(AirGetElementUniqueID, 1);
  CONVERT_ARG(arg0, 0);

  int64_t unique_id = -1;
  if (arg0->RefCounted()) {
    auto* element =
        fml::static_ref_ptr_cast<AirLepusRef>(arg0->RefCounted())->Get();
    unique_id = element->impl_id();
  }
  RETURN(lepus::Value(unique_id));
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirGetElementTag) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirGetElementTag");
  // parameter size = 1
  // [0] RefCounted -> element
  CHECK_ARGC_EQ(AirGetElementTag, 1);
  CONVERT_ARG(arg0, 0);

  auto* element =
      fml::static_ref_ptr_cast<AirLepusRef>(arg0->RefCounted())->Get();
  RETURN(lepus::Value(element->GetTag()));
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirSetAttribute) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirSetAttribute");
  // parameter size = 3
  // [0] ptr -> element
  // [1] String -> key
  // [2] any -> value
  CHECK_ARGC_EQ(AirSetAttribute, 3);
  CONVERT_ARG_AND_CHECK(arg0, 0, RefCounted, AirSetAttribute);
  CONVERT_ARG_AND_CHECK(arg1, 1, String, AirSetAttribute);
  CONVERT_ARG(arg2, 2)

  auto element =
      fml::static_ref_ptr_cast<AirLepusRef>(arg0->RefCounted())->Get();
  element->SetAttribute(arg1->String(), *arg2, !element->EnableAsyncCalc());
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirSetInlineStyles) {
#if ENABLE_AIR
  // parameter size = 2
  // [0] ptr -> element
  // [1] value -> styles
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirSetInlineStyles");
  CHECK_ARGC_EQ(AirSetInlineStyles, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, RefCounted, AirSetInlineStyles);
  CONVERT_ARG(arg1, 1);
  auto* element =
      fml::static_ref_ptr_cast<AirLepusRef>(arg0->RefCounted())->Get();
  element->SetInlineStyle(arg1->StdString(), !element->EnableAsyncCalc());
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirSetEvent) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirSetEvent");
  CHECK_ARGC_EQ(AirSetEvent, 4);
  CONVERT_ARG_AND_CHECK(arg0, 0, RefCounted, AirSetEvent);
  CONVERT_ARG_AND_CHECK(type, 1, String, AirSetEvent);
  CONVERT_ARG_AND_CHECK(name, 2, String, AirSetEvent);
  CONVERT_ARG(callback, 3);
  auto* element =
      fml::static_ref_ptr_cast<AirLepusRef>(arg0->RefCounted())->Get();
  auto type_str = type->String();
  auto name_str = name->String();
  if (callback->IsString()) {
    element->SetEventHandler(
        name_str, element->SetEvent(type_str, name_str, callback->String()));
  }
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirSetID) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirSetID");
  CHECK_ARGC_EQ(AirSetID, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, RefCounted, SetId);
  CONVERT_ARG(arg1, 1);

  // if arg1 is not a String, it will return empty string
  auto* element =
      fml::static_ref_ptr_cast<AirLepusRef>(arg0->RefCounted())->Get();
  element->SetIdSelector(*arg1);
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirGetElementByID) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirGetElementByID");
  CHECK_ARGC_EQ(AirGetElementByID, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, String, AirGetElementByID);
  const auto& id = arg0->StdString();

  if (!id.empty()) {
    auto* self = GET_TASM_POINTER();
    auto& manager = self->page_proxy()->element_manager();
    auto element = manager->air_node_manager()->GetCustomId(id);
    RETURN(lepus::Value(AirLepusRef::Create(element)));
  }
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirGetElementByUniqueID) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirGetElementByUniqueID");
  CHECK_ARGC_EQ(AirGetElementByUniqueID, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, Number, AirGetElementByUniqueID);
  int id = static_cast<int>(arg0->Number());

  auto* self = GET_TASM_POINTER();
  auto& manager = self->page_proxy()->element_manager();
  auto element = manager->air_node_manager()->Get(id);
  if (element) {
    RETURN(lepus::Value(AirLepusRef::Create(element)));
  } else {
    RETURN_UNDEFINED();
  }
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirGetRootElement) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirGetRootElement");

  auto* self = GET_TASM_POINTER();
  auto& manager = self->page_proxy()->element_manager();
  AirElement* element = static_cast<AirElement*>(manager->AirRoot());
  if (element) {
    RETURN((lepus::Value(AirLepusRef::Create(
        manager->air_node_manager()->Get(element->impl_id())))));
  } else {
    RETURN_UNDEFINED();
  }
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirGetElementByLepusID) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirGetElementByLepusID");
  CHECK_ARGC_EQ(AirGetElementByLepusID, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, Number, AirGetElementByLepusID);

  int tag = static_cast<int>(arg0->Int64());

  auto* self = GET_TASM_POINTER();
  auto& manager = self->page_proxy()->element_manager();

  auto array = lepus::CArray::Create();
  AirPageElement* page = manager->AirRoot();
  auto* cur_for_element = page->GetCurrentForElement();
  auto* cur_component_element = page->GetCurrentComponentElement();
  if (cur_component_element) {
    if (!cur_for_element ||
        (cur_for_element &&
         cur_component_element->GetLepusId() > cur_for_element->GetLepusId())) {
      auto elements = manager->air_node_manager()->GetAllNodesForLepusId(tag);
      for (auto& element : elements) {
        if (element->Get()->GetParentComponent() == cur_component_element) {
          array->emplace_back(std::move(element));
        }
      }
    } else if (cur_for_element) {
      uint64_t key = page->GetKeyForCreatedElement(tag);
      auto node = manager->air_node_manager()->GetForLepusId(tag, key);
      if (node) {
        array->emplace_back(node);
      }
    }
  } else if (cur_for_element) {
    uint64_t key = page->GetKeyForCreatedElement(tag);
    auto node = manager->air_node_manager()->GetForLepusId(tag, key);
    if (node) {
      array->emplace_back(node);
    }
  } else {
    auto elements = manager->air_node_manager()->GetAllNodesForLepusId(tag);
    for (auto& element : elements) {
      array->emplace_back(element);
    }
  }

  RETURN(lepus::Value(std::move(array)));
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirUpdateIfNodeIndex) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirUpdateIfNodeIndex");
  CHECK_ARGC_EQ(AirUpdateIfNodeIndex, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, RefCounted, AirUpdateIfNodeIndex);
  CONVERT_ARG_AND_CHECK(arg1, 1, Number, AirUpdateIfNodeIndex);

  auto* element =
      fml::static_ref_ptr_cast<AirLepusRef>(arg0->RefCounted())->Get();
  int32_t index = static_cast<int32_t>(arg1->Int64());
  AirElementType type = element->GetElementType();
  if (type == kAirIf) {
    auto* if_element = static_cast<AirIfElement*>(element);
    if_element->UpdateIfIndex(index);
    RETURN_UNDEFINED();
  }
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirUpdateForNodeIndex) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirUpdateForNodeIndex");
  CHECK_ARGC_EQ(AirUpdateForNodeIndex, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, RefCounted, AirUpdateForNodeIndex);
  CONVERT_ARG_AND_CHECK(arg1, 1, Number, AirUpdateForNodeIndex);

  auto* element =
      fml::static_ref_ptr_cast<AirLepusRef>(arg0->RefCounted())->Get();
  if (element->is_for()) {
    uint32_t index = static_cast<uint32_t>(arg1->Int64());
    static_cast<AirForElement*>(element)->UpdateActiveIndex(index);
  }
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirUpdateForChildCount) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirUpdateForChildCount");
  CHECK_ARGC_EQ(AirUpdateForChildCount, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, RefCounted, AirUpdateForChildCount);
  CONVERT_ARG_AND_CHECK(arg1, 1, Number, AirUpdateForChildCount);

  auto* element =
      fml::static_ref_ptr_cast<AirLepusRef>(arg0->RefCounted())->Get();
  if (element->is_for()) {
    uint32_t count = static_cast<uint32_t>(arg1->Number());
    static_cast<AirForElement*>(element)->UpdateChildrenCount(count);
  }
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirGetForNodeChildWithIndex) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirGetForNodeChildWithIndex");
  CHECK_ARGC_GE(AirGetForNodeChildWithIndex, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, RefCounted, AirGetForNodeChildWithIndex);
  CONVERT_ARG_AND_CHECK(arg1, 1, Number, AirGetForNodeChildWithIndex);

  auto* self = GET_TASM_POINTER();
  auto& manager = self->page_proxy()->element_manager();

  auto* node = static_cast<AirForElement*>(
      fml::static_ref_ptr_cast<AirLepusRef>(arg0->RefCounted())->Get());
  uint32_t index = arg1->Number();
  auto* active_node = node->GetForNodeChildWithIndex(index);
  RETURN(lepus::Value(AirLepusRef::Create(
      manager->air_node_manager()->Get(active_node->impl_id()))));
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirPushForNode) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirPushForNode");
  CHECK_ARGC_EQ(AirPushForNode, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, RefCounted, AirPushForNode);

  auto* element = static_cast<AirForElement*>(
      fml::static_ref_ptr_cast<AirLepusRef>(arg0->RefCounted())->Get());
  auto* self = GET_TASM_POINTER();
  auto& manager = self->page_proxy()->element_manager();
  manager->AirRoot()->PushForElement(element);
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirPopForNode) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirPopForNode");

  auto* self = GET_TASM_POINTER();
  auto& manager = self->page_proxy()->element_manager();
  manager->AirRoot()->PopForElement();
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirGetChildElementByIndex) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirGetChildElementByIndex");
  CHECK_ARGC_EQ(AirGetChildElementByIndex, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, RefCounted, AirGetChildElementByIndex);
  CONVERT_ARG_AND_CHECK(arg1, 1, Number, AirGetChildElementByIndex);

  auto* ele = fml::static_ref_ptr_cast<AirLepusRef>(arg0->RefCounted())->Get();
  uint32_t index = static_cast<uint32_t>(arg1->Number());

  auto* child = ele->GetChildAt(index);

  if (child) {
    RETURN(lepus::Value(child));
  }
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirPushDynamicNode) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirPushDynamicNode");
  CHECK_ARGC_GE(PushDynamicNode, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, RefCounted, PushDynamicNode);
  CONVERT_ARG_AND_CHECK(arg1, 1, RefCounted, PushDynamicNode);

  auto* node = fml::static_ref_ptr_cast<AirLepusRef>(arg0->RefCounted())->Get();
  auto* child =
      fml::static_ref_ptr_cast<AirLepusRef>(arg1->RefCounted())->Get();
  node->PushDynamicNode(child);
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirGetDynamicNode) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirGetDynamicNode");
  CHECK_ARGC_GE(AirGetDynamicNode, 3);
  CONVERT_ARG_AND_CHECK(arg0, 0, RefCounted, AirGetDynamicNode);
  CONVERT_ARG_AND_CHECK(arg1, 1, Number, AirGetDynamicNode);
  CONVERT_ARG_AND_CHECK(arg2, 2, Number, AirGetDynamicNode);

  auto* self = GET_TASM_POINTER();
  auto& manager = self->page_proxy()->element_manager();

  auto* node = fml::static_ref_ptr_cast<AirLepusRef>(arg0->RefCounted())->Get();
  uint32_t index = arg1->Number();
  uint32_t node_index = arg2->Number();
  auto* element = node->GetDynamicNode(index, node_index);
  RETURN(lepus::Value(AirLepusRef::Create(
      manager->air_node_manager()->Get(element->impl_id()))));
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirSetComponentProp) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirSetComponentProp");
  CHECK_ARGC_EQ(AirSetComponentProp, 3);
  CONVERT_ARG_AND_CHECK(arg0, 0, RefCounted, AirSetComponentProp);
  CONVERT_ARG_AND_CHECK(arg1, 1, String, AirSetComponentProp);
  CONVERT_ARG(arg2, 2);

  auto* element =
      fml::static_ref_ptr_cast<AirLepusRef>(arg0->RefCounted())->Get();
  if (element->is_component()) {
    static_cast<AirComponentElement*>(element)->SetProperty(arg1->String(),
                                                            *arg2);
  }

#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirRenderComponentInLepus) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirRenderComponentInLepus");
  DCHECK(ARGC() == 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, RefCounted, AirRenderComponentInLepus);

  auto* component = static_cast<AirComponentElement*>(
      fml::static_ref_ptr_cast<AirLepusRef>(arg0->RefCounted())->Get());
  component->CreateComponentInLepus();
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirUpdateComponentInLepus) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirUpdateComponentInLepus");
  CHECK_ARGC_GE(AirUpdateComponentInLepus, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, RefCounted, AirUpdateComponentInLepus);
  CONVERT_ARG_AND_CHECK(arg1, 1, Object, AirUpdateComponentInLepus);

  auto* component = static_cast<AirComponentElement*>(
      fml::static_ref_ptr_cast<AirLepusRef>(arg0->RefCounted())->Get());
  component->UpdateComponentInLepus(*arg1);
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirGetComponentInfo) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirGetComponentInfo");
  CHECK_ARGC_EQ(AirGetComponentInfo, 1);
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirUpdateComponentInfo) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirUpdateComponentInfo");
  CHECK_ARGC_GE(AirUpdateComponentInfo, 4);
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirGetData) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirGetData");
  CHECK_ARGC_EQ(AirGetData, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, RefCounted, AirGetData);

  auto* element =
      fml::static_ref_ptr_cast<AirLepusRef>(arg0->RefCounted())->Get();
  RETURN(lepus::Value(element->GetData()));
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirGetProps) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirGetProps");
  CHECK_ARGC_EQ(AirGetProps, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, RefCounted, AirGetProps);

  auto* element =
      fml::static_ref_ptr_cast<AirLepusRef>(arg0->RefCounted())->Get();
  RETURN(lepus::Value(element->GetProperties()));
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirSetData) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirSetData");
  CHECK_ARGC_GE(AirSetData, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, RefCounted, AirSetData);
  CONVERT_ARG(arg1, 1);

  auto* element =
      fml::static_ref_ptr_cast<AirLepusRef>(arg0->RefCounted())->Get();
  AirElement* component;
  if (element->is_page() || element->is_component()) {
    component = element;
  } else {
    component = element->GetParentComponent();
  }

  if (component && component->is_page()) {
    auto* page = static_cast<AirPageElement*>(component);
    UpdatePageOption update_option;
    update_option.update_first_time = false;
    update_option.from_native = false;
    PipelineOptions pipeline_options;
    page->UpdatePageData(*arg1, update_option, pipeline_options);
  } else if (component && component->is_component()) {
    static_cast<AirComponentElement*>(component)->SetData(*arg1);
  }
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirFlushElement) {
#if ENABLE_AIR
  // parameter size == 1
  // [0] RefCounted -> air element
  CHECK_ARGC_EQ(AirFlushElement, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, RefCounted, AirFlushElement);

  auto element =
      fml::static_ref_ptr_cast<AirLepusRef>(arg0->RefCounted())->Get();
  if (!element->is_page()) {
    element->FlushProps();
  }
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirFlushElementTree) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirFlushRecursively");
  CHECK_ARGC_EQ(AirFlushRecursively, 1);

  CONVERT_ARG_AND_CHECK(arg0, 0, RefCounted, AirFlushRecursively);
  auto* element =
      fml::static_ref_ptr_cast<AirLepusRef>(arg0->RefCounted())->Get();
  element->FlushRecursively();
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(TriggerLepusBridge) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TriggerLepusBridge");
  CHECK_ARGC_GE(TriggerLepusBridge, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, Object, TriggerLepusBridge);

  constexpr const static char* kCallAsync = "call";

  BASE_STATIC_STRING_DECL(kLepusMethod, "lepusMethod");
  BASE_STATIC_STRING_DECL(kEventDetail, "methodDetail");
  BASE_STATIC_STRING_DECL(kEventEntryName, "tasmEntryName");
  BASE_STATIC_STRING_DECL(kLepusUseUIThreadKey, "lepusUseUIThread");
  BASE_STATIC_STRING_DECL(kFromPiper, "fromPiper");

  auto tasm = GET_TASM_POINTER();
  auto dictionary = lepus::Dictionary::Create();
  if (arg0->GetProperty(kLepusUseUIThreadKey).IsTrue()) {
    // When the lepusUseUIThread option is true, it means that we should not
    // switch threads when calling lepusBridge
    dictionary->SetValue(kLepusUseUIThreadKey, true);
  } else {
    BASE_STATIC_STRING_DECL(kUseAirThreadKey, "useAirThread");
    dictionary->SetValue(kUseAirThreadKey, true);
  }
  dictionary->SetValue(kEventDetail, *arg0);
  dictionary->SetValue(kFromPiper, arg0->GetProperty(kFromPiper));
  dictionary->SetValue(kEventEntryName, LEPUS_CONTEXT()->name());
  lepus::Value param(std::move(dictionary));
  auto function_name = arg0->GetProperty(kLepusMethod).StdString();
  function_name = function_name.empty() ? kCallAsync : function_name;
  auto callback_manager = LEPUS_CONTEXT()->GetCallbackManager();
  std::unique_ptr<lepus::Value> callback_closure;
  if (ARGC() == 1) {
    if (LEPUS_CONTEXT()->IsLepusNGContext()) {
      constexpr const static auto default_callback =
          [](LEPUSContext* context, LEPUSValue value, int argc,
             LEPUSValue* argv) -> LEPUSValue { return LEPUS_UNDEFINED; };
      constexpr const static char* kCallback = "callback";
      callback_closure = std::make_unique<lepus::Value>(
          LEPUS_CONTEXT()->context(),
          LEPUS_NewCFunction(LEPUS_CONTEXT()->context(), default_callback,
                             kCallback, 0));
    } else {
      callback_closure = std::make_unique<lepus::Value>(
          lepus::Closure::Create(lepus::Function::Create()));
    }

  } else {
    CONVERT_ARG_AND_CHECK(arg1, 1, Callable, TriggerLepusBridge);
    callback_closure = std::make_unique<lepus::Value>(*arg1);
  }
  auto current_task_id =
      callback_manager->CacheTask(LEPUS_CONTEXT(), std::move(callback_closure));
  BASE_STATIC_STRING_DECL(kEventCallbackId, "callbackId");
  param.Table()->SetValue(kEventCallbackId, current_task_id);
  tasm->TriggerLepusBridgeAsync(function_name, param, true);
#endif
  RETURN_UNDEFINED()
}

RENDERER_FUNCTION_CC(TriggerLepusBridgeSync) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TriggerLepusBridgeSync");
  CHECK_ARGC_GE(TriggerLepusBridge, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, Object, TriggerLepusBridge);

  constexpr const static char* kCallSync = "callSync";
  BASE_STATIC_STRING_DECL(kLepusMethod, "lepusMethod");
  BASE_STATIC_STRING_DECL(kEventDetail, "methodDetail");
  BASE_STATIC_STRING_DECL(kEventEntryName, "tasmEntryName");
  BASE_STATIC_STRING_DECL(kEventCallbackId, "callbackId");
  auto dictionary = lepus::Dictionary::Create();
  dictionary->SetValue(kEventDetail, *arg0);
  dictionary->SetValue(kEventEntryName, LEPUS_CONTEXT()->name());
  dictionary->SetValue(kEventCallbackId, -1);
  lepus::Value param(std::move(dictionary));
  const auto& function_name = arg0->GetProperty(kLepusMethod).StdString();
  auto tasm = GET_TASM_POINTER();
  RETURN(tasm->TriggerLepusBridge(
      function_name.empty() ? kCallSync : function_name, param));
#endif
  RETURN_UNDEFINED()
}

RENDERER_FUNCTION_CC(AirSetDataSet) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirSetDataSet");
  CHECK_ARGC_EQ(AirSetDataSet, 3);
  CONVERT_ARG_AND_CHECK(arg0, 0, RefCounted, AirSetDataSet);
  CONVERT_ARG_AND_CHECK(arg1, 1, String, AirSetDataSet);
  CONVERT_ARG(arg2, 2);

  auto key = arg1->String();
  auto value = CONVERT(arg2);

  auto* element =
      fml::static_ref_ptr_cast<AirLepusRef>(arg0->RefCounted())->Get();
  element->SetDataSet(key, value);
#endif
  RETURN_UNDEFINED()
}

RENDERER_FUNCTION_CC(AirSendGlobalEvent) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirSendGlobalEvent");
  CHECK_ARGC_EQ(AirSendGlobalEvent, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, String, AirSendGlobalEvent);
  CONVERT_ARG(arg1, 1);
  auto* tasm = GET_TASM_POINTER();
  tasm->SendGlobalEventToLepus(arg0->StdString(), *arg1);
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(RemoveEventListener) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RemoveEventListener");
  CHECK_ARGC_GE(RemoveEventListener, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, String, RemoveEventListener);
  auto* tasm = GET_TASM_POINTER();
  tasm->RemoveLepusEventListener(arg0->StdString());
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(SetTimeout) {
  CHECK_ARGC_GE(SetTimeout, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, Callable, SetTimeout);
  int64_t delay = 0;

  if (argc > 1) {
    CONVERT_ARG_AND_CHECK(arg1, 1, Number, SetTimeout);
    delay = arg1->Int64();
  }

  auto callback_manager = LEPUS_CONTEXT()->GetCallbackManager();
  uint32_t task_id = callback_manager->SetTimeOut(
      LEPUS_CONTEXT(), std::make_unique<lepus::Value>(*arg0), delay);
  RETURN(lepus::Value(static_cast<int64_t>(task_id)));
}

RENDERER_FUNCTION_CC(ClearTimeout) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ClearTimeout");
  CHECK_ARGC_GE(ClearTimeout, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, Number, ClearTimeout);

  auto callback_manager = LEPUS_CONTEXT()->GetCallbackManager();
  callback_manager->RemoveTimeTask(static_cast<uint32_t>(arg0->Int64()));
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(SetInterval) {
  CHECK_ARGC_GE(SetInterval, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, Callable, SetInterval);
  int64_t delay = 0;
  if (argc > 1) {
    CONVERT_ARG_AND_CHECK(arg1, 1, Number, SetInterval);
    delay = arg1->Int64();
  }

  auto callback_manager = LEPUS_CONTEXT()->GetCallbackManager();
  uint32_t task_id = callback_manager->SetInterval(
      LEPUS_CONTEXT(), std::make_unique<lepus::Value>(*arg0), delay);
  RETURN(lepus::Value(static_cast<int64_t>(task_id)));
}

RENDERER_FUNCTION_CC(ClearTimeInterval) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ClearTimeInterval");
  CHECK_ARGC_GE(ClearTimeInterval, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, Number, ClearTimeInterval);

  auto callback_manager = LEPUS_CONTEXT()->GetCallbackManager();
  callback_manager->RemoveTimeTask(static_cast<uint32_t>(arg0->Int64()));
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(RequestAnimationFrame) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RequestAnimationFrame");
  CHECK_ARGC_GE(RequestAnimationFrame, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, Callable, RequestAnimationFrame);

  static constexpr int64_t kNanoSecondsPerMilliSecond = 1e+6;

  auto* tasm = GET_TASM_POINTER();
  const std::shared_ptr<AnimationFrameManager> animationFrameManager =
      LEPUS_CONTEXT()->GetAnimationFrameManager();

  tasm->GetDelegate().RequestVsync(
      reinterpret_cast<uintptr_t>(&animationFrameManager),
      [animationFrameManager](int64_t frame_start, int64_t frame_end) {
        animationFrameManager->DoFrame(frame_start /
                                       kNanoSecondsPerMilliSecond);
      });

  uint64_t task_id = animationFrameManager->RequestAnimationFrame(
      LEPUS_CONTEXT(), std::make_unique<lepus::Value>(*arg0));
  RETURN(lepus::Value(task_id));
}

RENDERER_FUNCTION_CC(CancelAnimationFrame) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CancelAnimationFrame");
  CHECK_ARGC_GE(CancelAnimationFrame, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, Number, CancelAnimationFrame);

  auto animationFrameManager = LEPUS_CONTEXT()->GetAnimationFrameManager();
  animationFrameManager->CancelAnimationFrame(
      static_cast<uint32_t>(arg0->Int64()));
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(TriggerComponentEvent) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TriggerComponentEvent");
  CHECK_ARGC_GE(TriggerComponentEvent, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, String, TriggerComponentEvent);
  CONVERT_ARG_AND_CHECK(arg1, 1, Object, TriggerComponentEvent);

  auto tasm = GET_TASM_POINTER();
  tasm->TriggerComponentEvent(arg0->StdString(), *arg1);
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirCreateRawText) {
#if ENABLE_AIR
  BASE_STATIC_STRING_DECL(kRawText, "raw-text");
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirCreateRawText");
  CHECK_ARGC_GE(AirCreateRawText, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, Number, AirCreateRawText);
  const auto& lepus_id = static_cast<int32_t>(arg0->Number());
  int32_t impl_id = -1;
  uint64_t key = 0;
  if (argc >= 5) {
    GET_IMPL_ID_AND_KEY(impl_id, 3, key, 4, AirCreateRawText);
  }

  auto& manager = GET_TASM_POINTER()->page_proxy()->element_manager();
  key = key > 0 ? key : manager->AirRoot()->GetKeyForCreatedElement(lepus_id);
  auto elementRef = manager->CreateAirNode(kRawText, lepus_id, impl_id, key);
  AirElement* element = elementRef->Get();
  ON_AIR_NODE_CREATED(element);
  if (argc >= 3) {
    CONVERT_ARG_AND_CHECK(arg1, 1, Object, AirCreateRawText);
    CONVERT_ARG(arg2, 2);
    tasm::ForEachLepusValue(
        *arg1, [element, enableAsync](const lepus::Value& key,
                                      const lepus::Value& value) {
          element->SetAttribute(key.String(), value, !enableAsync);
        });
    if (arg2->IsRefCounted()) {
      auto parent =
          fml::static_ref_ptr_cast<AirLepusRef>(arg2->RefCounted())->Get();
      parent->InsertNode(element);
    } else if (arg2->IsNumber()) {
      // In the new proposal about Lepus Tree, the third parameter is the unique
      // id of parent element.
      auto parent =
          manager->air_node_manager()->Get(static_cast<int>(arg2->Number()));
      if (parent) {
        parent->InsertNode(element);
      }
    }
  }
  RETURN(lepus::Value(std::move(elementRef)));
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirSetClasses) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirSetClasses");
  CHECK_ARGC_EQ(AirSetClasses, 2);
  CONVERT_ARG_AND_CHECK(arg0, 0, RefCounted, AirSetClasses);
  CONVERT_ARG_AND_CHECK(arg1, 1, String, AirSetClasses);

  auto* element =
      fml::static_ref_ptr_cast<AirLepusRef>(arg0->RefCounted())->Get();
  element->SetClasses(*arg1);
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirPushComponentNode) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirPushComponentNode");
  CHECK_ARGC_EQ(AirPushComponentNode, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, RefCounted, AirPushComponentNode);

  auto* element = static_cast<AirComponentElement*>(
      fml::static_ref_ptr_cast<AirLepusRef>(arg0->RefCounted())->Get());
  auto* self = GET_TASM_POINTER();
  auto& manager = self->page_proxy()->element_manager();
  manager->AirRoot()->PushComponentElement(element);
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirPopComponentNode) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirPopComponentNode");

  auto* self = GET_TASM_POINTER();
  auto& manager = self->page_proxy()->element_manager();
  manager->AirRoot()->PopComponentElement();
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirGetParentForNode) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirGetParentForNode");
  CHECK_ARGC_GE(AirGetParentForNode, 1);
  CONVERT_ARG_AND_CHECK(arg0, 0, RefCounted, AirGetParentForNode);

  auto* element =
      fml::static_ref_ptr_cast<AirLepusRef>(arg0->RefCounted())->Get();
  auto* parent_component_element = element->GetParentComponent();
  auto* air_parent = element->air_parent();
  AirElement* for_node = nullptr;
  while (air_parent != parent_component_element) {
    if (air_parent->is_for()) {
      for_node = air_parent;
      break;
    }
    air_parent = air_parent->air_parent();
  }

  if (for_node) {
    auto* self = GET_TASM_POINTER();
    auto& manager = self->page_proxy()->element_manager();
    return lepus::Value(AirLepusRef::Create(
        manager->air_node_manager()->Get(for_node->impl_id())));
  }
  RETURN_UNDEFINED();
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(AirFlushTree) {
#if ENABLE_AIR
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "AirFlushTree");
  CONVERT_ARG_AND_CHECK(arg0, 0, Object, AirFlushTree);

  auto* self = GET_TASM_POINTER();
  auto& manager = self->page_proxy()->element_manager();
  int page_impl_id = manager->AirRoot()->impl_id();

  // arg0 is an object, the key is an interger and value is an array of lepus
  // elements. The key has the following situations:
  // 1. key is equal to unique id of root node, which means that it is the first
  // screen flush.
  // 2. key is less than 0, which means that all the nodes in value need to be
  // updated separately.
  // 3. in other cases, key represents the root node id of element subtree.
  tasm::ForEachLepusValue(*arg0, [ctx, &manager, page_impl_id](
                                     const lepus::Value& key,
                                     const lepus::Value& value) {
    const auto& key_str = key.StdString();
    int key_id = 0;
    bool ret = base::StringToInt(key_str, &key_id, 10);
    if (!ret) {
      return;
    }
    if (key_id < 0) {
      // flush every single element
      tasm::ForEachLepusValue(value, [ctx](const lepus::Value& idx,
                                           const lepus::Value& lepus_element) {
        UpdateAirElement(ctx, lepus_element, true);
      });
    } else if (key_id == page_impl_id) {
      // first screen
      auto* page = manager->AirRoot();
      page->InitFirstScreenList(value.GetLength());
      tasm::ForEachLepusValue(value, [ctx](const lepus::Value& idx,
                                           const lepus::Value& lepus_element) {
        CreateAirElement(ctx, lepus_element);
      });

      page->FlushRecursively();
    } else {
      // flush subtree
      BASE_STATIC_STRING_DECL(kFlushOp, "flushOp");
      tasm::ForEachLepusValue(
          value, [ctx, &kFlushOp](const lepus::Value& idx,
                                  const lepus::Value& lepus_element) {
            auto flush_op =
                static_cast<int>(lepus_element.GetProperty(kFlushOp).Number());
            if (flush_op == 1) {
              CreateAirElement(ctx, lepus_element);
            } else if (flush_op == 0) {
              UpdateAirElement(ctx, lepus_element, false);
            }
          });
      manager->air_node_manager()->Get(key_id)->FlushRecursively();
    }
  });
#endif
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(InvokeUIMethod) {
  CHECK_ARGC_EQ(InvokeUIMethod, 4);

  // arg0 -> element id array | fiber element
  // arg1 -> method name
  // arg2 -> method params
  // arg3 -> callback
  CONVERT_ARG(arg0, 0);
  CONVERT_ARG_AND_CHECK(arg1, 1, String, InvokeUIMethod);
  CONVERT_ARG_AND_CHECK(arg2, 2, Object, InvokeUIMethod);
  CONVERT_ARG_AND_CHECK(arg3, 3, Callable, InvokeUIMethod);

  std::vector<int32_t> element_ids;
  if (arg0->IsArrayOrJSArray() && arg0->GetLength() > 0) {
    tasm::ForEachLepusValue(
        *arg0, [&element_ids](const auto& index, const auto& value) {
          if (value.IsNumber()) {
            element_ids.emplace_back(static_cast<int32_t>(value.Number()));
          }
        });
  } else if (arg0->IsRefCounted()) {
    const auto element =
        fml::static_ref_ptr_cast<FiberElement>(arg0->RefCounted());
    element_ids.push_back(element->impl_id());
  } else {
    RETURN_UNDEFINED();
  }
  auto* tasm = GET_TASM_POINTER();
  tasm->LepusInvokeUIMethod(std::move(element_ids), arg1->StdString(), *arg2,
                            LEPUS_CONTEXT(),
                            std::make_unique<lepus::Value>(*arg3));
  RETURN_UNDEFINED();
}

#if ENABLE_TRACE_PERFETTO
static void HandleProfileNameAndOption(
    int argc, lepus::Value* argv, lepus::Context* ctx,
    lynx::perfetto::EventContext& event_context) {
  if (argc < 1) {
    return;
  }
  CONVERT_ARG(arg0, 0);
  if (!arg0->IsString()) {
    return;
  }
  event_context.event()->set_name(arg0->StdString());
  if (argc >= 2) {
    CONVERT_ARG(arg1, 1);
    if (!arg1->IsObject()) {
      return;
    }
    auto args = arg1->GetProperty(BASE_STATIC_STRING(runtime::kArgs));
    if (args.IsObject()) {
      tasm::ForEachLepusValue(
          args,
          [&event_context](const lepus::Value& key, const lepus::Value& value) {
            if (key.IsString() && value.IsString()) {
              event_context.event()->add_debug_annotations(key.StdString(),
                                                           value.StdString());
            }
          });
    }
    auto flow_id = arg1->GetProperty(BASE_STATIC_STRING(runtime::kFlowId));
    if (flow_id.IsNumber()) {
      event_context.event()->add_flow_ids(flow_id.Number());
    }
  }
}
#endif

RENDERER_FUNCTION_CC(ProfileStart) {
  // parameter size = >= 1
  // [0] trace name -> String
  // optional -> Object {args: {}, flowId: number}
  TRACE_EVENT_BEGIN(
      LYNX_TRACE_CATEGORY_JAVASCRIPT, nullptr,
      [&argc, &argv, &ctx](lynx::perfetto::EventContext event_context) {
        HandleProfileNameAndOption(argc, argv, ctx, event_context);
      });
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(ProfileEnd) {
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY_JAVASCRIPT);
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(ProfileMark) {
  // parameter size >= 1
  // [0] trace name -> String
  // optional -> Object {args: {}, flowId: number}
  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY_JAVASCRIPT, nullptr,
      [&argc, &argv, &ctx](lynx::perfetto::EventContext event_context) {
        HandleProfileNameAndOption(argc, argv, ctx, event_context);
      });
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION_CC(ProfileFlowId) {
  uint64_t flow_id = TRACE_FLOW_ID();
  return lepus::Value(static_cast<int>(flow_id));
}

RENDERER_FUNCTION_CC(IsProfileRecording) {
#if ENABLE_TRACE_PERFETTO
  return lepus::Value(
      TRACE_EVENT_CATEGORY_ENABLED(LYNX_TRACE_CATEGORY_JAVASCRIPT));
#elif ENABLE_TRACE_SYSTRACE
  return lepus::Value(true);
#else
  return lepus::Value(false);
#endif
}

}  // namespace tasm
}  // namespace lynx
