// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/recorder/fixture_writer.h"

#include <zlib.h>

#include <cstdint>
#include <cstdio>

#include "core/base/json/json_util.h"
#include "third_party/modp_b64/modp_b64.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace tasm {
namespace recorder {

namespace {

constexpr const char* kDefaultIgnoredKeys[] = {
    "timestamp", "card_version", "containerID", "header", "request_time"};

}  // namespace

std::string JsonToCompactString(const rapidjson::Value& value) {
  return base::ToJson(value);
}

namespace {

// ---- JSON helpers ----

std::string ParseThenCompact(const std::string& json) {
  rapidjson::Document doc;
  doc.Parse(json.c_str());
  if (doc.HasParseError()) {
    return json;
  }
  return JsonToCompactString(doc);
}

std::string CompactJsonOrFallback(const std::string& json,
                                  const char* fallback) {
  rapidjson::Document doc;
  doc.Parse(json.c_str());
  return doc.HasParseError() ? fallback : JsonToCompactString(doc);
}

// Matches the number of newlines produced by JSON.stringify(value, null, 2)
// without instantiating a second JSON writer just to decide asset placement.
size_t CountJsonLines(const rapidjson::Value& value) {
  size_t lines = 0;
  if (value.IsArray() && !value.Empty()) {
    lines = value.Size() + 1;
    for (const auto& item : value.GetArray()) {
      lines += CountJsonLines(item);
    }
  } else if (value.IsObject() && !value.ObjectEmpty()) {
    lines = value.MemberCount() + 1;
    for (const auto& member : value.GetObject()) {
      lines += CountJsonLines(member.value);
    }
  }
  return lines;
}

std::string JsonEscapeString(const std::string& input) {
  rapidjson::Document doc;
  rapidjson::Value v;
  v.SetString(input.c_str(), doc.GetAllocator());
  return JsonToCompactString(v);
}

// A small append-only builder avoids pulling the locale-heavy iostream
// formatting machinery into production recorder binaries.
class ScriptBuilder {
 public:
  ScriptBuilder& operator<<(const char* value) {
    value_.append(value);
    return *this;
  }

  ScriptBuilder& operator<<(const std::string& value) {
    value_.append(value);
    return *this;
  }

  ScriptBuilder& operator<<(int value) {
    value_.append(std::to_string(value));
    return *this;
  }

  ScriptBuilder& operator<<(int64_t value) {
    value_.append(std::to_string(value));
    return *this;
  }

  const std::string& str() const { return value_; }

 private:
  std::string value_;
};

// Percent-encodes any byte that is unsafe inside a single zip path component so
// a recorded (module, method) name can never inject a path separator or escape
// the assets/ directory (zip-slip). The unreserved set [A-Za-z0-9._-] is kept
// verbatim; everything else (including '/', '\\' and NUL) becomes %XX. The
// whole-component traversal names "." and ".." are additionally encoded so they
// cannot resolve to the current/parent directory.
std::string SanitizePathComponent(const std::string& input) {
  static const char kHex[] = "0123456789ABCDEF";
  std::string out;
  out.reserve(input.size());
  for (unsigned char c : input) {
    const bool unreserved = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                            (c >= '0' && c <= '9') || c == '_' || c == '-' ||
                            c == '.';
    if (unreserved) {
      out.push_back(static_cast<char>(c));
    } else {
      out.push_back('%');
      out.push_back(kHex[c >> 4]);
      out.push_back(kHex[c & 0x0F]);
    }
  }
  if (out == "." || out == "..") {
    std::string encoded;
    for (size_t i = 0; i < out.size(); ++i) {
      encoded += "%2E";
    }
    return encoded;
  }
  return out;
}

// ---- base64 ----

size_t Base64DecodedSize(const std::string& input) {
  // The decoder uses strict padding: a nonempty input has complete groups of
  // four characters, with at most two trailing padding characters.
  if (input.empty() || input.size() % 4 != 0) {
    return 0;
  }
  size_t size = input.size() / 4 * 3;
  if (input.back() == '=') {
    --size;
    if (input[input.size() - 2] == '=') {
      --size;
    }
  }
  return size;
}

std::string Base64Decode(const std::string& input, size_t max_decoded_bytes) {
  const size_t decoded_size = Base64DecodedSize(input);
  if (decoded_size == 0 || decoded_size > max_decoded_bytes) {
    return "";
  }
  std::string out(decoded_size, '\0');
  size_t decoded_len =
      lynx_modp_b64_decode(&out[0], input.c_str(), input.length());
  // Defensive: a decode failure must never turn into a (size_t)-1 resize.
  if (decoded_len == MODP_B64_ERROR) {
    return "";
  }
  out.resize(decoded_len);
  return out;
}

// ---- minimal zip writer (deflate + central directory) ----

bool DeflateRaw(const std::string& in, std::string* out) {
  if (in.empty()) {
    out->clear();
    return true;
  }
  z_stream strm = {};
  if (deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8,
                   Z_DEFAULT_STRATEGY) != Z_OK) {
    return false;
  }
  strm.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(in.data()));
  strm.avail_in = static_cast<uInt>(in.size());
  std::string compressed;
  char buf[16384];
  int ret = Z_OK;
  do {
    strm.next_out = reinterpret_cast<Bytef*>(buf);
    strm.avail_out = sizeof(buf);
    ret = deflate(&strm, Z_FINISH);
    compressed.append(buf, sizeof(buf) - strm.avail_out);
  } while (ret == Z_OK || ret == Z_BUF_ERROR);
  deflateEnd(&strm);
  if (ret != Z_STREAM_END) {
    return false;
  }
  *out = std::move(compressed);
  return true;
}

void AppendUint16(std::string* buffer, uint32_t value) {
  buffer->push_back(static_cast<char>(value & 0xFF));
  buffer->push_back(static_cast<char>((value >> 8) & 0xFF));
}

void AppendUint32(std::string* buffer, uint32_t value) {
  buffer->push_back(static_cast<char>(value & 0xFF));
  buffer->push_back(static_cast<char>((value >> 8) & 0xFF));
  buffer->push_back(static_cast<char>((value >> 16) & 0xFF));
  buffer->push_back(static_cast<char>((value >> 24) & 0xFF));
}

uint32_t ToUint32(size_t value) { return static_cast<uint32_t>(value); }

// The traditional (non-Zip64) central directory records the entry count in a
// 16-bit field, so a fixture must never hold more than 65535 entries or the
// EOCD would wrap and the archive become unreadable. We do not emit Zip64.
constexpr size_t kMaxZipEntries = 0xFFFF;

bool WriteString(std::FILE* file, const std::string& value) {
  return value.empty() ||
         std::fwrite(value.data(), 1, value.size(), file) == value.size();
}

class ZipWriter {
 public:
  size_t RemainingBytes() const {
    return kMaxFixtureTotalBytes - total_uncompressed_;
  }

  // Adds an entry (a same-name entry keeps the latest content); refuses and
  // returns false when total uncompressed content would exceed
  // kMaxFixtureTotalBytes (OOM guard) or when a new entry would push the
  // entry count past the 16-bit zip limit.
  bool AddFile(const std::string& name, const std::string& content,
               bool force_store = false) {
    size_t replaced_size = 0;
    Entry* target = nullptr;
    for (auto& existing : entries_) {
      if (existing.name == name) {
        replaced_size = existing.uncompressed_size;
        target = &existing;
        break;
      }
    }
    if (target == nullptr && entries_.size() >= kMaxZipEntries) {
      over_limit_ = true;
      return false;
    }
    const size_t new_total =
        total_uncompressed_ - replaced_size + content.size();
    if (new_total > kMaxFixtureTotalBytes) {
      over_limit_ = true;
      return false;
    }
    if (target == nullptr) {
      target = &entries_.emplace_back();
    }
    BuildEntry(name, content, force_store, target);
    total_uncompressed_ = new_total;
    return true;
  }

  bool WriteToFile(const std::string& path) {
    if (over_limit_) {
      return false;
    }
    std::FILE* file = std::fopen(path.c_str(), "wb");
    if (file == nullptr) {
      return false;
    }
    bool ok = true;

    // Stream entries instead of building the whole zip in memory: peak
    // memory stays proportional to the compressed entries, not the zip size.
    uint32_t offset = 0;
    for (auto& entry : entries_) {
      entry.local_header_offset = offset;
      std::string header;
      AppendUint32(&header, 0x04034b50);  // local file header signature
      AppendUint16(&header, 20);          // version needed
      AppendUint16(&header, 0);           // flags
      AppendUint16(&header, entry.method);
      header.append(4, '\0');  // mod time and date
      AppendEntryMeta(&header, entry);
      header.append(entry.name);
      ok = ok && WriteString(file, header);
      ok = ok && WriteString(file, entry.data);
      offset += 30 + entry.name.size() + entry.compressed_size;
    }

    // central directory
    const uint32_t central_dir_offset = offset;
    for (const auto& entry : entries_) {
      std::string cd;
      AppendUint32(&cd, 0x02014b50);  // central directory signature
      AppendUint16(&cd, 20);          // version made by
      AppendUint16(&cd, 20);          // version needed
      AppendUint16(&cd, 0);           // flags
      AppendUint16(&cd, entry.method);
      cd.append(4, '\0');  // mod time and date
      AppendEntryMeta(&cd, entry);
      cd.append(6, '\0');    // comment len, disk number and internal attrs
      AppendUint32(&cd, 0);  // external attrs
      AppendUint32(&cd, entry.local_header_offset);
      cd.append(entry.name);
      ok = ok && WriteString(file, cd);
      offset += cd.size();
    }

    // EOCD
    const uint32_t central_dir_size = offset - central_dir_offset;
    std::string end_of_central_directory;
    AppendUint32(&end_of_central_directory, 0x06054b50);
    end_of_central_directory.append(4, '\0');  // disk numbers
    AppendUint16(&end_of_central_directory, ToUint32(entries_.size()));
    AppendUint16(&end_of_central_directory, ToUint32(entries_.size()));
    AppendUint32(&end_of_central_directory, central_dir_size);
    AppendUint32(&end_of_central_directory, central_dir_offset);
    AppendUint16(&end_of_central_directory, 0);  // comment len
    ok = ok && WriteString(file, end_of_central_directory);
    ok = ok && std::fflush(file) == 0;
    if (std::fclose(file) != 0) {
      ok = false;
    }
    if (!ok) {
      // Never leave a partial zip behind (e.g. disk full mid-write).
      std::remove(path.c_str());
    }
    return ok;
  }

 private:
  struct Entry {
    std::string name;
    uint32_t crc = 0;
    uint32_t method = 0;
    uint32_t compressed_size = 0;
    uint32_t uncompressed_size = 0;
    uint32_t local_header_offset = 0;
    std::string data;
  };

  // crc, sizes and name length shared by the local header and the central
  // directory entry.
  static void AppendEntryMeta(std::string* out, const Entry& entry) {
    AppendUint32(out, entry.crc);
    AppendUint32(out, entry.compressed_size);
    AppendUint32(out, entry.uncompressed_size);
    AppendUint16(out, ToUint32(entry.name.size()));
    AppendUint16(out, 0);  // extra len
  }

  static void BuildEntry(const std::string& name, const std::string& content,
                         bool force_store, Entry* entry) {
    entry->name = name;
    entry->uncompressed_size = ToUint32(content.size());
    uLong crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, reinterpret_cast<const Bytef*>(content.data()),
                static_cast<uInt>(content.size()));
    entry->crc = static_cast<uint32_t>(crc);
    if (force_store || content.empty()) {
      entry->method = 0;
      entry->data = content;
    } else if (DeflateRaw(content, &entry->data) &&
               entry->data.size() < content.size()) {
      entry->method = 8;
    } else {
      entry->method = 0;
      entry->data = content;
    }
    entry->compressed_size = ToUint32(entry->data.size());
  }

  std::vector<Entry> entries_;
  size_t total_uncompressed_ = 0;
  bool over_limit_ = false;
};

// ---- fixture.js generation ----

constexpr const char* kMatcherHelperPrefix =
    R"JS(  // --- NativeModule mock matching (mirrors V1 strict matcher) ---
  var __ignoredKeys = [)JS";

// This helper defines the replay matching contract. Keep the implementation
// readable so changes can be reviewed against the JSON recorder behavior.
constexpr const char* kMatcherHelperSuffix = R"JS(];
  function __isIgnored(key) {
    return __ignoredKeys.indexOf(key) !== -1;
  }
  function __hasOwn(object, key) {
    return Object.prototype.hasOwnProperty.call(object, key);
  }
  function __sameUrl(actual, recorded) {
    if (actual.indexOf("http") !== 0 || recorded.indexOf("http") !== 0) {
      return false;
    }
    var actualParts = actual.split("?");
    var recordedParts = recorded.split("?");
    if (actualParts.length !== recordedParts.length ||
        actualParts[0] !== recordedParts[0]) {
      return false;
    }
    if (actualParts.length < 2) return true;
    var actualParams = actualParts[1].split("&");
    var recordedParams = recordedParts[1].split("&");
    if (actualParams.length !== recordedParams.length) return false;
    for (var i = 0; i < actualParams.length; i++) {
      if (actualParams[i] === recordedParams[i]) continue;
      var actualParam = actualParams[i].split("=");
      var recordedParam = recordedParams[i].split("=");
      if (actualParam[0] === recordedParam[0] &&
          __isIgnored(actualParam[0])) {
        continue;
      }
      return false;
    }
    return true;
  }
  function __match(actual, recorded) {
    if (recorded === "function" && typeof actual === "function") return true;
    if (recorded === "undefined" && actual === undefined) return true;
    if (recorded === "NaN" && typeof actual === "number" && actual !== actual) {
      return true;
    }
    if (typeof recorded === "string") {
      return typeof actual === "string" &&
             (actual === recorded || __sameUrl(actual, recorded));
    }
    if (typeof recorded === "number") {
      return typeof actual === "number" && Math.abs(actual - recorded) < 1e-7;
    }
    if (typeof recorded === "boolean") return actual === recorded;
    if (recorded === null) return actual === null;
    if (Array.isArray(recorded)) {
      if (!Array.isArray(actual) || actual.length !== recorded.length) {
        return false;
      }
      for (var i = 0; i < recorded.length; i++) {
        if (!__match(actual[i], recorded[i])) return false;
      }
      return true;
    }
    if (typeof recorded === "object") {
      if (actual === null || typeof actual !== "object" ||
          Array.isArray(actual)) {
        return false;
      }
      for (var key in actual) {
        if (!__hasOwn(actual, key)) continue;
        if (__isIgnored(key) && __hasOwn(recorded, key)) continue;
        if (!__hasOwn(recorded, key) ||
            !__match(actual[key], recorded[key])) {
          return false;
        }
      }
      return true;
    }
    return false;
  }
  function __matchArgs(actualArgs, recordedArgs) {
    if (!recordedArgs || actualArgs.length !== recordedArgs.length) {
      return false;
    }
    for (var i = 0; i < recordedArgs.length; i++) {
      if (!__match(actualArgs[i], recordedArgs[i])) return false;
    }
    return true;
  }
  function __dispatch(recordedCalls, actualArgs, callbacks) {
    for (var i = 0; i < recordedCalls.length; i++) {
      if (!__matchArgs(actualArgs, recordedCalls[i].args)) continue;
      var entry = recordedCalls[i];
      var recordedCallbacks = entry.callbacks || [];
      for (var j = 0; j < recordedCallbacks.length; j++) {
        var callback = callbacks[recordedCallbacks[j].index];
        if (callback) {
          callback(recordedCallbacks[j].value,
                   recordedCallbacks[j].delay || 0);
        }
      }
      return entry.returnValue;
    }
    return undefined;
  }
)JS";

bool IsImmediateLifecycleAction(const std::string& fn) {
  return fn == "setThreadStrategy" || fn == "updateViewPort" ||
         fn == "setGlobalProps" || fn == "loadTemplate" ||
         fn == "loadTemplateBundle";
}

// Actions replayed as timed ctx method calls; anything else falls back to
// ctx.dispatch. Recorded names map to the ctx method by decapitalizing the
// first letter (SendCustomEvent -> sendCustomEvent).
bool IsTimedEventAction(const std::string& fn) {
  return fn == "sendGlobalEvent" || fn == "SendCustomEvent" ||
         fn == "SendTouchEvent" || fn == "sendEventAndroid" ||
         fn == "updateViewPort" || fn == "reloadTemplate";
}

// Unwraps {"global_props": {...}}; malformed input becomes an empty object so
// callers never emit a paren-less ctx.setGlobalProps() with no argument.
std::string ExtractGlobalPropsInner(const std::string& params_json) {
  rapidjson::Document doc;
  doc.Parse(params_json.c_str());
  if (doc.HasParseError() || !doc.IsObject()) {
    return "{}";
  }
  if (doc.HasMember("global_props") && doc["global_props"].IsObject()) {
    return JsonToCompactString(doc["global_props"]);
  }
  return params_json;
}

// Only explicitly identified legacy envelopes are flattened. Plain business
// data may contain any of the legacy envelope's field names.
std::string NormalizeTemplateDataJson(const std::string& template_data_json,
                                      FixtureTemplateDataFormat format) {
  rapidjson::Document doc;
  doc.Parse(template_data_json.c_str());
  if (doc.HasParseError() || !doc.IsObject()) {
    return "{}";
  }
  if (format == FixtureTemplateDataFormat::kPlain || !doc.HasMember("value") ||
      !doc["value"].IsObject()) {
    return template_data_json;
  }
  rapidjson::Value flat(rapidjson::kObjectType);
  for (auto it = doc["value"].MemberBegin(); it != doc["value"].MemberEnd();
       ++it) {
    flat.AddMember(it->name, it->value, doc.GetAllocator());
  }
  for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it) {
    if (strcmp(it->name.GetString(), "value") != 0) {
      flat.AddMember(it->name, it->value, doc.GetAllocator());
    }
  }
  return JsonToCompactString(flat);
}

// Builds the recorded-call asset JSON for a single (module, method):
// [{"args": [...], "returnValue": ..., "callbacks":
// [{"index":0,"value":...,"delay":...}]}]
std::string BuildModuleAssetJson(const std::vector<FixtureCall>& calls) {
  ScriptBuilder out;
  out << "[";
  bool first_call = true;
  for (const auto& call : calls) {
    out << (first_call ? "{\"args\":" : ",{\"args\":")
        << CompactJsonOrFallback(call.args_json, "[]");
    first_call = false;
    if (!call.return_value_json.empty()) {
      out << ",\"returnValue\":"
          << CompactJsonOrFallback(call.return_value_json, "null");
    }
    if (!call.callbacks.empty()) {
      out << ",\"callbacks\":[";
      bool first_callback = true;
      for (const auto& cb : call.callbacks) {
        out << (first_callback ? "{\"index\":" : ",{\"index\":") << cb.index
            << ",\"value\":" << CompactJsonOrFallback(cb.value_json, "null");
        first_callback = false;
        if (cb.delay_ms > 0) {
          out << ",\"delay\":" << cb.delay_ms;
        }
        out << "}";
      }
      out << "]";
    }
    out << "}";
  }
  out << "]";
  return out.str();
}

void AppendHandlerLine(ScriptBuilder* out, int index, const std::string& module,
                       const std::string& method,
                       const std::string& asset_path) {
  // readAsset must reference the sanitized on-disk asset path, while register
  // keeps the raw (module, method) so replay dispatch still matches the
  // originally recorded bridge call identity.
  *out << "  var __rec_" << index << " = ctx.readAsset("
       << JsonEscapeString(asset_path) << ");\n"
       << "  ctx.register(" << JsonEscapeString(module) << ", "
       << JsonEscapeString(method) << ", function(args, callbacks) {\n";
  *out << "    return __dispatch(__rec_" << index
       << ", args, callbacks);\n  });\n\n";
}

}  // namespace

bool WriteFixtureZip(const std::string& zip_path, const FixtureData& data) {
  if (data.actions.empty()) {
    return false;
  }
  // Reject oversized templates before parsing actions, generating fixture.js,
  // or allocating a decoded buffer. The remaining zip budget is checked again
  // immediately before decoding below.
  if (data.has_load_template && !data.load_template_source_base64.empty()) {
    const size_t template_size =
        Base64DecodedSize(data.load_template_source_base64);
    if (template_size == 0 || template_size > kMaxFixtureTotalBytes) {
      return false;
    }
  }

  ZipWriter zip;
  ScriptBuilder fixture;
  fixture << "export default function(ctx) {\n";

  // Immediate lifecycle actions (no delay). Use the smallest record_ms as the
  // zero point rather than actions.front(): if the stream is ever not strictly
  // time-sorted, a front()-based origin would make ctx.after() delays negative
  // and scramble the immediate/timed split.
  int64_t start_time = data.actions.front().record_ms;
  for (const auto& action : data.actions) {
    if (action.record_ms < start_time) {
      start_time = action.record_ms;
    }
  }
  int global_props_seq = 0;
  for (const auto& action : data.actions) {
    const std::string& fn = action.function_name;
    if (!IsImmediateLifecycleAction(fn)) {
      continue;  // emitted as timed events below
    }
    const int64_t delay = action.record_ms - start_time;
    if (delay >= 100) {
      continue;  // emitted as timed events below
    }
    if (fn == "setThreadStrategy" || fn == "updateViewPort") {
      fixture << "  ctx." << fn << "(" << action.params_json << ");\n";
    } else if (fn == "setGlobalProps") {
      const std::string inner = ExtractGlobalPropsInner(action.params_json);
      rapidjson::Document inner_doc;
      inner_doc.Parse(inner.c_str());
      if (!inner_doc.HasParseError() && CountJsonLines(inner_doc) > 50) {
        // Each extracted call gets a unique asset name; a shared name would
        // let a later setGlobalProps overwrite an earlier one's data.
        const std::string asset_name = "lifecycle/global_props_" +
                                       std::to_string(global_props_seq++) +
                                       ".json";
        if (!zip.AddFile("assets/" + asset_name,
                         JsonToCompactString(inner_doc))) {
          return false;
        }
        fixture << "  ctx.setGlobalProps(ctx.readAsset("
                << JsonEscapeString(asset_name) << "));\n";
      } else {
        fixture << "  ctx.setGlobalProps(" << inner << ");\n";
      }
    } else if (fn == "loadTemplate" || fn == "loadTemplateBundle") {
      // Both actions normalize to ctx.loadTemplate on purpose: the fixture ctx
      // runtime only exposes loadTemplate, and the replay side already decides
      // bundle vs. binary from the config (enablePreDecode) rather than the
      // recorded action name. This mirrors tools/testbench/json2fixture.js.
      rapidjson::Document params_doc;
      params_doc.Parse(action.params_json.c_str());
      std::string url = data.load_template_url;
      if (!params_doc.HasParseError() && params_doc.HasMember("url") &&
          params_doc["url"].IsString()) {
        url = params_doc["url"].GetString();
      }
      fixture << "  ctx.loadTemplate(" << JsonEscapeString(url);
      if (data.has_load_template && !data.load_template_source_base64.empty()) {
        fixture << ", " << JsonEscapeString("template/template.bin");
        // The replay side requires the templateData third argument
        // (omitting it loses initial data and can crash the page); fall back
        // to an empty object when the recording carries no templateData.
        if (!data.load_template_data_json.empty() &&
            data.load_template_data_json != "{}") {
          fixture << ", ctx.readAsset("
                  << JsonEscapeString("lifecycle/template_data.json") << ")";
        } else {
          fixture << ", {}";
        }
      }
      fixture << ");\n";
    }
  }
  fixture << "\n";

  // Timed events (delay relative to the first action).
  bool has_timed = false;
  int delayed_seq = 0;
  for (const auto& action : data.actions) {
    const std::string& fn = action.function_name;
    if (IsImmediateLifecycleAction(fn) && action.record_ms - start_time < 100) {
      continue;  // already emitted above
    }
    const int64_t delay = action.record_ms - start_time;
    rapidjson::Document params_doc;
    params_doc.Parse(action.params_json.c_str());
    if (!has_timed) {
      fixture << "  // --- Timed events ---\n";
      has_timed = true;
    }
    // Large event payloads (>30 pretty-printed lines) go to assets/.
    // setGlobalProps is handled separately below: it must extract its
    // unwrapped inner value (not the {"global_props":...} envelope), so it
    // does not use this generic events-asset path.
    const bool as_asset = fn != "setGlobalProps" &&
                          !params_doc.HasParseError() &&
                          CountJsonLines(params_doc) > 30;
    std::string params_ref = action.params_json;
    if (as_asset) {
      std::string asset_name =
          (fn == "SendCustomEvent" ? "custom_" : "global_") +
          std::to_string(delayed_seq) + ".json";
      if (!zip.AddFile("assets/events/" + asset_name,
                       JsonToCompactString(params_doc))) {
        return false;
      }
      params_ref =
          "ctx.readAsset(" + JsonEscapeString("events/" + asset_name) + ")";
    }
    if (fn == "setGlobalProps") {
      const std::string inner = ExtractGlobalPropsInner(action.params_json);
      rapidjson::Document inner_doc;
      inner_doc.Parse(inner.c_str());
      std::string inner_ref = inner;
      if (!inner_doc.HasParseError() && CountJsonLines(inner_doc) > 30) {
        const std::string asset_name = "events/global_props_" +
                                       std::to_string(global_props_seq++) +
                                       ".json";
        if (!zip.AddFile("assets/" + asset_name,
                         JsonToCompactString(inner_doc))) {
          return false;
        }
        inner_ref = "ctx.readAsset(" + JsonEscapeString(asset_name) + ")";
      }
      fixture << "  ctx.after(" << delay << ", () => ctx.setGlobalProps("
              << inner_ref << "));\n";
    } else if (IsTimedEventAction(fn)) {
      std::string method = fn;
      if (method[0] >= 'A' && method[0] <= 'Z') {
        method[0] = static_cast<char>(method[0] - 'A' + 'a');
      }
      fixture << "  ctx.after(" << delay << ", () => ctx." << method << "("
              << params_ref << "));\n";
    } else {
      fixture << "  ctx.after(" << delay << ", () => ctx.dispatch("
              << JsonEscapeString(fn) << ", " << params_ref << "));\n";
    }
    delayed_seq++;
  }
  if (has_timed) {
    fixture << "\n";
  }

  // Shared data.
  for (const auto& pair : data.shared_data) {
    fixture << "  ctx.sharedData(" << JsonEscapeString(pair.first) << ", "
            << pair.second << ");\n";
  }
  if (!data.shared_data.empty()) {
    fixture << "\n";
  }

  // NativeModule mock matcher.
  fixture << kMatcherHelperPrefix;
  bool first_key = true;
  for (const char* key : kDefaultIgnoredKeys) {
    fixture << (first_key ? "\"" : ",\"") << key << "\"";
    first_key = false;
  }
  rapidjson::Document config_doc;
  config_doc.Parse(data.config_json.c_str());
  if (!config_doc.HasParseError() && config_doc.IsObject() &&
      config_doc.HasMember("jsbIgnoredInfo") &&
      config_doc["jsbIgnoredInfo"].IsArray()) {
    for (auto& item : config_doc["jsbIgnoredInfo"].GetArray()) {
      if (item.IsString()) {
        fixture << "," << JsonEscapeString(item.GetString());
      }
    }
  }
  fixture << kMatcherHelperSuffix;
  fixture << "\n";

  // Mock handlers.
  // Recording order can differ across threads. Select the next group by key so
  // equal recordings produce byte-identical archives. Group counts are small;
  // the quadratic scan avoids a temporary vector and sorting machinery.
  size_t previous_index = 0;
  bool has_previous = false;
  int handler_index = 0;
  for (size_t emitted = 0; emitted < data.call_groups.size(); ++emitted) {
    size_t candidate = data.call_groups.size();
    for (size_t i = 0; i < data.call_groups.size(); ++i) {
      const auto& group = data.call_groups[i];
      if (has_previous) {
        const auto& previous = data.call_groups[previous_index];
        const bool after_previous =
            group.module_name > previous.module_name ||
            (group.module_name == previous.module_name &&
             (group.method_name > previous.method_name ||
              (group.method_name == previous.method_name &&
               i > previous_index)));
        if (!after_previous) {
          continue;
        }
      }
      if (candidate == data.call_groups.size()) {
        candidate = i;
        continue;
      }
      const auto& selected = data.call_groups[candidate];
      if (group.module_name < selected.module_name ||
          (group.module_name == selected.module_name &&
           (group.method_name < selected.method_name ||
            (group.method_name == selected.method_name && i < candidate)))) {
        candidate = i;
      }
    }
    if (candidate == data.call_groups.size()) {
      return false;
    }
    const FixtureCallGroup* group = &data.call_groups[candidate];
    const std::string& module = group->module_name;
    const std::string& method = group->method_name;
    // module/method originate from recorded bridge call names; sanitize each
    // path component so they cannot inject a separator or traverse out of
    // assets/ (zip-slip). readAsset below references this same sanitized path.
    const std::string asset_path = SanitizePathComponent(module) + "/" +
                                   SanitizePathComponent(method) + ".json";
    if (!zip.AddFile("assets/" + asset_path,
                     BuildModuleAssetJson(group->calls))) {
      // A cap was hit: WriteToFile will refuse to emit anyway, so stop the
      // per-call asset work rather than burning CPU on the EndRecord hot path.
      return false;
    }
    AppendHandlerLine(&fixture, handler_index, module, method, asset_path);
    handler_index++;
    previous_index = candidate;
    has_previous = true;
  }

  fixture << "}\n";

  // ---- zip contents ----
  if (!zip.AddFile("fixture.js", fixture.str())) {
    return false;
  }
  if (!data.config_json.empty() &&
      !zip.AddFile("config.json", ParseThenCompact(data.config_json))) {
    return false;
  }
  if (!data.components.empty()) {
    ScriptBuilder components;
    components << "[";
    bool first_component = true;
    for (const auto& comp : data.components) {
      components << (first_component ? "{\"Name\":" : ",{\"Name\":")
                 << JsonEscapeString(comp.first) << ",\"Type\":" << comp.second
                 << "}";
      first_component = false;
    }
    components << "]";
    if (!zip.AddFile("component_list.json", components.str())) {
      return false;
    }
  }
  if (data.has_load_template && !data.load_template_source_base64.empty()) {
    // fixture.js references template.bin, so a decode failure must fail the
    // whole write instead of shipping an empty (but seemingly valid) binary
    // that would only surface as a crash/black screen on the replay side.
    const std::string template_bin =
        Base64Decode(data.load_template_source_base64, zip.RemainingBytes());
    if (template_bin.empty()) {
      return false;
    }
    if (!zip.AddFile("assets/template/template.bin", template_bin,
                     /*force_store=*/true)) {
      return false;
    }
  }
  if (!data.load_template_data_json.empty() &&
      data.load_template_data_json != "{}") {
    if (!zip.AddFile("assets/lifecycle/template_data.json",
                     ParseThenCompact(NormalizeTemplateDataJson(
                         data.load_template_data_json,
                         data.load_template_data_format)))) {
      return false;
    }
  }

  return zip.WriteToFile(zip_path);
}

}  // namespace recorder
}  // namespace tasm
}  // namespace lynx
