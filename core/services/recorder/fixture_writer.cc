// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/recorder/fixture_writer.h"

#include <zlib.h>

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <sstream>

#include "third_party/modp_b64/modp_b64.h"
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/prettywriter.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

namespace lynx {
namespace tasm {
namespace recorder {

namespace {

constexpr const char* kDefaultIgnoredKeys[] = {
    "timestamp", "card_version", "containerID", "header", "request_time"};

}  // namespace

std::string JsonToCompactString(const rapidjson::Value& value) {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  value.Accept(writer);
  return buffer.GetString();
}

namespace {

// ---- JSON helpers ----

std::string JsonToPrettyString(rapidjson::Value& value) {
  rapidjson::StringBuffer buffer;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
  value.Accept(writer);
  return buffer.GetString();
}

std::string ParseThenPretty(const std::string& json) {
  rapidjson::Document doc;
  doc.Parse(json.c_str());
  if (doc.HasParseError()) {
    return json;
  }
  return JsonToPrettyString(doc);
}

// Approximates json2fixture.js's countLines(JSON.stringify(v, null, 2))
size_t CountJsonLines(rapidjson::Value& value) {
  const std::string pretty = JsonToPrettyString(value);
  return static_cast<size_t>(std::count(pretty.begin(), pretty.end(), '\n'));
}

rapidjson::Value ParseJson(
    const std::string& json, rapidjson::Document::AllocatorType& allocator,
    rapidjson::Type fallback_type = rapidjson::kNullType) {
  rapidjson::Document parsed;
  parsed.Parse(json.c_str());
  rapidjson::Value value(fallback_type);
  if (!parsed.HasParseError()) {
    value.CopyFrom(parsed, allocator);
  }
  return value;
}

std::string JsonEscapeString(const std::string& input) {
  rapidjson::Document doc;
  rapidjson::Value v;
  v.SetString(input.c_str(), doc.GetAllocator());
  return JsonToCompactString(v);
}

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

std::string Base64Decode(const std::string& input) {
  if (input.empty()) {
    return "";
  }
  size_t decoded_capacity = lynx_modp_b64_decode_len(input.length());
  std::string out(decoded_capacity, '\0');
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

class ZipWriter {
 public:
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
    std::ofstream ofs(path, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!ofs.is_open()) {
      return false;
    }

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
      ofs.write(header.data(), header.size());
      ofs.write(entry.data.data(), entry.data.size());
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
      ofs.write(cd.data(), cd.size());
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
    ofs.write(end_of_central_directory.data(), end_of_central_directory.size());

    ofs.flush();
    const bool ok = ofs.good();
    ofs.close();
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

constexpr const char* kMatcherHelperSuffix = R"JS(];
  function __isIgnored(k) { return __ignoredKeys.indexOf(k) !== -1; }
  function __sameUrl(a, b) {
    if (a.indexOf("http") !== 0 || b.indexOf("http") !== 0) return false;
    var pa = a.split("?"), pb = b.split("?");
    if (pa.length !== pb.length || pa[0] !== pb[0]) return false;
    if (pa.length < 2) return true;
    var la = pa[1].split("&"), lb = pb[1].split("&");
    if (la.length !== lb.length) return false;
    for (var i = 0; i < la.length; i++) {
      if (la[i] === lb[i]) continue;
      var ka = la[i].split("="), kb = lb[i].split("=");
      if (ka[0] === kb[0] && __isIgnored(ka[0])) continue;
      return false;
    }
    return true;
  }
  function __match(jsVal, recVal) {
    if (recVal === "function") return true;
    if (recVal === "undefined") return jsVal === undefined || jsVal === null;
    if (typeof recVal === "string") {
      if (typeof jsVal !== "string") return false;
      return jsVal === recVal || __sameUrl(recVal, jsVal);
    }
    if (typeof recVal === "number") return typeof jsVal === "number" && Math.abs(jsVal - recVal) < 1e-7;
    if (typeof recVal === "boolean") return jsVal === recVal;
    if (recVal === null) return jsVal === null;
    if (Array.isArray(recVal)) {
      if (!Array.isArray(jsVal) || jsVal.length !== recVal.length) return false;
      for (var i = 0; i < recVal.length; i++) if (!__match(jsVal[i], recVal[i])) return false;
      return true;
    }
    if (typeof recVal === "object") {
      if (jsVal === null || typeof jsVal !== "object" || Array.isArray(jsVal)) return false;
      for (var k in jsVal) {
        if (!jsVal.hasOwnProperty(k)) continue;
        if (__isIgnored(k) && recVal.hasOwnProperty(k)) continue;
        if (!recVal.hasOwnProperty(k)) return false;
        if (!__match(jsVal[k], recVal[k])) return false;
      }
      return true;
    }
    return false;
  }
  function __matchArgs(args, recArgs) {
    if (!recArgs || args.length !== recArgs.length) return false;
    for (var i = 0; i < recArgs.length; i++) if (!__match(args[i], recArgs[i])) return false;
    return true;
  }
  function __dispatch(recorded, args, callbacks) {
    for (var i = 0; i < recorded.length; i++) {
      if (!__matchArgs(args, recorded[i].args)) continue;
      var entry = recorded[i];
      var cbs = entry.callbacks || [];
      for (var c = 0; c < cbs.length; c++) {
        var cb = callbacks[cbs[c].index];
        if (cb) cb(cbs[c].value, cbs[c].delay || 0);
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

// The recorded templateData may be wrapped as {"value": {...},
// "preprocessorName": ..., "readOnly": ...} (legacy) or already flat (new).
// Flatten it uniformly.
std::string FlattenTemplateDataJson(const std::string& template_data_json) {
  rapidjson::Document doc;
  doc.Parse(template_data_json.c_str());
  if (doc.HasParseError() || !doc.IsObject()) {
    return "{}";
  }
  if (!doc.HasMember("value") || !doc["value"].IsObject()) {
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
  rapidjson::Document doc(rapidjson::kArrayType);
  for (const auto& call : calls) {
    rapidjson::Value entry(rapidjson::kObjectType);
    entry.AddMember(
        "args",
        ParseJson(call.args_json, doc.GetAllocator(), rapidjson::kArrayType),
        doc.GetAllocator());
    if (!call.return_value_json.empty()) {
      entry.AddMember("returnValue",
                      ParseJson(call.return_value_json, doc.GetAllocator()),
                      doc.GetAllocator());
    }
    if (!call.callbacks.empty()) {
      rapidjson::Value cbs_val(rapidjson::kArrayType);
      for (const auto& cb : call.callbacks) {
        rapidjson::Value cb_entry(rapidjson::kObjectType);
        cb_entry.AddMember("index", cb.index, doc.GetAllocator());
        cb_entry.AddMember("value",
                           ParseJson(cb.value_json, doc.GetAllocator()),
                           doc.GetAllocator());
        if (cb.delay_ms > 0) {
          cb_entry.AddMember("delay", cb.delay_ms, doc.GetAllocator());
        }
        cbs_val.PushBack(cb_entry, doc.GetAllocator());
      }
      entry.AddMember("callbacks", cbs_val, doc.GetAllocator());
    }
    doc.PushBack(entry, doc.GetAllocator());
  }
  return JsonToPrettyString(doc);
}

void AppendHandlerLine(std::ostringstream* out, int index,
                       const std::string& module, const std::string& method,
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

  ZipWriter zip;
  std::ostringstream fixture;
  fixture << "export default function(ctx) {\n";

  // Immediate lifecycle actions (no delay). Use the smallest record_ms as the
  // zero point rather than actions.front(): if the stream is ever not strictly
  // time-sorted, a front()-based origin would make ctx.after() delays negative
  // and scramble the immediate/timed split.
  int64_t start_time = data.actions.front().record_ms;
  for (const auto& action : data.actions) {
    start_time = std::min(start_time, action.record_ms);
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
                         JsonToPrettyString(inner_doc))) {
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
                       JsonToPrettyString(params_doc))) {
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
                         JsonToPrettyString(inner_doc))) {
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
        fixture << ",\"" << item.GetString() << "\"";
      }
    }
  }
  fixture << kMatcherHelperSuffix;
  fixture << "\n";

  // Mock handlers.
  // Iterate in sorted key order: unordered_map iteration order would
  // otherwise make the zip bytes differ across identical recordings.
  std::vector<std::string> call_keys;
  call_keys.reserve(data.calls.size());
  for (const auto& pair : data.calls) {
    call_keys.push_back(pair.first);
  }
  std::sort(call_keys.begin(), call_keys.end());

  int handler_index = 0;
  for (const auto& key : call_keys) {
    const auto calls_it = data.calls.find(key);
    const size_t sep = key.find('\0');
    if (sep == std::string::npos) {
      continue;
    }
    const std::string module = key.substr(0, sep);
    const std::string method = key.substr(sep + 1);
    // module/method originate from recorded bridge call names; sanitize each
    // path component so they cannot inject a separator or traverse out of
    // assets/ (zip-slip). readAsset below references this same sanitized path.
    const std::string asset_path = SanitizePathComponent(module) + "/" +
                                   SanitizePathComponent(method) + ".json";
    if (!zip.AddFile("assets/" + asset_path,
                     BuildModuleAssetJson(calls_it->second))) {
      // A cap was hit: WriteToFile will refuse to emit anyway, so stop the
      // per-call asset work rather than burning CPU on the EndRecord hot path.
      return false;
    }
    AppendHandlerLine(&fixture, handler_index, module, method, asset_path);
    handler_index++;
  }

  fixture << "}\n";

  // ---- zip contents ----
  zip.AddFile("fixture.js", fixture.str());
  if (!data.config_json.empty()) {
    zip.AddFile("config.json", ParseThenPretty(data.config_json));
  }
  if (!data.components.empty()) {
    rapidjson::Document comp_doc(rapidjson::kArrayType);
    for (const auto& comp : data.components) {
      rapidjson::Value item(rapidjson::kObjectType);
      rapidjson::Value name_val;
      name_val.SetString(comp.first.c_str(), comp_doc.GetAllocator());
      item.AddMember("Name", name_val, comp_doc.GetAllocator());
      item.AddMember("Type", comp.second, comp_doc.GetAllocator());
      comp_doc.PushBack(item, comp_doc.GetAllocator());
    }
    zip.AddFile("component_list.json", JsonToPrettyString(comp_doc));
  }
  if (data.has_load_template && !data.load_template_source_base64.empty()) {
    // fixture.js references template.bin, so a decode failure must fail the
    // whole write instead of shipping an empty (but seemingly valid) binary
    // that would only surface as a crash/black screen on the replay side.
    const std::string template_bin =
        Base64Decode(data.load_template_source_base64);
    if (template_bin.empty()) {
      return false;
    }
    zip.AddFile("assets/template/template.bin", template_bin,
                /*force_store=*/true);
  }
  if (!data.load_template_data_json.empty() &&
      data.load_template_data_json != "{}") {
    zip.AddFile(
        "assets/lifecycle/template_data.json",
        ParseThenPretty(FlattenTemplateDataJson(data.load_template_data_json)));
  }

  return zip.WriteToFile(zip_path);
}

}  // namespace recorder
}  // namespace tasm
}  // namespace lynx
