// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use crate::LayoutUnit;

#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct BaseLength {
    fixed: f32,
    percentage: f32,
    has_value: bool,
    has_percentage: bool,
}

impl BaseLength {
    #[must_use]
    pub const fn empty() -> Self {
        Self {
            fixed: 0.0,
            percentage: 0.0,
            has_value: false,
            has_percentage: false,
        }
    }

    #[must_use]
    pub const fn fixed(fixed: f32) -> Self {
        Self {
            fixed,
            percentage: 0.0,
            has_value: true,
            has_percentage: false,
        }
    }

    #[must_use]
    pub const fn fixed_and_percent(fixed: f32, percentage: f32) -> Self {
        Self {
            fixed,
            percentage,
            has_value: true,
            has_percentage: true,
        }
    }

    #[must_use]
    pub const fn has_value(self) -> bool {
        self.has_value
    }

    #[must_use]
    pub const fn contains_percentage(self) -> bool {
        self.has_value && self.has_percentage
    }

    #[must_use]
    pub fn contains_fixed_value(self) -> bool {
        (self.fixed != 0.0 || !self.has_percentage) && self.has_value
    }

    #[must_use]
    pub const fn fixed_part(self) -> f32 {
        self.fixed
    }

    #[must_use]
    pub const fn percentage_part(self) -> f32 {
        self.percentage
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum NLengthType {
    Auto,
    Unit,
    Percentage,
    Calc,
    MinContent,
    MaxContent,
    FitContent,
    Fr,
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub struct NLength {
    numeric_length: BaseLength,
    length_type: NLengthType,
}

impl NLength {
    #[must_use]
    pub const fn auto() -> Self {
        Self {
            numeric_length: BaseLength::empty(),
            length_type: NLengthType::Auto,
        }
    }

    #[must_use]
    pub const fn max_content() -> Self {
        Self {
            numeric_length: BaseLength::empty(),
            length_type: NLengthType::MaxContent,
        }
    }

    #[must_use]
    pub const fn min_content() -> Self {
        Self {
            numeric_length: BaseLength::empty(),
            length_type: NLengthType::MinContent,
        }
    }

    #[must_use]
    pub const fn fit_content() -> Self {
        Self {
            numeric_length: BaseLength::empty(),
            length_type: NLengthType::FitContent,
        }
    }

    #[must_use]
    pub const fn fit_content_with(base: BaseLength) -> Self {
        Self {
            numeric_length: base,
            length_type: NLengthType::FitContent,
        }
    }

    #[must_use]
    pub const fn unit(value: f32) -> Self {
        Self {
            numeric_length: BaseLength::fixed(value),
            length_type: NLengthType::Unit,
        }
    }

    #[must_use]
    pub const fn percentage(value: f32) -> Self {
        Self {
            numeric_length: BaseLength::fixed_and_percent(0.0, value),
            length_type: NLengthType::Percentage,
        }
    }

    #[must_use]
    pub const fn calc_fixed(fixed: f32) -> Self {
        Self {
            numeric_length: BaseLength::fixed(fixed),
            length_type: NLengthType::Calc,
        }
    }

    #[must_use]
    pub const fn calc(fixed: f32, percentage: f32) -> Self {
        Self {
            numeric_length: BaseLength::fixed_and_percent(fixed, percentage),
            length_type: NLengthType::Calc,
        }
    }

    #[must_use]
    pub const fn fr(value: f32) -> Self {
        Self {
            numeric_length: BaseLength::fixed(value),
            length_type: NLengthType::Fr,
        }
    }

    #[must_use]
    pub const fn length_type(self) -> NLengthType {
        self.length_type
    }

    #[must_use]
    pub const fn numeric_length(self) -> BaseLength {
        self.numeric_length
    }

    #[must_use]
    pub const fn is_auto(self) -> bool {
        matches!(self.length_type, NLengthType::Auto)
    }

    #[must_use]
    pub const fn is_percent(self) -> bool {
        matches!(self.length_type, NLengthType::Percentage)
    }

    #[must_use]
    pub const fn contains_percentage(self) -> bool {
        self.numeric_length.contains_percentage()
    }

    #[must_use]
    pub fn to_css_string(self) -> String {
        let result = match self.length_type {
            NLengthType::Auto => "auto".to_owned(),
            NLengthType::Unit | NLengthType::Percentage | NLengthType::Fr => {
                numeric_length_to_string(self.numeric_length)
            }
            NLengthType::Calc => format!("calc({})", numeric_length_to_string(self.numeric_length)),
            NLengthType::MinContent => "min-content".to_owned(),
            NLengthType::MaxContent => "max-content".to_owned(),
            NLengthType::FitContent => {
                if self.numeric_length.has_value() {
                    format!(
                        "fit-content({})",
                        numeric_length_to_string(self.numeric_length)
                    )
                } else {
                    "fit-content".to_owned()
                }
            }
        };
        format!("{result};")
    }
}

impl Default for NLength {
    fn default() -> Self {
        Self::auto()
    }
}

#[must_use]
pub fn nlength_to_layout_unit(length: NLength, parent_value: LayoutUnit) -> LayoutUnit {
    let numeric_length = length.numeric_length();
    if !numeric_length.has_value() {
        return LayoutUnit::indefinite();
    }
    if numeric_length.contains_percentage() {
        LayoutUnit::new(numeric_length.fixed_part())
            + parent_value * (numeric_length.percentage_part() / 100.0)
    } else {
        LayoutUnit::new(numeric_length.fixed_part())
    }
}

fn numeric_length_to_string(length: BaseLength) -> String {
    if !length.has_value() {
        "0".to_owned()
    } else if length.contains_fixed_value() && !length.contains_percentage() {
        format!("{:.6}unit", length.fixed_part())
    } else if !length.contains_fixed_value() && length.contains_percentage() {
        format!("{:.6}%", length.percentage_part())
    } else {
        format!(
            "{:.6}unit+{:.6}%",
            length.fixed_part(),
            length.percentage_part()
        )
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn min_content_serializes_to_css_keyword() {
        let length = NLength::min_content();

        assert_eq!(length.length_type(), NLengthType::MinContent);
        assert_eq!(length.to_css_string(), "min-content;");
    }
}
