// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_CBOR_CODEC_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_CBOR_CODEC_H_

#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace lynx {
namespace cbor {

// CBOR major types
enum class MajorType : uint8_t {
  UNSIGNED = 0,  // unsigned integer
  NEGATIVE = 1,  // negative integer
  BYTES = 2,     // byte string
  TEXT = 3,      // text string (UTF-8)
  ARRAY = 4,     // array
  MAP = 5,       // map
  TAG = 6,       // semantic tag
  FLOAT = 7,     // float or simple values
};

// CBOR simple values
enum class SimpleValue : uint8_t {
  FALSE = 20,
  TRUE = 21,
  NULL_ = 22,
  UNDEFINED = 23,
};

// CBOR value types
enum class ValueType {
  UNSIGNED,
  NEGATIVE,
  BYTES,
  TEXT,
  ARRAY,
  MAP,
  FLOAT,
  BOOLEAN,
  NULL_,
};

class Value {
 public:
  Value() : type_(ValueType::NULL_) {}
  explicit Value(uint64_t v) : type_(ValueType::UNSIGNED), uint_val_(v) {}
  explicit Value(int64_t v)
      : type_(v >= 0 ? ValueType::UNSIGNED : ValueType::NEGATIVE) {
    if (v >= 0)
      uint_val_ = static_cast<uint64_t>(v);
    else
      int_val_ = v;
  }
  explicit Value(const std::string& v) : type_(ValueType::TEXT), text_val_(v) {}
  explicit Value(std::string&& v)
      : type_(ValueType::TEXT), text_val_(std::move(v)) {}
  explicit Value(const std::vector<uint8_t>& v)
      : type_(ValueType::BYTES), bytes_val_(v) {}
  explicit Value(bool v) : type_(ValueType::BOOLEAN), bool_val_(v) {}
  explicit Value(std::nullptr_t) : type_(ValueType::NULL_) {}
  explicit Value(const std::vector<Value>& arr)
      : type_(ValueType::ARRAY), array_val_(arr) {}
  explicit Value(std::vector<Value>&& arr)
      : type_(ValueType::ARRAY), array_val_(std::move(arr)) {}
  explicit Value(const std::map<std::string, Value>& m)
      : type_(ValueType::MAP), map_val_(m) {}
  explicit Value(std::map<std::string, Value>&& m)
      : type_(ValueType::MAP), map_val_(std::move(m)) {}

  ValueType type() const { return type_; }

  bool IsNull() const { return type_ == ValueType::NULL_; }
  bool IsUnsigned() const { return type_ == ValueType::UNSIGNED; }
  bool IsInt() const {
    return type_ == ValueType::UNSIGNED || type_ == ValueType::NEGATIVE;
  }
  bool IsText() const { return type_ == ValueType::TEXT; }
  bool IsBytes() const { return type_ == ValueType::BYTES; }
  bool IsBool() const { return type_ == ValueType::BOOLEAN; }
  bool IsArray() const { return type_ == ValueType::ARRAY; }
  bool IsMap() const { return type_ == ValueType::MAP; }

  uint64_t AsUnsigned() const { return uint_val_; }
  int64_t AsInt() const {
    return type_ == ValueType::NEGATIVE ? int_val_
                                        : static_cast<int64_t>(uint_val_);
  }
  const std::string& AsText() const { return text_val_; }
  const std::vector<uint8_t>& AsBytes() const { return bytes_val_; }
  bool AsBool() const { return bool_val_; }
  const std::vector<Value>& AsArray() const { return array_val_; }
  const std::map<std::string, Value>& AsMap() const { return map_val_; }

  // Accessors with defaults
  std::string AsTextOr(const std::string& def) const {
    return IsText() ? text_val_ : def;
  }
  uint64_t AsUnsignedOr(uint64_t def) const {
    return IsUnsigned() ? uint_val_ : def;
  }
  int64_t AsIntOr(int64_t def) const { return IsInt() ? AsInt() : def; }
  bool AsBoolOr(bool def) const { return IsBool() ? bool_val_ : def; }

 private:
  ValueType type_;
  union {
    uint64_t uint_val_;
    int64_t int_val_;
    bool bool_val_;
  };
  std::string text_val_;
  std::vector<uint8_t> bytes_val_;
  std::vector<Value> array_val_;
  std::map<std::string, Value> map_val_;
};

// Encoder
class Encoder {
 public:
  std::vector<uint8_t> Encode(const Value& val);

 private:
  void EncodeValue(const Value& val);
  void EncodeUnsigned(uint64_t val);
  void EncodeNegative(int64_t val);
  void EncodeBytes(const std::vector<uint8_t>& val);
  void EncodeText(const std::string& val);
  void EncodeArray(const std::vector<Value>& val);
  void EncodeMap(const std::map<std::string, Value>& val);
  void EncodeBool(bool val);
  void EncodeNull();
  void EncodeInitialByte(MajorType mt, uint8_t addinfo);

  std::vector<uint8_t> buf_;
};

// Decoder
class Decoder {
 public:
  Value Decode(const std::vector<uint8_t>& data);
  bool HasError() const { return !error_.empty(); }
  const std::string& Error() const { return error_; }

 private:
  Value DecodeValue();
  uint64_t DecodeUnsigned(uint8_t addinfo);
  int64_t DecodeNegative(uint8_t addinfo);
  std::vector<uint8_t> DecodeBytes(uint8_t addinfo);
  std::string DecodeText(uint8_t addinfo);
  std::vector<Value> DecodeArray(uint8_t addinfo);
  std::map<std::string, Value> DecodeMap(uint8_t addinfo);
  double DecodeFloat(uint8_t addinfo);
  Value DecodeSimple(uint8_t addinfo);
  uint64_t ReadUint64(size_t n);

  const uint8_t* data_;
  size_t size_;
  size_t pos_;
  std::string error_;
};

}  // namespace cbor
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_CBOR_CODEC_H_
