// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/lynxml/lynxml_source_observer.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace lynxml {
namespace {

TEST(LynxMLSourceObserverTest, CollectsSupportedSourceBlocks) {
  auto result = ParseLynxMLSources(
      "<!doctype lynx><lynx engine-version=5.4.2>"
      "<style>.first { width: 1px; }</style>"
      "<script thread=main>main-one</script>"
      "<script thread=background>background-one</script>"
      "</lynx>");

  EXPECT_TRUE(result.error.empty()) << result.error;
  EXPECT_EQ(result.sources.engine_version, "5.4.2");
  EXPECT_EQ(result.sources.main_thread_script, "main-one");
  EXPECT_EQ(result.sources.background_thread_script, "background-one");
  EXPECT_EQ(result.sources.style, ".first { width: 1px; }");
}

TEST(LynxMLSourceObserverTest, RejectsRepeatedSourceBlockTypes) {
  const char* sources[] = {
      ("<!doctype lynx><lynx><style>one</style><style>two</style>"
       "<script thread=main>main</script></lynx>"),
      ("<!doctype lynx><lynx><script thread=main>one</script>"
       "<script thread=main>two</script></lynx>"),
      ("<!doctype lynx><lynx><script thread=main>main</script>"
       "<script thread=background>one</script>"
       "<script thread=background>two</script></lynx>"),
  };

  for (const char* source : sources) {
    auto result = ParseLynxMLSources(source);
    EXPECT_FALSE(result.error.empty()) << source;
  }
}

TEST(LynxMLSourceObserverTest, RejectsUnsupportedSourceBlockAttributes) {
  const char* sources[] = {
      ("<!doctype lynx><lynx><style scoped>style</style>"
       "<script thread=main>main</script></lynx>"),
      "<!doctype lynx><lynx><script THREAD=main>main</script></lynx>",
      "<!doctype lynx><lynx><script thread=main flag>main</script></lynx>",
  };

  for (const char* source : sources) {
    auto result = ParseLynxMLSources(source);
    EXPECT_FALSE(result.error.empty()) << source;
  }
}

TEST(LynxMLSourceObserverTest, RejectsInvalidOrRepeatedThreadAttribute) {
  const char* sources[] = {
      "<!doctype lynx><lynx><script>main</script></lynx>",
      "<!doctype lynx><lynx><script thread=worker>main</script></lynx>",
      ("<!doctype lynx><lynx><script thread=main thread=background>main"
       "</script></lynx>"),
  };

  for (const char* source : sources) {
    auto result = ParseLynxMLSources(source);
    EXPECT_FALSE(result.error.empty()) << source;
  }
}

TEST(LynxMLSourceObserverTest, PropagatesParserErrors) {
  auto result = ParseLynxMLSources("<!doctype lynx><lynx>");

  EXPECT_FALSE(result.error.empty());
  EXPECT_NE(result.error.find("missing closing tag '</lynx>'"),
            std::string::npos);
}

}  // namespace
}  // namespace lynxml
}  // namespace lynx
