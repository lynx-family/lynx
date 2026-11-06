// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <utility>

#include "base/include/fml/message_loop.h"
#include "clay/ui/common/value_utils.h"
#include "clay/ui/component/base_image_view.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/scroll_view.h"
#include "clay/ui/component/text/text_view.h"
#include "clay/ui/component/view.h"
#include "clay/ui/compositing/pending_picture_layer.h"
#include "clay/ui/rendering/render_container.h"
#include "clay/ui/rendering/renderer.h"
#include "clay/ui/rendering/text/render_inline_text.h"
#ifdef ENABLE_ACCESSIBILITY
#include "clay/ui/semantics/semantics_owner.h"
#endif
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

class CountingImageForCacheTest : public RenderImage {
 public:
  void Paint(PaintingContext& context, const FloatPoint& offset) override {
    ++paint_count;
    RenderImage::Paint(context, offset);
  }
  int paint_count = 0;
};

class TrackingImageViewForCacheTest : public BaseImageView {
 public:
  using BaseImageView::BaseImageView;
  int invalidations = 0;

 protected:
  void Invalidate() override {
    ++invalidations;
    BaseImageView::Invalidate();
  }
};

class AttributeInvalidationTest : public ::testing::Test {
 protected:
  static fml::RefPtr<GPUUnrefQueue> CreateUnrefQueue() {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    // Recorded pictures need a release queue even without a GPU context.
    return fml::MakeRefCounted<GPUUnrefQueue>(
        fml::MessageLoop::GetCurrent().GetTaskRunner(), false);
  }

  void SetUp() override {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    page_ = std::make_unique<PageView>(
        0, nullptr, fml::MessageLoop::GetCurrent().GetTaskRunner());
    auto image = std::make_unique<CountingImageForCacheTest>();
    render_ = image.get();
    view_ = std::make_unique<TrackingImageViewForCacheTest>(
        1, "image", std::move(image), page_.get());
    view_->SetBound(0, 0, 100, 100);
    render_->SetOpacity(0.5f);
    // Generate a real content picture without an asynchronous image loader.
    render_->SetBackgroundColor(Color::kBlack());
    root_.SetRepaintBoundary(true);
    root_.SetWidth(400);
    root_.SetHeight(400);
    root_.AddChild(render_);
    renderer_.SetRoot(&root_);
    Paint();
  }

  void TearDown() override {
#ifdef ENABLE_ACCESSIBILITY
    page_->GetSemanticsOwner()->RemoveDirtySemanticsForDescendants(view_.get());
#endif
    root_.RemoveChild(render_);
    render_->SetRenderer(nullptr);
    root_.SetRenderer(nullptr);
    view_.reset();
    page_.reset();
  }

  void Paint() { renderer_.Paint(); }
  void BeginOpacity(float opacity) {
    Value::Map attributes;
    attributes.emplace("opacity", Value(opacity));
    view_->BeforeSetAttribute(attributes);
    view_->SetAttribute("opacity", Value(opacity));
  }
  void SetOpacity(float opacity) {
    BeginOpacity(opacity);
    view_->DidUpdateAttributes();
  }
  Value TransformValue(ClayTransformType type = ClayTransformType::kScale,
                       float value = 0.8f) {
    Value::Array operation;
    operation.emplace_back(static_cast<int>(type));
    operation.emplace_back(value);
    operation.emplace_back(0);
    operation.emplace_back(value);
    operation.emplace_back(0);
    operation.emplace_back(0.f);
    operation.emplace_back(0);
    Value::Array operations;
    operations.emplace_back(std::move(operation));
    return Value(std::move(operations));
  }
  void BeginTransform(const Value& transform, bool with_opacity = false) {
    Value::Map attributes;
    attributes.emplace("transform", CloneClayValue(transform));
    if (with_opacity) attributes.emplace("opacity", Value(0.25f));
    view_->BeforeSetAttribute(attributes);
    view_->SetAttribute("transform", transform);
    if (with_opacity) view_->SetAttribute("opacity", Value(0.25f));
  }
  void SetTransform(const Value& transform, bool with_opacity = false) {
    BeginTransform(transform, with_opacity);
    view_->DidUpdateAttributes();
  }
  const Picture* ContentPicture(PendingLayer* layer) {
    if (!layer) return nullptr;
    if (layer->GetName() == "PendingPictureLayer") {
      auto* picture = static_cast<PendingPictureLayer*>(layer)->picture();
      if (picture && !picture->IsEmpty()) return picture;
    }
    for (auto* child = layer->FirstChild(); child;
         child = child->NextSibling()) {
      if (const auto* picture = ContentPicture(child)) return picture;
    }
    return nullptr;
  }
  void ExpectBatchClassification(const std::string& tag) {
    BaseImageView other(2, tag, std::make_unique<RenderImage>(), page_.get());
    Value::Map attributes;
    attributes.emplace("opacity", Value(0.25f));
    attributes.emplace("transform", TransformValue());
    other.BeforeSetAttribute(attributes);
    EXPECT_EQ(other.pending_attribute_invalidation_,
              BaseView::AttributeInvalidation::kEffect);
  }

#ifdef ENABLE_ACCESSIBILITY
  void EnableSemantics() {
    auto* owner = page_->GetSemanticsOwner();
    owner->SetSemanticsEnabled(true);
    owner->semantics_nodes_to_update_descendants_.clear();
  }
  bool SemanticsUpdateQueued() {
    return page_->GetSemanticsOwner()
               ->semantics_nodes_to_update_descendants_.count(view_.get()) != 0;
  }
#endif

  void CheckComponentContentRetention(BaseView& other) {
    other.SetBound(0, 0, 100, 100);
    auto* object = other.render_object();
    object->SetBackgroundColor(Color::kBlack());
    object->SetOpacity(0.5f);
    root_.AddChild(object);
    Paint();
    const auto* picture = ContentPicture(object->GetContainerLayer());
    EXPECT_NE(picture, nullptr);
    Value::Map attributes;
    attributes.emplace("opacity", Value(0.25f));
    other.BeforeSetAttribute(attributes);
    other.SetAttribute("opacity", Value(0.25f));
    other.DidUpdateAttributes();
    EXPECT_FALSE(object->NeedsPaint());
    EXPECT_TRUE(object->NeedsEffect());
    Paint();
    EXPECT_EQ(ContentPicture(object->GetContainerLayer()), picture);
    EXPECT_EQ(
        object->GetOrCreateEffectLayer(EffectType::kOpacity)->GetOpacity(),
        static_cast<int>(0.25f * 255));
#ifdef ENABLE_ACCESSIBILITY
    page_->GetSemanticsOwner()->RemoveDirtySemanticsForDescendants(&other);
#endif
    root_.RemoveChild(object);
    object->SetRenderer(nullptr);
  }

  int GroupAlpha() {
    return render_->GetOrCreateEffectLayer(EffectType::kOpacity)->GetOpacity();
  }

  std::unique_ptr<PageView> page_;
  std::unique_ptr<TrackingImageViewForCacheTest> view_;
  CountingImageForCacheTest* render_ = nullptr;
  Renderer renderer_{nullptr, CreateUnrefQueue()};
  RenderContainer root_;
};

TEST_F(AttributeInvalidationTest, OpacityOnlyUpdateRetainsContentPicture) {
  const auto* picture = ContentPicture(render_->GetContainerLayer());
  ASSERT_NE(picture, nullptr);
  const int paints = render_->paint_count;
  SetOpacity(0.25f);
  EXPECT_FALSE(render_->NeedsPaint());
  EXPECT_TRUE(render_->NeedsEffect());
  Paint();
  EXPECT_EQ(render_->paint_count, paints);
  EXPECT_EQ(ContentPicture(render_->GetContainerLayer()), picture);
  EXPECT_EQ(GroupAlpha(), static_cast<int>(0.25f * 255));
}

TEST_F(AttributeInvalidationTest, CustomInvalidationHookStillRuns) {
  const int invalidations = view_->invalidations;
  const int paints = render_->paint_count;
  SetOpacity(0.25f);
  EXPECT_EQ(view_->invalidations, invalidations + 1);
  Paint();
  EXPECT_EQ(render_->paint_count, paints);
}

TEST_F(AttributeInvalidationTest, ContentBatchInvalidatesContent) {
  const int paints = render_->paint_count;
  BeginOpacity(0.25f);
  view_->BeforeSetAttribute(Value::Map{});
  view_->DidUpdateAttributes();
  EXPECT_TRUE(render_->NeedsPaint());
  Paint();
  EXPECT_EQ(render_->paint_count, paints + 1);
}

TEST_F(AttributeInvalidationTest, MixedAttributeBatchInvalidatesContent) {
  Value::Map attributes;
  attributes.emplace("opacity", Value(0.25f));
  attributes.emplace("mode", Value("aspectFit"));
  view_->BeforeSetAttribute(attributes);
  view_->SetAttribute("mode", Value("aspectFit"));
  view_->SetAttribute("opacity", Value(0.25f));
  view_->DidUpdateAttributes();
  EXPECT_TRUE(render_->NeedsPaint());
}

TEST_F(AttributeInvalidationTest,
       AnimationBatchStillInvalidatesAfterMapProcessing) {
  Value::Map attributes;
  attributes.emplace("opacity", Value(0.25f));
  attributes.emplace("transition", Value(Value::Array{}));
  view_->BeforeSetAttribute(attributes);
  // The adaptor processes transition/animation before the remaining attributes.
  view_->SetAttribute("opacity", Value(0.25f));
  view_->DidUpdateAttributes();
  EXPECT_TRUE(render_->NeedsPaint());
}

TEST_F(AttributeInvalidationTest, AttributeClassificationIsSharedAcrossTags) {
  ExpectBatchClassification("svg");
  ExpectBatchClassification("inline-image");
  ExpectBatchClassification("filter-image");
}

TEST_F(AttributeInvalidationTest, EffectClassificationIsEnabledByDefault) {
  ExpectBatchClassification("image");
}

TEST_F(AttributeInvalidationTest, ExtraAttributeClearsOpacityOnlyMarker) {
  BeginOpacity(0.25f);
  view_->SetAttribute("mode", Value("aspectFit"));
  view_->DidUpdateAttributes();
  EXPECT_TRUE(render_->NeedsPaint());
}

TEST_F(AttributeInvalidationTest,
       MissingBatchBoundaryUsesConservativeInvalidation) {
  view_->SetAttribute("opacity", Value(0.25f));
  view_->DidUpdateAttributes();
  EXPECT_TRUE(render_->NeedsPaint());
}

TEST_F(AttributeInvalidationTest, RepeatedDidUpdateConsumesMarkerOnce) {
  SetOpacity(0.25f);
  Paint();
  EXPECT_FALSE(render_->NeedsPaint());
  view_->DidUpdateAttributes();
  EXPECT_TRUE(render_->NeedsPaint());
}

TEST_F(AttributeInvalidationTest, EmptyBatchResetsOpacityOnlyMarker) {
  BeginOpacity(0.25f);
  view_->BeforeSetAttribute(Value::Map{});
  view_->DidUpdateAttributes();
  EXPECT_TRUE(render_->NeedsPaint());
}

TEST_F(AttributeInvalidationTest, MultipleOpacityWritesKeepLastEffectValue) {
  const int paints = render_->paint_count;
  BeginOpacity(0.25f);
  view_->SetAttribute("opacity", Value(0.75f));
  view_->DidUpdateAttributes();
  Paint();
  EXPECT_EQ(render_->paint_count, paints);
  EXPECT_EQ(GroupAlpha(), static_cast<int>(0.75f * 255));
}

TEST_F(AttributeInvalidationTest, ExistingImageInvalidationIsPreserved) {
  const int paints = render_->paint_count;
  render_->SetImageOpacity(0.4f);
  SetOpacity(0.25f);
  EXPECT_TRUE(render_->NeedsPaint());
  Paint();
  EXPECT_EQ(render_->paint_count, paints + 1);
}

TEST_F(AttributeInvalidationTest, ImageResourceReplacementKeepsPendingRepaint) {
  const int paints = render_->paint_count;
  render_->SetImage(nullptr);
  SetOpacity(0.25f);
  EXPECT_TRUE(render_->NeedsPaint());
  Paint();
  EXPECT_EQ(render_->paint_count, paints + 1);
}

#ifdef ENABLE_ACCESSIBILITY
TEST_F(AttributeInvalidationTest, OpacityOnlyUpdateStillQueuesSemantics) {
  EnableSemantics();
  const int paints = render_->paint_count;
  SetOpacity(0.25f);
  EXPECT_TRUE(SemanticsUpdateQueued());
  Paint();
  EXPECT_EQ(render_->paint_count, paints);
}
#endif

TEST_F(AttributeInvalidationTest, LayoutInvalidationIsPreserved) {
  const int paints = render_->paint_count;
  view_->SetBound(0, 0, 120, 100);
  SetOpacity(0.25f);
  EXPECT_TRUE(render_->NeedsPaint());
  Paint();
  EXPECT_EQ(render_->paint_count, paints + 1);
}

TEST_F(AttributeInvalidationTest, SourceUpdateInvalidatesContent) {
  BeginOpacity(0.25f);
  view_->SetAttribute("src", Value(""));
  view_->DidUpdateAttributes();
  EXPECT_TRUE(render_->NeedsPaint());
}

TEST_F(AttributeInvalidationTest, PlaceholderUpdateInvalidatesContent) {
  BeginOpacity(0.25f);
  view_->SetAttribute("placeholder", Value(""));
  view_->DidUpdateAttributes();
  EXPECT_TRUE(render_->NeedsPaint());
}

TEST_F(AttributeInvalidationTest, OpaqueEndpointRebuildsRepaintBoundary) {
  const int paints = render_->paint_count;
  SetOpacity(1.f);
  EXPECT_TRUE(render_->NeedsPaint());
  Paint();
  EXPECT_GT(render_->paint_count, paints);
  SetOpacity(0.5f);
  Paint();
  EXPECT_EQ(GroupAlpha(), static_cast<int>(0.5f * 255));
}

TEST_F(AttributeInvalidationTest, TransparentEndpointRetainsGroupSemantics) {
  SetOpacity(0.f);
  Paint();
  EXPECT_EQ(GroupAlpha(), 0);
  SetOpacity(0.75f);
  Paint();
  EXPECT_EQ(GroupAlpha(), static_cast<int>(0.75f * 255));
}

TEST_F(AttributeInvalidationTest, UnchangedOpacityKeepsContent) {
  const int paints = render_->paint_count;
  SetOpacity(0.5f);
  Paint();
  EXPECT_EQ(render_->paint_count, paints);
}

TEST_F(AttributeInvalidationTest,
       ExplicitBoundaryStopsParentButStillRepaintsImage) {
  render_->SetRepaintBoundary(true);
  Paint();
  ASSERT_FALSE(root_.NeedsPaint());
  ASSERT_FALSE(root_.NeedsEffect());
  const int paints = render_->paint_count;
  BeginOpacity(0.25f);
  EXPECT_FALSE(root_.NeedsPaint());
  EXPECT_FALSE(root_.NeedsEffect());
  EXPECT_FALSE(render_->NeedsPaint());
  EXPECT_TRUE(render_->NeedsEffect());
  ASSERT_NE(render_->GetContainerLayer(), nullptr);
  view_->BeforeSetAttribute(Value::Map{});
  view_->DidUpdateAttributes();
  EXPECT_FALSE(root_.NeedsPaint());
  EXPECT_FALSE(root_.NeedsEffect());
  EXPECT_TRUE(render_->NeedsPaint());
  EXPECT_EQ(render_->GetContainerLayer(), nullptr);
  Paint();
  EXPECT_EQ(render_->paint_count, paints + 1);
}

TEST_F(AttributeInvalidationTest, RetainedOpacityUpdatesOnlyImageEffect) {
  const auto* picture = ContentPicture(render_->GetContainerLayer());
  const int paints = render_->paint_count;
  SetOpacity(0.25f);
  EXPECT_FALSE(root_.NeedsPaint());
  EXPECT_FALSE(root_.NeedsEffect());
  EXPECT_FALSE(render_->NeedsPaint());
  EXPECT_TRUE(render_->NeedsEffect());
  Paint();
  EXPECT_EQ(render_->paint_count, paints);
  EXPECT_EQ(ContentPicture(render_->GetContainerLayer()), picture);
}

TEST_F(AttributeInvalidationTest,
       EffectBoundaryTransitionRequiresParentRebuild) {
  ASSERT_TRUE(render_->IsRepaintBoundary());
  render_->SetOpacity(1.f);
  EXPECT_FALSE(render_->IsRepaintBoundary());
  EXPECT_TRUE(root_.NeedsPaint());
  Paint();
  ASSERT_FALSE(root_.NeedsPaint());
  ASSERT_EQ(render_->GetContainerLayer(), nullptr);
  render_->SetOpacity(0.25f);
  EXPECT_TRUE(render_->IsRepaintBoundary());
  EXPECT_TRUE(root_.NeedsPaint());
  EXPECT_TRUE(render_->NeedsPaint());
  Paint();
  EXPECT_EQ(GroupAlpha(), static_cast<int>(0.25f * 255));
}

TEST_F(AttributeInvalidationTest, ViewUsesEffectOnlyInvalidation) {
  View other(2, page_.get());
  CheckComponentContentRetention(other);
}

TEST_F(AttributeInvalidationTest, TextViewUsesEffectOnlyInvalidation) {
  TextView other(2, page_.get());
  CheckComponentContentRetention(other);
}

TEST_F(AttributeInvalidationTest, ScrollViewKeepsItsInvalidationHook) {
  ScrollView other(2, page_.get());
  CheckComponentContentRetention(other);
}

TEST_F(AttributeInvalidationTest, FlattenedInlineTextRepaintsParentContent) {
  BaseView other(2, "inline-text", std::make_unique<RenderInlineText>(),
                 page_.get());
  other.SetBound(0, 0, 100, 100);
  auto* object = other.render_object();
  root_.AddChild(object);
  Paint();
  ASSERT_FALSE(root_.NeedsPaint());
  Value::Map attributes;
  attributes.emplace("opacity", Value(0.25f));
  other.BeforeSetAttribute(attributes);
  other.SetAttribute("opacity", Value(0.25f));
  other.DidUpdateAttributes();
  EXPECT_FALSE(object->IsRepaintBoundary());
  EXPECT_TRUE(root_.NeedsPaint());
#ifdef ENABLE_ACCESSIBILITY
  page_->GetSemanticsOwner()->RemoveDirtySemanticsForDescendants(&other);
#endif
  root_.RemoveChild(object);
  object->SetRenderer(nullptr);
}

TEST_F(AttributeInvalidationTest, TransformOriginRetainsExistingContent) {
  lynx::gfx::TransformOperations transform;
  transform.AppendScale(2.f, 2.f);
  view_->SetTransform(transform, FloatPoint(0.f, 0.f));
  Paint();
  const auto* picture = ContentPicture(render_->GetContainerLayer());
  const int paints = render_->paint_count;
  Value::Map attributes;
  attributes.emplace("transform-origin", Value(Value::Array{}));
  view_->BeforeSetAttribute(attributes);
  view_->SetTransformOrigin(std::vector<Length>{
      Length(25.f, LengthUnit::kNum), Length(50.f, LengthUnit::kNum)});
  view_->DidUpdateAttributes();
  EXPECT_FALSE(render_->NeedsPaint());
  EXPECT_TRUE(render_->NeedsEffect());
  Paint();
  EXPECT_EQ(render_->paint_count, paints);
  EXPECT_EQ(ContentPicture(render_->GetContainerLayer()), picture);
}

TEST_F(AttributeInvalidationTest, MultipleEffectPropertiesRetainContent) {
  const int paints = render_->paint_count;
  Value::Map attributes;
  attributes.emplace("opacity", Value(0.25f));
  attributes.emplace("transform-origin", Value(Value::Array{}));
  view_->BeforeSetAttribute(attributes);
  view_->SetAttribute("opacity", Value(0.25f));
  view_->SetAttribute("transform-origin", Value(Value::Array{}));
  view_->DidUpdateAttributes();
  EXPECT_FALSE(render_->NeedsPaint());
  Paint();
  EXPECT_EQ(render_->paint_count, paints);
}

TEST_F(AttributeInvalidationTest, TransformAndOpacityRetainContent) {
  const auto* picture = ContentPicture(render_->GetContainerLayer());
  ASSERT_NE(picture, nullptr);
  const int paints = render_->paint_count;
  SetTransform(TransformValue());
  EXPECT_TRUE(render_->HasTransform());
  EXPECT_FALSE(render_->NeedsPaint());
  EXPECT_TRUE(render_->NeedsEffect());
  Paint();
  EXPECT_EQ(render_->paint_count, paints);
  EXPECT_EQ(ContentPicture(render_->GetContainerLayer()), picture);
  SetTransform(TransformValue(ClayTransformType::kScale, 0.9f), true);
  EXPECT_FALSE(render_->NeedsPaint());
  Paint();
  EXPECT_EQ(render_->paint_count, paints);
  EXPECT_EQ(ContentPicture(render_->GetContainerLayer()), picture);
  EXPECT_EQ(GroupAlpha(), static_cast<int>(0.25f * 255));
}

TEST_F(AttributeInvalidationTest,
       TransformWithContentOrAnimationBatchRepaints) {
  for (const char* attribute : {"mode", "transition", "animation"}) {
    Value::Map attributes;
    attributes.emplace("transform", TransformValue());
    attributes.emplace(attribute, Value(Value::Array{}));
    view_->BeforeSetAttribute(attributes);
    // Animation properties can be consumed by the adaptor before setters run.
    view_->SetAttribute("transform", TransformValue());
    view_->DidUpdateAttributes();
    EXPECT_TRUE(render_->NeedsPaint());
    Paint();
  }
}

TEST_F(AttributeInvalidationTest, TransformIdentityPreservesSetterRepaint) {
  SetTransform(TransformValue());
  Paint();
  const int paints = render_->paint_count;
  BeginTransform(Value(Value::Array{}), true);
  ASSERT_TRUE(render_->NeedsPaint());
  view_->DidUpdateAttributes();
  EXPECT_TRUE(render_->NeedsPaint());
  Paint();
  EXPECT_EQ(render_->paint_count, paints + 1);
}

TEST_F(AttributeInvalidationTest, TransformBoundaryTransitionRepaintsParent) {
  SetOpacity(1.f);
  Paint();
  ASSERT_FALSE(render_->IsRepaintBoundary());
  ASSERT_FALSE(root_.NeedsPaint());
  SetTransform(TransformValue());
  EXPECT_TRUE(render_->IsRepaintBoundary());
  EXPECT_TRUE(render_->NeedsPaint());
  EXPECT_TRUE(root_.NeedsPaint());
  Paint();
  SetTransform(Value(Value::Array{}));
  EXPECT_FALSE(render_->IsRepaintBoundary());
  EXPECT_TRUE(root_.NeedsPaint());
}

TEST_F(AttributeInvalidationTest, TransformPreservesResourceAndLayoutRepaint) {
  for (bool layout : {false, true}) {
    const int paints = render_->paint_count;
    if (layout) {
      view_->SetBound(0, 0, 120, 100);
    } else {
      render_->SetImage(nullptr);
    }
    ASSERT_TRUE(render_->NeedsPaint());
    SetTransform(TransformValue(), true);
    EXPECT_TRUE(render_->NeedsPaint());
    Paint();
    EXPECT_EQ(render_->paint_count, paints + 1);
  }
}

TEST_F(AttributeInvalidationTest, TransformWithSourceOrPlaceholderRepaints) {
  for (const char* attribute : {"src", "placeholder"}) {
    BeginTransform(TransformValue(), true);
    view_->SetAttribute(attribute, Value(""));
    view_->DidUpdateAttributes();
    EXPECT_TRUE(render_->NeedsPaint());
    Paint();
  }
}

TEST_F(AttributeInvalidationTest, TransformWithOpaqueEndpointRepaints) {
  BeginTransform(TransformValue());
  view_->SetAttribute("opacity", Value(1.f));
  ASSERT_TRUE(render_->NeedsPaint());
  view_->DidUpdateAttributes();
  EXPECT_TRUE(render_->NeedsPaint());
}

TEST_F(AttributeInvalidationTest, TransformDepthChangeRepaintsParentOrder) {
  RenderContainer sibling;
  root_.AddChild(&sibling);
  Paint();
  // Painting populates the cached child order before changing translateZ.
  EXPECT_FALSE(root_.NeedsPaint());
  SetTransform(TransformValue(ClayTransformType::kTranslateZ, 10.f), true);
  EXPECT_FLOAT_EQ(render_->GetTranslateZ(), 10.f);
  EXPECT_TRUE(root_.NeedsPaint());
  Paint();
  SetTransform(TransformValue(ClayTransformType::kTranslateZ, -10.f), true);
  EXPECT_FLOAT_EQ(render_->GetTranslateZ(), -10.f);
  EXPECT_TRUE(root_.NeedsPaint());
  root_.RemoveChild(&sibling);
  sibling.SetRenderer(nullptr);
}

TEST_F(AttributeInvalidationTest, TransformRasterAnimationRepaintsContent) {
  render_->MarkHasAnimation(ClayAnimationPropertyType::kTransform, true);
  SetTransform(TransformValue(), true);
  EXPECT_TRUE(render_->NeedsPaint());
  render_->MarkHasAnimation(ClayAnimationPropertyType::kTransform, false);
}

TEST_F(AttributeInvalidationTest, TransformRasterTransitionRepaintsContent) {
  render_->MarkHasTransition(ClayAnimationPropertyType::kTransform, true);
  SetTransform(TransformValue(), true);
  EXPECT_TRUE(render_->NeedsPaint());
  render_->MarkHasTransition(ClayAnimationPropertyType::kTransform, false);
}

TEST_F(AttributeInvalidationTest,
       UnknownAttributeBatchUsesContentInvalidation) {
  Value::Map attributes;
  attributes.emplace("custom-component-property", Value(1));
  view_->BeforeSetAttribute(attributes);
  view_->DidUpdateAttributes();
  EXPECT_TRUE(render_->NeedsPaint());
}

TEST_F(AttributeInvalidationTest, DirectInvalidationAfterBatchRepaintsContent) {
  SetOpacity(0.25f);
  Paint();
  view_->MarkDirty();
  EXPECT_TRUE(render_->NeedsPaint());
}

TEST_F(AttributeInvalidationTest, RasterTransitionKeepsContentInvalidation) {
  render_->MarkHasTransition(ClayAnimationPropertyType::kOpacity, true);
  SetOpacity(0.25f);
  EXPECT_TRUE(render_->NeedsPaint());
  render_->MarkHasTransition(ClayAnimationPropertyType::kOpacity, false);
}

TEST_F(AttributeInvalidationTest, RasterAnimationKeepsContentInvalidation) {
  render_->MarkHasAnimation(ClayAnimationPropertyType::kTransform, true);
  SetOpacity(0.25f);
  EXPECT_TRUE(render_->NeedsPaint());
  render_->MarkHasAnimation(ClayAnimationPropertyType::kTransform, false);
}

TEST_F(AttributeInvalidationTest,
       TransformOriginWithoutBoundaryRepaintsParent) {
  render_->SetOpacity(1.f);
  Paint();
  ASSERT_FALSE(render_->IsRepaintBoundary());
  ASSERT_FALSE(root_.NeedsPaint());
  Value::Map attributes;
  attributes.emplace("transform-origin", Value(Value::Array{}));
  view_->BeforeSetAttribute(attributes);
  view_->SetAttribute("transform-origin", Value(Value::Array{}));
  view_->DidUpdateAttributes();
  EXPECT_TRUE(root_.NeedsPaint());
}

}  // namespace clay
