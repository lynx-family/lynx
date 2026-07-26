// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/binary_encoder/encode_tracer.h"

#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

namespace lynx {
namespace tasm {

EncodeTracer::ScopedEvent::ScopedEvent(EncodeTracer* tracer, const char* name)
    : tracer_(tracer),
      name_(name),
      start_(tracer_ != nullptr ? Clock::now() : TimePoint{}) {}

EncodeTracer::ScopedEvent::~ScopedEvent() {
  if (tracer_ != nullptr) {
    tracer_->AddEvent(name_, start_);
  }
}

std::string EncodeTracer::Finish() {
  AddEvent("Encode", encode_start_);

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  writer.StartArray();
  for (const auto& event : events_) {
    writer.StartObject();
    writer.Key("name");
    writer.String(event.name);
    writer.Key("duration_us");
    writer.Double(event.duration_us);
    writer.EndObject();
  }
  writer.EndArray();
  return std::string(buffer.GetString(), buffer.GetSize());
}

void EncodeTracer::AddEvent(const char* name, TimePoint start) {
  events_.push_back(Event{
      name,
      std::chrono::duration<double, std::micro>(Clock::now() - start).count()});
}

}  // namespace tasm
}  // namespace lynx
