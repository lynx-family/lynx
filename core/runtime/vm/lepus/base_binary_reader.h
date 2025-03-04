// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_VM_LEPUS_BASE_BINARY_READER_H_
#define CORE_RUNTIME_VM_LEPUS_BASE_BINARY_READER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/fml/memory/ref_counted.h"
#include "core/runtime/vm/lepus/binary_reader.h"
#include "core/runtime/vm/lepus/function.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/regexp.h"
#include "core/template_bundle/template_codec/compile_options.h"

namespace lynx {
namespace lepus {

#define DECODE_VALUE(name) \
  lepus::Value name;       \
  ERROR_UNLESS(DecodeValue(&name))

#define DECODE_VALUE_INTO(value) ERROR_UNLESS(DecodeValue(&(value)))

#define DECODE_VALUE_HEADER(name) \
  lepus::Value name;              \
  ERROR_UNLESS(DecodeValue(&name, true))

#define DECODE_VALUE_HEADER_INTO(value) \
  ERROR_UNLESS(DecodeValue(&(value), true))

#define DECODE_STR(name) \
  base::String name;     \
  ERROR_UNLESS(DecodeUtf8Str(name))

#define DECODE_STR_INTO(name) ERROR_UNLESS(DecodeUtf8Str(name))

#define DECODE_STDSTR(name) \
  std::string name;         \
  ERROR_UNLESS(DecodeUtf8Str(&name))

#define DECODE_DICTIONARY(name, is_header)                    \
  fml::RefPtr<lepus::Dictionary> name = Dictionary::Create(); \
  ERROR_UNLESS(DecodeTable(name, is_header))

#define DECODE_CLOSURE(name)                                   \
  fml::RefPtr<lepus::Closure> name = Closure::Create(nullptr); \
  ERROR_UNLESS(DecodeClosure(name))

#define DECODE_ARRAY(name)                            \
  fml::RefPtr<lepus::CArray> name = CArray::Create(); \
  ERROR_UNLESS(DecodeArray(name))

#define DECODE_DATE(name)                           \
  fml::RefPtr<lepus::CDate> name = CDate::Create(); \
  ERROR_UNLESS(DecodeDate(name))

#define DECODE_REGEXP(name)                           \
  fml::RefPtr<lepus::RegExp> name = RegExp::Create(); \
  ERROR_UNLESS(DecodeRegExp(name))

#define DECODE_COMPACT_U32(name) \
  uint32_t name = 0;             \
  ERROR_UNLESS(ReadCompactU32(&name))

#define DECODE_COMPACT_S32(name) \
  int32_t name = 0;              \
  ERROR_UNLESS(ReadCompactS32(&name))

#define DECODE_COMPACT_U64(name) \
  uint64_t name = 0;             \
  ERROR_UNLESS(ReadCompactU64(&name))

#define DECODE_U8(name) \
  uint8_t name = 0;     \
  ERROR_UNLESS(ReadU8(&name))

#define DECODE_U32(name) \
  uint32_t name = 0;     \
  ERROR_UNLESS(ReadU32(&name))

#define DECODE_DOUBLE(name) \
  double name = 0.0;        \
  ERROR_UNLESS(ReadCompactD64(&name))

#define DECODE_BOOL(name)             \
  [[maybe_unused]] bool name = false; \
  do {                                \
    uint8_t value = 0;                \
    ERROR_UNLESS(ReadU8(&value));     \
    name = (bool)value;               \
  } while (0)

#define DECODE_FUNCTION(parent, name)                            \
  fml::RefPtr<lepus::Function> name = lepus::Function::Create(); \
  ERROR_UNLESS(DeserializeFunction(parent, name))

class InputStream;
class Closure;
class Dictionary;
class CArray;
class Value;
class CDate;
class Function;
class Context;
class ContextBundle;

class BaseBinaryReader : public BinaryReader {
 public:
  BaseBinaryReader(std::unique_ptr<InputStream> stream)
      : BinaryReader(std::move(stream)) {
#if !ENABLE_JUST_LEPUSNG
    // Reserve to avoid frequent reallocations from 0 capacity.
    func_vec.reserve(128);
#endif
  }
  ~BaseBinaryReader() override = default;

  BaseBinaryReader(const BaseBinaryReader&) = delete;
  BaseBinaryReader& operator=(const BaseBinaryReader&) = delete;

  BaseBinaryReader(BaseBinaryReader&&) = default;
  BaseBinaryReader& operator=(BaseBinaryReader&&) = default;

#if !ENABLE_JUST_LEPUSNG
  bool DeserializeFunction(fml::RefPtr<Function>& parent,
                           fml::RefPtr<Function>& function);
  bool DeserializeGlobal(
      std::unordered_map<base::String, lepus::Value>& global);
  bool DeserializeTopVariables(
      std::unordered_map<base::String, long>& top_level_variables);
  bool DecodeClosure(fml::RefPtr<Closure>&);
  bool DecodeRegExp(fml::RefPtr<RegExp>& reg);
  bool DecodeDate(fml::RefPtr<CDate>&);
#endif

  // base::String section
  bool DeserializeStringSection();

  bool DecodeUtf8Str(base::String&);
  bool DecodeUtf8Str(std::string*);
  bool DecodeTable(fml::RefPtr<Dictionary>&, bool = false);
  bool DecodeArray(fml::RefPtr<CArray>&);
  bool DecodeValue(Value*, bool = false);

  bool DecodeContextBundle(ContextBundle* bundle);

 protected:
  virtual std::vector<base::String>& string_list();

#if !ENABLE_JUST_LEPUSNG
  // for serialize/deserialize
  std::unordered_map<fml::RefPtr<Function>, int> func_map;
  std::vector<fml::RefPtr<Function>> func_vec;
#endif
  tasm::CompileOptions compile_options_;

 private:
  std::vector<base::String> string_list_;
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_BASE_BINARY_READER_H_
