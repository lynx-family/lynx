/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the current directory.
 */
// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/jsi/jsi_unittest.h"

#include <memory>
#include <optional>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include "core/runtime/bindings/jsi/lynx_js_error.h"
#include "core/runtime/jsi/jsi.h"
#ifdef OS_OSX
#include "core/runtime/jsi/v8/v8_runtime.h"
#endif
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx::piper {
namespace test {

namespace {
std::optional<Value> ObjectKeys(Runtime& rt, Value value) {
  auto object_ctor = rt.global().getPropertyAsObject(rt, "Object");
  EXPECT_TRUE(object_ctor.has_value());

  auto object_keys = object_ctor->getPropertyAsFunction(rt, "keys");
  EXPECT_TRUE(object_keys.has_value());

  auto ret = object_keys->call(rt, {std::move(value)});
  EXPECT_TRUE(ret.has_value());
  return ret;
}
}  // namespace

class JSITest : public JSITestBase {
 public:
  JSITest() { scope_ = std::make_unique<Scope>(rt); }
  std::string ToString(Value& value) { return value.toString(rt)->utf8(rt); }
  std::string ToString(std::optional<Value>& value) {
    return ToString(value.value());
  }

 private:
  std::unique_ptr<Scope> scope_;
};

TEST_P(JSITest, RuntimeTest) {
  auto v = rt.evaluateJavaScript(std::make_unique<StringBuffer>("1"), "");
  EXPECT_EQ(v->getNumber(), 1);

  auto ret = rt.evaluateJavaScript(std::make_unique<StringBuffer>("x = 1"), "");
  EXPECT_TRUE(ret.has_value());
  EXPECT_EQ(rt.global().getProperty(rt, "x")->getNumber(), 1);
}

TEST_P(JSITest, JSINativeExceptionConstructTest) {
  auto native_error = BUILD_JSI_NATIVE_EXCEPTION("foo");
  auto error = Object::createFromHostObject(
      rt,
      std::make_shared<LynxError>(native_error.name(), native_error.message(),
                                  native_error.stack()));

  auto name = error.getProperty(rt, "name");
  auto message = error.getProperty(rt, "message");
  auto stack = error.getProperty(rt, "stack");

  ASSERT_TRUE(name.has_value());
  ASSERT_TRUE(message.has_value());
  ASSERT_TRUE(stack.has_value());

  EXPECT_EQ(ToString(name), "NativeError");
  EXPECT_EQ(ToString(message), "foo");
  EXPECT_NE(ToString(stack), "")
      << "Stack of JSINativeException shouldn't be empty";
}

TEST_P(JSITest, JSINativeExceptionIterateTest) {
  auto native_error = BUILD_JSI_NATIVE_EXCEPTION("bar");
  auto error = Object::createFromHostObject(
      rt,
      std::make_shared<LynxError>(native_error.name(), native_error.message(),
                                  native_error.stack()));

  auto keys = ObjectKeys(rt, std::move(error));

  ASSERT_TRUE(keys.has_value());
  EXPECT_EQ(keys->asObject(rt)->asArray(rt)->size(rt).value(), 0);
}

TEST_P(JSITest, JSINativeExceptionSetterTest) {
  auto native_error = BUILD_JSI_NATIVE_EXCEPTION("bar");
  auto error = Object::createFromHostObject(
      rt, std::make_shared<LynxError>(native_error));

  error.setProperty(rt, "name", "CustomError");
  error.setProperty(rt, "message", "CustomMessage");
  error.setProperty(rt, "stack", "CustomStack");

  auto name = error.getProperty(rt, "name");
  auto message = error.getProperty(rt, "message");
  auto stack = error.getProperty(rt, "stack");

  // can set name, message and stack
  EXPECT_EQ(ToString(name), "CustomError");
  EXPECT_EQ(ToString(message), "CustomMessage");
  EXPECT_EQ(ToString(stack), "CustomStack");
}

TEST_P(JSITest, JSErrorConstructTest) {
  auto js_error = JSError(
      rt, function("function() { return new Error('foo'); }").call(rt).value());

  auto error = Object::createFromHostObject(
      rt, std::make_shared<LynxError>(js_error.name(), js_error.message(),
                                      js_error.stack()));
  auto name = error.getProperty(rt, "name");
  auto message = error.getProperty(rt, "message");
  auto stack = error.getProperty(rt, "stack");

  ASSERT_TRUE(name.has_value());
  ASSERT_TRUE(message.has_value());
  ASSERT_TRUE(stack.has_value());

  EXPECT_EQ(ToString(name), "Error");
  EXPECT_EQ(ToString(message), "foo");
  std::unordered_map<JSRuntimeType, std::string> stack_map{
      {JSRuntimeType::quickjs, "    at <anonymous> (<input>:1:38)\n"},
      {JSRuntimeType::jsc, "@"},
      {JSRuntimeType::v8,
       "Error: foo\n    at Object.eval (eval at <anonymous> (unknown source), "
       "<anonymous>:1:22)"}};
  EXPECT_EQ(ToString(stack), stack_map[rt.type()]);
}

TEST_P(JSITest, JSErrorIterateTest) {
  auto js_error = JSError(
      rt, function("function() { return new Error('foo'); }").call(rt).value());

  auto error =
      Object::createFromHostObject(rt, std::make_shared<LynxError>(js_error));

  auto keys = ObjectKeys(rt, std::move(error));

  ASSERT_TRUE(keys.has_value());
  EXPECT_EQ(keys->asObject(rt)->asArray(rt)->size(rt).value(), 0);
}

TEST_P(JSITest, JSErrorSetterTest) {
  auto js_error = JSError(
      rt, function("function() { return new Error('foo'); }").call(rt).value());

  auto error = Object::createFromHostObject(
      rt, std::make_shared<LynxError>(js_error.name(), js_error.message(),
                                      js_error.stack()));

  error.setProperty(rt, "name", "CustomError");
  error.setProperty(rt, "message", "CustomMessage");
  error.setProperty(rt, "stack", "CustomStack");

  auto name = error.getProperty(rt, "name");
  auto message = error.getProperty(rt, "message");
  auto stack = error.getProperty(rt, "stack");

  // can set name, message and stack
  EXPECT_EQ(ToString(name), "CustomError");
  EXPECT_EQ(ToString(message), "CustomMessage");
  EXPECT_EQ(ToString(stack), "CustomStack");
}

TEST_P(JSITest, PropNameIDTest) {
  // This is a little weird to test, because it doesn't really exist
  // in JS yet.  All I can do is create them, compare them, and
  // receive one as an argument to a HostObject.

  PropNameID quux = PropNameID::forAscii(rt, "quux1", 4);
  PropNameID movedQuux = std::move(quux);
  EXPECT_EQ(movedQuux.utf8(rt), "quux");
  movedQuux = PropNameID::forAscii(rt, "quux2");
  EXPECT_EQ(movedQuux.utf8(rt), "quux2");
  PropNameID copiedQuux = PropNameID(rt, movedQuux);
  EXPECT_TRUE(PropNameID::compare(rt, movedQuux, copiedQuux));

  EXPECT_TRUE(PropNameID::compare(rt, movedQuux, movedQuux));
  EXPECT_TRUE(PropNameID::compare(
      rt, movedQuux, PropNameID::forAscii(rt, std::string("quux2"))));
  EXPECT_FALSE(PropNameID::compare(
      rt, movedQuux, PropNameID::forAscii(rt, std::string("foo"))));
  uint8_t utf8[] = {0xF0, 0x9F, 0x86, 0x97};
  PropNameID utf8PropNameID = PropNameID::forUtf8(rt, utf8, sizeof(utf8));
  EXPECT_EQ(utf8PropNameID.utf8(rt), u8"\U0001F197");
  EXPECT_TRUE(PropNameID::compare(rt, utf8PropNameID,
                                  PropNameID::forUtf8(rt, utf8, sizeof(utf8))));
  PropNameID nonUtf8PropNameID = PropNameID::forUtf8(rt, "meow");
  EXPECT_TRUE(PropNameID::compare(rt, nonUtf8PropNameID,
                                  PropNameID::forAscii(rt, "meow")));
  EXPECT_EQ(nonUtf8PropNameID.utf8(rt), "meow");
  PropNameID strPropNameID =
      PropNameID::forString(rt, String::createFromAscii(rt, "meow"));
  EXPECT_TRUE(PropNameID::compare(rt, nonUtf8PropNameID, strPropNameID));

  auto names = PropNameID::names(rt, "Ala", std::string("ma"),
                                 PropNameID::forAscii(rt, "kota"));
  EXPECT_EQ(names.size(), 3);
  EXPECT_TRUE(
      PropNameID::compare(rt, names[0], PropNameID::forAscii(rt, "Ala")));
  EXPECT_TRUE(
      PropNameID::compare(rt, names[1], PropNameID::forAscii(rt, "ma")));
  EXPECT_TRUE(
      PropNameID::compare(rt, names[2], PropNameID::forAscii(rt, "kota")));
}

TEST_P(JSITest, StringTest) {
  EXPECT_TRUE(checkValue(String::createFromAscii(rt, "foobar", 3), "'foo'"));
  EXPECT_TRUE(checkValue(String::createFromAscii(rt, "foobar"), "'foobar'"));

  std::string baz = "baz";
  EXPECT_TRUE(checkValue(String::createFromAscii(rt, baz), "'baz'"));

  uint8_t utf8[] = {0xF0, 0x9F, 0x86, 0x97};
  EXPECT_TRUE(checkValue(String::createFromUtf8(rt, utf8, sizeof(utf8)),
                         "'\\uD83C\\uDD97'"));

  EXPECT_EQ(eval("'quux'")->getString(rt).utf8(rt), "quux");
  EXPECT_EQ(eval("'\\u20AC'")->getString(rt).utf8(rt), "\xe2\x82\xac");

  String quux = String::createFromAscii(rt, "quux");
  String movedQuux = std::move(quux);
  EXPECT_EQ(movedQuux.utf8(rt), "quux");
  movedQuux = String::createFromAscii(rt, "quux2");
  EXPECT_EQ(movedQuux.utf8(rt), "quux2");
}

TEST_P(JSITest, ObjectTest) {
  eval("x = {1:2, '3':4, 5:'six', 'seven':['eight', 'nine']}");
  Object x = rt.global().getPropertyAsObject(rt, "x").value();
  EXPECT_EQ(x.getPropertyNames(rt)->size(rt), 4);
  EXPECT_TRUE(x.hasProperty(rt, "1"));
  EXPECT_TRUE(x.hasProperty(rt, PropNameID::forAscii(rt, "1")));
  EXPECT_FALSE(x.hasProperty(rt, "2"));
  EXPECT_FALSE(x.hasProperty(rt, PropNameID::forAscii(rt, "2")));
  EXPECT_TRUE(x.hasProperty(rt, "3"));
  EXPECT_TRUE(x.hasProperty(rt, PropNameID::forAscii(rt, "3")));
  EXPECT_TRUE(x.hasProperty(rt, "seven"));
  EXPECT_TRUE(x.hasProperty(rt, PropNameID::forAscii(rt, "seven")));
  EXPECT_EQ(x.getProperty(rt, "1")->getNumber(), 2);
  EXPECT_EQ(x.getProperty(rt, PropNameID::forAscii(rt, "1"))->getNumber(), 2);
  EXPECT_EQ(x.getProperty(rt, "3")->getNumber(), 4);
  Value five = 5;
  EXPECT_EQ(
      x.getProperty(rt, PropNameID::forString(rt, five.toString(rt).value()))
          ->getString(rt)
          .utf8(rt),
      "six");

  x.setProperty(rt, "ten", 11);
  EXPECT_EQ(x.getPropertyNames(rt)->size(rt), 5);
  EXPECT_TRUE(eval("x.ten == 11")->getBool());

  x.setProperty(rt, "e_as_float", 2.71f);
  EXPECT_TRUE(eval("Math.abs(x.e_as_float - 2.71) < 0.001")->getBool());

  x.setProperty(rt, "e_as_double", 2.71);
  EXPECT_TRUE(eval("x.e_as_double == 2.71")->getBool());

  uint8_t utf8[] = {0xF0, 0x9F, 0x86, 0x97};
  String nonAsciiName = String::createFromUtf8(rt, utf8, sizeof(utf8));
  x.setProperty(rt, PropNameID::forString(rt, nonAsciiName), "emoji");
  EXPECT_EQ(x.getPropertyNames(rt)->size(rt), 8);
  EXPECT_TRUE(eval("x['\\uD83C\\uDD97'] == 'emoji'")->getBool());

  Object seven = x.getPropertyAsObject(rt, "seven").value();
  EXPECT_TRUE(seven.isArray(rt));
  Object evalf = rt.global().getPropertyAsObject(rt, "eval").value();
  EXPECT_TRUE(evalf.isFunction(rt));

  Object movedX = Object(rt);
  movedX = std::move(x);
  EXPECT_EQ(movedX.getPropertyNames(rt)->size(rt), 8);
  EXPECT_EQ(movedX.getProperty(rt, "1")->getNumber(), 2);

  Object obj = Object(rt);
  obj.setProperty(rt, "roses", "red");
  obj.setProperty(rt, "violets", "blue");
  Object oprop = Object(rt);
  obj.setProperty(rt, "oprop", oprop);
  obj.setProperty(rt, "aprop", Array::createWithLength(rt, 1).value());

  EXPECT_TRUE(function("function (obj) { return "
                       "obj.roses == 'red' && "
                       "obj['violets'] == 'blue' && "
                       "typeof obj.oprop == 'object' && "
                       "Array.isArray(obj.aprop); }")
                  .call(rt, obj)
                  ->getBool());

  // Check that getPropertyNames doesn't return non-enumerable
  // properties.
  obj = function(
            "function () {"
            "  obj = {};"
            "  obj.a = 1;"
            "  Object.defineProperty(obj, 'b', {"
            "    enumerable: false,"
            "    value: 2"
            "  });"
            "  return obj;"
            "}")
            .call(rt)
            ->getObject(rt);
  EXPECT_EQ(obj.getProperty(rt, "a")->getNumber(), 1);
  EXPECT_EQ(obj.getProperty(rt, "b")->getNumber(), 2);
  Array names = obj.getPropertyNames(rt).value();
  EXPECT_EQ(names.size(rt), 1);
  EXPECT_EQ(names.getValueAtIndex(rt, 0)->getString(rt).utf8(rt), "a");
}

TEST_P(JSITest, ArrayBufferTest) {
  // Test ArrayBuffer of zero-length with null src buffer
  auto array_buffer = ArrayBuffer(rt, nullptr, 0);
  rt.global().setProperty(rt, "arrayBuffer", array_buffer);
  EXPECT_EQ(eval("arrayBuffer instanceof ArrayBuffer")->getBool(), true);

  array_buffer = ArrayBuffer(rt, nullptr, 0);
  rt.global().setProperty(rt, "arrayBuffer", array_buffer);
  EXPECT_EQ(eval("arrayBuffer instanceof ArrayBuffer")->getBool(), true);

  // Test ArrayBuffer of zero-length with non-null src buffer
  uint8_t bytes[] = {
      1,
      2,
  };
  array_buffer = ArrayBuffer(rt, bytes, 0);
  rt.global().setProperty(rt, "arrayBuffer", array_buffer);
  EXPECT_EQ(eval("arrayBuffer instanceof ArrayBuffer")->getBool(), true);
  EXPECT_EQ(eval("arrayBuffer.byteLength")->getNumber(), 0);

  auto bytes_move = std::make_unique<uint8_t[]>(2);
  array_buffer = ArrayBuffer(rt, std::move(bytes_move), 0);
  rt.global().setProperty(rt, "arrayBuffer", array_buffer);
  EXPECT_EQ(eval("arrayBuffer instanceof ArrayBuffer")->getBool(), true);
  EXPECT_EQ(eval("arrayBuffer.byteLength")->getNumber(), 0);

  // Test ArrayBuffer of positive-length with non-null src buffer
  array_buffer = ArrayBuffer(rt, bytes, 2);
  rt.global().setProperty(rt, "arrayBuffer", array_buffer);
  EXPECT_EQ(eval("var view = new Uint8Array(arrayBuffer); view[0].toString() + "
                 "view[1].toString()")
                ->getString(rt)
                .utf8(rt),
            "12");

  bytes_move = std::make_unique<uint8_t[]>(2);
  bytes_move[0] = 1;
  bytes_move[1] = 2;
  array_buffer = ArrayBuffer(rt, std::move(bytes_move), 2);
  rt.global().setProperty(rt, "arrayBuffer", array_buffer);
  EXPECT_EQ(eval("var view = new Uint8Array(arrayBuffer); view[0].toString() + "
                 "view[1].toString()")
                ->getString(rt)
                .utf8(rt),
            "12");

  // Test ArrayBuffer of positive-length with null src buffer
  array_buffer = ArrayBuffer(rt, nullptr, 1);
  rt.global().setProperty(rt, "arrayBuffer", array_buffer);
  EXPECT_EQ(eval("arrayBuffer instanceof ArrayBuffer")->getBool(), false);

  array_buffer = ArrayBuffer(rt, nullptr, 1);
  rt.global().setProperty(rt, "arrayBuffer", array_buffer);
  EXPECT_EQ(eval("arrayBuffer instanceof ArrayBuffer")->getBool(), false);
}

TEST_P(JSITest, HostObjectTest) {
  if (rt.type() == JSRuntimeType::v8) {
    GTEST_SKIP() << "no-exception";
  }

  class ConstantHostObject : public HostObject {
    Value get(Runtime*, const PropNameID& sym) override { return 9000; }

    void set(Runtime*, const PropNameID&, const Value&) override {}
  };

  Object cho =
      Object::createFromHostObject(rt, std::make_shared<ConstantHostObject>());
  EXPECT_TRUE(function("function (obj) { return obj.someRandomProp == 9000; }")
                  .call(rt, cho)
                  ->getBool());
  EXPECT_TRUE(cho.isHostObject(rt));
  // EXPECT_TRUE(cho.getHostObject<ConstantHostObject>(rt).get() != nullptr);

  struct SameRuntimeHostObject : HostObject {
    explicit SameRuntimeHostObject(Runtime& rt) : rt_(rt){};

    Value get(Runtime* rt, const PropNameID& sym) override {
      EXPECT_EQ(rt, &rt_);
      return Value();
    }

    void set(Runtime* rt, const PropNameID& name, const Value& value) override {
      EXPECT_EQ(rt, &rt_);
    }

    std::vector<PropNameID> getPropertyNames(Runtime& rt) override {
      EXPECT_EQ(&rt, &rt_);
      return {};
    }

    Runtime& rt_;
  };

  Object srho = Object::createFromHostObject(
      rt, std::make_shared<SameRuntimeHostObject>(rt));
  // Test get's Runtime is as expected
  function("function (obj) { return obj.isSame; }").call(rt, srho);
  // ... and set
  function("function (obj) { obj['k'] = 'v'; }").call(rt, srho);
  // ... and getPropertyNames
  function("function (obj) { for (k in obj) {} }").call(rt, srho);

  class TwiceHostObject : public HostObject {
    Value get(Runtime* rt, const PropNameID& sym) override {
      return String::createFromUtf8(*rt, sym.utf8(*rt) + sym.utf8(*rt));
    }

    void set(Runtime*, const PropNameID&, const Value&) override {}
  };

  Object tho =
      Object::createFromHostObject(rt, std::make_shared<TwiceHostObject>());
  EXPECT_TRUE(function("function (obj) { return obj.abc == 'abcabc'; }")
                  .call(rt, tho)
                  ->getBool());
  EXPECT_TRUE(function("function (obj) { return obj['def'] == 'defdef'; }")
                  .call(rt, tho)
                  ->getBool());
  EXPECT_TRUE(function("function (obj) { return obj[12] === '1212'; }")
                  .call(rt, tho)
                  ->getBool());
  EXPECT_TRUE(tho.isHostObject(rt));
  // EXPECT_TRUE(std::dynamic_pointer_cast<ConstantHostObject>(
  //                 tho.getHostObject(rt)) == nullptr);
  // EXPECT_TRUE(tho.getHostObject<TwiceHostObject>(rt).get() != nullptr);

  class PropNameIDHostObject : public HostObject {
    Value get(Runtime* rt, const PropNameID& sym) override {
      if (PropNameID::compare(*rt, sym, PropNameID::forAscii(*rt, "undef"))) {
        return Value::undefined();
      } else {
        return PropNameID::compare(*rt, sym,
                                   PropNameID::forAscii(*rt, "somesymbol"));
      }
    }

    void set(Runtime*, const PropNameID&, const Value&) override {}
  };

  Object sho = Object::createFromHostObject(
      rt, std::make_shared<PropNameIDHostObject>());
  EXPECT_TRUE(sho.isHostObject(rt));
  EXPECT_TRUE(function("function (obj) { return obj.undef; }")
                  .call(rt, sho)
                  ->isUndefined());
  EXPECT_TRUE(function("function (obj) { return obj.somesymbol; }")
                  .call(rt, sho)
                  ->getBool());
  EXPECT_FALSE(function("function (obj) { return obj.notsomuch; }")
                   .call(rt, sho)
                   ->getBool());

  class BagHostObject : public HostObject {
   public:
    const std::string& getThing() { return bag_["thing"]; }

   private:
    Value get(Runtime* rt, const PropNameID& sym) override {
      if (sym.utf8(*rt) == "thing") {
        return String::createFromUtf8(*rt, bag_[sym.utf8(*rt)]);
      }
      return Value::undefined();
    }

    void set(Runtime* rt, const PropNameID& sym, const Value& val) override {
      std::string key(sym.utf8(*rt));
      if (key == "thing") {
        bag_[key] = val.toString(*rt)->utf8(*rt);
      }
    }

    std::unordered_map<std::string, std::string> bag_;
  };

  std::shared_ptr<BagHostObject> shbho = std::make_shared<BagHostObject>();
  Object bho = Object::createFromHostObject(rt, shbho);
  EXPECT_TRUE(bho.isHostObject(rt));
  EXPECT_TRUE(function("function (obj) { return obj.undef; }")
                  .call(rt, bho)
                  ->isUndefined());
  EXPECT_EQ(
      function("function (obj) { obj.thing = 'hello'; return obj.thing; }")
          .call(rt, bho)
          ->toString(rt)
          ->utf8(rt),
      "hello");
  EXPECT_EQ(shbho->getThing(), "hello");

  // TODO(wangqingyu): no-exception
  // class ThrowingHostObject : public HostObject {
  //   Value get(Runtime* rt, const PropNameID& sym) override {
  //     throw std::runtime_error("Cannot get");
  //   }

  //   void set(Runtime* rt, const PropNameID& sym, const Value& val) override {
  //     throw std::runtime_error("Cannot set");
  //   }
  // };

  // Object thro =
  //     Object::createFromHostObject(rt,
  //     std::make_shared<ThrowingHostObject>());
  // EXPECT_TRUE(thro.isHostObject(rt));
  // std::string exc;
  // try {
  //   function("function (obj) { return obj.thing; }").call(rt, thro);
  // } catch (const JSError& ex) {
  //   exc = ex.what();
  // }
  // EXPECT_NE(exc.find("Cannot get"), std::string::npos);
  // exc = "";
  // try {
  //   function("function (obj) { obj.thing = 'hello'; }").call(rt, thro);
  // } catch (const JSError& ex) {
  //   exc = ex.what();
  // }
  // EXPECT_NE(exc.find("Cannot set"), std::string::npos);

  // TODO(wangqingyu): isHostObject
  // class NopHostObject : public HostObject {};
  // Object nopHo =
  //     Object::createFromHostObject(rt, std::make_shared<NopHostObject>());
  // EXPECT_TRUE(nopHo.isHostObject(rt));
  // EXPECT_TRUE(function("function (obj) { return obj.thing; }")
  //                 .call(rt, nopHo)
  //                 ->isUndefined());

  // std::string nopExc;
  // try {
  //   function("function (obj) { obj.thing = 'pika'; }").call(rt, nopHo);
  // } catch (const JSError& ex) {
  //   nopExc = ex.what();
  // }
  // EXPECT_NE(nopExc.find("TypeError: "), std::string::npos);

  // FIXME(wangqingyu): Support return duplicated keys in getPropertyNames
  class HostObjectWithPropertyNames : public HostObject {
    std::vector<PropNameID> getPropertyNames(Runtime& rt) override {
      return PropNameID::names(rt, "1", "false", "a_prop", "3", "c_prop");
    }
  };

  Object howpn = Object::createFromHostObject(
      rt, std::make_shared<HostObjectWithPropertyNames>());
  EXPECT_TRUE(
      function(
          "function (o) { return Object.getOwnPropertyNames(o).length == 5 }")
          .call(rt, howpn)
          ->getBool());

  auto hasOwnPropertyName = function(
      "function (o, p) {"
      "  return Object.getOwnPropertyNames(o).indexOf(p) >= 0"
      "}");
  EXPECT_TRUE(
      hasOwnPropertyName.call(rt, howpn, String::createFromAscii(rt, "a_prop"))
          ->getBool());
  EXPECT_TRUE(
      hasOwnPropertyName.call(rt, howpn, String::createFromAscii(rt, "1"))
          ->getBool());
  EXPECT_TRUE(
      hasOwnPropertyName.call(rt, howpn, String::createFromAscii(rt, "false"))
          ->getBool());
  EXPECT_TRUE(
      hasOwnPropertyName.call(rt, howpn, String::createFromAscii(rt, "3"))
          ->getBool());
  EXPECT_TRUE(
      hasOwnPropertyName.call(rt, howpn, String::createFromAscii(rt, "c_prop"))
          ->getBool());
  EXPECT_FALSE(hasOwnPropertyName
                   .call(rt, howpn, String::createFromAscii(rt, "not_existing"))
                   ->getBool());

  class HostObjectWithRecursiveJSValue : public HostObject {
   public:
    explicit HostObjectWithRecursiveJSValue(Function child)
        : child_(std::move(child)) {}

   private:
    Value get(Runtime* rt, const PropNameID& sym) override {
      if (sym.utf8(*rt) == "child") {
        return Value(child_);
      }
      return Value::undefined();
    }

    std::vector<PropNameID> getPropertyNames(Runtime& rt) override {
      return PropNameID::names(rt, "child");
    }

    Function child_;
  };

  Object host_object_with_recursive_js_value = Object::createFromHostObject(
      rt, std::make_shared<HostObjectWithRecursiveJSValue>(
              Function::createFromHostFunction(
                  rt, PropNameID::forAscii(rt, "dot"), 2,
                  [](Runtime& rt, const Value& thisVal, const Value* args,
                     size_t count)
                      -> base::expected<piper::Value, JSINativeException> {
                    EXPECT_TRUE(Value::strictEquals(rt, thisVal, rt.global()) ||
                                thisVal.isUndefined());
                    if (count != 2) {
                      return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                          "Exception in HostFunction: expected 2 args"));
                    }
                    std::string ret = args[0].getString(rt).utf8(rt) + "." +
                                      args[1].getString(rt).utf8(rt);
                    return String::createFromUtf8(
                        rt, reinterpret_cast<const uint8_t*>(ret.data()),
                        ret.size());
                  })));

  EXPECT_TRUE(
      host_object_with_recursive_js_value.getPropertyAsFunction(rt, "child")
          ->call(rt, "foo", "bar")
          ->getString(rt)
          .utf8(rt) == "foo.bar");
}

TEST_P(JSITest, HostObjectProtoTest) {
  class ProtoHostObject : public HostObject {
    Value get(Runtime* rt, const PropNameID&) override {
      return String::createFromAscii(*rt, "phoprop");
    }
  };

  rt.global().setProperty(
      rt, "pho",
      Object::createFromHostObject(rt, std::make_shared<ProtoHostObject>()));

  EXPECT_EQ(
      eval("({__proto__: pho})[Symbol.toPrimitive]")->getString(rt).utf8(rt),
      "phoprop");
}

TEST_P(JSITest, HostObjectCreateDestroyTest) {
  class FooHostObject : public HostObject {
    Value get(Runtime* rt, const PropNameID&) override {
      return String::createFromAscii(*rt, "foo");
    }
  };

  auto host_obj = std::make_shared<FooHostObject>();
  auto* host_ptr = host_obj.get();
  auto obj = Object::createFromHostObject(rt, std::move(host_obj));
  rt.global().setProperty(rt, "destroyFoo", obj);
  auto weak_host_obj = obj.getHostObject(rt);
  EXPECT_FALSE(weak_host_obj.expired());
  auto lock_host_obj = weak_host_obj.lock();
  EXPECT_EQ(host_ptr, lock_host_obj.get());
}

TEST_P(JSITest, ArrayTest) {
  eval("x = {1:2, '3':4, 5:'six', 'seven':['eight', 'nine']}");

  Object x = rt.global().getPropertyAsObject(rt, "x").value();
  Array names = x.getPropertyNames(rt).value();
  EXPECT_EQ(names.size(rt), 4);
  std::unordered_set<std::string> strNames;
  for (size_t i = 0; i < names.size(rt); ++i) {
    Value n = names.getValueAtIndex(rt, i).value();
    EXPECT_TRUE(n.isString());
    strNames.insert(n.getString(rt).utf8(rt));
  }

  EXPECT_EQ(strNames.size(), 4);
  EXPECT_EQ(strNames.count("1"), 1);
  EXPECT_EQ(strNames.count("3"), 1);
  EXPECT_EQ(strNames.count("5"), 1);
  EXPECT_EQ(strNames.count("seven"), 1);

  Object seven = x.getPropertyAsObject(rt, "seven").value();
  Array arr = seven.getArray(rt);

  EXPECT_EQ(arr.size(rt), 2);
  EXPECT_EQ(arr.getValueAtIndex(rt, 0)->getString(rt).utf8(rt), "eight");
  EXPECT_EQ(arr.getValueAtIndex(rt, 1)->getString(rt).utf8(rt), "nine");
  // TODO: test out of range

  EXPECT_EQ(x.getPropertyAsObject(rt, "seven")->getArray(rt).size(rt), 2);

  // Check that property access with both symbols and strings can access
  // array values.
  EXPECT_EQ(seven.getProperty(rt, "0")->getString(rt).utf8(rt), "eight");
  EXPECT_EQ(seven.getProperty(rt, "1")->getString(rt).utf8(rt), "nine");
  seven.setProperty(rt, "1", "modified");
  EXPECT_EQ(seven.getProperty(rt, "1")->getString(rt).utf8(rt), "modified");
  EXPECT_EQ(arr.getValueAtIndex(rt, 1)->getString(rt).utf8(rt), "modified");
  EXPECT_EQ(seven.getProperty(rt, PropNameID::forAscii(rt, "0"))
                ->getString(rt)
                .utf8(rt),
            "eight");
  seven.setProperty(rt, PropNameID::forAscii(rt, "0"), "modified2");
  EXPECT_EQ(arr.getValueAtIndex(rt, 0)->getString(rt).utf8(rt), "modified2");

  Array alpha = Array::createWithLength(rt, 4).value();
  // length should not be enumerable
  // see: https://tc39.es/ecma262/#sec-arraycreate
  EXPECT_EQ(alpha.getPropertyNames(rt).value().length(rt), 0);

  EXPECT_TRUE(alpha.getValueAtIndex(rt, 0)->isUndefined());
  EXPECT_TRUE(alpha.getValueAtIndex(rt, 3)->isUndefined());
  EXPECT_EQ(alpha.size(rt), 4);
  alpha.setValueAtIndex(rt, 0, "a");
  alpha.setValueAtIndex(rt, 1, "b");
  EXPECT_EQ(alpha.length(rt), 4);
  alpha.setValueAtIndex(rt, 2, "c");
  alpha.setValueAtIndex(rt, 3, "d");
  EXPECT_EQ(alpha.size(rt), 4);

  EXPECT_TRUE(
      function(
          "function (arr) { return "
          "arr.length == 4 && "
          "['a','b','c','d'].every(function(v,i) { return v === arr[i]}); }")
          .call(rt, alpha)
          ->getBool());

  Array alpha2 = Array::createWithLength(rt, 1).value();
  alpha2 = std::move(alpha);
  EXPECT_EQ(alpha2.size(rt), 4);

  auto proxy_arr_value = function(R"(function () {
  var targetArray = [0, 1, 2, 3, 4];
	var handler = {
		get(target, property) {
			return target[property];
		},
		set(target, property, value) {
			target[property] = value;
			return true;
		},
		deleteProperty(target, property) {
			return delete target[property];
		}
	};
	return new Proxy(targetArray, handler);
}
)")
                             .call(rt);
  EXPECT_TRUE(proxy_arr_value->isObject());
  auto proxy_arr_obj = proxy_arr_value->getObject(rt);
  EXPECT_TRUE(proxy_arr_obj.isArray(rt));
  auto proxy_arr = proxy_arr_obj.getArray(rt);
  EXPECT_EQ(proxy_arr.size(rt).value(), 5);
  for (int i = 0; i < 5; i++) {
    EXPECT_EQ(
        static_cast<int>(
            proxy_arr.getValueAtIndex(rt, static_cast<size_t>(i))->getNumber()),
        i);
  }
}

TEST_P(JSITest, FunctionTest) {
  // test move ctor
  Function fmove = function("function() { return 1 }");
  {
    Function g = function("function() { return 2 }");
    fmove = std::move(g);
  }
  EXPECT_EQ(fmove.call(rt)->getNumber(), 2);

  // This tests all the function argument converters, and all the
  // non-lvalue overloads of call().
  Function f = function(
      "function(n, b, d, df, i, s1, s2, s3, s_sun, s_bad, o, a, f, v) { "
      "return "
      "n === null && "
      "b === true && "
      "d === 3.14 && "
      "Math.abs(df - 2.71) < 0.001 && "
      "i === 17 && "
      "s1 == 's1' && "
      "s2 == 's2' && "
      "s3 == 's3' && "
      "s_sun == 's\\u2600' && "
      "typeof s_bad == 'string' && "
      "typeof o == 'object' && "
      "Array.isArray(a) && "
      "typeof f == 'function' && "
      "v == 42 }");
  EXPECT_TRUE(
      f.call(rt, nullptr, true, 3.14, 2.71f, 17, "s1",
             String::createFromAscii(rt, "s2"), std::string{"s3"},
             std::string{u8"s\u2600"},
             // invalid UTF8 sequence due to unexpected continuation byte
             std::string{"s\x80"}, Object(rt),
             Array::createWithLength(rt, 1).value(), function("function(){}"),
             Value(42))
          ->getBool());

  // lvalue overloads of call()
  Function flv = function(
      "function(s, o, a, f, v) { return "
      "s == 's' && "
      "typeof o == 'object' && "
      "Array.isArray(a) && "
      "typeof f == 'function' && "
      "v == 42 }");

  String s = String::createFromAscii(rt, "s");
  Object o = Object(rt);
  Array a = Array::createWithLength(rt, 1).value();
  Value v = 42;
  EXPECT_TRUE(flv.call(rt, s, o, a, f, v)->getBool());

  Function f1 = function("function() { return 1; }");
  Function f2 = function("function() { return 2; }");
  f2 = std::move(f1);
  EXPECT_EQ(f2.call(rt)->getNumber(), 1);
}

TEST_P(JSITest, FunctionThisTest) {
  Function checkPropertyFunction =
      function("function() { return this.a === 'a_property' }");

  Object jsObject = Object(rt);
  jsObject.setProperty(rt, "a", String::createFromUtf8(rt, "a_property"));

  class APropertyHostObject : public HostObject {
    Value get(Runtime* rt, const PropNameID& sym) override {
      return String::createFromUtf8(*rt, "a_property");
    }

    void set(Runtime*, const PropNameID&, const Value&) override {}
  };
  Object hostObject =
      Object::createFromHostObject(rt, std::make_shared<APropertyHostObject>());

  EXPECT_TRUE(checkPropertyFunction.callWithThis(rt, jsObject, {})->getBool());
  EXPECT_TRUE(
      checkPropertyFunction.callWithThis(rt, hostObject, {})->getBool());
  EXPECT_FALSE(checkPropertyFunction
                   .callWithThis(rt, Array::createWithLength(rt, 5).value(), {})
                   ->getBool());
  EXPECT_FALSE(checkPropertyFunction.call(rt)->getBool());
}

TEST_P(JSITest, FunctionConstructorTest) {
  Function ctor = function(
      "function (a) {"
      "  if (typeof a !== 'undefined') {"
      "   this.pika = a;"
      "  }"
      "}");
  ctor.getProperty(rt, "prototype")
      ->getObject(rt)
      .setProperty(rt, "pika", "chu");
  auto empty = ctor.callAsConstructor(rt);
  EXPECT_TRUE(empty->isObject());
  auto emptyObj = std::move(empty)->getObject(rt);
  EXPECT_EQ(emptyObj.getProperty(rt, "pika")->getString(rt).utf8(rt), "chu");
  auto who = ctor.callAsConstructor(rt, "who");
  EXPECT_TRUE(who->isObject());
  auto whoObj = std::move(who)->getObject(rt);
  EXPECT_EQ(whoObj.getProperty(rt, "pika")->getString(rt).utf8(rt), "who");

  auto instanceof = function("function (o, b) { return o instanceof b; }");
  EXPECT_TRUE(instanceof.call(rt, emptyObj, ctor)->getBool());
  EXPECT_TRUE(instanceof.call(rt, whoObj, ctor)->getBool());

  auto dateCtor = rt.global().getPropertyAsFunction(rt, "Date");
  auto date = dateCtor->callAsConstructor(rt);
  EXPECT_TRUE(date->isObject());
  EXPECT_TRUE(instanceof.call(rt, date.value(), dateCtor.value())->getBool());
  // Sleep for 50 milliseconds
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_GE(
      function("function (d) { return (new Date()).getTime() - d.getTime(); }")
          .call(rt, date.value())
          ->getNumber(),
      50);
}

TEST_P(JSITest, InstanceOfTest) {
  if (rt.type() == JSRuntimeType::v8) {
    GTEST_SKIP() << "no-exception";
  }

  auto ctor = function("function Rick() { this.say = 'wubalubadubdub'; }");
  auto newObj = function("function (ctor) { return new ctor(); }");
  auto instance = newObj.call(rt, ctor)->getObject(rt);
  EXPECT_TRUE(instance.instanceOf(rt, ctor));
  EXPECT_EQ(instance.getProperty(rt, "say")->getString(rt).utf8(rt),
            "wubalubadubdub");
  EXPECT_FALSE(Object(rt).instanceOf(rt, ctor));
  EXPECT_TRUE(ctor.callAsConstructor(rt, nullptr, 0)
                  ->getObject(rt)
                  .instanceOf(rt, ctor));
}

TEST_P(JSITest, HostFunctionTest) {
  if (rt.type() == JSRuntimeType::v8) {
    GTEST_SKIP() << "no-exception";
  }

  auto one = std::make_shared<int>(1);
  Function plusOne = Function::createFromHostFunction(
      rt, PropNameID::forAscii(rt, "plusOne"), 2,
      [one, savedRt = &rt](Runtime& rt, const Value& thisVal, const Value* args,
                           size_t count) {
        EXPECT_EQ(savedRt, &rt);
        // We don't know if we're in strict mode or not, so it's either global
        // or undefined.
        EXPECT_TRUE(Value::strictEquals(rt, thisVal, rt.global()) ||
                    thisVal.isUndefined());
        return *one + args[0].getNumber() + args[1].getNumber();
      });

  EXPECT_EQ(plusOne.call(rt, 1, 2)->getNumber(), 4);
  EXPECT_TRUE(checkValue(plusOne.call(rt, 3, 5).value(), "9"));
  rt.global().setProperty(rt, "plusOne", plusOne);
  EXPECT_TRUE(eval("plusOne(20, 300) == 321")->getBool());

  Function dot = Function::createFromHostFunction(
      rt, PropNameID::forAscii(rt, "dot"), 2,
      [](Runtime& rt, const Value& thisVal, const Value* args,
         size_t count) -> base::expected<piper::Value, JSINativeException> {
        EXPECT_TRUE(Value::strictEquals(rt, thisVal, rt.global()) ||
                    thisVal.isUndefined());
        if (count != 2) {
          return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
              "Exception in HostFunction: expected 2 args"));
        }
        std::string ret = args[0].getString(rt).utf8(rt) + "." +
                          args[1].getString(rt).utf8(rt);
        return String::createFromUtf8(
            rt, reinterpret_cast<const uint8_t*>(ret.data()), ret.size());
      });

  rt.global().setProperty(rt, "cons", dot);
  EXPECT_TRUE(eval("cons('left', 'right') == 'left.right'")->getBool());
  EXPECT_TRUE(eval("cons.name == 'dot'")->getBool());
  EXPECT_TRUE(eval("cons.length == 2")->getBool());
  EXPECT_TRUE(eval("cons instanceof Function")->getBool());
  EXPECT_TRUE(eval("cons('fail') == undefined")->getBool());

  auto error_cons =
      "(function() {"
      "  try {"
      "    cons('fail'); return false;"
      "  } catch (e) {"
      "    return e instanceof Error &&"
      "    e.message =="
      "      'Exception in HostFunction: expected 2 args'"
      "  }"
      "})()";
  rt.SetEnableJsBindingApiThrowException(true);
  EXPECT_TRUE(eval(error_cons)->getBool());
  rt.SetEnableJsBindingApiThrowException(false);
  EXPECT_FALSE(eval(error_cons)->getBool());

  Function coolify = Function::createFromHostFunction(
      rt, PropNameID::forAscii(rt, "coolify"), 0,
      [](Runtime& rt, const Value& thisVal, const Value* args, size_t count) {
        EXPECT_EQ(count, 0);
        std::string ret = thisVal.toString(rt)->utf8(rt) + " is cool";
        return String::createFromUtf8(
            rt, reinterpret_cast<const uint8_t*>(ret.data()), ret.size());
      });
  rt.global().setProperty(rt, "coolify", coolify);
  EXPECT_TRUE(eval("coolify.name == 'coolify'")->getBool());
  EXPECT_TRUE(eval("coolify.length == 0")->getBool());
  EXPECT_TRUE(eval("coolify.bind('R&M')() == 'R&M is cool'")->getBool());
  EXPECT_TRUE(eval("(function() {"
                   "  var s = coolify.bind(function(){})();"
                   "  return s.lastIndexOf(' is cool') == (s.length - 8);"
                   "})()")
                  ->getBool());

  Function lookAtMe = Function::createFromHostFunction(
      rt, PropNameID::forAscii(rt, "lookAtMe"), 0,
      [](Runtime& rt, const Value& thisVal, const Value* args,
         size_t count) -> Value {
        EXPECT_TRUE(thisVal.isObject());
        EXPECT_EQ(thisVal.getObject(rt)
                      .getProperty(rt, "name")
                      ->getString(rt)
                      .utf8(rt),
                  "mr.meeseeks");
        return Value();
      });
  rt.global().setProperty(rt, "lookAtMe", lookAtMe);
  EXPECT_TRUE(eval("lookAtMe.bind({'name': 'mr.meeseeks'})()")->isUndefined());

  struct Callable {
    explicit Callable(std::string s) : str(s) {}

    Value operator()(Runtime& rt, const Value&, const Value* args,
                     size_t count) {
      if (count != 1) {
        return Value();
      }
      return String::createFromUtf8(
          rt, args[0].toString(rt)->utf8(rt) + " was called with " + str);
    }

    std::string str;
  };

  Function callable =
      Function::createFromHostFunction(rt, PropNameID::forAscii(rt, "callable"),
                                       1, Callable("std::function::target"));
  EXPECT_EQ(function("function (f) { return f('A cat'); }")
                .call(rt, callable)
                ->getString(rt)
                .utf8(rt),
            "A cat was called with std::function::target");
  EXPECT_TRUE(callable.isHostFunction(rt));
  // TODO(wangqingyu): HostFunction supports `target`
  // EXPECT_NE(callable.getHostFunction(rt).target<Callable>(), nullptr);

  std::string strval = "strval1";
  auto getter = Object(rt);
  getter.setProperty(rt, "get",
                     Function::createFromHostFunction(
                         rt, PropNameID::forAscii(rt, "getter"), 1,
                         [&strval](Runtime& rt, const Value& thisVal,
                                   const Value* args, size_t count) -> Value {
                           return String::createFromUtf8(rt, strval);
                         }));
  auto obj = Object(rt);
  rt.global()
      .getPropertyAsObject(rt, "Object")
      ->getPropertyAsFunction(rt, "defineProperty")
      ->call(rt, obj, "prop", getter);
  EXPECT_TRUE(function("function(value) { return value.prop == 'strval1'; }")
                  .call(rt, obj)
                  ->getBool());
  strval = "strval2";
  EXPECT_TRUE(function("function(value) { return value.prop == 'strval2'; }")
                  .call(rt, obj)
                  ->getBool());
}

TEST_P(JSITest, ValueTest) {
  EXPECT_TRUE(checkValue(Value::undefined(), "undefined"));
  EXPECT_TRUE(checkValue(Value(), "undefined"));
  EXPECT_TRUE(checkValue(Value::null(), "null"));
  EXPECT_TRUE(checkValue(nullptr, "null"));

  EXPECT_TRUE(checkValue(Value(false), "false"));
  EXPECT_TRUE(checkValue(false, "false"));
  EXPECT_TRUE(checkValue(true, "true"));

  EXPECT_TRUE(checkValue(Value(1.5), "1.5"));
  EXPECT_TRUE(checkValue(2.5, "2.5"));

  EXPECT_TRUE(checkValue(Value(10), "10"));
  EXPECT_TRUE(checkValue(20, "20"));
  EXPECT_TRUE(checkValue(30, "30"));

  // rvalue implicit conversion
  EXPECT_TRUE(checkValue(String::createFromAscii(rt, "one"), "'one'"));
  // lvalue explicit copy
  String s = String::createFromAscii(rt, "two");
  EXPECT_TRUE(checkValue(Value(rt, s), "'two'"));

  {
    // rvalue assignment of trivial value
    Value v1 = 100;
    Value v2 = String::createFromAscii(rt, "hundred");
    v2 = std::move(v1);
    EXPECT_TRUE(v2.isNumber());
    EXPECT_EQ(v2.getNumber(), 100);
  }

  {
    // rvalue assignment of js heap value
    Value v1 = String::createFromAscii(rt, "hundred");
    Value v2 = 100;
    v2 = std::move(v1);
    EXPECT_TRUE(v2.isString());
    EXPECT_EQ(v2.getString(rt).utf8(rt), "hundred");
  }

  Object o = Object(rt);
  EXPECT_TRUE(function("function(value) { return typeof(value) == 'object'; }")
                  .call(rt, Value(rt, o))
                  ->getBool());

  uint8_t utf8[] = "[null, 2, \"c\", \"emoji: \xf0\x9f\x86\x97\", {}]";

  EXPECT_TRUE(
      function("function (arr) { return "
               "Array.isArray(arr) && "
               "arr.length == 5 && "
               "arr[0] === null && "
               "arr[1] == 2 && "
               "arr[2] == 'c' && "
               "arr[3] == 'emoji: \\uD83C\\uDD97' && "
               "typeof arr[4] == 'object'; }")
          .call(rt,
                Value::createFromJsonUtf8(rt, utf8, sizeof(utf8) - 1).value())
          ->getBool());

  EXPECT_TRUE(eval("undefined")->isUndefined());
  EXPECT_TRUE(eval("null")->isNull());
  EXPECT_TRUE(eval("true")->isBool());
  EXPECT_TRUE(eval("false")->isBool());
  EXPECT_TRUE(eval("123")->isNumber());
  EXPECT_TRUE(eval("123.4")->isNumber());
  EXPECT_TRUE(eval("'str'")->isString());
  // "{}" returns undefined->  empty code block?
  EXPECT_TRUE(eval("({})")->isObject());
  EXPECT_TRUE(eval("[]")->isObject());
  EXPECT_TRUE(eval("(function(){})")->isObject());

  EXPECT_EQ(eval("123")->getNumber(), 123);
  EXPECT_EQ(eval("123.4")->getNumber(), 123.4);
  EXPECT_EQ(eval("'str'")->getString(rt).utf8(rt), "str");
  EXPECT_TRUE(eval("[]")->getObject(rt).isArray(rt));

  EXPECT_TRUE(eval("true")->getBool());
  EXPECT_EQ(eval("456")->asNumber(rt), 456);
  EXPECT_EQ(eval("'word'")->asNumber(rt), std::nullopt);
  EXPECT_EQ(
      eval("({1:2, 3:4})")->asObject(rt)->getProperty(rt, "1")->getNumber(), 2);
  EXPECT_EQ(eval("'oops'")->asObject(rt), std::nullopt);

  EXPECT_EQ(eval("['zero',1,2,3]")->toString(rt)->utf8(rt), "zero,1,2,3");
}

TEST_P(JSITest, EqualsTest) {
  EXPECT_TRUE(Object::strictEquals(rt, rt.global(), rt.global()));
  EXPECT_TRUE(Value::strictEquals(rt, 1, 1));
  EXPECT_FALSE(Value::strictEquals(rt, true, 1));
  EXPECT_FALSE(Value::strictEquals(rt, true, false));
  EXPECT_TRUE(Value::strictEquals(rt, false, false));
  EXPECT_FALSE(Value::strictEquals(rt, nullptr, 1));
  EXPECT_TRUE(Value::strictEquals(rt, nullptr, nullptr));
  EXPECT_TRUE(Value::strictEquals(rt, Value::undefined(), Value()));
  EXPECT_TRUE(Value::strictEquals(rt, rt.global(), Value(rt.global())));
  EXPECT_FALSE(Value::strictEquals(rt, std::numeric_limits<double>::quiet_NaN(),
                                   std::numeric_limits<double>::quiet_NaN()));
  EXPECT_FALSE(
      Value::strictEquals(rt, std::numeric_limits<double>::signaling_NaN(),
                          std::numeric_limits<double>::signaling_NaN()));
  EXPECT_TRUE(Value::strictEquals(rt, +0.0, -0.0));
  EXPECT_TRUE(Value::strictEquals(rt, -0.0, +0.0));

  Function noop = Function::createFromHostFunction(
      rt, PropNameID::forAscii(rt, "noop"), 0,
      [](const Runtime&, const Value&, const Value*, size_t) {
        return Value();
      });
  auto noopDup = Value(rt, noop).getObject(rt);
  EXPECT_TRUE(Object::strictEquals(rt, noop, noopDup));
  EXPECT_TRUE(Object::strictEquals(rt, noopDup, noop));
  EXPECT_FALSE(Object::strictEquals(rt, noop, rt.global()));
  EXPECT_TRUE(Object::strictEquals(rt, noop, noop));
  EXPECT_TRUE(Value::strictEquals(rt, Value(rt, noop), Value(rt, noop)));

  String str = String::createFromAscii(rt, "rick");
  String strDup = String::createFromAscii(rt, "rick");
  String otherStr = String::createFromAscii(rt, "morty");
  EXPECT_TRUE(String::strictEquals(rt, str, str));
  EXPECT_TRUE(String::strictEquals(rt, str, strDup));
  EXPECT_TRUE(String::strictEquals(rt, strDup, str));
  EXPECT_FALSE(String::strictEquals(rt, str, otherStr));
  EXPECT_TRUE(Value::strictEquals(rt, Value(rt, str), Value(rt, str)));
  EXPECT_FALSE(Value::strictEquals(rt, Value(rt, str), Value(rt, noop)));
  EXPECT_FALSE(Value::strictEquals(rt, Value(rt, str), 1.0));
}

TEST_P(JSITest, ExceptionInfoTest) {
  // exec error
  static const char invokeUndefinedScript[] =
      "function hello() {"
      "  var a = {}; a.log(); }"
      "function world() { hello(); }"
      "world()";
  // compile error
  static const char compileErrorScript[] =
      "let foo = 1;"
      "let foo = 2;";
  std::string stack;
  auto ret = rt.evaluateJavaScript(
      std::make_unique<StringBuffer>(invokeUndefinedScript), "");
  EXPECT_FALSE(ret.has_value());
  EXPECT_NE(ret.error().stack().find("world"), std::string::npos);

  ret = rt.evaluateJavaScript(
      std::make_unique<StringBuffer>(compileErrorScript), "");
  EXPECT_FALSE(ret.has_value());
  EXPECT_EQ(std::string(ret.error().name()), "SyntaxError");
}

TEST_P(JSITest, PreparedJavaScriptSourceTest) {
  rt.SetEnableUserBytecode(true);
  auto ret =
      rt.evaluateJavaScript(std::make_unique<StringBuffer>("var q = 0;"), "");
  EXPECT_TRUE(ret.has_value());
  auto prep = rt.prepareJavaScript(std::make_unique<StringBuffer>("q++;"),
                                   "/app-service.js");
  EXPECT_EQ(rt.global().getProperty(rt, "q")->getNumber(), 0);
  ret = rt.evaluatePreparedJavaScript(prep);
  EXPECT_TRUE(ret.has_value());
  EXPECT_EQ(rt.global().getProperty(rt, "q")->getNumber(), 1);
  ret = rt.evaluatePreparedJavaScript(prep);
  EXPECT_TRUE(ret.has_value());
  EXPECT_EQ(rt.global().getProperty(rt, "q")->getNumber(), 2);
  prep = rt.prepareJavaScript(std::make_unique<StringBuffer>("q++;"),
                              "/app-service.js");
  ret = rt.evaluatePreparedJavaScript(prep);
  EXPECT_TRUE(ret.has_value());
  EXPECT_EQ(rt.global().getProperty(rt, "q")->getNumber(), 3);
  rt.SetEnableUserBytecode(false);
}

// TODO(wangqingyu): no-exception
// TEST_P(JSITest, PreparedJavaScriptURLInBacktrace) {
//   std::string sourceURL = "//PreparedJavaScriptURLInBacktrace/Test/URL";
//   std::string throwingSource =
//       "function thrower() { throw new Error('oops')}"
//       "thrower();";
//   auto prep = rt.prepareJavaScript(
//       std::make_unique<StringBuffer>(throwingSource), sourceURL);
//   try {
//     rt.evaluatePreparedJavaScript(prep);
//     FAIL() << "prepareJavaScript should have thrown an exception";
//   } catch (JSError err) {
//     EXPECT_NE(std::string::npos, err.stack().find(sourceURL))
//         << "Backtrace should contain source URL";
//   }
// }

// TODO(wangqingyu): no-exception
// namespace {

// unsigned countOccurences(const std::string& of, const std::string& in) {
//   unsigned occurences = 0;
//   std::string::size_type lastOccurence = -1;
//   while ((lastOccurence = in.find(of, lastOccurence + 1)) !=
//          std::string::npos) {
//     occurences++;
//   }
//   return occurences;
// }

// }  // namespace
// TEST_P(JSITest, JSErrorsArePropagatedNicely) {
//   unsigned callsBeforeError = 5;

//   Function sometimesThrows = function(
//       "function sometimesThrows(shouldThrow, callback) {"
//       "  if (shouldThrow) {"
//       "    throw Error('Omg, what a nasty exception')"
//       "  }"
//       "  callback(callback);"
//       "}");

//   Function callback = Function::createFromHostFunction(
//       rt, PropNameID::forAscii(rt, "callback"), 0,
//       [&sometimesThrows, &callsBeforeError](Runtime& rt, const Value&
//       thisVal,
//                                             const Value* args, size_t count)
//                                             {
//         return sometimesThrows.call(rt, --callsBeforeError == 0, args[0]);
//       });

//   try {
//     sometimesThrows.call(rt, false, callback);
//   } catch (JSError& error) {
//     EXPECT_EQ(error.message(), "Omg, what a nasty exception");
//     EXPECT_EQ(countOccurences("sometimesThrows", error.stack()), 6);

//     // system JSC JSI does not implement host function names
//     // EXPECT_EQ(countOccurences("callback", error.stack(rt)), 5);
//   }
// }

TEST_P(JSITest, JSErrorsCanBeConstructedWithStack) {
  auto err = JSError(rt, std::string("message"), std::string("stack"));
  EXPECT_EQ(err.message(), "message");
  EXPECT_EQ(err.stack(), "stack");
}

TEST_P(JSITest, JSErrorDoesNotInfinitelyRecurse) {
  Value globalError = rt.global().getProperty(rt, "Error").value();
  rt.global().setProperty(rt, "Error", Value::undefined());
  // TODO(wangqingyu): no-exception
  GTEST_SKIP() << "no-exception";
  // try {
  //   rt.global().getPropertyAsFunction(rt, "NotAFunction");
  //   FAIL() << "expected exception";
  // } catch (const JSError& ex) {
  //   EXPECT_EQ(ex.message(),
  //             "callGlobalFunction: JS global property 'Error' is undefined, "
  //             "expected a Function (while raising getPropertyAsObject: "
  //             "property 'NotAFunction' is undefined, expected an Object)");
  // }

  // If Error is missing, this is fundamentally a problem with JS code
  // messing up the global object, so it should present in JS code as
  // a catchable string.  Not an Error (because that's broken), or as
  // a C++ failure.

  auto fails = [](Runtime& rt, const Value&, const Value*, size_t) -> Value {
    return rt.global().getPropertyAsObject(rt, "NotAProperty").value();
  };
  EXPECT_EQ(function("function (f) { try { f(); return 'undefined'; }"
                     "catch (e) { return typeof e; } }")
                .call(rt, Function::createFromHostFunction(
                              rt, PropNameID::forAscii(rt, "fails"), 0,
                              std::move(fails)))
                ->getString(rt)
                .utf8(rt),
            "string");

  rt.global().setProperty(rt, "Error", globalError);
}

// TODO(wangqingyu): no-exception
// TEST_P(JSITest, JSErrorStackOverflowHandling) {
//   rt.global().setProperty(
//       rt, "callSomething",
//       Function::createFromHostFunction(
//           rt, PropNameID::forAscii(rt, "callSomething"), 0,
//           [this](Runtime& rt2, const Value& thisVal, const Value* args,
//                  size_t count) {
//             EXPECT_EQ(&rt, &rt2);
//             return function("function() { return 0; }").call(rt);
//           }));
//   try {
//     eval("(function f() { callSomething(); f.apply(); })()");
//     FAIL();
//   } catch (const JSError& ex) {
//     EXPECT_NE(std::string(ex.message()).find("exceeded"), std::string::npos);
//   }
// }

TEST_P(JSITest, ScopeDoesNotCrashTest) {
  Scope scope(rt);
  Object o(rt);
}

TEST_P(JSITest, ScopeDoesNotCrashWhenValueEscapes) {
  Value v;
  Scope::callInNewScope(rt, [&]() {
    Object o(rt);
    o.setProperty(rt, "a", 5);
    v = std::move(o);
  });
  EXPECT_EQ(v.getObject(rt).getProperty(rt, "a")->getNumber(), 5);
}

// Verifies you can have a host object that emulates a normal object
TEST_P(JSITest, HostObjectWithValueMembers) {
  class Bag : public HostObject {
   public:
    Bag() = default;

    const Value& operator[](const std::string& name) const {
      auto iter = data_.find(name);
      if (iter == data_.end()) {
        return undef_;
      }
      return iter->second;
    }

   protected:
    Value get(Runtime* rt, const PropNameID& name) override {
      return Value(*rt, (*this)[name.utf8(*rt)]);
    }

    void set(Runtime* rt, const PropNameID& name, const Value& val) override {
      data_.emplace(name.utf8(*rt), Value(*rt, val));
    }

    Value undef_;
    std::map<std::string, Value> data_;
  };

  auto sharedBag = std::make_shared<Bag>();
  auto& bag = *sharedBag;
  Object jsbag = Object::createFromHostObject(rt, std::move(sharedBag));
  auto set = function(
      "function (o) {"
      "  o.foo = 'bar';"
      "  o.count = 37;"
      "  o.nul = null;"
      "  o.iscool = true;"
      "  o.obj = { 'foo': 'bar' };"
      "}");
  set.call(rt, jsbag);
  auto checkFoo = function("function (o) { return o.foo === 'bar'; }");
  auto checkCount = function("function (o) { return o.count === 37; }");
  auto checkNul = function("function (o) { return o.nul === null; }");
  auto checkIsCool = function("function (o) { return o.iscool === true; }");
  auto checkObj = function(
      "function (o) {"
      "  return (typeof o.obj) === 'object' && o.obj.foo === 'bar';"
      "}");
  // Check this looks good from js
  EXPECT_TRUE(checkFoo.call(rt, jsbag)->getBool());
  EXPECT_TRUE(checkCount.call(rt, jsbag)->getBool());
  EXPECT_TRUE(checkNul.call(rt, jsbag)->getBool());
  EXPECT_TRUE(checkIsCool.call(rt, jsbag)->getBool());
  EXPECT_TRUE(checkObj.call(rt, jsbag)->getBool());

  // Check this looks good from c++
  EXPECT_EQ(bag["foo"].getString(rt).utf8(rt), "bar");
  EXPECT_EQ(bag["count"].getNumber(), 37);
  EXPECT_TRUE(bag["nul"].isNull());
  EXPECT_TRUE(bag["iscool"].getBool());
  EXPECT_EQ(
      bag["obj"].getObject(rt).getProperty(rt, "foo")->getString(rt).utf8(rt),
      "bar");
}

TEST_P(JSITest, SymbolTest) {
  if (!rt.global().hasProperty(rt, "Symbol")) {
    // Symbol is an es6 feature which doesn't exist in older VMs.  So
    // the tests which might be elsewhere are all here, so they can be
    // skipped.
    return;
  }

  // ObjectTest
  eval("x = {1:2, 'three':Symbol('four')}");
  Object x = rt.global().getPropertyAsObject(rt, "x").value();
  EXPECT_EQ(x.getPropertyNames(rt)->size(rt), 2);
  EXPECT_TRUE(x.hasProperty(rt, "three"));
  EXPECT_EQ(x.getProperty(rt, "three")->getSymbol(rt).toString(rt),
            "Symbol(four)");

  // ValueTest
  EXPECT_TRUE(eval("Symbol('sym')")->isSymbol());
  EXPECT_EQ(eval("Symbol('sym')")->getSymbol(rt).toString(rt), "Symbol(sym)");

  // EqualsTest
  EXPECT_FALSE(Symbol::strictEquals(rt, eval("Symbol('a')")->getSymbol(rt),
                                    eval("Symbol('a')")->getSymbol(rt)));
  EXPECT_TRUE(Symbol::strictEquals(rt, eval("Symbol.for('a')")->getSymbol(rt),
                                   eval("Symbol.for('a')")->getSymbol(rt)));
  EXPECT_FALSE(Value::strictEquals(rt, eval("Symbol('a')").value(),
                                   eval("Symbol('a')").value()));
  EXPECT_TRUE(Value::strictEquals(rt, eval("Symbol.for('a')").value(),
                                  eval("Symbol.for('a')").value()));
  EXPECT_FALSE(Value::strictEquals(rt, eval("Symbol('a')").value(),
                                   eval("'a'").value()));
}

// TODO(wangqingyu): no-exception
// TEST_P(JSITest, JSErrorTest) {
//   // JSError creation can lead to further errors.  Make sure these
//   // cases are handled and don't cause weird crashes or other issues.
//   //
//   // Getting message property can throw

//   EXPECT_THROW(
//       eval("var messageThrows = {get message() { throw Error('ex'); }};"
//            "throw messageThrows;"),
//       JSIException);

//   EXPECT_THROW(
//       eval("var messageThrows = {get message() { throw messageThrows; }};"
//            "throw messageThrows;"),
//       JSIException);

//   // Converting exception message to String can throw

//   EXPECT_THROW(eval("Object.defineProperty("
//                     "  globalThis, 'String', {configurable:true, get() { var
//                     e "
//                     "= Error(); e.message = 23; throw e; }});"
//                     "var e = Error();"
//                     "e.message = 17;"
//                     "throw e;"),
//                JSIException);

//   EXPECT_THROW(eval("var e = Error();"
//                     "Object.defineProperty("
//                     "  e, 'message', {configurable:true, get() { throw "
//                     "Error('getter'); }});"
//                     "throw e;"),
//                JSIException);

//   EXPECT_THROW(eval("var e = Error();"
//                     "String = function() { throw Error('ctor'); };"
//                     "throw e;"),
//                JSIException);

//   // Converting an exception message to String can return a non-String

//   EXPECT_THROW(eval("String = function() { return 42; };"
//                     "var e = Error();"
//                     "e.message = 17;"
//                     "throw e;"),
//                JSIException);

//   // Exception can be non-Object

//   EXPECT_THROW(eval("throw 17;"), JSIException);

//   EXPECT_THROW(eval("throw undefined;"), JSIException);

//   // Converting exception with no message or stack property to String can
//   throw

//   EXPECT_THROW(eval("var e = {toString() { throw new Error('errstr'); }};"
//                     "throw e;"),
//                JSIException);
// }

TEST_P(JSITest, JSONParseTest) {
  auto data_string = piper::String::createFromUtf8(
      rt, R"---({"name":"test","age":21,"ok":true})---");
  auto ret = Value::createFromJsonString(rt, data_string);
  EXPECT_TRUE(ret && ret->isObject());

  data_string =
      piper::String::createFromUtf8(rt, R"---({"name":"test","ag")---");
  ret = Value::createFromJsonString(rt, data_string);
  EXPECT_TRUE(!ret);
}

TEST_P(JSITest, JSINativeExceptionCollector) {
  auto hostFunction = Function::createFromHostFunction(
      rt, PropNameID::forAscii(rt, "testFuncton"), 0,

      [](Runtime& rt, const piper::Value& thisVal, const piper::Value* args,
         size_t count) -> base::expected<Value, JSINativeException> {
        JSINativeExceptionCollector::Instance()->SetMessage(
            "test exception msg!");
        JSINativeExceptionCollector::Instance()->AddStack(__FUNCTION__,
                                                          __FILE__, 1);
        return piper::Value();
      });

  rt.SetEnableJsBindingApiThrowException(true);
  rt.global().setProperty(rt, "testFuncton", hostFunction);

  auto ret = eval(R"---(
    try {
      testFuncton();
    } catch (e) {
      e.message + " " + e.stack;
    }
  )---");

  std::unordered_map<JSRuntimeType, std::string> exception_map{
      {JSRuntimeType::quickjs,
       "test exception msg!     at <eval> (<input>:3:20)\n    at eval "
       "(native)\n"},
      {JSRuntimeType::jsc,
       "test exception msg! @[native code]\neval code@\neval@[native code]"},
      {JSRuntimeType::v8, "test exception msg! Error: test exception msg!"}};

  EXPECT_TRUE(ToString(ret).find(exception_map[rt.type()]) !=
              std::string::npos);
}

inline std::vector<RuntimeFactory> runtimeGeneratorsFull() {
  std::vector<RuntimeFactory> runtime_factories{};

  runtime_factories.emplace_back(MakeRuntimeFactory<QuickjsRuntime>);
#ifdef OS_OSX
  runtime_factories.emplace_back(MakeRuntimeFactory<JSCRuntime>);
  runtime_factories.emplace_back(MakeRuntimeFactory<V8Runtime>);
#endif

  return runtime_factories;
}

INSTANTIATE_TEST_SUITE_P(
    Runtimes, JSITest, ::testing::ValuesIn(runtimeGeneratorsFull()),
    [](const ::testing::TestParamInfo<JSITest::ParamType>& info) {
      auto rt = info.param(nullptr);
      switch (rt->type()) {
        case JSRuntimeType::v8:
          return "v8";
        case JSRuntimeType::jsc:
          return "jsc";
        case JSRuntimeType::quickjs:
          return "quickjs";
      }
    });

}  // namespace test
}  // namespace lynx::piper
