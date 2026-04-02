// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/cbor_codec.h"

#include <cmath>
#include <cstring>
#include <utility>

namespace lynx {
namespace cbor {

// ===== Encoder Implementation =====

std::vector<uint8_t> Encoder::Encode(const Value& val) {
  buf_.clear();
  EncodeValue(val);
  return buf_;
}

void Encoder::EncodeInitialByte(MajorType mt, uint8_t addinfo) {
  buf_.push_back((static_cast<uint8_t>(mt) << 5) | (addinfo & 0x1F));
}

void Encoder::EncodeUnsigned(uint64_t val) {
  if (val <= 23) {
    EncodeInitialByte(MajorType::UNSIGNED, static_cast<uint8_t>(val));
  } else if (val <= 0xFF) {
    EncodeInitialByte(MajorType::UNSIGNED, 24);
    buf_.push_back(static_cast<uint8_t>(val));
  } else if (val <= 0xFFFF) {
    EncodeInitialByte(MajorType::UNSIGNED, 25);
    buf_.push_back(static_cast<uint8_t>(val >> 8));
    buf_.push_back(static_cast<uint8_t>(val & 0xFF));
  } else if (val <= 0xFFFFFFFF) {
    EncodeInitialByte(MajorType::UNSIGNED, 26);
    for (int i = 3; i >= 0; --i) {
      buf_.push_back(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
    }
  } else {
    EncodeInitialByte(MajorType::UNSIGNED, 27);
    for (int i = 7; i >= 0; --i) {
      buf_.push_back(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
    }
  }
}

void Encoder::EncodeNegative(int64_t val) {
  // CBOR represents negative as (-1 - val), so -1 becomes 0, -2 becomes 1, etc.
  uint64_t encoded = static_cast<uint64_t>(-(val + 1));
  if (encoded <= 23) {
    EncodeInitialByte(MajorType::NEGATIVE, static_cast<uint8_t>(encoded));
  } else if (encoded <= 0xFF) {
    EncodeInitialByte(MajorType::NEGATIVE, 24);
    buf_.push_back(static_cast<uint8_t>(encoded));
  } else if (encoded <= 0xFFFF) {
    EncodeInitialByte(MajorType::NEGATIVE, 25);
    buf_.push_back(static_cast<uint8_t>(encoded >> 8));
    buf_.push_back(static_cast<uint8_t>(encoded & 0xFF));
  } else if (encoded <= 0xFFFFFFFF) {
    EncodeInitialByte(MajorType::NEGATIVE, 26);
    for (int i = 3; i >= 0; --i) {
      buf_.push_back(static_cast<uint8_t>((encoded >> (i * 8)) & 0xFF));
    }
  } else {
    EncodeInitialByte(MajorType::NEGATIVE, 27);
    for (int i = 7; i >= 0; --i) {
      buf_.push_back(static_cast<uint8_t>((encoded >> (i * 8)) & 0xFF));
    }
  }
}

void Encoder::EncodeBytes(const std::vector<uint8_t>& val) {
  const size_t len = val.size();
  if (len <= 23) {
    EncodeInitialByte(MajorType::BYTES, static_cast<uint8_t>(len));
  } else if (len <= 0xFF) {
    EncodeInitialByte(MajorType::BYTES, 24);
    buf_.push_back(static_cast<uint8_t>(len));
  } else if (len <= 0xFFFF) {
    EncodeInitialByte(MajorType::BYTES, 25);
    buf_.push_back(static_cast<uint8_t>(len >> 8));
    buf_.push_back(static_cast<uint8_t>(len & 0xFF));
  } else if (len <= 0xFFFFFFFF) {
    EncodeInitialByte(MajorType::BYTES, 26);
    for (int i = 3; i >= 0; --i) {
      buf_.push_back(static_cast<uint8_t>((len >> (i * 8)) & 0xFF));
    }
  } else {
    EncodeInitialByte(MajorType::BYTES, 27);
    for (int i = 7; i >= 0; --i) {
      buf_.push_back(static_cast<uint8_t>((len >> (i * 8)) & 0xFF));
    }
  }
  buf_.insert(buf_.end(), val.begin(), val.end());
}

void Encoder::EncodeText(const std::string& val) {
  const size_t len = val.size();
  if (len <= 23) {
    EncodeInitialByte(MajorType::TEXT, static_cast<uint8_t>(len));
  } else if (len <= 0xFF) {
    EncodeInitialByte(MajorType::TEXT, 24);
    buf_.push_back(static_cast<uint8_t>(len));
  } else if (len <= 0xFFFF) {
    EncodeInitialByte(MajorType::TEXT, 25);
    buf_.push_back(static_cast<uint8_t>(len >> 8));
    buf_.push_back(static_cast<uint8_t>(len & 0xFF));
  } else if (len <= 0xFFFFFFFF) {
    EncodeInitialByte(MajorType::TEXT, 26);
    for (int i = 3; i >= 0; --i) {
      buf_.push_back(static_cast<uint8_t>((len >> (i * 8)) & 0xFF));
    }
  } else {
    EncodeInitialByte(MajorType::TEXT, 27);
    for (int i = 7; i >= 0; --i) {
      buf_.push_back(static_cast<uint8_t>((len >> (i * 8)) & 0xFF));
    }
  }
  buf_.insert(buf_.end(), val.begin(), val.end());
}

void Encoder::EncodeArray(const std::vector<Value>& val) {
  const size_t len = val.size();
  if (len <= 23) {
    EncodeInitialByte(MajorType::ARRAY, static_cast<uint8_t>(len));
  } else if (len <= 0xFF) {
    EncodeInitialByte(MajorType::ARRAY, 24);
    buf_.push_back(static_cast<uint8_t>(len));
  } else if (len <= 0xFFFF) {
    EncodeInitialByte(MajorType::ARRAY, 25);
    buf_.push_back(static_cast<uint8_t>(len >> 8));
    buf_.push_back(static_cast<uint8_t>(len & 0xFF));
  } else if (len <= 0xFFFFFFFF) {
    EncodeInitialByte(MajorType::ARRAY, 26);
    for (int i = 3; i >= 0; --i) {
      buf_.push_back(static_cast<uint8_t>((len >> (i * 8)) & 0xFF));
    }
  } else {
    EncodeInitialByte(MajorType::ARRAY, 27);
    for (int i = 7; i >= 0; --i) {
      buf_.push_back(static_cast<uint8_t>((len >> (i * 8)) & 0xFF));
    }
  }
  for (const auto& v : val) {
    EncodeValue(v);
  }
}

void Encoder::EncodeMap(const std::map<std::string, Value>& val) {
  const size_t len = val.size();
  if (len <= 23) {
    EncodeInitialByte(MajorType::MAP, static_cast<uint8_t>(len));
  } else if (len <= 0xFF) {
    EncodeInitialByte(MajorType::MAP, 24);
    buf_.push_back(static_cast<uint8_t>(len));
  } else if (len <= 0xFFFF) {
    EncodeInitialByte(MajorType::MAP, 25);
    buf_.push_back(static_cast<uint8_t>(len >> 8));
    buf_.push_back(static_cast<uint8_t>(len & 0xFF));
  } else if (len <= 0xFFFFFFFF) {
    EncodeInitialByte(MajorType::MAP, 26);
    for (int i = 3; i >= 0; --i) {
      buf_.push_back(static_cast<uint8_t>((len >> (i * 8)) & 0xFF));
    }
  } else {
    EncodeInitialByte(MajorType::MAP, 27);
    for (int i = 7; i >= 0; --i) {
      buf_.push_back(static_cast<uint8_t>((len >> (i * 8)) & 0xFF));
    }
  }
  for (const auto& [k, v] : val) {
    EncodeText(k);
    EncodeValue(v);
  }
}

void Encoder::EncodeBool(bool val) {
  EncodeInitialByte(MajorType::FLOAT,
                    val ? static_cast<uint8_t>(SimpleValue::TRUE)
                        : static_cast<uint8_t>(SimpleValue::FALSE));
}

void Encoder::EncodeNull() {
  EncodeInitialByte(MajorType::FLOAT, static_cast<uint8_t>(SimpleValue::NULL_));
}

void Encoder::EncodeValue(const Value& val) {
  switch (val.type()) {
    case ValueType::UNSIGNED:
      EncodeUnsigned(val.AsUnsigned());
      break;
    case ValueType::NEGATIVE:
      EncodeNegative(val.AsInt());
      break;
    case ValueType::BYTES:
      EncodeBytes(val.AsBytes());
      break;
    case ValueType::TEXT:
      EncodeText(val.AsText());
      break;
    case ValueType::ARRAY:
      EncodeArray(val.AsArray());
      break;
    case ValueType::MAP:
      EncodeMap(val.AsMap());
      break;
    case ValueType::BOOLEAN:
      EncodeBool(val.AsBool());
      break;
    case ValueType::NULL_:
      EncodeNull();
      break;
    default:
      EncodeNull();
      break;
  }
}

// ===== Decoder Implementation =====

Value Decoder::Decode(const std::vector<uint8_t>& data) {
  data_ = data.data();
  size_ = data.size();
  pos_ = 0;
  error_.clear();
  return DecodeValue();
}

Value Decoder::DecodeValue() {
  if (pos_ >= size_) {
    error_ = "unexpected end of data";
    return Value(nullptr);
  }

  uint8_t ib = data_[pos_++];
  auto mt = static_cast<MajorType>(ib >> 5);
  uint8_t addinfo = ib & 0x1F;

  switch (mt) {
    case MajorType::UNSIGNED:
      return Value(DecodeUnsigned(addinfo));
    case MajorType::NEGATIVE:
      return Value(DecodeNegative(addinfo));
    case MajorType::BYTES:
      return Value(DecodeBytes(addinfo));
    case MajorType::TEXT:
      return Value(DecodeText(addinfo));
    case MajorType::ARRAY:
      return Value(DecodeArray(addinfo));
    case MajorType::MAP:
      return Value(DecodeMap(addinfo));
    case MajorType::TAG:
      // Skip tag and decode next value
      DecodeUnsigned(addinfo);
      return DecodeValue();
    case MajorType::FLOAT:
      return DecodeSimple(addinfo);
    default:
      error_ = "invalid major type";
      return Value(nullptr);
  }
}

uint64_t Decoder::DecodeUnsigned(uint8_t addinfo) {
  if (addinfo <= 23) {
    return addinfo;
  } else if (addinfo == 24) {
    return ReadUint64(1);
  } else if (addinfo == 25) {
    // 16-bit float - skip for now
    pos_ += 2;
    return 0;
  } else if (addinfo == 26) {
    // 32-bit float - skip for now
    pos_ += 4;
    return 0;
  } else if (addinfo == 27) {
    // 64-bit float - skip for now
    pos_ += 8;
    return 0;
  } else if (addinfo == 31) {
    // Indefinite length - not supported
    error_ = "indefinite length not supported";
    return 0;
  }
  return 0;
}

int64_t Decoder::DecodeNegative(uint8_t addinfo) {
  uint64_t encoded = DecodeUnsigned(addinfo);
  return -1 - static_cast<int64_t>(encoded);
}

std::vector<uint8_t> Decoder::DecodeBytes(uint8_t addinfo) {
  uint64_t len = DecodeUnsigned(addinfo);
  if (pos_ + len > size_) {
    error_ = "truncated byte string";
    return {};
  }
  std::vector<uint8_t> result(data_ + pos_, data_ + pos_ + len);
  pos_ += len;
  return result;
}

std::string Decoder::DecodeText(uint8_t addinfo) {
  std::vector<uint8_t> bytes = DecodeBytes(addinfo);
  return std::string(bytes.begin(), bytes.end());
}

std::vector<Value> Decoder::DecodeArray(uint8_t addinfo) {
  uint64_t len = DecodeUnsigned(addinfo);
  std::vector<Value> result;
  result.reserve(len);
  for (uint64_t i = 0; i < len; ++i) {
    result.push_back(DecodeValue());
  }
  return result;
}

std::map<std::string, Value> Decoder::DecodeMap(uint8_t addinfo) {
  uint64_t len = DecodeUnsigned(addinfo);
  std::map<std::string, Value> result;
  for (uint64_t i = 0; i < len; ++i) {
    Value key = DecodeValue();
    Value val = DecodeValue();
    if (key.IsText()) {
      result[key.AsText()] = std::move(val);
    }
  }
  return result;
}

Value Decoder::DecodeSimple(uint8_t addinfo) {
  if (addinfo == static_cast<uint8_t>(SimpleValue::FALSE)) {
    return Value(false);
  } else if (addinfo == static_cast<uint8_t>(SimpleValue::TRUE)) {
    return Value(true);
  } else if (addinfo == static_cast<uint8_t>(SimpleValue::NULL_)) {
    return Value(nullptr);
  } else if (addinfo == 26) {
    // 32-bit float
    if (pos_ + 4 > size_) {
      error_ = "truncated float";
      return Value(nullptr);
    }
    uint32_t bits = 0;
    for (int i = 0; i < 4; ++i) {
      bits = (bits << 8) | data_[pos_++];
    }
    float f;
    std::memcpy(&f, &bits, sizeof(f));
    return Value(static_cast<int64_t>(f));  // Convert to int for simplicity
  } else if (addinfo == 27) {
    // 64-bit float
    if (pos_ + 8 > size_) {
      error_ = "truncated double";
      return Value(nullptr);
    }
    uint64_t bits = 0;
    for (int i = 0; i < 8; ++i) {
      bits = (bits << 8) | data_[pos_++];
    }
    double d;
    std::memcpy(&d, &bits, sizeof(d));
    return Value(static_cast<int64_t>(d));  // Convert to int for simplicity
  }
  return Value(nullptr);
}

uint64_t Decoder::ReadUint64(size_t n) {
  if (pos_ + n > size_) {
    error_ = "unexpected end of data";
    return 0;
  }
  uint64_t result = 0;
  for (size_t i = 0; i < n; ++i) {
    result = (result << 8) | data_[pos_++];
  }
  return result;
}

}  // namespace cbor
}  // namespace lynx
