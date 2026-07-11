// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use std::ops::{Add, Div, Mul, Sub};

#[derive(Clone, Copy, Debug)]
pub struct LayoutUnit {
    value: f32,
    is_indefinite: bool,
}

impl LayoutUnit {
    #[must_use]
    pub const fn new(value: f32) -> Self {
        Self {
            value,
            is_indefinite: false,
        }
    }

    #[must_use]
    pub const fn indefinite() -> Self {
        Self {
            value: 0.0,
            is_indefinite: true,
        }
    }

    #[must_use]
    pub const fn zero() -> Self {
        Self::new(0.0)
    }

    #[must_use]
    pub const fn is_indefinite(self) -> bool {
        self.is_indefinite
    }

    #[must_use]
    pub const fn is_definite(self) -> bool {
        !self.is_indefinite
    }

    #[must_use]
    pub fn to_float(self) -> f32 {
        assert!(!self.is_indefinite);
        self.value
    }

    pub fn assign_if_indefinite(&mut self, other: Self) {
        if self.is_indefinite() {
            *self = other;
        }
    }

    pub fn override_with(&mut self, other: Self) {
        if other.is_definite() {
            *self = other;
        }
    }

    #[must_use]
    pub fn clamp_indefinite_to_zero(mut self) -> Self {
        if self.is_indefinite {
            self.value = 0.0;
            self.is_indefinite = false;
        }
        self
    }

    #[must_use]
    pub fn lesser(a: Self, b: Self) -> Self {
        match (a.is_indefinite(), b.is_indefinite()) {
            (false, false) => {
                if a.value > b.value {
                    b
                } else {
                    a
                }
            }
            (false, true) => a,
            (true, false) | (true, true) => b,
        }
    }

    #[must_use]
    pub fn larger(a: Self, b: Self) -> Self {
        match (a.is_indefinite(), b.is_indefinite()) {
            (false, false) => {
                if a.value < b.value {
                    b
                } else {
                    a
                }
            }
            (false, true) => a,
            (true, false) | (true, true) => b,
        }
    }

    #[must_use]
    pub fn clamp_with_min_max(target: Self, min: Self, max: Self) -> Self {
        if target.is_indefinite() {
            return Self::indefinite();
        }
        Self::lesser(max, Self::larger(min, target))
    }
}

impl Default for LayoutUnit {
    fn default() -> Self {
        Self::indefinite()
    }
}

impl PartialEq for LayoutUnit {
    fn eq(&self, other: &Self) -> bool {
        if self.is_indefinite && other.is_indefinite {
            return true;
        }
        self.is_indefinite == other.is_indefinite && self.value == other.value
    }
}

impl Add for LayoutUnit {
    type Output = Self;

    fn add(self, rhs: Self) -> Self::Output {
        Self {
            value: self.value + rhs.value,
            is_indefinite: self.is_indefinite || rhs.is_indefinite,
        }
    }
}

impl Add<f32> for LayoutUnit {
    type Output = Self;

    fn add(self, rhs: f32) -> Self::Output {
        Self {
            value: self.value + rhs,
            is_indefinite: self.is_indefinite,
        }
    }
}

impl Sub for LayoutUnit {
    type Output = Self;

    fn sub(self, rhs: Self) -> Self::Output {
        Self {
            value: self.value - rhs.value,
            is_indefinite: self.is_indefinite || rhs.is_indefinite,
        }
    }
}

impl Sub<f32> for LayoutUnit {
    type Output = Self;

    fn sub(self, rhs: f32) -> Self::Output {
        Self {
            value: self.value - rhs,
            is_indefinite: self.is_indefinite,
        }
    }
}

impl Mul<f32> for LayoutUnit {
    type Output = Self;

    fn mul(self, rhs: f32) -> Self::Output {
        Self {
            value: self.value * rhs,
            is_indefinite: self.is_indefinite,
        }
    }
}

impl Mul<LayoutUnit> for f32 {
    type Output = LayoutUnit;

    fn mul(self, rhs: LayoutUnit) -> Self::Output {
        rhs * self
    }
}

impl Div<f32> for LayoutUnit {
    type Output = Self;

    fn div(self, rhs: f32) -> Self::Output {
        if self.is_indefinite || rhs == 0.0 {
            return Self::indefinite();
        }
        Self::new(self.value / rhs)
    }
}
