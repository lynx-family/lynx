// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use crate::style_data::{
    LinearCrossGravity, LinearGravity, LinearLayoutGravity, ListComponentType,
};
use crate::types::{Length, Rect};

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub enum Display {
    None,
    #[default]
    Block,
    Flex,
    Linear,
    Relative,
    Grid,
}

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub enum PositionType {
    #[default]
    Static,
    Relative,
    Absolute,
    Fixed,
    Sticky,
}

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub enum BoxSizing {
    #[default]
    ContentBox,
    BorderBox,
}

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub enum Direction {
    #[default]
    Ltr,
    Rtl,
}

impl Direction {
    pub fn is_rtl(self) -> bool {
        self == Self::Rtl
    }

    pub fn is_any_rtl(self) -> bool {
        self.is_rtl()
    }
}

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub enum Visibility {
    #[default]
    Visible,
    Hidden,
    Collapse,
}

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub enum FlexDirection {
    #[default]
    Row,
    RowReverse,
    Column,
    ColumnReverse,
}

impl FlexDirection {
    pub fn is_row(self) -> bool {
        matches!(self, Self::Row | Self::RowReverse)
    }

    pub fn is_reverse(self) -> bool {
        matches!(self, Self::RowReverse | Self::ColumnReverse)
    }
}

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub enum LinearOrientation {
    Horizontal,
    HorizontalReverse,
    #[default]
    Vertical,
    VerticalReverse,
    Row,
    RowReverse,
    Column,
    ColumnReverse,
}

impl LinearOrientation {
    pub fn is_row(self) -> bool {
        matches!(
            self,
            Self::Horizontal | Self::HorizontalReverse | Self::Row | Self::RowReverse
        )
    }

    pub fn is_reverse(self) -> bool {
        matches!(
            self,
            Self::HorizontalReverse
                | Self::VerticalReverse
                | Self::RowReverse
                | Self::ColumnReverse
        )
    }
}

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub enum FlexWrap {
    #[default]
    NoWrap,
    Wrap,
    WrapReverse,
}

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub enum JustifyContent {
    #[default]
    Stretch,
    FlexStart,
    Start,
    Center,
    FlexEnd,
    End,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly,
}

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub enum AlignItems {
    #[default]
    Stretch,
    FlexStart,
    Start,
    Center,
    FlexEnd,
    End,
    Baseline,
}

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub enum AlignContent {
    FlexStart,
    Start,
    Center,
    FlexEnd,
    End,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly,
    #[default]
    Stretch,
}

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub enum JustifyItems {
    Auto,
    #[default]
    Stretch,
    Start,
    Center,
    End,
}

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub enum GridAutoFlow {
    #[default]
    Row,
    Column,
    Dense,
    RowDense,
    ColumnDense,
}

impl GridAutoFlow {
    pub fn is_column(self) -> bool {
        matches!(self, Self::Column | Self::ColumnDense)
    }

    pub fn is_dense(self) -> bool {
        matches!(self, Self::Dense | Self::RowDense | Self::ColumnDense)
    }
}

pub const RELATIVE_ALIGN_NONE: i32 = -1;
pub const RELATIVE_ALIGN_PARENT: i32 = 0;

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub enum RelativeCenter {
    #[default]
    None,
    Horizontal,
    Vertical,
    Both,
}

impl RelativeCenter {
    pub fn is_horizontal(self) -> bool {
        matches!(self, Self::Horizontal | Self::Both)
    }

    pub fn is_vertical(self) -> bool {
        matches!(self, Self::Vertical | Self::Both)
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct Style {
    pub display: Display,
    pub position: PositionType,
    pub box_sizing: BoxSizing,
    pub direction: Direction,
    pub visibility: Visibility,
    pub width: Length,
    pub height: Length,
    pub min_width: Length,
    pub min_height: Length,
    pub max_width: Length,
    pub max_height: Length,
    pub aspect_ratio: Option<f32>,
    pub left: Length,
    pub right: Length,
    pub top: Length,
    pub bottom: Length,
    pub margin: Rect<Length>,
    pub padding: Rect<Length>,
    pub border: Rect<f32>,
    pub flex_direction: FlexDirection,
    pub flex_wrap: FlexWrap,
    pub justify_content: JustifyContent,
    pub align_items: AlignItems,
    pub align_self: Option<AlignItems>,
    pub align_content: AlignContent,
    pub justify_items: JustifyItems,
    pub justify_self: JustifyItems,
    pub flex_grow: f32,
    pub flex_shrink: f32,
    pub flex_basis: Length,
    pub order: i32,
    pub row_gap: Length,
    pub column_gap: Length,
    pub linear_orientation: LinearOrientation,
    pub linear_gravity: LinearGravity,
    pub linear_layout_gravity: LinearLayoutGravity,
    pub linear_cross_gravity: LinearCrossGravity,
    pub linear_weight: f32,
    pub linear_weight_sum: f32,
    pub linear_column_count: Option<usize>,
    pub list_main_axis_gap: Length,
    pub list_cross_axis_gap: Length,
    pub list_component_type: Option<ListComponentType>,
    pub grid_template_columns: Vec<Length>,
    pub grid_template_rows: Vec<Length>,
    pub grid_template_columns_max: Vec<Length>,
    pub grid_template_rows_max: Vec<Length>,
    pub grid_auto_columns: Vec<Length>,
    pub grid_auto_rows: Vec<Length>,
    pub grid_auto_columns_max: Vec<Length>,
    pub grid_auto_rows_max: Vec<Length>,
    pub grid_auto_flow: GridAutoFlow,
    pub grid_column_start: Option<i32>,
    pub grid_column_end: Option<i32>,
    pub grid_row_start: Option<i32>,
    pub grid_row_end: Option<i32>,
    pub grid_column_span: usize,
    pub grid_row_span: usize,
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
    pub relative_center: RelativeCenter,
}

impl Style {
    pub fn display_none() -> Self {
        Self {
            display: Display::None,
            ..Self::default()
        }
    }
}

impl Default for Style {
    fn default() -> Self {
        Self {
            display: Display::Block,
            position: PositionType::Static,
            box_sizing: BoxSizing::ContentBox,
            direction: Direction::Ltr,
            visibility: Visibility::Visible,
            width: Length::Auto,
            height: Length::Auto,
            min_width: Length::Auto,
            min_height: Length::Auto,
            max_width: Length::Auto,
            max_height: Length::Auto,
            aspect_ratio: None,
            left: Length::Auto,
            right: Length::Auto,
            top: Length::Auto,
            bottom: Length::Auto,
            margin: Rect::all(Length::ZERO),
            padding: Rect::all(Length::ZERO),
            border: Rect::all(0.0),
            flex_direction: FlexDirection::Row,
            flex_wrap: FlexWrap::NoWrap,
            justify_content: JustifyContent::Stretch,
            align_items: AlignItems::Stretch,
            align_self: None,
            align_content: AlignContent::Stretch,
            justify_items: JustifyItems::Stretch,
            justify_self: JustifyItems::Auto,
            flex_grow: 0.0,
            flex_shrink: 1.0,
            flex_basis: Length::Auto,
            order: 0,
            row_gap: Length::ZERO,
            column_gap: Length::ZERO,
            linear_orientation: LinearOrientation::Vertical,
            linear_gravity: LinearGravity::None,
            linear_layout_gravity: LinearLayoutGravity::None,
            linear_cross_gravity: LinearCrossGravity::None,
            linear_weight: 0.0,
            linear_weight_sum: 0.0,
            linear_column_count: None,
            list_main_axis_gap: Length::ZERO,
            list_cross_axis_gap: Length::ZERO,
            list_component_type: None,
            grid_template_columns: Vec::new(),
            grid_template_rows: Vec::new(),
            grid_template_columns_max: Vec::new(),
            grid_template_rows_max: Vec::new(),
            grid_auto_columns: Vec::new(),
            grid_auto_rows: Vec::new(),
            grid_auto_columns_max: Vec::new(),
            grid_auto_rows_max: Vec::new(),
            grid_auto_flow: GridAutoFlow::Row,
            grid_column_start: None,
            grid_column_end: None,
            grid_row_start: None,
            grid_row_end: None,
            grid_column_span: 1,
            grid_row_span: 1,
            relative_id: RELATIVE_ALIGN_NONE,
            relative_align_top: RELATIVE_ALIGN_NONE,
            relative_align_right: RELATIVE_ALIGN_NONE,
            relative_align_bottom: RELATIVE_ALIGN_NONE,
            relative_align_left: RELATIVE_ALIGN_NONE,
            relative_top_of: RELATIVE_ALIGN_NONE,
            relative_right_of: RELATIVE_ALIGN_NONE,
            relative_bottom_of: RELATIVE_ALIGN_NONE,
            relative_left_of: RELATIVE_ALIGN_NONE,
            relative_layout_once: false,
            relative_center: RelativeCenter::None,
        }
    }
}
