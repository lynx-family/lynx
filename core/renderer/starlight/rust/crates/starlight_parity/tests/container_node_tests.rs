// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use std::panic::{catch_unwind, AssertUnwindSafe};

use starlight_layout::ContainerNode;

#[test]
fn container_node_empty_init() {
    let node = ContainerNode::new();
    assert_eq!(None, node.next());
    assert_eq!(None, node.previous());
    assert_eq!(None, node.parent());
    assert_eq!(None, node.first_child());
    assert_eq!(None, node.last_child());
    assert_eq!(0, node.child_count());
}

#[test]
fn container_node_empty_insert() {
    let parent = ContainerNode::new();
    let child = ContainerNode::new();
    parent.insert_child_before(&child, None);
    assert_eq!(Some(parent.clone()), child.parent());
    assert_eq!(Some(child.clone()), parent.first_child());
    assert_eq!(Some(child.clone()), parent.last_child());
    assert_eq!(1, parent.child_count());
}

#[test]
fn container_node_empty_append() {
    let parent = ContainerNode::new();
    let child = ContainerNode::new();
    parent.append_child(&child);
    assert_eq!(Some(parent.clone()), child.parent());
    assert_eq!(Some(child.clone()), parent.first_child());
    assert_eq!(Some(child.clone()), parent.last_child());
    assert_eq!(1, parent.child_count());
}

#[test]
fn container_node_append() {
    let parent = ContainerNode::new();
    let child0 = ContainerNode::new();
    let child1 = ContainerNode::new();
    parent.append_child(&child0);
    parent.append_child(&child1);
    assert_eq!(Some(child0.clone()), parent.first_child());
    assert_eq!(Some(child1.clone()), parent.last_child());
    assert_eq!(Some(child0.clone()), child1.previous());
    assert_eq!(Some(child1.clone()), child0.next());
    assert_eq!(None, child1.next());
    assert_eq!(None, child0.previous());
    assert_eq!(2, parent.child_count());
}

#[test]
fn container_node_insert_front() {
    let parent = ContainerNode::new();
    let child0 = ContainerNode::new();
    let child1 = ContainerNode::new();
    let child2 = ContainerNode::new();
    parent.append_child(&child1);
    parent.append_child(&child2);

    parent.insert_child_before(&child0, Some(&child1));
    assert_eq!(Some(child0.clone()), parent.first_child());
    assert_eq!(Some(child2.clone()), parent.last_child());
    assert_eq!(Some(child0.clone()), child1.previous());
    assert_eq!(Some(child1.clone()), child0.next());
    assert_eq!(None, child0.previous());
    assert_eq!(3, parent.child_count());
}

#[test]
fn container_node_insert_middle() {
    let parent = ContainerNode::new();
    let child0 = ContainerNode::new();
    let child1 = ContainerNode::new();
    let child2 = ContainerNode::new();
    parent.append_child(&child0);
    parent.append_child(&child2);

    parent.insert_child_before(&child1, Some(&child2));
    assert_eq!(Some(child0.clone()), parent.first_child());
    assert_eq!(Some(child2.clone()), parent.last_child());
    assert_eq!(Some(child0.clone()), child1.previous());
    assert_eq!(Some(child1.clone()), child0.next());
    assert_eq!(Some(child2.clone()), child1.next());
    assert_eq!(Some(child1.clone()), child2.previous());
    assert_eq!(3, parent.child_count());
}

#[test]
fn container_node_insert_end() {
    let parent = ContainerNode::new();
    let child0 = ContainerNode::new();
    let child1 = ContainerNode::new();
    let child2 = ContainerNode::new();
    parent.append_child(&child0);
    parent.append_child(&child1);

    parent.insert_child_before(&child2, None);
    assert_eq!(Some(child0.clone()), parent.first_child());
    assert_eq!(Some(child2.clone()), parent.last_child());
    assert_eq!(Some(child1.clone()), child2.previous());
    assert_eq!(None, child2.next());
    assert_eq!(Some(child2.clone()), child1.next());
    assert_eq!(3, parent.child_count());
}

#[test]
fn container_node_remove_first() {
    let parent = ContainerNode::new();
    let child0 = ContainerNode::new();
    let child1 = ContainerNode::new();
    let child2 = ContainerNode::new();
    parent.append_child(&child0);
    parent.append_child(&child1);
    parent.append_child(&child2);

    parent.remove_child(Some(&child0));
    assert_eq!(Some(child1.clone()), parent.first_child());
    assert_eq!(Some(child2.clone()), parent.last_child());
    assert_eq!(None, child1.previous());
    assert_eq!(None, child0.parent());
    assert_eq!(None, child0.next());
    assert_eq!(None, child0.previous());
    assert_eq!(2, parent.child_count());
}

#[test]
fn container_node_remove_middle() {
    let parent = ContainerNode::new();
    let child0 = ContainerNode::new();
    let child1 = ContainerNode::new();
    let child2 = ContainerNode::new();
    parent.append_child(&child0);
    parent.append_child(&child1);
    parent.append_child(&child2);

    parent.remove_child(Some(&child1));
    assert_eq!(Some(child0.clone()), parent.first_child());
    assert_eq!(Some(child2.clone()), parent.last_child());
    assert_eq!(Some(child0.clone()), child2.previous());
    assert_eq!(Some(child2.clone()), child0.next());
}

#[test]
fn container_node_remove_last() {
    let parent = ContainerNode::new();
    let child0 = ContainerNode::new();
    let child1 = ContainerNode::new();
    let child2 = ContainerNode::new();
    parent.append_child(&child0);
    parent.append_child(&child1);
    parent.append_child(&child2);

    parent.remove_child(Some(&child2));
    assert_eq!(Some(child0.clone()), parent.first_child());
    assert_eq!(Some(child1.clone()), parent.last_child());
    assert_eq!(None, child1.next());
}

#[test]
fn container_node_remove_only_node() {
    let parent = ContainerNode::new();
    let child = ContainerNode::new();
    parent.append_child(&child);

    parent.remove_child(Some(&child));
    assert_eq!(None, parent.first_child());
    assert_eq!(None, parent.last_child());
}

#[test]
fn container_node_find_index_and_defensive_branches() {
    let parent = ContainerNode::new();
    let child0 = ContainerNode::new();
    let child1 = ContainerNode::new();
    let child2 = ContainerNode::new();
    let outsider = ContainerNode::new();
    parent.append_child(&child0);
    parent.append_child(&child1);
    parent.append_child(&child2);

    assert_eq!(Some(child0.clone()), parent.find(0));
    assert_eq!(Some(child1.clone()), parent.find(1));
    assert_eq!(Some(child2.clone()), parent.find(2));
    assert_eq!(None, parent.find(3));
    assert_eq!(None, parent.find(-1));

    assert_eq!(0, parent.index_of(&child0));
    assert_eq!(1, parent.index_of(&child1));
    assert_eq!(2, parent.index_of(&child2));
    assert_eq!(-1, parent.index_of(&outsider));

    parent.remove_child(None);
    assert_eq!(3, parent.child_count());

    let empty_parent = ContainerNode::new();
    empty_parent.remove_child(Some(&outsider));
    assert_eq!(0, empty_parent.child_count());

    assert_eq!(None, outsider.parent());
}

#[test]
fn container_node_find_past_end_and_insert_before_owned_reference() {
    let empty_parent = ContainerNode::new();
    assert_eq!(None, empty_parent.find(1));

    let parent = ContainerNode::new();
    let child0 = ContainerNode::new();
    let child1 = ContainerNode::new();
    let inserted = ContainerNode::new();
    parent.append_child(&child0);
    parent.append_child(&child1);

    parent.insert_child_before(&inserted, Some(&child1));
    assert_eq!(Some(child0.clone()), parent.first_child());
    assert_eq!(Some(child1.clone()), parent.last_child());
    assert_eq!(Some(child0.clone()), inserted.previous());
    assert_eq!(Some(child1.clone()), inserted.next());
    assert_eq!(Some(inserted.clone()), child1.previous());
    assert_eq!(3, parent.child_count());
}

#[test]
fn container_node_drop_detaches_children() {
    let child0 = ContainerNode::new();
    let child1 = ContainerNode::new();
    let child2 = ContainerNode::new();
    {
        let parent = ContainerNode::new();
        parent.append_child(&child0);
        parent.append_child(&child1);
        parent.append_child(&child2);
        assert_eq!(3, parent.child_count());
        assert_eq!(Some(parent.clone()), child0.parent());
        assert_eq!(Some(parent.clone()), child1.parent());
        assert_eq!(Some(parent.clone()), child2.parent());
    }

    assert_eq!(None, child0.parent());
    assert_eq!(None, child0.next());
    assert_eq!(None, child0.previous());
    assert_eq!(None, child1.parent());
    assert_eq!(None, child1.next());
    assert_eq!(None, child1.previous());
    assert_eq!(None, child2.parent());
    assert_eq!(None, child2.next());
    assert_eq!(None, child2.previous());
}

#[test]
fn container_node_insert_with_foreign_reference_panics() {
    let parent = ContainerNode::new();
    let other_parent = ContainerNode::new();
    let child = ContainerNode::new();
    let foreign_reference = ContainerNode::new();
    other_parent.append_child(&foreign_reference);

    let result = catch_unwind(AssertUnwindSafe(|| {
        parent.insert_child_before(&child, Some(&foreign_reference));
    }));
    assert!(result.is_err());
}
