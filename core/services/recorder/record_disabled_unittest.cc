// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// This source has its own recorder-disabled GN configuration.
#include "core/services/recorder/record.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

TEST(RecordMacro, DisabledArgumentsAreNotEvaluatedOrTypeChecked) {
  constexpr auto action = lynx::tasm::recorder::RecordType::LoadTemplate;
  (void)action;
  int condition = 0;
  int evaluated = 0;
  RECORD_OPTIONAL(++condition, UnknownAction, ++evaluated, missing_symbol);
  EXPECT_EQ(condition, 0);
  RECORD(LoadTemplate, ++evaluated, recorder_only_symbol);
  // A disabled invocation remains one statement in an unbraced if/else.
  if (evaluated == 0)
    RECORD(UnknownAction, another_recorder_only_symbol);
  else
    ++evaluated;
  EXPECT_EQ(evaluated, 0);
}

TEST(RecordMacro, DisabledPreludeDoesNotRunOrReturn) {
  int evaluated = 0;
  auto invoke = [&evaluated]() {
    if (evaluated == 0)
      RECORD_WITH_EARLY_RETURN(++evaluated; return missing_return_value;
                               , UnknownAction, missing_argument);
    else
      ++evaluated;
    return 11;
  };
  EXPECT_EQ(invoke(), 11);
  EXPECT_EQ(evaluated, 0);
}
