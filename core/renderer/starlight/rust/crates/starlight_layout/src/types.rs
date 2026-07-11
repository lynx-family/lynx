// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use crate::nlength::BaseLength;

#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Size {
    pub width: f32,
    pub height: f32,
}

impl Size {
    pub const ZERO: Self = Self {
        width: 0.0,
        height: 0.0,
    };

    pub const fn new(width: f32, height: f32) -> Self {
        Self { width, height }
    }
}

#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Point {
    pub x: f32,
    pub y: f32,
}

impl Point {
    pub const ZERO: Self = Self { x: 0.0, y: 0.0 };

    pub const fn new(x: f32, y: f32) -> Self {
        Self { x, y }
    }
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Rect<T> {
    pub left: T,
    pub right: T,
    pub top: T,
    pub bottom: T,
}

impl<T: Copy> Rect<T> {
    pub const fn all(value: T) -> Self {
        Self {
            left: value,
            right: value,
            top: value,
            bottom: value,
        }
    }

    pub const fn new(left: T, right: T, top: T, bottom: T) -> Self {
        Self {
            left,
            right,
            top,
            bottom,
        }
    }
}

impl<T: Default + Copy> Default for Rect<T> {
    fn default() -> Self {
        Self::all(T::default())
    }
}

impl Rect<f32> {
    pub fn horizontal(self) -> f32 {
        self.left + self.right
    }

    pub fn vertical(self) -> f32 {
        self.top + self.bottom
    }
}

pub type Edges = Rect<f32>;

#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub enum Length {
    #[default]
    Auto,
    Points(f32),
    Percent(f32),
    Calc {
        fixed: f32,
        percent: f32,
    },
    Fr(f32),
    MinContent,
    MaxContent,
    FitContent(Option<BaseLength>),
}

impl Length {
    pub const ZERO: Self = Self::Points(0.0);

    pub const fn points(value: f32) -> Self {
        Self::Points(value)
    }

    pub const fn percent(value: f32) -> Self {
        Self::Percent(value)
    }

    pub const fn calc(fixed: f32, percent: f32) -> Self {
        Self::Calc { fixed, percent }
    }

    pub const fn fr(value: f32) -> Self {
        Self::Fr(value)
    }

    pub const fn min_content() -> Self {
        Self::MinContent
    }

    pub const fn max_content() -> Self {
        Self::MaxContent
    }

    pub const fn fit_content(base: Option<BaseLength>) -> Self {
        Self::FitContent(base)
    }

    pub const fn is_intrinsic(self) -> bool {
        matches!(
            self,
            Self::MinContent | Self::MaxContent | Self::FitContent(_)
        )
    }

    pub const fn is_flexible(self) -> bool {
        matches!(self, Self::Fr(_))
    }

    pub fn resolve(self, percent_base: Option<f32>) -> Option<f32> {
        match self {
            Self::Auto => None,
            Self::Points(value) => Some(value),
            Self::Percent(value) => percent_base.map(|base| base * (value / 100.0)),
            Self::Calc { fixed, percent } => percent_base
                .map(|base| fixed + base * (percent / 100.0))
                .or_else(|| (percent == 0.0).then_some(fixed)),
            Self::Fr(_) | Self::MinContent | Self::MaxContent | Self::FitContent(_) => None,
        }
    }
}

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub enum MeasureMode {
    #[default]
    Indefinite,
    Definite,
    AtMost,
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub struct SideConstraint {
    pub size: f32,
    pub mode: MeasureMode,
}

impl SideConstraint {
    const INDEFINITE_SENTINEL: f32 = 10E7;

    pub const fn indefinite() -> Self {
        Self {
            size: Self::INDEFINITE_SENTINEL,
            mode: MeasureMode::Indefinite,
        }
    }

    pub const fn definite(size: f32) -> Self {
        Self {
            size,
            mode: MeasureMode::Definite,
        }
    }

    pub const fn at_most(size: f32) -> Self {
        Self {
            size,
            mode: MeasureMode::AtMost,
        }
    }

    pub fn bounded_size(self) -> Option<f32> {
        match self.mode {
            MeasureMode::Indefinite => None,
            MeasureMode::Definite | MeasureMode::AtMost => Some(self.size),
        }
    }

    pub fn percent_base(self) -> Option<f32> {
        match self.mode {
            MeasureMode::Definite => Some(self.size),
            MeasureMode::AtMost | MeasureMode::Indefinite => None,
        }
    }

    pub fn to_percent_base(self) -> crate::LayoutUnit {
        match self.mode {
            MeasureMode::Definite => crate::LayoutUnit::new(self.size),
            MeasureMode::AtMost | MeasureMode::Indefinite => crate::LayoutUnit::indefinite(),
        }
    }

    pub fn near(self, other: Self) -> bool {
        (self.mode == MeasureMode::Indefinite && other.mode == MeasureMode::Indefinite)
            || (self.mode == other.mode && (self.size - other.size).abs() < 0.00001)
    }

    pub fn apply_size(&mut self, size: crate::LayoutUnit) {
        if size.is_definite() {
            self.mode = MeasureMode::Definite;
            self.size = size.to_float();
        }
    }

    pub fn is_definite(self) -> bool {
        self.mode == MeasureMode::Definite
    }

    pub fn is_at_most(self) -> bool {
        self.mode == MeasureMode::AtMost
    }

    pub fn clamp(self, value: f32) -> f32 {
        match self.mode {
            MeasureMode::AtMost => value.min(self.size),
            MeasureMode::Definite | MeasureMode::Indefinite => value,
        }
    }
}

impl Default for SideConstraint {
    fn default() -> Self {
        Self::indefinite()
    }
}

#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Constraints {
    pub width: SideConstraint,
    pub height: SideConstraint,
}

impl Constraints {
    pub const fn new(width: SideConstraint, height: SideConstraint) -> Self {
        Self { width, height }
    }

    pub const fn indefinite() -> Self {
        Self {
            width: SideConstraint::indefinite(),
            height: SideConstraint::indefinite(),
        }
    }

    pub const fn definite(width: f32, height: f32) -> Self {
        Self {
            width: SideConstraint::definite(width),
            height: SideConstraint::definite(height),
        }
    }

    pub const fn at_most(width: f32, height: f32) -> Self {
        Self {
            width: SideConstraint::at_most(width),
            height: SideConstraint::at_most(height),
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub struct CompactConstraints {
    pub sizes: Size,
    pub modes: [MeasureMode; 2],
}

impl Default for CompactConstraints {
    fn default() -> Self {
        Self::from(Constraints::default())
    }
}

impl From<Constraints> for CompactConstraints {
    fn from(value: Constraints) -> Self {
        Self {
            sizes: Size::new(value.width.size, value.height.size),
            modes: [value.width.mode, value.height.mode],
        }
    }
}

impl CompactConstraints {
    #[must_use]
    pub fn width(self) -> SideConstraint {
        SideConstraint {
            size: self.sizes.width,
            mode: self.modes[0],
        }
    }

    #[must_use]
    pub fn height(self) -> SideConstraint {
        SideConstraint {
            size: self.sizes.height,
            mode: self.modes[1],
        }
    }

    #[must_use]
    pub fn equals_constraints(self, constraints: Constraints) -> bool {
        self.width() == constraints.width && self.height() == constraints.height
    }
}

#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct LayoutResult {
    pub offset: Point,
    pub size: Size,
    /// Baseline offset from the content-box top edge. `None` means consumers
    /// should use the border-box bottom fallback.
    pub baseline: Option<f32>,
    pub padding: Edges,
    pub border: Edges,
    pub margin: Edges,
    pub sticky_pos: Edges,
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn min_content_is_intrinsic_and_indefinite() {
        let length = Length::min_content();

        assert!(length.is_intrinsic());
        assert_eq!(length.resolve(Some(100.0)), None);
        assert_eq!(length.resolve(None), None);
    }
}
