// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#define private public
#define protected public

#include <cmath>
#include <numeric>

#include "base/include/fml/message_loop.h"
#include "core/animation/lynx_basic_animator/basic_animator.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

class LynxBasicAnimatorTest : public ::testing::Test {
 public:
  LynxBasicAnimatorTest() {}
  ~LynxBasicAnimatorTest() override {}
  double calculateMSE(std::vector<double>& data_set_1,
                      std::vector<double>& data_set_2) {
    if (data_set_1.empty() || data_set_2.empty()) {
      return 10.0;
    }
    if (data_set_1.size() != data_set_2.size()) {
      if (data_set_1.size() == 61) {
        data_set_1.push_back(1.0);
      } else if (data_set_2.size() == 61) {
        data_set_2.push_back(2.0);
      }
    }

    double mse = 0.0;
    size_t n = data_set_1.size();

    for (size_t i = 0; i < n; ++i) {
      double diff = data_set_1[i] - data_set_2[i];
      mse += diff * diff;
    }

    return mse / n;
  }

  double calculatePearsonCorrelation(std::vector<double>& data_set_1,
                                     std::vector<double>& data_set_2) {
    if (data_set_1.empty() || data_set_2.empty()) {
      return 0.0;
    }
    if (data_set_1.size() != data_set_2.size()) {
      if (data_set_1.size() == 61) {
        data_set_1.push_back(1.0);
      } else if (data_set_2.size() == 61) {
        data_set_2.push_back(2.0);
      }
    }

    size_t n = data_set_1.size();
    double sum1 = 0, sum2 = 0, sum1Sq = 0, sum2Sq = 0, pSum = 0;

    for (size_t i = 0; i < n; ++i) {
      sum1 += data_set_1[i];
      sum2 += data_set_2[i];
      sum1Sq += std::pow(data_set_1[i], 2);
      sum2Sq += std::pow(data_set_2[i], 2);
      pSum += data_set_1[i] * data_set_2[i];
    }

    double num = pSum - (sum1 * sum2 / n);
    double den = std::sqrt((sum1Sq - std::pow(sum1, 2) / n) *
                           (sum2Sq - std::pow(sum2, 2) / n));

    return den == 0 ? 0 : num / den;
  }

  std::shared_ptr<animation::basic::LynxBasicAnimator> lynx_basic_animator_1;
};

TEST_F(LynxBasicAnimatorTest, HowToUseBasicAnimator) {
  starlight::AnimationData data;
  data.duration = 2000;
  data.fill_mode = starlight::AnimationFillModeType::kForwards;
  starlight::TimingFunctionData timing_function_data;
  timing_function_data.timing_func = starlight::TimingFunctionType::kLinear;
  data.timing_func = timing_function_data;

  lynx_basic_animator_1 =
      std::make_shared<animation::basic::LynxBasicAnimator>(data);
  std::vector<double> data_set_1;
  lynx_basic_animator_1->RegisterCustomCallback([&](float progress) {
    std::cout << "CallBack called with value: " << progress << std::endl;
    data_set_1.push_back(progress);
  });
  // lynx_basic_animator_1->Start();
  lynx_basic_animator_1->InitializeAnimator();

  fml::TimeDelta value1 = fml::TimeDelta::FromSecondsF(0.f);
  fml::TimeDelta value2 = fml::TimeDelta::FromSecondsF(1.f);
  fml::TimeDelta value3 = fml::TimeDelta::FromSecondsF(2.f);

  auto model = lynx_basic_animator_1->animation_->effect_->keyframe_models_
                   .find("BASIC_TYPE_FLOAT")
                   ->second.get();
  auto curve = model->curve_.get();
  auto res1 = static_cast<animation::BasicFloatPropertyValue*>(
                  curve->GetValue(value1).get())
                  ->GetFloatValue();
  auto res2 = static_cast<animation::BasicFloatPropertyValue*>(
                  curve->GetValue(value2).get())
                  ->GetFloatValue();
  auto res3 = static_cast<animation::BasicFloatPropertyValue*>(
                  curve->GetValue(value3).get())
                  ->GetFloatValue();
  EXPECT_FLOAT_EQ(0.0f, res1);
  EXPECT_FLOAT_EQ(0.5f, res2);
  EXPECT_FLOAT_EQ(1.0f, res3);
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
