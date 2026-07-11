// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use std::rc::Rc;

#[derive(Debug)]
pub struct DataRef<T> {
    inner: Option<Rc<T>>,
}

impl<T> DataRef<T> {
    #[must_use]
    pub fn new() -> Self {
        Self { inner: None }
    }

    #[must_use]
    pub fn from_value(value: T) -> Self {
        Self {
            inner: Some(Rc::new(value)),
        }
    }

    pub fn init_with(&mut self, value: T) {
        self.inner = Some(Rc::new(value));
    }

    pub fn clear(&mut self) {
        self.inner = None;
    }

    #[must_use]
    pub fn get(&self) -> Option<&T> {
        self.inner.as_deref()
    }

    #[must_use]
    pub fn has_one_ref(&self) -> bool {
        self.inner
            .as_ref()
            .is_some_and(|inner| Rc::strong_count(inner) == 1)
    }

    #[must_use]
    pub fn ref_count(&self) -> usize {
        self.inner.as_ref().map_or(0, Rc::strong_count)
    }

    #[must_use]
    pub fn ptr_eq(&self, other: &Self) -> bool {
        match (&self.inner, &other.inner) {
            (Some(left), Some(right)) => Rc::ptr_eq(left, right),
            (None, None) => true,
            _ => false,
        }
    }
}

impl<T: Default> DataRef<T> {
    pub fn init(&mut self) {
        self.init_with(T::default());
    }
}

impl<T: Clone> DataRef<T> {
    pub fn access(&mut self) -> Option<&mut T> {
        self.inner.as_mut().map(Rc::make_mut)
    }
}

impl<T> Clone for DataRef<T> {
    fn clone(&self) -> Self {
        Self {
            inner: self.inner.clone(),
        }
    }
}

impl<T> Default for DataRef<T> {
    fn default() -> Self {
        Self::new()
    }
}

impl<T: PartialEq> PartialEq for DataRef<T> {
    fn eq(&self, other: &Self) -> bool {
        self.get() == other.get()
    }
}

impl<T: Eq> Eq for DataRef<T> {}
