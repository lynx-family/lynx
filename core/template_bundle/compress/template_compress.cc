// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/compress/template_compress.h"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <limits>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

// cspell:ignore Wmacro Wsign
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmacro-redefined"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#pragma clang diagnostic ignored "-Wsign-compare"
#endif
#define SKANDA_IMPLEMENTATION
#include "base/include/skanda.h"
#undef SKANDA_IMPLEMENTATION
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

namespace lynx {
namespace tasm {
namespace {

constexpr uint32_t kWholeCompressedTemplateMagic = 0x55000000;
constexpr uint32_t kNoCompression = 0;
constexpr uint32_t kSkandaCompression = 1;
constexpr uint32_t kMaxChunkCount = 256;
constexpr uint32_t kSingleChunkMaxSize = 400 * 1024;

struct ChunkInfo {
  uint32_t compressed_offset{0};
  uint32_t compressed_size{0};
  uint32_t uncompressed_offset{0};
  uint32_t uncompressed_size{0};
};

void SetError(std::string* error_message, const char* message) {
  if (error_message != nullptr) {
    *error_message = message;
  }
}

void AppendCompactU32(std::vector<uint8_t>& buffer, uint32_t value) {
  do {
    uint8_t byte = static_cast<uint8_t>(value & 0x7f);
    value >>= 7;
    if (value != 0) {
      byte |= 0x80;
    }
    buffer.push_back(byte);
  } while (value != 0);
}

bool ReadCompactU32(const uint8_t* buffer, size_t length, size_t& offset,
                    uint32_t& out) {
  uint32_t result = 0;
  uint32_t shift = 0;
  while (offset < length && shift <= 28) {
    uint8_t byte = buffer[offset++];
    result |= static_cast<uint32_t>(byte & 0x7f) << shift;
    if ((byte & 0x80) == 0) {
      out = result;
      return true;
    }
    shift += 7;
  }
  return false;
}

uint32_t NormalizeChunkCount(uint32_t requested, uint32_t total_size) {
  if (total_size == 0 || requested == 0) {
    return 1;
  }
  uint32_t max_chunks = std::max<uint32_t>(
      1, (total_size + kSingleChunkMaxSize - 1) / kSingleChunkMaxSize);
  return std::min({requested, max_chunks, kMaxChunkCount});
}

bool IsChunkRangeValid(uint32_t offset, uint32_t size, size_t limit) {
  return static_cast<uint64_t>(offset) + static_cast<uint64_t>(size) <=
         static_cast<uint64_t>(limit);
}

bool ParseCompressedPayload(const uint8_t* src, size_t length,
                            uint32_t& algorithm, uint32_t& total_size,
                            std::vector<ChunkInfo>& chunks,
                            const uint8_t*& payload, size_t& payload_size,
                            std::string* error_message) {
  if (src == nullptr || length == 0) {
    SetError(error_message, "compressed payload is empty");
    return false;
  }

  size_t offset = 0;
  uint32_t chunk_count = 0;
  if (!ReadCompactU32(src, length, offset, algorithm) ||
      !ReadCompactU32(src, length, offset, total_size) ||
      !ReadCompactU32(src, length, offset, chunk_count) || chunk_count == 0 ||
      chunk_count > kMaxChunkCount) {
    SetError(error_message, "invalid compressed template header");
    return false;
  }

  chunks.clear();
  chunks.reserve(chunk_count);
  for (uint32_t i = 0; i < chunk_count; ++i) {
    ChunkInfo chunk;
    if (!ReadCompactU32(src, length, offset, chunk.compressed_offset) ||
        !ReadCompactU32(src, length, offset, chunk.compressed_size) ||
        !ReadCompactU32(src, length, offset, chunk.uncompressed_offset) ||
        !ReadCompactU32(src, length, offset, chunk.uncompressed_size)) {
      SetError(error_message, "invalid compressed template chunk metadata");
      return false;
    }
    chunks.push_back(chunk);
  }

  if (offset > length) {
    SetError(error_message, "compressed template payload offset overflow");
    return false;
  }

  payload = src + offset;
  payload_size = length - offset;
  return true;
}

std::vector<uint8_t> CompressPayload(const uint8_t* data, size_t length,
                                     uint32_t requested_chunk_count) {
  if (data == nullptr || length == 0 ||
      length > std::numeric_limits<uint32_t>::max()) {
    return {};
  }

  const uint32_t total_size = static_cast<uint32_t>(length);
  const uint32_t chunk_count =
      NormalizeChunkCount(requested_chunk_count, total_size);

  std::vector<ChunkInfo> chunks;
  chunks.reserve(chunk_count);
  uint32_t base_size = total_size / chunk_count;
  uint32_t remainder = total_size % chunk_count;
  uint32_t uncompressed_offset = 0;
  for (uint32_t i = 0; i < chunk_count; ++i) {
    uint32_t current_size = base_size;
    if (i + 1 == chunk_count) {
      current_size += remainder;
    }
    chunks.push_back({0, 0, uncompressed_offset, current_size});
    uncompressed_offset += current_size;
  }

  uint32_t algorithm = kSkandaCompression;
  std::vector<uint8_t> compressed_payload;
  for (auto& chunk : chunks) {
    const uint8_t* src = data + chunk.uncompressed_offset;
    size_t src_size = static_cast<size_t>(chunk.uncompressed_size);
    size_t bound = lynx::base::skanda::compress_bound(src_size);
    std::vector<uint8_t> dst(bound);
    size_t encoded =
        lynx::base::skanda::compress(src, src_size, dst.data(), 10, 0.0f);
    if (lynx::base::skanda::is_error(encoded) || encoded == 0 ||
        encoded >= src_size) {
      algorithm = kNoCompression;
      compressed_payload.clear();
      break;
    }
    chunk.compressed_offset = static_cast<uint32_t>(compressed_payload.size());
    chunk.compressed_size = static_cast<uint32_t>(encoded);
    compressed_payload.insert(compressed_payload.end(), dst.begin(),
                              dst.begin() + encoded);
  }

  if (algorithm == kNoCompression) {
    for (auto& chunk : chunks) {
      chunk.compressed_offset =
          static_cast<uint32_t>(compressed_payload.size());
      chunk.compressed_size = chunk.uncompressed_size;
      const uint8_t* src = data + chunk.uncompressed_offset;
      compressed_payload.insert(compressed_payload.end(), src,
                                src + chunk.uncompressed_size);
    }
  }

  std::vector<uint8_t> result;
  AppendCompactU32(result, algorithm);
  AppendCompactU32(result, total_size);
  AppendCompactU32(result, static_cast<uint32_t>(chunks.size()));
  for (const auto& chunk : chunks) {
    AppendCompactU32(result, chunk.compressed_offset);
    AppendCompactU32(result, chunk.compressed_size);
    AppendCompactU32(result, chunk.uncompressed_offset);
    AppendCompactU32(result, chunk.uncompressed_size);
  }
  result.insert(result.end(), compressed_payload.begin(),
                compressed_payload.end());
  return result;
}

bool DecompressPayload(const uint8_t* src, size_t length,
                       std::vector<uint8_t>& output,
                       std::string* error_message) {
  uint32_t algorithm = 0;
  uint32_t total_size = 0;
  std::vector<ChunkInfo> chunks;
  const uint8_t* payload = nullptr;
  size_t payload_size = 0;
  if (!ParseCompressedPayload(src, length, algorithm, total_size, chunks,
                              payload, payload_size, error_message)) {
    return false;
  }

  output.resize(total_size);
  for (const auto& chunk : chunks) {
    if (!IsChunkRangeValid(chunk.compressed_offset, chunk.compressed_size,
                           payload_size) ||
        !IsChunkRangeValid(chunk.uncompressed_offset, chunk.uncompressed_size,
                           output.size())) {
      SetError(error_message, "compressed template chunk range overflow");
      output.clear();
      return false;
    }
  }

  if (algorithm == kNoCompression) {
    for (const auto& chunk : chunks) {
      if (chunk.compressed_size != chunk.uncompressed_size) {
        SetError(error_message, "stored chunk size mismatch");
        output.clear();
        return false;
      }
      const uint8_t* compressed = payload + chunk.compressed_offset;
      uint8_t* decompressed = output.data() + chunk.uncompressed_offset;
      std::memcpy(decompressed, compressed, chunk.compressed_size);
    }
    return true;
  }

  if (algorithm != kSkandaCompression) {
    SetError(error_message, "unsupported compression algorithm");
    output.clear();
    return false;
  }

  auto decode_chunk = [&](const ChunkInfo& chunk) {
    const uint8_t* compressed = payload + chunk.compressed_offset;
    uint8_t* decompressed = output.data() + chunk.uncompressed_offset;
    return lynx::base::skanda::decompress(compressed, chunk.compressed_size,
                                          decompressed, chunk.uncompressed_size,
                                          nullptr) == 0;
  };

  if (chunks.size() == 1) {
    if (!decode_chunk(chunks.front())) {
      SetError(error_message, "skanda decompress failed");
      output.clear();
      return false;
    }
    return true;
  }

  std::atomic<bool> ok{true};
  std::vector<std::thread> workers;
  workers.reserve(chunks.size());
  for (size_t i = 0; i < chunks.size(); ++i) {
    workers.emplace_back([&, i]() {
      if (!decode_chunk(chunks[i])) {
        ok.store(false, std::memory_order_relaxed);
      }
    });
  }
  for (auto& worker : workers) {
    worker.join();
  }
  if (!ok.load(std::memory_order_relaxed)) {
    SetError(error_message, "skanda decompress failed");
    output.clear();
    return false;
  }

  return true;
}

uint32_t ReadLittleEndianU32(const uint8_t* data) {
  uint32_t value = 0;
  std::memcpy(&value, data, sizeof(value));
  return value;
}

}  // namespace

bool IsWholeCompressedTemplate(const uint8_t* data, size_t length) {
  if (data == nullptr || length < sizeof(uint32_t) * 2) {
    return false;
  }
  return ReadLittleEndianU32(data + sizeof(uint32_t)) ==
         kWholeCompressedTemplateMagic;
}

std::vector<uint8_t> WrapWholeCompressedTemplate(
    const uint8_t* data, size_t length, uint32_t requested_chunk_count) {
  std::vector<uint8_t> payload =
      CompressPayload(data, length, requested_chunk_count);
  if (payload.empty()) {
    return {};
  }

  const uint32_t total_size =
      static_cast<uint32_t>(payload.size() + sizeof(uint32_t) * 2);
  std::vector<uint8_t> result(sizeof(uint32_t) * 2);
  std::memcpy(result.data(), &total_size, sizeof(total_size));
  std::memcpy(result.data() + sizeof(uint32_t), &kWholeCompressedTemplateMagic,
              sizeof(kWholeCompressedTemplateMagic));
  result.insert(result.end(), payload.begin(), payload.end());
  return result;
}

bool UnwrapWholeCompressedTemplate(const uint8_t* data, size_t length,
                                   std::vector<uint8_t>& output,
                                   std::string* error_message) {
  return DecompressPayload(data + sizeof(uint32_t) * 2,
                           length - sizeof(uint32_t) * 2, output,
                           error_message);
}

std::pair<bool, std::unique_ptr<lepus::InputStream>>
MaybeDecompressWholeCompressedTemplateStream(const lepus::InputStream* stream,
                                             std::string& error_message) {
  if (stream == nullptr ||
      stream->size() < static_cast<size_t>(sizeof(uint32_t) * 2) ||
      stream->size() > std::numeric_limits<uint32_t>::max()) {
    return {true, nullptr};
  }

  const uint8_t* bytes = stream->cursor();
  const size_t length = stream->size();
  if (!IsWholeCompressedTemplate(bytes, length)) {
    return {true, nullptr};
  }

  std::vector<uint8_t> decompressed;
  if (!UnwrapWholeCompressedTemplate(bytes, length, decompressed,
                                     &error_message)) {
    return {false, nullptr};
  }

  return {true, std::make_unique<lepus::ByteArrayInputStream>(
                    std::move(decompressed))};
}

}  // namespace tasm
}  // namespace lynx
