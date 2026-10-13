// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0.
#include <dirent.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

#include "clay/gfx/shared_image/angle_software_shm_image_backing.h"
#include "clay/gfx/shared_image/shared_image_representation.h"
#include "clay/gfx/skity/skity_image.h"
#include "skity/graphic/image.hpp"
#include "skity/io/pixmap.hpp"
static void Check(bool value) {
  if (!value) std::abort();
}
static size_t Fds() {
  DIR* directory = opendir("/proc/self/fd");
  Check(directory);
  size_t count = 0;
  while (auto* entry = readdir(directory))
    if (entry->d_name[0] != '.') ++count;
  closedir(directory);
  return count;
}
static size_t Mappings() {
  std::ifstream maps("/proc/self/maps");
  std::string line;
  size_t count = 0;
  while (std::getline(maps, line))
    if (line.find("/dev/shm/angle_shared_image_backing") != std::string::npos)
      ++count;
  return count;
}
int main() {
  const auto fds = Fds(), maps = Mappings();
  auto backing = lynx::fml::MakeRefCounted<clay::AngleSoftwareShmImageBacking>(
      clay::SharedImageBacking::PixelFormat::kNative8888, skity::Vec2{4, 4});
  auto* bytes = static_cast<uint8_t*>(backing->GetGFXHandle());
  Check(bytes);
  std::memset(bytes, 0, 64);
  bytes[0] = 80;
  bytes[1] = 180;
  bytes[2] = 20;
  bytes[3] = 255;
  auto representation = backing->CreateSkityRepresentation(nullptr);
  Check(representation.get() != nullptr);
  auto first = representation->GetSkityImage();
  Check(first != nullptr);
  auto* pixmap = first->gr_image()->GetPixmap();
  Check(pixmap && *pixmap);
  Check((*pixmap)->GetColorType() == skity::ColorType::kBGRA);
  Check(std::memcmp((*pixmap)->Addr8(0, 0), bytes, 4) == 0);
  std::memset(bytes, 0, 4);
  Check((*pixmap)->Addr8(0, 0)[1] == 180);
  for (int i = 0; i < 1000; ++i)
    Check(representation->GetSkityImage() != nullptr);
  Check(Fds() == fds + 1 && Mappings() == maps + 1);
  // A truncated external frame must fail rather than SIGBUS during the copy.
  Check(ftruncate(backing->GetShmFd(), 63) == 0);
  Check(representation->GetSkityImage() == nullptr);
  Check(ftruncate(backing->GetShmFd(), 64) == 0);
  representation = nullptr;
  backing = nullptr;
  Check(Fds() == fds && Mappings() == maps);
  std::puts(
      "GTK4_SHM Skity owned pixels, 1000 reads, truncated-frame rejection, "
      "zero FD/mmap growth PASS");
}
