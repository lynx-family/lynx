// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

//! Rust core for Starlight layout.
//!
//! The public entry point is [`LayoutTree`]. It deliberately models the tree as
//! an adapter trait instead of owning nodes, so embedders can run layout over
//! existing DOM, shadow, or test trees without copying them into a Starlight
//! container type.

#![forbid(unsafe_code)]

mod container_node;
mod data_ref;
mod engine;
mod layout_unit;
mod nlength;
mod style;
mod style_data;
mod tree;
mod types;

pub use container_node::ContainerNode;
pub use data_ref::DataRef;
pub use engine::{LayoutEngine, LayoutTree};
pub use layout_unit::LayoutUnit;
pub use nlength::{nlength_to_layout_unit, BaseLength, NLength, NLengthType};
pub use style::{
    AlignContent, AlignItems, BoxSizing, Direction, Display, FlexDirection, FlexWrap, GridAutoFlow,
    JustifyContent, JustifyItems, LinearOrientation, PositionType, RelativeCenter, Style,
    Visibility, RELATIVE_ALIGN_NONE, RELATIVE_ALIGN_PARENT,
};
pub use style_data::{
    list_component_type_is_row, BorderStyle, BordersData, BoxData, FlexData, GridData,
    LayoutComputedStyle, LinearCrossGravity, LinearData, LinearGravity, LinearLayoutGravity,
    ListComponentType, RelativeData, DEFAULT_BORDER, DEFAULT_COLOR, DEFAULT_FLEX_GROW,
    DEFAULT_GRID_GAP, DEFAULT_RELATIVE_ID,
};
pub use tree::{LayoutNode, SimpleNode, SimpleTree};
pub use types::{
    CompactConstraints, Constraints, Edges, LayoutResult, Length, MeasureMode, Point, Rect,
    SideConstraint, Size,
};
