// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/resource/zip_resource_helper.h"

#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "clay/fml/file.h"
#include "clay/fml/logging.h"

#ifdef OS_WIN
#include <filesystem>

#include "third_party/zlib/zlib.h"
#else
#include <dirent.h>
#include <zlib.h>
#endif

namespace clay {

namespace {

#ifdef OS_WIN
static constexpr const char* kPathSeparator = "\\";
#else
static constexpr const char* kPathSeparator = "/";
#endif

uint32_t ReadUInt32(const char* data) {
  return static_cast<uint32_t>((static_cast<unsigned char>(data[0])) |
                               (static_cast<unsigned char>(data[1]) << 8) |
                               (static_cast<unsigned char>(data[2]) << 16) |
                               (static_cast<unsigned char>(data[3]) << 24));
}

uint16_t ReadUInt16(const char* data) {
  return static_cast<uint16_t>((static_cast<unsigned char>(data[0])) |
                               (static_cast<unsigned char>(data[1]) << 8));
}

bool CreateDirectories(const std::string& path) {
  if (path.empty()) {
    return false;
  }
  {
    fml::UniqueFD dir =
        OpenDirectory(path.c_str(), false, fml::FilePermission::kRead);
    if (dir.is_valid()) {
      return fml::IsDirectory(dir);
    }
  }
  if (fml::IsFile(path)) {
    FML_LOG(ERROR) << "Path exists but is not a directory: " << path;
    return false;
  }
  size_t pos = path.find_last_of('/');
#ifdef OS_WIN
  if (pos == std::string::npos) {
    pos = path.find_last_of('\\');
  }
#endif
  if (pos != std::string::npos) {
    std::string parent = path.substr(0, pos);
    if (!parent.empty()) {
      if (!CreateDirectories(parent)) {
        return false;
      }
    }
  }
  fml::UniqueFD dir =
      OpenDirectory(path.c_str(), true, fml::FilePermission::kReadWrite);
  if (!dir.is_valid()) {
    FML_LOG(ERROR) << "Failed to create directory: " << path;
    return false;
  }
  return true;
}

}  // namespace

int ZipResourceHelper::Decompress(const std::vector<char>& compressed_data,
                                  std::vector<char>& decompressed_data) {
  z_stream zs;
  memset(&zs, 0, sizeof(zs));

  if (inflateInit2(&zs, -MAX_WBITS) != Z_OK) {
    FML_LOG(ERROR) << "inflateInit2 failed";
    return -1;
  }

  zs.next_in =
      reinterpret_cast<Bytef*>(const_cast<char*>(compressed_data.data()));
  zs.avail_in = compressed_data.size();

  int ret;
  char buffer[32768];

  do {
    zs.next_out = reinterpret_cast<Bytef*>(buffer);
    zs.avail_out = sizeof(buffer);

    ret = inflate(&zs, 0);

    if (decompressed_data.size() < zs.total_out) {
      decompressed_data.insert(
          decompressed_data.end(), buffer,
          buffer + zs.total_out - decompressed_data.size());
    }
  } while (ret == Z_OK);

  inflateEnd(&zs);

  if (ret != Z_STREAM_END) {
    FML_LOG(ERROR) << "inflate failed: " << ret;
    return -1;
  }

  return 0;
}

bool ZipResourceHelper::UnzipToPath(const std::string& src_path,
                                    const std::string& target_path) {
  std::ifstream file(src_path, std::ios::binary);
  if (!file.is_open()) {
    FML_LOG(ERROR) << "Failed to open file: " << src_path;
    return false;
  }

  std::vector<char> zip_data((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
  file.close();

  size_t offset = 0;
  while (offset < zip_data.size()) {
    if (offset + 4 > zip_data.size()) {
      FML_LOG(ERROR) << "Unexpected end of file while reading header signature";
      return false;
    }
    if (memcmp(&zip_data[offset], "PK\x01\x02", 4) == 0) {
      return true;
    }
    if (memcmp(&zip_data[offset], "PK\x03\x04", 4) != 0) {
      FML_LOG(ERROR) << "Invalid local file header signature";
      return false;
    }
    // Advance past local file header signature
    offset += 4;
    // Ensure there are 2 bytes for version needed to extract
    if (offset + 2 > zip_data.size()) {
      FML_LOG(ERROR)
          << "Unexpected end of file while reading version needed to extract";
      return false;
    }
    offset += 2;
    // Ensure there are 2 bytes available for flags before reading
    if (offset + 2 > zip_data.size()) {
      FML_LOG(ERROR)
          << "Unexpected end of file while reading general purpose bit flag";
      return false;
    }
    uint16_t flags = ReadUInt16(&zip_data[offset]);
    bool has_data_descriptor = (flags & 0x8) != 0;
    offset += 2;

    offset += 10;

    uint32_t compressed_size = ReadUInt32(&zip_data[offset]);
    offset += 4;
    offset += 4;

    uint16_t file_name_length = ReadUInt16(&zip_data[offset]);
    offset += 2;
    uint16_t extra_field_length = ReadUInt16(&zip_data[offset]);
    offset += 2;

    std::string file_name(zip_data.data() + offset, file_name_length);
#ifdef OS_WIN
    std::replace(file_name.begin(), file_name.end(), '/', '\\');
    bool is_absolute = (!file_name.empty() && (file_name[0] == '\\')) ||
                       (file_name.size() >= 3 &&
                        (((file_name[0] >= 'A' && file_name[0] <= 'Z') ||
                          (file_name[0] >= 'a' && file_name[0] <= 'z')) &&
                         file_name[1] == ':' &&
                         (file_name[2] == '\\' || file_name[2] == '/')));
    char sep = '\\';
#else
    bool is_absolute = (!file_name.empty() && file_name[0] == '/');
    char sep = '/';
#endif
    if (is_absolute) {
      FML_LOG(ERROR) << "Unsafe absolute zip entry path: " << file_name;
      return false;
    }
    size_t pos = 0;
    while (pos < file_name.size()) {
      size_t next = file_name.find(sep, pos);
      size_t len = (next == std::string::npos ? file_name.size() : next) - pos;
      if (len == 2 && file_name[pos] == '.' && file_name[pos + 1] == '.') {
        FML_LOG(ERROR) << "Unsafe zip entry path traversal: " << file_name;
        return false;
      }
      if (next == std::string::npos) break;
      pos = next + 1;
    }
    offset += file_name_length;

    FML_LOG(INFO) << "Try to unzip file_name:" << file_name
                  << ", compressed_size:" << compressed_size
                  << ", has_data_descriptor:" << has_data_descriptor;

    offset += extra_field_length;

    // Sanitize file_name to avoid directory traversal
    if (file_name.empty()) {
      FML_LOG(ERROR) << "Empty file name in zip entry";
      return false;
    }
#ifdef OS_WIN
    // Reject absolute or drive-qualified paths like "\\...", "/..." or
    // "C:\\..."
    if (file_name[0] == '\\' || file_name[0] == '/') {
      FML_LOG(ERROR) << "Reject absolute path in zip entry: " << file_name;
      return false;
    }
    if (file_name.size() >= 2 && file_name[1] == ':') {
      FML_LOG(ERROR) << "Reject drive-qualified path in zip entry: "
                     << file_name;
      return false;
    }
#else
    // Reject absolute paths like "/..."
    if (file_name[0] == '/') {
      FML_LOG(ERROR) << "Reject absolute path in zip entry: " << file_name;
      return false;
    }
#endif
    // Reject traversal segments like ".."
    size_t start = 0;
    while (start < file_name.size()) {
      size_t pos = file_name.find_first_of("/\\", start);
      size_t len =
          (pos == std::string::npos) ? std::string::npos : (pos - start);
      std::string segment = file_name.substr(start, len);
      if (segment == "..") {
        FML_LOG(ERROR) << "Reject path traversal segment in zip entry: "
                       << file_name;
        return false;
      }
      if (pos == std::string::npos) {
        break;
      }
      start = pos + 1;
    }
    std::string full_path = target_path + kPathSeparator + file_name;
    if (file_name.back() == kPathSeparator[0]) {
      if (!CreateDirectories(full_path)) {
        FML_LOG(ERROR) << "Cannot create directory: " << full_path;
        return false;
      }
      continue;
    }
    if (!CreateDirectories(
            full_path.substr(0, full_path.find_last_of(kPathSeparator)))) {
      FML_LOG(ERROR) << "Cannot create directory for file: " << full_path;
      return false;
    }

    if (has_data_descriptor && compressed_size == 0) {
      size_t data_start = offset;
      while (offset + 12 <= zip_data.size()) {
        if (memcmp(&zip_data[offset], "PK\x07\x08", 4) == 0) {
          if (offset + 12 > zip_data.size()) {
            FML_LOG(ERROR) << "Corrupted data descriptor for file: "
                           << file_name;
            return false;
          }
          compressed_size = ReadUInt32(&zip_data[offset + 8]);
          break;
        }
        offset++;
      }

      if (compressed_size == 0) {
        FML_LOG(ERROR) << "Failed to find valid data descriptor for: "
                       << file_name;
        return false;
      }

      offset = data_start;
    }
    if (offset + compressed_size > zip_data.size()) {
      FML_LOG(ERROR) << "Compressed data out of range for file: " << file_name;
      return false;
    }
    std::vector<char> compressed_data(
        zip_data.begin() + offset, zip_data.begin() + offset + compressed_size);
    std::vector<char> decompressed_data;
    if (Decompress(compressed_data, decompressed_data) != 0) {
      return false;
    }

    std::ofstream out_file(full_path, std::ios::binary);
    if (!out_file.is_open()) {
      FML_LOG(ERROR) << "Failed to open output file: " << full_path;
      return false;
    }

    out_file.write(decompressed_data.data(), decompressed_data.size());
    out_file.close();

    FML_LOG(INFO) << "Unzip file success: " << full_path;

    offset += compressed_size;

    if (has_data_descriptor) {
      offset += 16;
    }
  }

  return true;
}

}  // namespace clay
