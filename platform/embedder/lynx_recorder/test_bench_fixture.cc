// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// cspell:ignore ZCALLBACK FILEFUNC zread ztell zseek zclose zerror

#include "platform/embedder/lynx_recorder/test_bench_fixture.h"

#include <algorithm>
#include <cstring>
#include <set>
#include <utility>
#include <vector>

#include "clay/fml/mapping.h"
#include "platform/embedder/lynx_recorder/test_bench_utils.h"
#include "platform/embedder/public/capi/lynx_replay_capi.h"
#include "third_party/zlib/contrib/minizip/unzip.h"

namespace lynx {
namespace embedder {
namespace {
constexpr size_t kMaxBytes = 128 * 1024 * 1024;
constexpr size_t kMaxTemplateBytes = 16 * 1024 * 1024;

// Use memory IO so ZIP input is binary-safe and independent of OS path
// encoding.
struct ZipInput {
  const std::string& bytes;
  size_t offset = 0;
};
voidpf ZCALLBACK OpenZip(voidpf opaque, const char*, int) { return opaque; }
uLong ZCALLBACK ReadZip(voidpf, voidpf stream, void* buffer, uLong length) {
  auto* input = static_cast<ZipInput*>(stream);
  size_t count = std::min<size_t>(length, input->bytes.size() - input->offset);
  memcpy(buffer, input->bytes.data() + input->offset, count);
  input->offset += count;
  return static_cast<uLong>(count);
}
long ZCALLBACK TellZip(voidpf, voidpf stream) {
  return static_cast<long>(static_cast<ZipInput*>(stream)->offset);
}
long ZCALLBACK SeekZip(voidpf, voidpf stream, uLong offset, int origin) {
  auto* input = static_cast<ZipInput*>(stream);
  size_t base = origin == ZLIB_FILEFUNC_SEEK_SET   ? 0
                : origin == ZLIB_FILEFUNC_SEEK_CUR ? input->offset
                                                   : input->bytes.size();
  if (offset > input->bytes.size() - base) return -1;
  input->offset = base + offset;
  return 0;
}
int ZCALLBACK CloseZip(voidpf, voidpf) { return 0; }
int ZCALLBACK ErrorZip(voidpf, voidpf) { return 0; }

// Reject OS-dependent aliases too: Windows device names, trailing dots/spaces,
// alternate streams and case aliases must not overwrite another ZIP entry.
bool SplitPath(const std::string& name, std::vector<std::string>* parts) {
  if (name.empty() || name.size() > 4096) return false;
  size_t start = 0;
  while (start < name.size()) {
    size_t end = name.find('/', start);
    auto part = name.substr(start, end - start);
    if (part.empty() || part == "." || part == ".." || part.back() == '.' ||
        part.back() == ' ')
      return false;
    for (unsigned char c : part) {
      if (c < 32 || c == 127 || c == '\\' || c == ':' || c == '*' || c == '?' ||
          c == '"' || c == '<' || c == '>' || c == '|')
        return false;
    }
    auto device = part.substr(0, part.find('.'));
    for (auto& c : device)
      if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
    if (device == "CON" || device == "PRN" || device == "AUX" ||
        device == "NUL" ||
        (device.size() == 4 &&
         (device.substr(0, 3) == "COM" || device.substr(0, 3) == "LPT") &&
         device[3] >= '0' && device[3] <= '9'))
      return false;
    parts->push_back(std::move(part));
    if (end == std::string::npos) return true;
    start = end + 1;
  }
  return false;
}
std::string Read(const ::fml::UniqueFD& root, const std::string& name,
                 size_t max) {
  auto mapping = ::fml::FileMapping::CreateReadOnly(root, name);
  if (!mapping || !mapping->IsValid() || mapping->GetSize() > max) return {};
  if (!mapping->GetSize()) return {};
  return {reinterpret_cast<const char*>(mapping->GetMapping()),
          mapping->GetSize()};
}
}  // namespace

bool TestBenchFixture::IsArchive(const std::string& bytes) {
  return bytes.size() >= 4 && bytes[0] == 'P' && bytes[1] == 'K' &&
         ((bytes[2] == 3 && bytes[3] == 4) || (bytes[2] == 5 && bytes[3] == 6));
}
std::shared_ptr<TestBenchFixture> TestBenchFixture::Open(
    const std::string& bytes, std::string* error) {
  auto fixture = std::make_shared<TestBenchFixture>();
  if (bytes.size() > kMaxBytes || !fixture->directory_.fd().is_valid() ||
      !fixture->Extract(bytes)) {
    *error = "Invalid Fixture archive";
    return nullptr;
  }
  if (!fixture->Evaluate(error)) return nullptr;
  return fixture;
}

bool TestBenchFixture::Extract(const std::string& bytes) {
  ZipInput input{bytes};
  zlib_filefunc_def io{};
  io.zopen_file = OpenZip;
  io.zread_file = ReadZip;
  io.ztell_file = TellZip;
  io.zseek_file = SeekZip;
  io.zclose_file = CloseZip;
  io.zerror_file = ErrorZip;
  io.opaque = &input;
  std::unique_ptr<void, decltype(&unzClose)> zip(unzOpen2("", &io), unzClose);
  if (!zip) return false;
  unz_global_info global{};
  if (unzGetGlobalInfo(zip.get(), &global) != UNZ_OK || !global.number_entry ||
      global.number_entry > 65535)
    return false;
  if (unzGoToFirstFile(zip.get()) != UNZ_OK) return false;
  std::set<std::string> entries;
  size_t total = 0;
  for (uLong index = 0; index < global.number_entry; ++index) {
    unz_file_info info{};
    char filename[4097]{};
    if (unzGetCurrentFileInfo(zip.get(), &info, filename, sizeof(filename),
                              nullptr, 0, nullptr, 0) != UNZ_OK ||
        !info.size_filename || info.size_filename > 4096 ||
        info.uncompressed_size > kMaxBytes - total || (info.flag & 1))
      return false;
    std::string name(filename, info.size_filename);
    bool is_directory = name.back() == '/';
    if (is_directory) name.pop_back();
    std::vector<std::string> parts;
    if (!SplitPath(name, &parts)) return false;
    std::string key = name;
    for (auto& c : key)
      if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
    if (!entries.insert(key).second) return false;
    if (unzOpenCurrentFile(zip.get()) != UNZ_OK) return false;
    std::string data;
    char buffer[8192];
    int count = 0;
    bool valid = true;
    while ((count = unzReadCurrentFile(zip.get(), buffer, sizeof(buffer))) >
           0) {
      if (static_cast<size_t>(count) > kMaxBytes - total || is_directory) {
        valid = false;
        break;
      }
      total += count;
      data.append(buffer, count);
    }
    int closed = unzCloseCurrentFile(zip.get());
    if (!valid || count < 0 || closed != UNZ_OK ||
        data.size() != info.uncompressed_size)
      return false;
    std::string leaf = parts.back();
    if (!is_directory) parts.pop_back();
    auto parent = parts.empty() ? ::fml::Duplicate(directory_.fd().get())
                                : ::fml::CreateDirectory(
                                      directory_.fd(), parts,
                                      ::fml::FilePermission::kReadWrite);
    if (!parent.is_valid()) return false;
    // ZIP links are never materialized: all non-directory entries are files.
    if (!is_directory) {
      if (::fml::FileExists(parent, leaf.c_str())) return false;
      // Windows cannot map a zero-length file in WriteAtomically.
      bool written = data.empty()
                         ? ::fml::OpenFile(parent, leaf.c_str(), true,
                                           ::fml::FilePermission::kReadWrite)
                               .is_valid()
                         : ::fml::WriteAtomically(parent, leaf.c_str(),
                                                  ::fml::DataMapping(data));
      if (!written) return false;
    }
    if (index + 1 < global.number_entry && unzGoToNextFile(zip.get()) != UNZ_OK)
      return false;
  }
  return !Read(directory_.fd(), "fixture.js", 4 * 1024 * 1024).empty();
}

bool TestBenchFixture::Evaluate(std::string* error) {
  std::string result;
  bool success = lynx_replay_evaluate_fixture(
      directory().c_str(), &result,
      [](void* target, const char* data, size_t size) {
        static_cast<std::string*>(target)->assign(data, size);
      });
  if (!success) {
    *error = std::move(result);
    return false;
  }
  rapidjson::Document record;
  record.Parse(result);
  if (record.HasParseError() || !record.IsObject() ||
      !record.HasMember("Action List") || !record["Action List"].IsArray()) {
    *error = "Invalid Fixture evaluation result";
    return false;
  }
  for (const auto& item :
       {std::make_pair("config.json", "Config"),
        std::make_pair("component_list.json", "Component List")}) {
    if (!::fml::FileExists(directory_.fd(), item.first)) continue;
    auto text = Read(directory_.fd(), item.first, kMaxTemplateBytes);
    rapidjson::Document value;
    value.Parse(text);
    if (value.HasParseError() ||
        (std::string(item.first) == "config.json" ? !value.IsObject()
                                                  : !value.IsArray())) {
      *error = "Invalid Fixture metadata";
      return false;
    }
    record.AddMember(rapidjson::Value(item.second, record.GetAllocator()),
                     rapidjson::Value(value, record.GetAllocator()),
                     record.GetAllocator());
  }
  size_t template_bytes = 0;
  bool has_template = false;
  for (const auto& action : record["Action List"].GetArray()) {
    // Match the desktop JSON handlers, including the array form of events.
    if (!action["Params"].IsObject() &&
        !(action["Function Name"] == "sendGlobalEvent" &&
          action["Params"].IsArray())) {
      *error = "Fixture action parameters must be an object";
      return false;
    }
    if (action["Function Name"] != "loadTemplate") continue;
    const auto& params = action["Params"];
    if (!params.IsObject() || !params.HasMember("templateAsset") ||
        !params["templateAsset"].IsString()) {
      *error = "Missing Fixture template";
      return false;
    }
    std::string asset(params["templateAsset"].GetString(),
                      params["templateAsset"].GetStringLength());
    std::string relative =
        asset.compare(0, 7, "assets/") == 0 ? asset.substr(7) : asset;
    std::vector<std::string> parts;
    if (!SplitPath(relative, &parts)) {
      *error = "Invalid template asset path";
      return false;
    }
    if (templates_.find(asset) == templates_.end()) {
      auto data = Read(directory_.fd(), "assets/" + relative,
                       kMaxTemplateBytes - template_bytes);
      if (data.empty()) {
        *error = "Missing or oversized template asset";
        return false;
      }
      template_bytes += data.size();
      templates_.emplace(asset, std::move(data));
    }
    has_template = true;
  }
  if (!has_template) {
    *error = "Fixture has no loadTemplate action";
    return false;
  }
  record_ = ToJson(record);
  return true;
}

const std::string* TestBenchFixture::Template(const std::string& asset) const {
  auto it = templates_.find(asset);
  return it == templates_.end() ? nullptr : &it->second;
}
}  // namespace embedder
}  // namespace lynx
