// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

use std::cell::RefCell;
use std::rc::{Rc, Weak};
use std::sync::atomic::{AtomicUsize, Ordering};

static NEXT_NODE_ID: AtomicUsize = AtomicUsize::new(1);

#[derive(Clone, Debug)]
pub struct ContainerNode {
    inner: Rc<RefCell<ContainerNodeInner>>,
}

impl ContainerNode {
    #[must_use]
    pub fn new() -> Self {
        Self {
            inner: Rc::new(RefCell::new(ContainerNodeInner {
                id: NEXT_NODE_ID.fetch_add(1, Ordering::Relaxed),
                parent: Weak::new(),
                previous: Weak::new(),
                next: Weak::new(),
                first_child: Weak::new(),
                last_child: Weak::new(),
                child_count: 0,
            })),
        }
    }

    #[must_use]
    pub fn next(&self) -> Option<Self> {
        self.inner.borrow().next.upgrade().map(Self::from_rc)
    }

    #[must_use]
    pub fn previous(&self) -> Option<Self> {
        self.inner.borrow().previous.upgrade().map(Self::from_rc)
    }

    #[must_use]
    pub fn parent(&self) -> Option<Self> {
        self.inner.borrow().parent.upgrade().map(Self::from_rc)
    }

    #[must_use]
    pub fn first_child(&self) -> Option<Self> {
        self.inner.borrow().first_child.upgrade().map(Self::from_rc)
    }

    #[must_use]
    pub fn last_child(&self) -> Option<Self> {
        self.inner.borrow().last_child.upgrade().map(Self::from_rc)
    }

    #[must_use]
    pub fn child_count(&self) -> usize {
        self.inner.borrow().child_count
    }

    pub fn append_child(&self, child: &Self) {
        self.insert_child_before(child, None);
    }

    pub fn insert_child_before(&self, child: &Self, reference: Option<&Self>) {
        if let Some(reference) = reference {
            assert!(
                reference.parent().as_ref() == Some(self),
                "reference node must be a child of this parent"
            );
        }

        let previous = if let Some(reference) = reference {
            reference.previous()
        } else {
            self.last_child()
        };

        {
            let mut child_inner = child.inner.borrow_mut();
            child_inner.parent = Rc::downgrade(&self.inner);
            child_inner.previous = previous.as_ref().map_or_else(Weak::new, Self::downgrade);
            child_inner.next = reference.map_or_else(Weak::new, Self::downgrade);
        }

        if let Some(reference) = reference {
            reference.inner.borrow_mut().previous = Self::downgrade(child);
        } else {
            self.inner.borrow_mut().last_child = Self::downgrade(child);
        }

        if let Some(previous) = previous {
            previous.inner.borrow_mut().next = Self::downgrade(child);
        } else {
            self.inner.borrow_mut().first_child = Self::downgrade(child);
        }

        self.inner.borrow_mut().child_count += 1;
    }

    pub fn remove_child(&self, child: Option<&Self>) {
        let Some(child) = child else {
            return;
        };
        if self.child_count() == 0 || child.parent().as_ref() != Some(self) {
            return;
        }

        let previous = child.previous();
        let next = child.next();

        if let Some(previous) = &previous {
            previous.inner.borrow_mut().next =
                next.as_ref().map_or_else(Weak::new, Self::downgrade);
        } else {
            self.inner.borrow_mut().first_child =
                next.as_ref().map_or_else(Weak::new, Self::downgrade);
        }

        if let Some(next) = &next {
            next.inner.borrow_mut().previous =
                previous.as_ref().map_or_else(Weak::new, Self::downgrade);
        } else {
            self.inner.borrow_mut().last_child =
                previous.as_ref().map_or_else(Weak::new, Self::downgrade);
        }

        {
            let mut child_inner = child.inner.borrow_mut();
            child_inner.parent = Weak::new();
            child_inner.previous = Weak::new();
            child_inner.next = Weak::new();
        }

        self.inner.borrow_mut().child_count -= 1;
    }

    #[must_use]
    pub fn find(&self, index: isize) -> Option<Self> {
        if index < 0 {
            return None;
        }
        let mut current = self.first_child();
        for _ in 0..index {
            current = current?.next();
        }
        current
    }

    #[must_use]
    pub fn index_of(&self, node: &Self) -> isize {
        let mut index = 0;
        let mut current = self.first_child();
        while let Some(candidate) = current {
            if candidate == *node {
                return index;
            }
            index += 1;
            current = candidate.next();
        }
        -1
    }

    #[must_use]
    pub fn id(&self) -> usize {
        self.inner.borrow().id
    }

    fn detach_on_drop(&self) {
        if Rc::strong_count(&self.inner) != 1 {
            return;
        }

        if let Some(parent) = self.parent() {
            parent.remove_child(Some(self));
        }

        while let Some(child) = self.first_child() {
            self.remove_child(Some(&child));
        }
    }

    fn from_rc(inner: Rc<RefCell<ContainerNodeInner>>) -> Self {
        Self { inner }
    }

    fn downgrade(node: &Self) -> Weak<RefCell<ContainerNodeInner>> {
        Rc::downgrade(&node.inner)
    }
}

impl Default for ContainerNode {
    fn default() -> Self {
        Self::new()
    }
}

impl Drop for ContainerNode {
    fn drop(&mut self) {
        self.detach_on_drop();
    }
}

impl PartialEq for ContainerNode {
    fn eq(&self, other: &Self) -> bool {
        Rc::ptr_eq(&self.inner, &other.inner)
    }
}

impl Eq for ContainerNode {}

#[derive(Debug)]
struct ContainerNodeInner {
    id: usize,
    parent: Weak<RefCell<ContainerNodeInner>>,
    previous: Weak<RefCell<ContainerNodeInner>>,
    next: Weak<RefCell<ContainerNodeInner>>,
    first_child: Weak<RefCell<ContainerNodeInner>>,
    last_child: Weak<RefCell<ContainerNodeInner>>,
    child_count: usize,
}
