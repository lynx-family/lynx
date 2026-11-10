// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gfx/public/path_impl_factory.h"

#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

#include "gfx/public/path.h"
#include "gfx/testing/path_stream_impl.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx::gfx {
namespace {
struct FactoryState {
  int constructions = 0;
  int destructions = 0;
  int realizations = 0;
  const PathImpl* last_created = nullptr;
};

class TestPathImpl final : public PathImpl {
 public:
  TestPathImpl(PathSource source, std::shared_ptr<FactoryState> state)
      : PathImpl(std::move(source)), state_(std::move(state)) {}
  ~TestPathImpl() override { ++state_->destructions; }
  void AccessNativeForTest() const {
    if (!realized_) {
      ++state_->realizations;
      realized_ = true;
    }
  }

 private:
  std::shared_ptr<FactoryState> state_;
  mutable bool realized_ = false;
};

class TestPathFactory final : public PathImplFactory {
 public:
  explicit TestPathFactory(std::shared_ptr<FactoryState> state)
      : state_(std::move(state)) {}
  std::unique_ptr<PathImpl> Create(PathSource source) const override {
    ++state_->constructions;
    auto impl = std::make_unique<TestPathImpl>(std::move(source), state_);
    state_->last_created = impl.get();
    return impl;
  }

 private:
  std::shared_ptr<FactoryState> state_;
};
static_assert(std::is_abstract<PathImpl>::value);
static_assert(std::has_virtual_destructor<PathImpl>::value);

void ExpectFactoryResult(const fml::RefPtr<Path>& path,
                         const FactoryState& state) {
  ASSERT_TRUE(path);
  EXPECT_EQ(&path->GetImpl(), state.last_created);
}
}  // namespace

TEST(PathImplFactoryTest, EachSourcePublishesOnceWithoutNativeAccess) {
  auto state = std::make_shared<FactoryState>();
  TestPathFactory factory(state);
  const float radii[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  auto rect = Path::MakeRect(10, 20, factory);
  ExpectFactoryResult(rect, *state);
  auto rounded = Path::MakeRoundedRect(30, 40, radii, factory);
  ExpectFactoryResult(rounded, *state);
  PathBuilder builder;
  builder.MoveTo(1, 2);
  auto commands = builder.Build(factory);
  ExpectFactoryResult(commands, *state);
  auto svg = Path::MakeSvgPathData("M0 0L10 10Z", 2, factory);
  ExpectFactoryResult(svg, *state);
  ASSERT_TRUE(rect && rounded && commands && svg);
  EXPECT_EQ(state->constructions, 4);
  for (const auto* path :
       {rect.get(), rounded.get(), commands.get(), svg.get()}) {
    EXPECT_EQ(path->GetImpl().GetKey(), path->GetKey());
    path->GetSourceView();
  }
  EXPECT_EQ(state->realizations, 0);
  EXPECT_EQ(std::get<SvgPathView>(svg->GetSourceView()).source_to_layout_scale,
            2);
}

TEST(PathImplFactoryTest, InvalidGeometryNeverReachesTheFactory) {
  auto state = std::make_shared<FactoryState>();
  TestPathFactory factory(state);
  const float inf = std::numeric_limits<float>::infinity();
  float radii[8] = {};
  radii[0] = -1;
  EXPECT_FALSE(Path::MakeRect(-1, 2, factory));
  EXPECT_FALSE(Path::MakeRoundedRect(10, 20, radii, factory));
  PathBuilder builder;
  builder.LineTo(0, 0);
  EXPECT_FALSE(builder.Build(factory));
  builder.MoveTo(inf, 0);
  EXPECT_FALSE(builder.Build(factory));
  EXPECT_FALSE(Path::MakeSvgPathData("M0 0", 0, factory));
  EXPECT_FALSE(Path::MakeSvgPathData(base::String(std::string("M0\0 0", 5)), 1,
                                     factory));
  EXPECT_EQ(state->constructions, 0);
  EXPECT_EQ(state->realizations, 0);
}

TEST(PathImplFactoryTest, SourceCopiesShareGeometryAndOriginalSvgBytes) {
  auto state = std::make_shared<FactoryState>();
  TestPathFactory first(state), second(state);
  PathBuilder builder;
  builder.MoveTo(1, 2);
  auto commands = builder.Build(first);
  auto copied = second.Create(commands->GetImpl().GetSource());
  EXPECT_NE(commands->GetKey(), copied->GetKey());
  auto a = std::get<PathView>(commands->GetSourceView());
  auto b = std::get<PathView>(copied->GetSourceView());
  EXPECT_EQ(a.commands, b.commands);
  EXPECT_EQ(a.points, b.points);
  commands = nullptr;
  EXPECT_EQ(b.points[0].y, 2);

  auto svg = Path::MakeSvgPathData("M0 0L20 20Z", 2, first);
  auto svg_copy = second.Create(svg->GetImpl().GetSource());
  const auto* bytes = std::get<SvgPathView>(svg->GetSourceView()).data.data();
  EXPECT_EQ(bytes,
            std::get<SvgPathView>(svg_copy->GetSourceView()).data.data());
  svg = nullptr;
  EXPECT_EQ(std::get<SvgPathView>(svg_copy->GetSourceView()).data,
            "M0 0L20 20Z");
  EXPECT_EQ(state->realizations, 0);
}

TEST(PathImplFactoryTest, BuilderCanReuseStorageAndSelectADifferentFactory) {
  auto state = std::make_shared<FactoryState>();
  TestPathFactory factory(state);
  PathBuilder builder;
  auto consumer = builder.GetConsumer();
  consumer.MoveToPoint(consumer.ctx, 1, 2);
  auto first = builder.Build(factory);
  ASSERT_TRUE(first);
  EXPECT_EQ(state->constructions, 1);
  consumer.MoveToPoint(consumer.ctx, 3, 4);
  auto stream = builder.Build(GetTestPathImplFactory());
  ASSERT_TRUE(stream);
  EXPECT_EQ(std::get<PathView>(stream->GetSourceView()).points[0].x, 3);
  EXPECT_NE(stream->GetKey(), first->GetKey());
  EXPECT_EQ(state->constructions, 1);
  EXPECT_EQ(std::get<PathView>(first->GetSourceView()).points[0].x, 1);
  consumer.LineToPoint(consumer.ctx, 5, 6);
  EXPECT_FALSE(builder.Build(factory));
  EXPECT_EQ(state->constructions, 1);
}

TEST(PathImplFactoryTest, GeometryHelpersAndEmptyShapesUseSelectedFactory) {
  auto state = std::make_shared<FactoryState>();
  TestPathFactory factory(state);
  const float radii[8] = {};
  ExpectFactoryResult(Path::MakeEllipse(10, 20, factory), *state);
  ExpectFactoryResult(Path::MakeEllipse(0, 20, factory), *state);
  ExpectFactoryResult(Path::MakeSuperEllipse(10, 20, 4, 3, factory), *state);
  ExpectFactoryResult(Path::MakeSuperRoundedRect(10, 20, radii, 4, 3, factory),
                      *state);
  ExpectFactoryResult(Path::MakeSuperRoundedRect(0, 20, radii, 4, 3, factory),
                      *state);
  EXPECT_EQ(state->constructions, 5);
  EXPECT_EQ(state->realizations, 0);
}

TEST(PathImplFactoryTest, RetainedFacadeOutlivesFactoryAndDefersNativeAccess) {
  auto state = std::make_shared<FactoryState>();
  fml::RefPtr<Path> retained;
  {
    TestPathFactory factory(state);
    auto path = Path::MakeRect(10, 20, factory);
    retained = path;
    EXPECT_EQ(state->realizations, 0);
  }
  EXPECT_EQ(state->destructions, 0);
  const auto& impl = static_cast<const TestPathImpl&>(retained->GetImpl());
  impl.AccessNativeForTest();
  impl.AccessNativeForTest();
  EXPECT_EQ(state->realizations, 1);
  EXPECT_EQ(std::get<PathView>(retained->GetSourceView()).header.width, 10);
  retained = nullptr;
  EXPECT_EQ(state->destructions, 1);
}

}  // namespace lynx::gfx
