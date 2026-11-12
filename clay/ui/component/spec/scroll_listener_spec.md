# Scroll listener lifecycle

## Scope

This contract applies to `Scrollable::Listener` registration and notification.
All operations run on the component's UI task runner. Weak references do not
provide synchronization for concurrent destruction.

## Ownership and registration

- A scrollable does not own its listeners.
- Registrations retain a weak reference whose validity ends when the listener
  is destroyed.
- Adding a null listener has no effect. Adding an already registered listener
  is idempotent.
- Removing a listener removes its registration. Removing all listeners clears
  all registrations.
- Components with tree-dependent registrations retain a weak reference to the
  actual scrollable used for registration. Detach and logical destruction
  unregister from that object, even if the current parent chain has changed.
- Reattachment unregisters the previous subscription before registering with
  the new ancestor.

## Notification

- Notification visits a snapshot of registrations in their existing order.
- Expired listeners are skipped without reading the listener object.
- A listener removed before its turn is skipped. Clearing registrations stops
  further callbacks from that snapshot.
- A newly registered listener absent from the snapshot waits for the next
  notification. Removing and readding the same live listener makes it eligible
  again if it was already in the snapshot.
- Callbacks may unregister or destroy themselves or other listeners.
- Reentrant notification takes its own snapshot.
- If a callback destroys the scrollable, the outer notification returns without
  accessing its members again.

## Integration

Changes to `Listener` layout require rebuilding every derived listener and its
consumers together. Existing compiled listeners cannot be mixed with a rebuilt
scrollable that uses a different listener layout.
