// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use crate::{DataRef, NLength};

pub const DEFAULT_COLOR: u32 = 0x0000_0000;
pub const DEFAULT_BORDER_COLOR: u32 = 0xff00_0000;
pub const DEFAULT_BORDER: f32 = 0.0;
pub const DEFAULT_FLEX_GROW: f32 = 0.0;
pub const DEFAULT_GRID_GAP: NLength = NLength::unit(0.0);
pub const DEFAULT_RELATIVE_ID: i32 = -1;

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub enum BorderStyle {
    #[default]
    Solid,
    Dashed,
    Dotted,
    Double,
    None,
}

#[derive(Clone, Debug, PartialEq)]
pub struct BordersData {
    pub width_top: f32,
    pub width_right: f32,
    pub width_bottom: f32,
    pub width_left: f32,
    pub radius_x_top_left: NLength,
    pub radius_x_top_right: NLength,
    pub radius_x_bottom_right: NLength,
    pub radius_x_bottom_left: NLength,
    pub radius_y_top_left: NLength,
    pub radius_y_top_right: NLength,
    pub radius_y_bottom_right: NLength,
    pub radius_y_bottom_left: NLength,
    pub color_top: u32,
    pub color_right: u32,
    pub color_bottom: u32,
    pub color_left: u32,
    pub style_top: BorderStyle,
    pub style_right: BorderStyle,
    pub style_bottom: BorderStyle,
    pub style_left: BorderStyle,
}

impl BordersData {
    #[must_use]
    pub fn new() -> Self {
        Self {
            width_top: DEFAULT_BORDER,
            width_right: DEFAULT_BORDER,
            width_bottom: DEFAULT_BORDER,
            width_left: DEFAULT_BORDER,
            radius_x_top_left: NLength::unit(0.0),
            radius_x_top_right: NLength::unit(0.0),
            radius_x_bottom_right: NLength::unit(0.0),
            radius_x_bottom_left: NLength::unit(0.0),
            radius_y_top_left: NLength::unit(0.0),
            radius_y_top_right: NLength::unit(0.0),
            radius_y_bottom_right: NLength::unit(0.0),
            radius_y_bottom_left: NLength::unit(0.0),
            color_top: DEFAULT_BORDER_COLOR,
            color_right: DEFAULT_BORDER_COLOR,
            color_bottom: DEFAULT_BORDER_COLOR,
            color_left: DEFAULT_BORDER_COLOR,
            style_top: BorderStyle::Solid,
            style_right: BorderStyle::Solid,
            style_bottom: BorderStyle::Solid,
            style_left: BorderStyle::Solid,
        }
    }

    pub fn reset(&mut self) {
        let reset = Self::new();
        self.width_top = reset.width_top;
        self.width_right = reset.width_right;
        self.width_bottom = reset.width_bottom;
        self.width_left = reset.width_left;
        self.radius_x_top_left = reset.radius_x_top_left;
        self.radius_x_top_right = reset.radius_x_top_right;
        self.radius_x_bottom_right = reset.radius_x_bottom_right;
        self.radius_x_bottom_left = reset.radius_x_bottom_left;
        self.radius_y_top_left = reset.radius_y_top_left;
        self.radius_y_top_right = reset.radius_y_top_right;
        self.radius_y_bottom_right = reset.radius_y_bottom_right;
        self.radius_y_bottom_left = reset.radius_y_bottom_left;
        self.color_top = DEFAULT_COLOR;
        self.color_right = DEFAULT_COLOR;
        self.color_bottom = DEFAULT_COLOR;
        self.color_left = DEFAULT_COLOR;
        self.style_top = reset.style_top;
        self.style_right = reset.style_right;
        self.style_bottom = reset.style_bottom;
        self.style_left = reset.style_left;
    }
}

impl Default for BordersData {
    fn default() -> Self {
        Self::new()
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum ListComponentType {
    Header,
    Footer,
    ListRow,
    Default,
}

#[must_use]
pub const fn list_component_type_is_row(component_type: ListComponentType) -> bool {
    matches!(
        component_type,
        ListComponentType::Header | ListComponentType::Footer | ListComponentType::ListRow
    )
}

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub enum LinearGravity {
    #[default]
    None,
    Top,
    Bottom,
    Left,
    Right,
    CenterVertical,
    CenterHorizontal,
    SpaceBetween,
    Start,
    End,
    Center,
}

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub enum LinearLayoutGravity {
    #[default]
    None,
    Top,
    Bottom,
    Left,
    Right,
    CenterVertical,
    CenterHorizontal,
    FillVertical,
    FillHorizontal,
    Center,
    Stretch,
    Start,
    End,
}

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub enum LinearCrossGravity {
    #[default]
    None,
    Start,
    End,
    Center,
    Stretch,
}

#[derive(Clone, Debug, Default, PartialEq)]
pub struct BoxData {
    pub width: NLength,
}

#[derive(Clone, Debug, PartialEq)]
pub struct FlexData {
    pub flex_grow: f32,
}

impl Default for FlexData {
    fn default() -> Self {
        Self {
            flex_grow: DEFAULT_FLEX_GROW,
        }
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct GridData {
    pub grid_column_gap: NLength,
}

impl Default for GridData {
    fn default() -> Self {
        Self {
            grid_column_gap: DEFAULT_GRID_GAP,
        }
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct LinearData {
    pub linear_gravity: LinearGravity,
}

impl Default for LinearData {
    fn default() -> Self {
        Self {
            linear_gravity: LinearGravity::None,
        }
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct RelativeData {
    pub relative_id: i32,
    pub relative_align_top: i32,
}

impl Default for RelativeData {
    fn default() -> Self {
        Self {
            relative_id: DEFAULT_RELATIVE_ID,
            relative_align_top: DEFAULT_RELATIVE_ID,
        }
    }
}

#[derive(Clone, Debug)]
pub struct LayoutComputedStyle {
    pub box_data: DataRef<BoxData>,
    pub flex_data: DataRef<FlexData>,
    pub grid_data: DataRef<GridData>,
    pub linear_data: DataRef<LinearData>,
    pub relative_data: DataRef<RelativeData>,
    physical_pixels_per_layout_unit: f32,
}

impl LayoutComputedStyle {
    #[must_use]
    pub fn new(physical_pixels_per_layout_unit: f32) -> Self {
        Self {
            box_data: DataRef::from_value(BoxData::default()),
            flex_data: DataRef::from_value(FlexData::default()),
            grid_data: DataRef::from_value(GridData::default()),
            linear_data: DataRef::from_value(LinearData::default()),
            relative_data: DataRef::from_value(RelativeData::default()),
            physical_pixels_per_layout_unit,
        }
    }

    pub fn copy_from(&mut self, source: &Self) {
        *self = Self {
            box_data: source.box_data.clone(),
            flex_data: source.flex_data.clone(),
            grid_data: source.grid_data.clone(),
            linear_data: source.linear_data.clone(),
            relative_data: source.relative_data.clone(),
            physical_pixels_per_layout_unit: source.physical_pixels_per_layout_unit,
        };
        self.ensure_data_refs();
    }

    pub fn ensure_data_refs(&mut self) {
        if self.box_data.get().is_none() {
            self.box_data.init();
        }
        if self.flex_data.get().is_none() {
            self.flex_data.init();
        }
        if self.grid_data.get().is_none() {
            self.grid_data.init();
        }
        if self.linear_data.get().is_none() {
            self.linear_data.init();
        }
        if self.relative_data.get().is_none() {
            self.relative_data.init();
        }
    }

    #[must_use]
    pub fn width(&self) -> NLength {
        self.box_data
            .get()
            .map_or_else(NLength::auto, |data| data.width)
    }

    #[must_use]
    pub fn flex_grow(&self) -> f32 {
        self.flex_data
            .get()
            .map_or(DEFAULT_FLEX_GROW, |data| data.flex_grow)
    }

    #[must_use]
    pub fn grid_column_gap(&self) -> NLength {
        self.grid_data
            .get()
            .map_or(DEFAULT_GRID_GAP, |data| data.grid_column_gap)
    }

    #[must_use]
    pub fn linear_gravity(&self) -> LinearGravity {
        self.linear_data
            .get()
            .map_or(LinearGravity::None, |data| data.linear_gravity)
    }

    #[must_use]
    pub fn relative_align_top(&self) -> i32 {
        self.relative_data
            .get()
            .map_or(DEFAULT_RELATIVE_ID, |data| data.relative_align_top)
    }

    #[must_use]
    pub fn relative_id(&self) -> i32 {
        self.relative_data
            .get()
            .map_or(DEFAULT_RELATIVE_ID, |data| data.relative_id)
    }

    #[must_use]
    pub fn physical_pixels_per_layout_unit(&self) -> f32 {
        self.physical_pixels_per_layout_unit
    }
}
