// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use std::cell::Cell;
use std::rc::Rc;

use starlight_layout::DataRef;

#[derive(Debug, PartialEq, Eq)]
struct StyleData {
    value: i32,
    destroyed_count: Rc<Cell<i32>>,
}

impl StyleData {
    fn new(destroyed_count: Rc<Cell<i32>>) -> Self {
        Self {
            value: 1,
            destroyed_count,
        }
    }
}

impl Clone for StyleData {
    fn clone(&self) -> Self {
        Self {
            value: self.value,
            destroyed_count: Rc::clone(&self.destroyed_count),
        }
    }
}

impl Drop for StyleData {
    fn drop(&mut self) {
        self.destroyed_count.set(self.destroyed_count.get() + 1);
    }
}

#[test]
fn data_ref_create_and_destroy() {
    let destroyed_count = Rc::new(Cell::new(0));
    {
        let mut data = DataRef::new();
        data.init_with(StyleData::new(Rc::clone(&destroyed_count)));
        assert_eq!(destroyed_count.get(), 0);
        assert!(data.has_one_ref());
    }
    assert_eq!(destroyed_count.get(), 1);
}

#[test]
fn data_ref_copy_on_write() {
    let destroyed_count = Rc::new(Cell::new(0));
    let mut init_data = DataRef::from_value(StyleData::new(Rc::clone(&destroyed_count)));
    assert!(init_data.has_one_ref());

    let mut new_data = init_data.clone();
    assert_eq!(
        new_data.get().unwrap().value,
        init_data.get().unwrap().value
    );
    assert!(!new_data.has_one_ref());

    init_data.access().unwrap().value = 100;
    assert_ne!(
        new_data.get().unwrap().value,
        init_data.get().unwrap().value
    );

    new_data.access().unwrap().value = 2;
    assert!(new_data.has_one_ref());
    assert_eq!(new_data.get().unwrap().value, 2);
}

#[test]
fn data_ref_ref_count() {
    let destroyed_count = Rc::new(Cell::new(0));
    {
        let init_data = DataRef::from_value(StyleData::new(Rc::clone(&destroyed_count)));
        assert!(init_data.has_one_ref());
        {
            let new_data = init_data.clone();
            assert_eq!(
                new_data.get().unwrap().value,
                init_data.get().unwrap().value
            );
            assert!(!new_data.has_one_ref());
            assert_eq!(init_data.ref_count(), 2);
        }
        assert_eq!(destroyed_count.get(), 0);
        assert!(init_data.has_one_ref());
        {
            let mut new_data = init_data.clone();
            assert_eq!(
                new_data.get().unwrap().value,
                init_data.get().unwrap().value
            );
            assert!(!new_data.has_one_ref());
            assert_eq!(init_data.ref_count(), 2);
            new_data.access().unwrap().value = 2;
            assert!(new_data.has_one_ref());
            assert_eq!(new_data.get().unwrap().value, 2);
        }
        assert_eq!(destroyed_count.get(), 1);
        assert!(init_data.has_one_ref());
    }
    assert_eq!(destroyed_count.get(), 2);
}

#[test]
fn data_ref_assignment_move_and_equality_branches() {
    let destroyed_count = Rc::new(Cell::new(0));
    let first = DataRef::from_value(StyleData::new(Rc::clone(&destroyed_count)));
    let mut second = DataRef::from_value(StyleData::new(Rc::clone(&destroyed_count)));

    assert_eq!(first, second);
    second.access().unwrap().value = 7;
    assert_ne!(first, second);

    let first = second.clone();
    assert_eq!(first, second);
    assert!(!first.has_one_ref());

    let moved = first;
    assert_eq!(moved.get().unwrap().value, 7);
    assert!(!moved.has_one_ref());

    let mut assigned = DataRef::from_value(StyleData::new(Rc::clone(&destroyed_count)));
    assigned.access().unwrap().value = 9;
    assert_eq!(assigned.get().unwrap().value, 9);
    assigned.clear();
    assert_eq!(None, assigned.get());
}

#[test]
fn data_ref_shared_access_and_inequality_false_branches() {
    let destroyed_count = Rc::new(Cell::new(0));
    let original = DataRef::from_value(StyleData::new(Rc::clone(&destroyed_count)));
    let mut shared = original.clone();

    assert!(!shared.has_one_ref());
    assert_eq!(original, shared);
    assert_eq!(original, shared);

    shared.access().unwrap().value = 11;
    assert!(shared.has_one_ref());
    assert!(original.has_one_ref());
    assert_ne!(original, shared);

    let mut same_value = DataRef::from_value(StyleData::new(Rc::clone(&destroyed_count)));
    same_value.access().unwrap().value = shared.get().unwrap().value;
    assert!(!same_value.ptr_eq(&shared));
    assert_eq!(same_value, shared);
}
