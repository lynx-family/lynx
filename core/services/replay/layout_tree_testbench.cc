// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/services/replay/layout_tree_testbench.h"

#include <string>
#include <vector>

#include "third_party/rapidjson/prettywriter.h"
#include "third_party/rapidjson/stringbuffer.h"

namespace lynx {
namespace tasm {
namespace replay {
double RoundToLayoutAccuracy(float value) {
  double result = std::round(value * 100) / 100.0;
  // The result may be -0.0 or 0.0,
  // the two values behave as equal in numerical comparisons.
  // In this case, to ensure consistent printing, return 0 uniformly.
  if (base::FloatsEqual(result, 0.0)) {
    result = 0.0;
  }
  return result;
}

void WriteNodeInfo(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                   std::vector<double>& box_model, float offset_top,
                   float offset_left) {
  writer.Key("width");
  writer.Double(RoundToLayoutAccuracy(box_model[0]));
  writer.Key("height");
  writer.Double(RoundToLayoutAccuracy(box_model[1]));

  writer.Key("offset_top");
  writer.Double(RoundToLayoutAccuracy(offset_top));
  writer.Key("offset_left");
  writer.Double(RoundToLayoutAccuracy(offset_left));
  // content
  writer.Key("content");
  writer.StartArray();
  for (int i = 2; i <= 9; ++i) {
    writer.Double(RoundToLayoutAccuracy(box_model[i]));
  }
  writer.EndArray();

  writer.Key("padding");
  writer.StartArray();
  for (int i = 10; i <= 17; ++i) {
    writer.Double(RoundToLayoutAccuracy(box_model[i]));
  }
  writer.EndArray();

  writer.Key("border");
  writer.StartArray();
  for (int i = 18; i <= 25; ++i) {
    writer.Double(RoundToLayoutAccuracy(box_model[i]));
  }
  writer.EndArray();

  writer.Key("margin");
  writer.StartArray();
  for (int i = 26; i <= 33; ++i) {
    writer.Double(RoundToLayoutAccuracy(box_model[i]));
  }
  writer.EndArray();
}

void GetLayoutTreeRecursive(rapidjson::Writer<rapidjson::StringBuffer>& writer,
                            SLNode* slnode) {
  writer.StartObject();
  std::vector<double> box_model = slnode->GetBoxModel();
  WriteNodeInfo(writer, box_model,
                slnode->GetBorderBoundTopFromParentPaddingBound(),
                slnode->GetBorderBoundLeftFromParentPaddingBound());
  int child_size = slnode->GetChildCount();
  if (child_size > 0) {
    writer.Key("children");
    writer.StartArray();
    for (int i = 0; i < child_size; ++i) {
      auto* child = static_cast<SLNode*>(slnode->Find(i));
      GetLayoutTreeRecursive(writer, child);
    }
    writer.EndArray();
  }
  writer.EndObject();
}

std::string LayoutTreeTestBench::GetLayoutTree(SLNode* slnode) {
  rapidjson::StringBuffer strBuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strBuf);
  GetLayoutTreeRecursive(writer, slnode);
  return strBuf.GetString();
}

}  // namespace replay
}  // namespace tasm
}  // namespace lynx
