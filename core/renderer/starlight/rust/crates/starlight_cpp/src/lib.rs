// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

//! C++ Starlight baseline adapter.
//!
//! This crate is the Rust workspace boundary for importing the existing C++
//! Starlight implementation. It intentionally keeps the public API in terms of
//! `starlight_layout` types so parity tests and benchmarks can compare C++ and
//! Rust without depending on C++ ownership details.
//!
//! The native glue is feature-gated behind `native-standalone` and uses the
//! repository's existing `starlight_standalone` C API. The public Rust API stays
//! safe; only the native backend module crosses the FFI boundary.

#![cfg_attr(not(feature = "native-standalone"), forbid(unsafe_code))]
#![deny(unsafe_op_in_unsafe_fn)]

use std::error::Error;
use std::fmt::{Display, Formatter};

use starlight_layout::{Constraints, Direction, LayoutResult, LayoutTree, SimpleTree, Size};

#[cfg(all(
    feature = "native-standalone",
    any(starlight_cpp_native_standalone, starlight_cpp_native_standalone_check)
))]
#[cfg_attr(
    all(
        starlight_cpp_native_standalone_check,
        not(starlight_cpp_native_standalone)
    ),
    allow(dead_code)
)]
mod native;

#[derive(Debug, Clone, Eq, PartialEq)]
pub enum CppBaselineError {
    NativeFeatureDisabled,
    NativeLinkUnavailable,
    UnsupportedStyle(&'static str),
    NativeLayoutUnavailable(&'static str),
}

impl Display for CppBaselineError {
    fn fmt(&self, formatter: &mut Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::NativeFeatureDisabled => formatter
                .write_str("C++ Starlight native backend is disabled; enable native-standalone"),
            Self::NativeLinkUnavailable => formatter.write_str(
                "C++ Starlight native backend is enabled but no standalone native library is configured",
            ),
            Self::UnsupportedStyle(reason) => {
                write!(
                    formatter,
                    "C++ Starlight standalone backend does not support {reason}"
                )
            }
            Self::NativeLayoutUnavailable(reason) => {
                write!(
                    formatter,
                    "C++ Starlight native layout unavailable: {reason}"
                )
            }
        }
    }
}

impl Error for CppBaselineError {}

/// Stable checkpoints used by the C++ public standalone tree-mutation probe.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum StandalonePublicTreeStage {
    AfterAppend,
    AfterCleanLayout,
    AfterNoopRemove,
    AfterIndexInsert,
    AfterCleanInsertedLayout,
    AfterReparentToStaging,
    AfterInsertBefore,
    AfterRemoveChild,
    AfterRemoveAllChildren,
    AfterResetStaging,
}

/// Snapshot of public standalone tree state.
///
/// Node ids are fixed to the probe's creation order:
/// root, first, second, third, inserted, staging.
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct StandalonePublicTreeSnapshot {
    pub stage: StandalonePublicTreeStage,
    pub root_children: Vec<usize>,
    pub staging_children: Vec<usize>,
    pub parents: Vec<Option<usize>>,
    pub dirty: Vec<bool>,
}

/// Snapshot of public standalone explicit dirty-state APIs.
///
/// Dirty arrays use node order: root, child, grandchild.
#[derive(Clone, Debug, PartialEq)]
pub struct StandalonePublicDirtySnapshot {
    pub after_clean_layout: Vec<bool>,
    pub after_mark_grandchild: Vec<bool>,
    pub after_reclean_layout: Vec<bool>,
    pub after_mark_root: Vec<bool>,
}

/// Public standalone style-edge probe cases.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum StandalonePublicStyleStage {
    Ltr,
    Rtl,
}

/// C ABI-shaped representation of `StarlightValue`.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct StandalonePublicLengthValue {
    pub value: f32,
    pub unit: i32,
    pub percentage: f32,
    pub flags: i32,
}

impl StandalonePublicLengthValue {
    pub const UNIT_POINT: i32 = 0;
    pub const UNIT_PERCENT: i32 = 1;
    pub const UNIT_AUTO: i32 = 2;
    pub const UNIT_MAX_CONTENT: i32 = 3;
    pub const UNIT_FIT_CONTENT: i32 = 4;
    pub const UNIT_FR: i32 = 5;
    pub const UNIT_CALC: i32 = 6;
    pub const FLAG_HAS_VALUE: i32 = 1 << 0;
    pub const FLAG_HAS_PERCENTAGE: i32 = 1 << 1;

    #[must_use]
    pub const fn default_point() -> Self {
        Self {
            value: 0.0,
            unit: Self::UNIT_POINT,
            percentage: 0.0,
            flags: 0,
        }
    }

    #[must_use]
    pub const fn points(value: f32) -> Self {
        Self {
            value,
            unit: Self::UNIT_POINT,
            percentage: 0.0,
            flags: Self::FLAG_HAS_VALUE,
        }
    }

    #[must_use]
    pub const fn percent(value: f32) -> Self {
        Self {
            value,
            unit: Self::UNIT_PERCENT,
            percentage: value,
            flags: Self::FLAG_HAS_VALUE | Self::FLAG_HAS_PERCENTAGE,
        }
    }

    #[must_use]
    pub const fn calc(fixed: f32, percentage: f32) -> Self {
        Self {
            value: fixed,
            unit: Self::UNIT_CALC,
            percentage,
            flags: Self::FLAG_HAS_VALUE | Self::FLAG_HAS_PERCENTAGE,
        }
    }

    #[must_use]
    pub const fn auto() -> Self {
        Self {
            value: 0.0,
            unit: Self::UNIT_AUTO,
            percentage: 0.0,
            flags: 0,
        }
    }
}

/// Snapshot of public standalone edge and gap style getters.
///
/// Edge arrays use public `SLEdge` order: left, right, top, bottom, start,
/// end, horizontal, vertical, all. Gap arrays use public `SLGap` order:
/// column, row, all.
#[derive(Clone, Debug, PartialEq)]
pub struct StandalonePublicEdgeStyleSnapshot {
    pub stage: StandalonePublicStyleStage,
    pub position: Vec<StandalonePublicLengthValue>,
    pub margin: Vec<StandalonePublicLengthValue>,
    pub padding: Vec<StandalonePublicLengthValue>,
    pub border: Vec<f32>,
    pub gap: Vec<StandalonePublicLengthValue>,
    pub dirty: bool,
}

/// Snapshot of public standalone edge and gap setter variants.
#[derive(Clone, Debug, PartialEq)]
pub struct StandalonePublicEdgeStyleVariantSnapshot {
    pub position: StandalonePublicLengthValue,
    pub margin: StandalonePublicLengthValue,
    pub padding: StandalonePublicLengthValue,
    pub gap: StandalonePublicLengthValue,
    pub dirty: bool,
}

/// Snapshot of public standalone scalar and enum style getters after setters.
#[derive(Clone, Debug, PartialEq)]
pub struct StandalonePublicScalarStyleSnapshot {
    pub flex_direction: i32,
    pub justify_content: i32,
    pub align_content: i32,
    pub align_items: i32,
    pub align_self: i32,
    pub position_type: i32,
    pub flex_wrap: i32,
    pub linear_orientation: i32,
    pub linear_gravity: i32,
    pub linear_layout_gravity: i32,
    pub linear_cross_gravity: i32,
    pub relative_center: i32,
    pub grid_auto_flow: i32,
    pub justify_items: i32,
    pub justify_self: i32,
    pub display: i32,
    pub box_sizing: i32,
    pub aspect_ratio: f32,
    pub order: i32,
    pub relative_id: i32,
    pub relative_align_top: i32,
    pub relative_align_right: i32,
    pub relative_align_bottom: i32,
    pub relative_align_left: i32,
    pub relative_top_of: i32,
    pub relative_right_of: i32,
    pub relative_bottom_of: i32,
    pub relative_left_of: i32,
    pub relative_layout_once: bool,
    pub grid_column_start: i32,
    pub grid_column_end: i32,
    pub grid_row_start: i32,
    pub grid_row_end: i32,
    pub grid_column_span: i32,
    pub grid_row_span: i32,
    pub flex_grow: f32,
    pub flex_shrink: f32,
    pub linear_weight: f32,
    pub linear_weight_sum: f32,
    pub dirty: bool,
}

/// Snapshot of public standalone dimension and flex-basis style getters.
#[derive(Clone, Debug, PartialEq)]
pub struct StandalonePublicDimensionStyleSnapshot {
    pub flex_basis: StandalonePublicLengthValue,
    pub width: StandalonePublicLengthValue,
    pub height: StandalonePublicLengthValue,
    pub min_width: StandalonePublicLengthValue,
    pub max_width: StandalonePublicLengthValue,
    pub min_height: StandalonePublicLengthValue,
    pub max_height: StandalonePublicLengthValue,
    pub dirty: bool,
}

/// Snapshot of public standalone dimension/flex-basis setter layout effects.
#[derive(Clone, Debug, PartialEq)]
pub struct StandalonePublicDimensionLayoutSnapshot {
    pub nodes: Vec<StandalonePublicLayoutNodeSnapshot>,
}

/// Snapshot of public standalone direction setter/query behavior.
#[derive(Clone, Debug, PartialEq)]
pub struct StandalonePublicDirectionSnapshot {
    pub default_is_rtl: bool,
    pub rtl_is_rtl: bool,
    pub ltr_is_rtl: bool,
    pub dirty_after_direction_updates: bool,
}

/// Snapshot of public standalone direction setter layout effects.
#[derive(Clone, Debug, PartialEq)]
pub struct StandalonePublicDirectionLayoutSnapshot {
    pub nodes: Vec<StandalonePublicLayoutNodeSnapshot>,
}

/// Snapshot of public standalone layout getters for one laid-out node.
#[derive(Clone, Debug, PartialEq)]
pub struct StandalonePublicLayoutNodeSnapshot {
    pub left: f32,
    pub top: f32,
    pub width: f32,
    pub height: f32,
    pub baseline: f32,
    pub margin: Vec<f32>,
    pub padding: Vec<f32>,
    pub border: Vec<f32>,
    pub sticky_position: Vec<f32>,
}

/// Snapshot of public standalone layout getters after layout.
#[derive(Clone, Debug, PartialEq)]
pub struct StandalonePublicLayoutGetterSnapshot {
    pub root: StandalonePublicLayoutNodeSnapshot,
    pub child: StandalonePublicLayoutNodeSnapshot,
}

/// Snapshot of public standalone box-sizing/aspect-ratio setter layout effects.
#[derive(Clone, Debug, PartialEq)]
pub struct StandalonePublicBoxAspectLayoutSnapshot {
    pub nodes: Vec<StandalonePublicLayoutNodeSnapshot>,
}

/// Snapshot of public standalone display setter layout effects.
#[derive(Clone, Debug, PartialEq)]
pub struct StandalonePublicDisplayLayoutSnapshot {
    pub nodes: Vec<StandalonePublicLayoutNodeSnapshot>,
}

/// Snapshot of public standalone position-type setter layout effects.
#[derive(Clone, Debug, PartialEq)]
pub struct StandalonePublicPositionLayoutSnapshot {
    pub nodes: Vec<StandalonePublicLayoutNodeSnapshot>,
}

/// Snapshot of public standalone relative setter layout effects.
#[derive(Clone, Debug, PartialEq)]
pub struct StandalonePublicRelativeLayoutSnapshot {
    pub nodes: Vec<StandalonePublicLayoutNodeSnapshot>,
}

/// Snapshot of public standalone linear setter layout effects.
#[derive(Clone, Debug, PartialEq)]
pub struct StandalonePublicLinearLayoutSnapshot {
    pub nodes: Vec<StandalonePublicLayoutNodeSnapshot>,
}

/// Snapshot of public standalone linear/list setter layout effects.
#[derive(Clone, Debug, PartialEq)]
pub struct StandalonePublicLinearListLayoutSnapshot {
    pub nodes: Vec<StandalonePublicLayoutNodeSnapshot>,
}

/// Snapshot of public standalone flex setter layout effects.
#[derive(Clone, Debug, PartialEq)]
pub struct StandalonePublicFlexLayoutSnapshot {
    pub nodes: Vec<StandalonePublicLayoutNodeSnapshot>,
}

/// Snapshot of public standalone edge/gap setter layout effects.
#[derive(Clone, Debug, PartialEq)]
pub struct StandalonePublicEdgeLayoutSnapshot {
    pub nodes: Vec<StandalonePublicLayoutNodeSnapshot>,
}

/// Snapshot of public standalone grid track-vector setter layout effects.
#[derive(Clone, Debug, PartialEq)]
pub struct StandalonePublicGridTrackLayoutSnapshot {
    pub nodes: Vec<StandalonePublicLayoutNodeSnapshot>,
}

/// Snapshot of public standalone grid alignment setter layout effects.
#[derive(Clone, Debug, PartialEq)]
pub struct StandalonePublicGridAlignmentLayoutSnapshot {
    pub nodes: Vec<StandalonePublicLayoutNodeSnapshot>,
}

/// Snapshot of public standalone config APIs and their layout effect.
#[derive(Clone, Debug, PartialEq)]
pub struct StandalonePublicConfigSnapshot {
    pub default_config_physical_pixels_per_layout_unit: f32,
    pub updated_config_physical_pixels_per_layout_unit: f32,
    pub default_node_width: f32,
    pub default_node_height: f32,
    pub configured_node_width: f32,
    pub configured_node_height: f32,
}

/// Snapshot of public standalone measure-delegate APIs.
#[derive(Clone, Debug, PartialEq)]
pub struct StandalonePublicMeasureDelegateSnapshot {
    pub initial_delegate_is_null: bool,
    pub initial_has_measure_func: bool,
    pub delegate_round_trips: bool,
    pub after_set_has_measure_func: bool,
    pub measure_call_count: usize,
    pub baseline_call_count: usize,
    pub measure_width: f32,
    pub measure_width_mode: i32,
    pub measure_height: f32,
    pub measure_height_mode: i32,
    pub baseline_width: f32,
    pub baseline_height: f32,
    pub layout_width: f32,
    pub layout_height: f32,
    pub layout_baseline: f32,
    pub after_clear_delegate_is_null: bool,
    pub after_clear_has_measure_func: bool,
}

/// Snapshot of one public standalone layout entry point call.
#[derive(Clone, Debug, PartialEq)]
pub struct StandalonePublicLayoutEntrypointStageSnapshot {
    pub measure_call_count: usize,
    pub measure_width: f32,
    pub measure_width_mode: i32,
    pub measure_height: f32,
    pub measure_height_mode: i32,
    pub layout_width: f32,
    pub layout_height: f32,
}

/// Snapshot of public standalone layout entry point mode semantics.
#[derive(Clone, Debug, PartialEq)]
pub struct StandalonePublicLayoutEntrypointSnapshot {
    pub finite_owner: StandalonePublicLayoutEntrypointStageSnapshot,
    pub sentinel_undefined_owner: StandalonePublicLayoutEntrypointStageSnapshot,
    pub at_most_owner: StandalonePublicLayoutEntrypointStageSnapshot,
    pub undefined_owner: StandalonePublicLayoutEntrypointStageSnapshot,
}

#[derive(Debug, Default)]
pub struct CppStarlightEngine;

impl CppStarlightEngine {
    #[must_use]
    pub fn new() -> Self {
        Self
    }

    /// Runs the C++ Starlight baseline for a tree that implements
    /// `LayoutTree`.
    ///
    /// The default build returns a typed error until the native backend feature
    /// is enabled and linked. With `native-standalone` plus native link
    /// metadata, supported flex/box, linear, relative, and grid trees are
    /// mirrored into C++ Starlight and results are written back to `tree`.
    ///
    /// # Errors
    ///
    /// Returns `CppBaselineError::NativeFeatureDisabled` unless the backend
    /// feature is enabled, or `NativeLinkUnavailable` if the feature is enabled
    /// without link metadata. The standalone backend can also report
    /// `UnsupportedStyle` for Starlight features not exposed by that C API.
    pub fn layout<T: LayoutTree>(
        &mut self,
        tree: &mut T,
        root: T::NodeId,
        constraints: Constraints,
    ) -> Result<Size, CppBaselineError> {
        #[cfg(all(feature = "native-standalone", starlight_cpp_native_standalone))]
        {
            native::layout_standalone(tree, root, constraints)
        }

        #[cfg(all(feature = "native-standalone", not(starlight_cpp_native_standalone)))]
        {
            let _ = (tree, root, constraints);
            Err(CppBaselineError::NativeLinkUnavailable)
        }

        #[cfg(not(feature = "native-standalone"))]
        {
            let _ = (tree, root, constraints);
            Err(CppBaselineError::NativeFeatureDisabled)
        }
    }

    /// Runs the C++ Starlight baseline with an explicit owner direction.
    ///
    /// This is needed for the standalone `calculate_layout_with_mode` contract:
    /// nodes that have not explicitly set their own `direction` inherit the
    /// owner direction only for the layout pass.
    ///
    /// # Errors
    ///
    /// Returns the same errors as [`Self::layout`].
    pub fn layout_with_owner_direction<T: LayoutTree>(
        &mut self,
        tree: &mut T,
        root: T::NodeId,
        constraints: Constraints,
        owner_direction: Direction,
    ) -> Result<Size, CppBaselineError> {
        #[cfg(all(feature = "native-standalone", starlight_cpp_native_standalone))]
        {
            native::layout_standalone_with_owner_direction(tree, root, constraints, owner_direction)
        }

        #[cfg(all(feature = "native-standalone", not(starlight_cpp_native_standalone)))]
        {
            let _ = (tree, root, constraints, owner_direction);
            Err(CppBaselineError::NativeLinkUnavailable)
        }

        #[cfg(not(feature = "native-standalone"))]
        {
            let _ = (tree, root, constraints, owner_direction);
            Err(CppBaselineError::NativeFeatureDisabled)
        }
    }

    /// Runs a fixed C++ public standalone tree-mutation transcript and returns
    /// structural and dirty-state snapshots for Rust standalone API parity.
    ///
    /// # Errors
    ///
    /// Returns the same native-backend availability errors as [`Self::layout`].
    pub fn standalone_public_tree_mutation_snapshots(
        &mut self,
    ) -> Result<Vec<StandalonePublicTreeSnapshot>, CppBaselineError> {
        #[cfg(all(feature = "native-standalone", starlight_cpp_native_standalone))]
        {
            native::standalone_public_tree_mutation_snapshots()
        }

        #[cfg(all(feature = "native-standalone", not(starlight_cpp_native_standalone)))]
        {
            Err(CppBaselineError::NativeLinkUnavailable)
        }

        #[cfg(not(feature = "native-standalone"))]
        {
            Err(CppBaselineError::NativeFeatureDisabled)
        }
    }

    /// Runs fixed C++ public standalone explicit dirty-state API cases.
    ///
    /// # Errors
    ///
    /// Returns the same native-backend availability errors as [`Self::layout`].
    pub fn standalone_public_dirty_snapshot(
        &mut self,
    ) -> Result<StandalonePublicDirtySnapshot, CppBaselineError> {
        #[cfg(all(feature = "native-standalone", starlight_cpp_native_standalone))]
        {
            native::standalone_public_dirty_snapshot()
        }

        #[cfg(all(feature = "native-standalone", not(starlight_cpp_native_standalone)))]
        {
            Err(CppBaselineError::NativeLinkUnavailable)
        }

        #[cfg(not(feature = "native-standalone"))]
        {
            Err(CppBaselineError::NativeFeatureDisabled)
        }
    }

    /// Runs fixed C++ public standalone edge-style setter/getter cases.
    ///
    /// # Errors
    ///
    /// Returns the same native-backend availability errors as [`Self::layout`].
    pub fn standalone_public_edge_style_snapshots(
        &mut self,
    ) -> Result<Vec<StandalonePublicEdgeStyleSnapshot>, CppBaselineError> {
        #[cfg(all(feature = "native-standalone", starlight_cpp_native_standalone))]
        {
            native::standalone_public_edge_style_snapshots()
        }

        #[cfg(all(feature = "native-standalone", not(starlight_cpp_native_standalone)))]
        {
            Err(CppBaselineError::NativeLinkUnavailable)
        }

        #[cfg(not(feature = "native-standalone"))]
        {
            Err(CppBaselineError::NativeFeatureDisabled)
        }
    }

    /// Runs fixed C++ public standalone edge and gap setter variant cases.
    ///
    /// # Errors
    ///
    /// Returns the same native-backend availability errors as [`Self::layout`].
    pub fn standalone_public_edge_style_variant_snapshots(
        &mut self,
    ) -> Result<Vec<StandalonePublicEdgeStyleVariantSnapshot>, CppBaselineError> {
        #[cfg(all(feature = "native-standalone", starlight_cpp_native_standalone))]
        {
            native::standalone_public_edge_style_variant_snapshots()
        }

        #[cfg(all(feature = "native-standalone", not(starlight_cpp_native_standalone)))]
        {
            Err(CppBaselineError::NativeLinkUnavailable)
        }

        #[cfg(not(feature = "native-standalone"))]
        {
            Err(CppBaselineError::NativeFeatureDisabled)
        }
    }

    /// Runs a fixed C++ public standalone scalar-style setter/getter case.
    ///
    /// # Errors
    ///
    /// Returns the same native-backend availability errors as [`Self::layout`].
    pub fn standalone_public_scalar_style_snapshot(
        &mut self,
    ) -> Result<StandalonePublicScalarStyleSnapshot, CppBaselineError> {
        #[cfg(all(feature = "native-standalone", starlight_cpp_native_standalone))]
        {
            native::standalone_public_scalar_style_snapshot()
        }

        #[cfg(all(feature = "native-standalone", not(starlight_cpp_native_standalone)))]
        {
            Err(CppBaselineError::NativeLinkUnavailable)
        }

        #[cfg(not(feature = "native-standalone"))]
        {
            Err(CppBaselineError::NativeFeatureDisabled)
        }
    }

    /// Runs a fixed C++ public standalone dimension-style setter/getter case.
    ///
    /// # Errors
    ///
    /// Returns the same native-backend availability errors as [`Self::layout`].
    pub fn standalone_public_dimension_style_snapshot(
        &mut self,
    ) -> Result<StandalonePublicDimensionStyleSnapshot, CppBaselineError> {
        #[cfg(all(feature = "native-standalone", starlight_cpp_native_standalone))]
        {
            native::standalone_public_dimension_style_snapshot()
        }

        #[cfg(all(feature = "native-standalone", not(starlight_cpp_native_standalone)))]
        {
            Err(CppBaselineError::NativeLinkUnavailable)
        }

        #[cfg(not(feature = "native-standalone"))]
        {
            Err(CppBaselineError::NativeFeatureDisabled)
        }
    }

    /// Runs fixed C++ public standalone dimension and flex-basis setter variant
    /// cases.
    ///
    /// # Errors
    ///
    /// Returns the same native-backend availability errors as [`Self::layout`].
    pub fn standalone_public_dimension_style_variant_snapshots(
        &mut self,
    ) -> Result<Vec<StandalonePublicDimensionStyleSnapshot>, CppBaselineError> {
        #[cfg(all(feature = "native-standalone", starlight_cpp_native_standalone))]
        {
            native::standalone_public_dimension_style_variant_snapshots()
        }

        #[cfg(all(feature = "native-standalone", not(starlight_cpp_native_standalone)))]
        {
            Err(CppBaselineError::NativeLinkUnavailable)
        }

        #[cfg(not(feature = "native-standalone"))]
        {
            Err(CppBaselineError::NativeFeatureDisabled)
        }
    }

    /// Runs fixed C++ public standalone dimension/flex-basis setter layout
    /// cases.
    ///
    /// # Errors
    ///
    /// Returns the same native-backend availability errors as [`Self::layout`].
    pub fn standalone_public_dimension_layout_snapshots(
        &mut self,
    ) -> Result<Vec<StandalonePublicDimensionLayoutSnapshot>, CppBaselineError> {
        #[cfg(all(feature = "native-standalone", starlight_cpp_native_standalone))]
        {
            native::standalone_public_dimension_layout_snapshots()
        }

        #[cfg(all(feature = "native-standalone", not(starlight_cpp_native_standalone)))]
        {
            Err(CppBaselineError::NativeLinkUnavailable)
        }

        #[cfg(not(feature = "native-standalone"))]
        {
            Err(CppBaselineError::NativeFeatureDisabled)
        }
    }

    /// Runs a fixed C++ public standalone direction setter/query case.
    ///
    /// # Errors
    ///
    /// Returns the same native-backend availability errors as [`Self::layout`].
    pub fn standalone_public_direction_snapshot(
        &mut self,
    ) -> Result<StandalonePublicDirectionSnapshot, CppBaselineError> {
        #[cfg(all(feature = "native-standalone", starlight_cpp_native_standalone))]
        {
            native::standalone_public_direction_snapshot()
        }

        #[cfg(all(feature = "native-standalone", not(starlight_cpp_native_standalone)))]
        {
            Err(CppBaselineError::NativeLinkUnavailable)
        }

        #[cfg(not(feature = "native-standalone"))]
        {
            Err(CppBaselineError::NativeFeatureDisabled)
        }
    }

    /// Runs fixed C++ public standalone direction setter layout cases.
    ///
    /// # Errors
    ///
    /// Returns the same native-backend availability errors as [`Self::layout`].
    pub fn standalone_public_direction_layout_snapshots(
        &mut self,
    ) -> Result<Vec<StandalonePublicDirectionLayoutSnapshot>, CppBaselineError> {
        #[cfg(all(feature = "native-standalone", starlight_cpp_native_standalone))]
        {
            native::standalone_public_direction_layout_snapshots()
        }

        #[cfg(all(feature = "native-standalone", not(starlight_cpp_native_standalone)))]
        {
            Err(CppBaselineError::NativeLinkUnavailable)
        }

        #[cfg(not(feature = "native-standalone"))]
        {
            Err(CppBaselineError::NativeFeatureDisabled)
        }
    }

    /// Runs a fixed C++ public standalone layout getter case.
    ///
    /// # Errors
    ///
    /// Returns the same native-backend availability errors as [`Self::layout`].
    pub fn standalone_public_layout_getter_snapshot(
        &mut self,
    ) -> Result<StandalonePublicLayoutGetterSnapshot, CppBaselineError> {
        #[cfg(all(feature = "native-standalone", starlight_cpp_native_standalone))]
        {
            native::standalone_public_layout_getter_snapshot()
        }

        #[cfg(all(feature = "native-standalone", not(starlight_cpp_native_standalone)))]
        {
            Err(CppBaselineError::NativeLinkUnavailable)
        }

        #[cfg(not(feature = "native-standalone"))]
        {
            Err(CppBaselineError::NativeFeatureDisabled)
        }
    }

    /// Runs a fixed C++ public standalone box-sizing/aspect-ratio setter layout
    /// case.
    ///
    /// # Errors
    ///
    /// Returns the same native-backend availability errors as [`Self::layout`].
    pub fn standalone_public_box_aspect_layout_snapshot(
        &mut self,
    ) -> Result<StandalonePublicBoxAspectLayoutSnapshot, CppBaselineError> {
        #[cfg(all(feature = "native-standalone", starlight_cpp_native_standalone))]
        {
            native::standalone_public_box_aspect_layout_snapshot()
        }

        #[cfg(all(feature = "native-standalone", not(starlight_cpp_native_standalone)))]
        {
            Err(CppBaselineError::NativeLinkUnavailable)
        }

        #[cfg(not(feature = "native-standalone"))]
        {
            Err(CppBaselineError::NativeFeatureDisabled)
        }
    }

    /// Runs fixed C++ public standalone display setter layout cases.
    ///
    /// # Errors
    ///
    /// Returns the same native-backend availability errors as [`Self::layout`].
    pub fn standalone_public_display_layout_snapshots(
        &mut self,
    ) -> Result<Vec<StandalonePublicDisplayLayoutSnapshot>, CppBaselineError> {
        #[cfg(all(feature = "native-standalone", starlight_cpp_native_standalone))]
        {
            native::standalone_public_display_layout_snapshots()
        }

        #[cfg(all(feature = "native-standalone", not(starlight_cpp_native_standalone)))]
        {
            Err(CppBaselineError::NativeLinkUnavailable)
        }

        #[cfg(not(feature = "native-standalone"))]
        {
            Err(CppBaselineError::NativeFeatureDisabled)
        }
    }

    /// Runs a fixed C++ public standalone position-type setter layout case.
    ///
    /// # Errors
    ///
    /// Returns the same native-backend availability errors as [`Self::layout`].
    pub fn standalone_public_position_layout_snapshot(
        &mut self,
    ) -> Result<StandalonePublicPositionLayoutSnapshot, CppBaselineError> {
        #[cfg(all(feature = "native-standalone", starlight_cpp_native_standalone))]
        {
            native::standalone_public_position_layout_snapshot()
        }

        #[cfg(all(feature = "native-standalone", not(starlight_cpp_native_standalone)))]
        {
            Err(CppBaselineError::NativeLinkUnavailable)
        }

        #[cfg(not(feature = "native-standalone"))]
        {
            Err(CppBaselineError::NativeFeatureDisabled)
        }
    }

    /// Runs fixed C++ public standalone relative setter layout cases.
    ///
    /// # Errors
    ///
    /// Returns the same native-backend availability errors as [`Self::layout`].
    pub fn standalone_public_relative_layout_snapshots(
        &mut self,
    ) -> Result<Vec<StandalonePublicRelativeLayoutSnapshot>, CppBaselineError> {
        #[cfg(all(feature = "native-standalone", starlight_cpp_native_standalone))]
        {
            native::standalone_public_relative_layout_snapshots()
        }

        #[cfg(all(feature = "native-standalone", not(starlight_cpp_native_standalone)))]
        {
            Err(CppBaselineError::NativeLinkUnavailable)
        }

        #[cfg(not(feature = "native-standalone"))]
        {
            Err(CppBaselineError::NativeFeatureDisabled)
        }
    }

    /// Runs fixed C++ public standalone linear setter layout cases.
    ///
    /// # Errors
    ///
    /// Returns the same native-backend availability errors as [`Self::layout`].
    pub fn standalone_public_linear_layout_snapshots(
        &mut self,
    ) -> Result<Vec<StandalonePublicLinearLayoutSnapshot>, CppBaselineError> {
        #[cfg(all(feature = "native-standalone", starlight_cpp_native_standalone))]
        {
            native::standalone_public_linear_layout_snapshots()
        }

        #[cfg(all(feature = "native-standalone", not(starlight_cpp_native_standalone)))]
        {
            Err(CppBaselineError::NativeLinkUnavailable)
        }

        #[cfg(not(feature = "native-standalone"))]
        {
            Err(CppBaselineError::NativeFeatureDisabled)
        }
    }

    /// Runs a fixed C++ public standalone linear/list setter layout case.
    ///
    /// # Errors
    ///
    /// Returns the same native-backend availability errors as [`Self::layout`].
    pub fn standalone_public_linear_list_layout_snapshot(
        &mut self,
    ) -> Result<StandalonePublicLinearListLayoutSnapshot, CppBaselineError> {
        #[cfg(all(feature = "native-standalone", starlight_cpp_native_standalone))]
        {
            native::standalone_public_linear_list_layout_snapshot()
        }

        #[cfg(all(feature = "native-standalone", not(starlight_cpp_native_standalone)))]
        {
            Err(CppBaselineError::NativeLinkUnavailable)
        }

        #[cfg(not(feature = "native-standalone"))]
        {
            Err(CppBaselineError::NativeFeatureDisabled)
        }
    }

    /// Runs fixed C++ public standalone list-gap setter variant layout cases.
    ///
    /// # Errors
    ///
    /// Returns the same native-backend availability errors as [`Self::layout`].
    pub fn standalone_public_list_gap_layout_snapshots(
        &mut self,
    ) -> Result<Vec<StandalonePublicLinearListLayoutSnapshot>, CppBaselineError> {
        #[cfg(all(feature = "native-standalone", starlight_cpp_native_standalone))]
        {
            native::standalone_public_list_gap_layout_snapshots()
        }

        #[cfg(all(feature = "native-standalone", not(starlight_cpp_native_standalone)))]
        {
            Err(CppBaselineError::NativeLinkUnavailable)
        }

        #[cfg(not(feature = "native-standalone"))]
        {
            Err(CppBaselineError::NativeFeatureDisabled)
        }
    }

    /// Runs fixed C++ public standalone flex setter layout cases.
    ///
    /// # Errors
    ///
    /// Returns the same native-backend availability errors as [`Self::layout`].
    pub fn standalone_public_flex_layout_snapshots(
        &mut self,
    ) -> Result<Vec<StandalonePublicFlexLayoutSnapshot>, CppBaselineError> {
        #[cfg(all(feature = "native-standalone", starlight_cpp_native_standalone))]
        {
            native::standalone_public_flex_layout_snapshots()
        }

        #[cfg(all(feature = "native-standalone", not(starlight_cpp_native_standalone)))]
        {
            Err(CppBaselineError::NativeLinkUnavailable)
        }

        #[cfg(not(feature = "native-standalone"))]
        {
            Err(CppBaselineError::NativeFeatureDisabled)
        }
    }

    /// Runs fixed C++ public standalone edge/gap setter layout cases.
    ///
    /// # Errors
    ///
    /// Returns the same native-backend availability errors as [`Self::layout`].
    pub fn standalone_public_edge_layout_snapshots(
        &mut self,
    ) -> Result<Vec<StandalonePublicEdgeLayoutSnapshot>, CppBaselineError> {
        #[cfg(all(feature = "native-standalone", starlight_cpp_native_standalone))]
        {
            native::standalone_public_edge_layout_snapshots()
        }

        #[cfg(all(feature = "native-standalone", not(starlight_cpp_native_standalone)))]
        {
            Err(CppBaselineError::NativeLinkUnavailable)
        }

        #[cfg(not(feature = "native-standalone"))]
        {
            Err(CppBaselineError::NativeFeatureDisabled)
        }
    }

    /// Runs a fixed C++ public standalone grid track-vector setter layout case.
    ///
    /// # Errors
    ///
    /// Returns the same native-backend availability errors as [`Self::layout`].
    pub fn standalone_public_grid_track_layout_snapshot(
        &mut self,
    ) -> Result<StandalonePublicGridTrackLayoutSnapshot, CppBaselineError> {
        #[cfg(all(feature = "native-standalone", starlight_cpp_native_standalone))]
        {
            native::standalone_public_grid_track_layout_snapshot()
        }

        #[cfg(all(feature = "native-standalone", not(starlight_cpp_native_standalone)))]
        {
            Err(CppBaselineError::NativeLinkUnavailable)
        }

        #[cfg(not(feature = "native-standalone"))]
        {
            Err(CppBaselineError::NativeFeatureDisabled)
        }
    }

    /// Runs C++ public standalone grid auto-flow setter layout cases.
    ///
    /// # Errors
    ///
    /// Returns the same native-backend availability errors as [`Self::layout`].
    pub fn standalone_public_grid_auto_flow_layout_snapshots(
        &mut self,
    ) -> Result<Vec<StandalonePublicGridTrackLayoutSnapshot>, CppBaselineError> {
        #[cfg(all(feature = "native-standalone", starlight_cpp_native_standalone))]
        {
            native::standalone_public_grid_auto_flow_layout_snapshots()
        }

        #[cfg(all(feature = "native-standalone", not(starlight_cpp_native_standalone)))]
        {
            Err(CppBaselineError::NativeLinkUnavailable)
        }

        #[cfg(not(feature = "native-standalone"))]
        {
            Err(CppBaselineError::NativeFeatureDisabled)
        }
    }

    /// Runs a fixed C++ public standalone grid alignment setter layout case.
    ///
    /// # Errors
    ///
    /// Returns the same native-backend availability errors as [`Self::layout`].
    pub fn standalone_public_grid_alignment_layout_snapshot(
        &mut self,
    ) -> Result<StandalonePublicGridAlignmentLayoutSnapshot, CppBaselineError> {
        #[cfg(all(feature = "native-standalone", starlight_cpp_native_standalone))]
        {
            native::standalone_public_grid_alignment_layout_snapshot()
        }

        #[cfg(all(feature = "native-standalone", not(starlight_cpp_native_standalone)))]
        {
            Err(CppBaselineError::NativeLinkUnavailable)
        }

        #[cfg(not(feature = "native-standalone"))]
        {
            Err(CppBaselineError::NativeFeatureDisabled)
        }
    }

    /// Runs C++ public standalone grid alignment variant setter layout cases.
    ///
    /// # Errors
    ///
    /// Returns the same native-backend availability errors as [`Self::layout`].
    pub fn standalone_public_grid_alignment_variant_layout_snapshots(
        &mut self,
    ) -> Result<Vec<StandalonePublicGridAlignmentLayoutSnapshot>, CppBaselineError> {
        #[cfg(all(feature = "native-standalone", starlight_cpp_native_standalone))]
        {
            native::standalone_public_grid_alignment_variant_layout_snapshots()
        }

        #[cfg(all(feature = "native-standalone", not(starlight_cpp_native_standalone)))]
        {
            Err(CppBaselineError::NativeLinkUnavailable)
        }

        #[cfg(not(feature = "native-standalone"))]
        {
            Err(CppBaselineError::NativeFeatureDisabled)
        }
    }

    /// Runs fixed C++ public standalone layout entry point cases.
    ///
    /// # Errors
    ///
    /// Returns the same native-backend availability errors as [`Self::layout`].
    pub fn standalone_public_layout_entrypoint_snapshot(
        &mut self,
    ) -> Result<StandalonePublicLayoutEntrypointSnapshot, CppBaselineError> {
        #[cfg(all(feature = "native-standalone", starlight_cpp_native_standalone))]
        {
            native::standalone_public_layout_entrypoint_snapshot()
        }

        #[cfg(all(feature = "native-standalone", not(starlight_cpp_native_standalone)))]
        {
            Err(CppBaselineError::NativeLinkUnavailable)
        }

        #[cfg(not(feature = "native-standalone"))]
        {
            Err(CppBaselineError::NativeFeatureDisabled)
        }
    }

    /// Runs fixed C++ public standalone config API cases.
    ///
    /// # Errors
    ///
    /// Returns the same native-backend availability errors as [`Self::layout`].
    pub fn standalone_public_config_snapshot(
        &mut self,
    ) -> Result<StandalonePublicConfigSnapshot, CppBaselineError> {
        #[cfg(all(feature = "native-standalone", starlight_cpp_native_standalone))]
        {
            native::standalone_public_config_snapshot()
        }

        #[cfg(all(feature = "native-standalone", not(starlight_cpp_native_standalone)))]
        {
            Err(CppBaselineError::NativeLinkUnavailable)
        }

        #[cfg(not(feature = "native-standalone"))]
        {
            Err(CppBaselineError::NativeFeatureDisabled)
        }
    }

    /// Runs fixed C++ public standalone measure-delegate API cases.
    ///
    /// # Errors
    ///
    /// Returns the same native-backend availability errors as [`Self::layout`].
    pub fn standalone_public_measure_delegate_snapshot(
        &mut self,
    ) -> Result<StandalonePublicMeasureDelegateSnapshot, CppBaselineError> {
        #[cfg(all(feature = "native-standalone", starlight_cpp_native_standalone))]
        {
            native::standalone_public_measure_delegate_snapshot()
        }

        #[cfg(all(feature = "native-standalone", not(starlight_cpp_native_standalone)))]
        {
            Err(CppBaselineError::NativeLinkUnavailable)
        }

        #[cfg(not(feature = "native-standalone"))]
        {
            Err(CppBaselineError::NativeFeatureDisabled)
        }
    }
}

pub trait BaselineLayoutTree: LayoutTree {
    fn layout_result(&self, node: Self::NodeId) -> LayoutResult;
}

impl BaselineLayoutTree for SimpleTree {
    fn layout_result(&self, node: Self::NodeId) -> LayoutResult {
        self.nodes[node].layout
    }
}

#[cfg(test)]
mod tests {
    #[test]
    #[cfg(not(feature = "native-standalone"))]
    fn layout_reports_native_feature_disabled_without_native_feature() {
        use super::{CppBaselineError, CppStarlightEngine};
        use starlight_layout::{Constraints, SimpleNode, SimpleTree, Style};

        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style::default()));

        let error = CppStarlightEngine::new()
            .layout(&mut tree, root, Constraints::indefinite())
            .expect_err("default build should not silently run native layout");

        assert_eq!(error, CppBaselineError::NativeFeatureDisabled);
    }

    #[test]
    #[cfg(all(feature = "native-standalone", not(starlight_cpp_native_standalone)))]
    fn layout_reports_native_link_unavailable_without_link_metadata() {
        use super::{CppBaselineError, CppStarlightEngine};
        use starlight_layout::{Constraints, SimpleNode, SimpleTree, Style};

        let mut tree = SimpleTree::default();
        let root = tree.push(SimpleNode::new(Style::default()));

        let error = CppStarlightEngine::new()
            .layout(&mut tree, root, Constraints::indefinite())
            .expect_err("native feature without link metadata should not call FFI");

        assert_eq!(error, CppBaselineError::NativeLinkUnavailable);
    }
}
