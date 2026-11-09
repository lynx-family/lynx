// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gfx/public/path.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <utility>

namespace lynx::gfx {
namespace {
bool ValidSize(float width, float height) {
  return std::isfinite(width) && std::isfinite(height) && width >= 0 &&
         height >= 0;
}
PathVerb* GeometryCommands(void* geometry) {
  return reinterpret_cast<PathVerb*>(static_cast<unsigned char*>(geometry) +
                                     sizeof(PathStorage));
}
PathPoint* GeometryPoints(void* geometry, size_t commands) {
  return reinterpret_cast<PathPoint*>(GeometryCommands(geometry) + commands);
}
}  // namespace

PathSourceView PathSource::GetSourceView() const {
  if (source_to_layout_scale_ > 0)
    return SvgPathView{svg_.str(), source_to_layout_scale_};
  return PathView{
      header_, geometry_ ? GeometryCommands(geometry_.get()) : nullptr,
      geometry_ ? GeometryPoints(geometry_.get(), header_.command_count)
                : nullptr};
}
Path::Path(PathSource source) : impl_(std::move(source)) {}
Path::~Path() = default;

fml::RefPtr<Path> Path::MakeRect(float width, float height) {
  if (!ValidSize(width, height)) return nullptr;
  PathHeader header{};
  header.kind = PathKind::kRect;
  header.width = width;
  header.height = height;
  return fml::AdoptRef(new Path(PathSource(header)));
}
fml::RefPtr<Path> Path::MakeSvgPathData(base::String data, float scale) {
  if (!std::isfinite(scale) || scale <= 0 || data.length() > kMaxSvgBytes ||
      std::memchr(data.c_str(), '\0', data.length()))
    return nullptr;
  return fml::AdoptRef(new Path(PathSource(std::move(data), scale)));
}

}  // namespace lynx::gfx
