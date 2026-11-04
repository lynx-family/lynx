// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "third_party/binding/napi/canvas_round_rect_radii.h"

namespace lynx::binding {
namespace {

bool ToDouble(Napi::Value value, double& result) {
  auto number = NativeValueTraits<IDLNumber>::NativeValue(value);
  if (value.Env().IsExceptionPending()) return false;
  result = number.DoubleValue();
  return true;
}

// CanvasPath.roundRect's standard union contains numbers and DOMPointInit.
// Convert a number to (r, r), or a point to its x/y radii with zero defaults.
bool AppendRadius(Napi::Value value, std::vector<double>& result) {
  const auto env = value.Env();
  double x = 0;
  double y = 0;
  if (value.IsNull() || value.IsUndefined()) {
    result.insert(result.end(), {0, 0});
    return true;
  }
  if (value.IsObject() || value.IsFunction()) {
    const auto point = value.ToObject();
    // WebIDL reads all dictionary members in lexicographic order. w/z do
    // not draw anything, but their getters and conversions can throw.
    for (const auto* name : {"w", "x", "y", "z"}) {
      auto member = point.Get(name);
      if (env.IsExceptionPending()) return false;
      if (member.IsUndefined()) continue;
      double number;
      if (!ToDouble(member, number)) return false;
      if (name[0] == 'x') x = number;
      if (name[0] == 'y') y = number;
    }
  } else {
    if (!ToDouble(value, x)) return false;
    y = x;
  }
  result.insert(result.end(), {x, y});
  return true;
}

// IteratorClose must preserve the original conversion exception even if the
// iterator's return getter or function throws while being closed.
void CloseIteratorAfterException(Napi::Object iterator) {
  auto env = iterator.Env();
  auto error = env.GetAndClearPendingException();
  auto close = iterator.Get("return");
  if (!env.IsExceptionPending() && close.IsFunction()) {
    close.As<Napi::Function>().Call(iterator, {});
  }
  if (env.IsExceptionPending()) env.GetAndClearPendingException();
  Napi::Error(env, error).ThrowAsJavaScriptException();
}

}  // namespace

std::vector<double> NativeValueTraits<IDLCanvasRoundRectRadii>::NativeValue(
    const Napi::CallbackInfo& info, int32_t index) {
  const auto env = info.Env();
  auto value = info[index];
  std::vector<double> result;
  if (value.IsUndefined()) return {0, 0};
  // WebIDL sequence conversion accepts any iterable, including a mixed
  // sequence of numbers/points. Finish conversion before the path validates
  // radius count or finiteness; this preserves observable getter exceptions.
  if (value.IsObject() || value.IsFunction()) {
    auto object = value.ToObject();
    auto symbol = env.Global().Get("Symbol").ToObject().Get("iterator");
    auto method = object.Get(symbol);
    if (env.IsExceptionPending()) return {};
    if (!method.IsNull() && !method.IsUndefined()) {
      if (!method.IsFunction()) {
        Napi::TypeError::New(env, "roundRect radii iterator must be callable")
            .ThrowAsJavaScriptException();
        return {};
      }
      auto iterator_value = method.As<Napi::Function>().Call(object, {});
      if (env.IsExceptionPending()) return {};
      if (!iterator_value.IsObject() && !iterator_value.IsFunction()) {
        Napi::TypeError::New(env, "roundRect iterator must return an object")
            .ThrowAsJavaScriptException();
        return {};
      }
      auto iterator = iterator_value.ToObject();
      auto next = iterator.Get("next");
      if (env.IsExceptionPending()) return {};
      if (!next.IsFunction()) {
        Napi::TypeError::New(env, "roundRect iterator next must be callable")
            .ThrowAsJavaScriptException();
        return {};
      }
      while (true) {
        auto step_value = next.As<Napi::Function>().Call(iterator, {});
        if (env.IsExceptionPending()) return {};
        if (!step_value.IsObject() && !step_value.IsFunction()) {
          Napi::TypeError::New(env,
                               "roundRect iterator result must be an object")
              .ThrowAsJavaScriptException();
          return {};
        }
        auto step = step_value.ToObject();
        auto done = step.Get("done");
        if (env.IsExceptionPending()) return {};
        if (done.ToBoolean().Value()) return result;
        auto radius = step.Get("value");
        if (env.IsExceptionPending()) return {};
        if (!AppendRadius(radius, result)) {
          CloseIteratorAfterException(iterator);
          return {};
        }
      }
    }
  }
  AppendRadius(value, result);
  return result;
}

}  // namespace lynx::binding
