// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_ANIMATION_TRACE_EVENT_DEF_H_
#define CORE_ANIMATION_ANIMATION_TRACE_EVENT_DEF_H_
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE

static constexpr const char* const ANIMATION_PLAY = "Animation::Play";

static constexpr const char* const ANIMATION_PAUSE = "Animation::Pause";

static constexpr const char* const ANIMATION_STOP = "Animation::Stop";

static constexpr const char* const ANIMATION_DESTORY = "Animation::Destroy";

static constexpr const char* const ANIMATION_REQUEST_NEXT_FRAME =
    "Animation::RequestNextFrame";
static constexpr const char* const ANIMATION_DOFRAME = "Animation::DoFrame";

static constexpr const char* const KEYFRAME_MANAGER_TICK_ALL_ANIMATION =
    "CSSKeyframeManager::TickAllAnimation";
static constexpr const char* const KEYFRAME_MANAGER_NEEDS_ANIMATION_RECALC =
    "CSSKeyframeManager::SetNeedsAnimationStyleRecalc";

static constexpr const char* const TRANSITION_MANAGER_NEEDS_ANIMATION_RECALC =
    "CSSTransitionManager::TickAllAnimation";

static constexpr const char* const KEYFRAME_EFFECT_TICK_MODEL =
    "KeyframeEffect::TickKeyframeModel";
static constexpr const char* const KEYFRAME_EFFECT_CHECK_HAS_FINISHED =
    "KeyframeEffect::CheckHasFinished";
static constexpr const char* const KEYFRAME_EFFECT_CLEAR_EFFECT =
    "KeyframeEffect::ClearEffect";

static constexpr const char* const KEYFRAME_LAYOUT_ANIMATION_CURVE_GET_VALUE =
    "KeyframedLayoutAnimationCurve::GetValue";
static constexpr const char* const KEYFRAME_OPACITY_ANIMATION_CURVE_GET_VALUE =
    "KeyframedOpacityAnimationCurve::GetValue";
static constexpr const char* const KEYFRAME_COLOR_ANIMATION_CURVE_GET_VALUE =
    "KeyframedColorAnimationCurve::GetValue";
static constexpr const char* const KEYFRAME_FONT_ANIMATION_CURVE_GET_VALUE =
    "KeyframedFloatAnimationCurve::GetValue";
static constexpr const char* const KEYFRAME_FILTER_ANIMATION_CURVE_GET_VALUE =
    "KeyframedFilterAnimationCurve::GetValue";
static constexpr const char* const
    KEYFRAME_TRANSFORM_ANIMATION_CURVE_GET_VALUE =
        "KeyframedTransformAnimationCurve::GetValue";
static constexpr const char* const
    KEYFRAME_BACKGROUND_POSITION_ANIMATION_CURVE_GET_VALUE =
        "KeyframedBackgroundPositionAnimationCurve::GetValue";
static constexpr const char* const ELEMENT_ANIMATE =
    "RendererFunction::ElementAnimate";

#endif  // #if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE

#endif  // CORE_ANIMATION_ANIMATION_TRACE_EVENT_DEF_H_
