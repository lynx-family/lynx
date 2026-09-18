# Fragment Layer Rendering Architecture

## 1. Overview

Fragment Layer Rendering records a fragment subtree into a platform-neutral
display list and applies that list on Android or Darwin.

The design has two independently updated parts:

- **Content items**: drawing commands such as fill, border, text, image, and
  gradients.
- **Subtree properties**: transform, opacity, and filter values that can change
  without rebuilding the content items.

The current content protocol is a typed, fixed-stride item buffer. The legacy
parallel operation/integer/float arrays are not part of the protocol.

## 2. Core Data Model

The implementation lives in:

- `core/renderer/dom/fragment/display_list.h`
- `core/renderer/dom/fragment/display_list.cc`
- `core/renderer/dom/fragment/display_list_builder.h`
- `core/renderer/dom/fragment/display_list_builder.cc`
- `core/renderer/dom/fragment/display_list_reader.h`

### 2.1 DisplayListItem

Each content command is one `DisplayListItem`:

```cpp
typedef struct DisplayListItem {
  DisplayListOpType type;
  union Payload {
    // One typed payload structure per DisplayListOpType.
  } payload;
} DisplayListItem;
```

`DisplayListItem` is a standard-layout, trivially copyable, 56-byte structure.
Its offsets are guarded by `static_assert` checks because Android consumes the
items through a direct byte buffer.

The operation type selects the matching payload:

| Operation | Payload |
| --- | --- |
| `kBegin` | fragment id/type and frame |
| `kEnd` | no payload |
| `kFill` | color and clip-box index |
| `kDrawView` | view id and final local offset |
| `kText` | text id and box index |
| `kImage` | image id and box index |
| `kBackgroundImage` | image, tiling/clip indices, repeat modes, and optional auto-size/position metadata |
| `kBorder` | box indices, four colors, and four styles |
| `kClipRect` | rectangle and optional eight radii |
| `kRecordBox` | rectangle and optional eight radii |
| `kLinearGradient` | data offsets/counts and gradient parameters |
| `kBoxShadow` | box indices, color, blur radius, and clip mode |

Unknown operation types can be skipped by advancing one fixed-size item.

Background color is recorded before background images, using the bottommost
CSS image layer's clip. Image layers are recorded from last to first, so the
first CSS background is painted on top, as required by
[CSS Backgrounds §2.1](https://www.w3.org/TR/css-backgrounds-3/#layering).
Property lists (size, position, origin, clip, repeat) and image resources remain
indexed by the original CSS layer index, not by painting order.

Background images retain a fully resolved fallback tiling box. Builder callers
provide `auto_width` and `auto_height` booleans; `DisplayListBuilder` owns their
encoding into the display-list bit mask. The appended `auto_size` mask (bit 0: width, bit 1: height) identifies axes whose intrinsic
size is only available after image loading. `position_x` and `position_y` store
percentage-position coefficients (percentage / 100), or zero for absolute
positions. These fields default to zero, fit within the existing 56-byte item,
and do not move any existing fields. Readers without intrinsic-size support
continue using the fallback box; gradients do not use this metadata.

Android resolves auto/auto to the loaded image's dimensions scaled by density,
and a single auto axis from the other dimension and intrinsic aspect ratio.
After resolution, it shifts each coordinate by
`(fallback_size - resolved_size) * position_coefficient` before clipping and
repeating tiles. This preserves percentage positioning even when the bitmap
is larger than the origin box. The recorded box is never mutated: image
completion can invalidate the renderer and redraw the same display list.
An unresolved image still binds its renderer host before drawing is deferred,
so asynchronous load completion can schedule that redraw.

### 2.2 DisplayList storage

`DisplayList` owns:

```cpp
base::auto_create_optional<base::InlineVector<DisplayListItem, 8>>
    content_items_;
base::auto_create_optional<base::Vector<uint8_t>> content_data_;
base::auto_create_optional<base::InlineVector<SubtreeProperty, 1>>
    subtree_properties_;
```

- `content_items_` stores the fixed-size commands.
- `content_data_` stores variable-length gradient colors and stops.
- `subtree_properties_` stores transform, opacity, and filter values.

Image references are retained separately so resources remain alive while a
display list is being consumed. Sublayer ids are also retained for platform
renderer hierarchy updates.

### 2.3 Variable-length gradient data

Gradient colors and stops do not fit in the fixed item payload. The builder
appends them to `content_data_` and records byte offsets and element counts in
the gradient item:

```cpp
item.payload.linear_gradient.color_count_offset = color_offset;
item.payload.linear_gradient.color_count = color_count;
item.payload.linear_gradient.stop_count_offset = stop_offset;
item.payload.linear_gradient.stop_count = stop_count;
```

Consumers must use the counts before dereferencing the corresponding data
offsets.

### 2.4 Subtree properties

`SubtreeProperty` is a separate fixed-layout structure:

```cpp
typedef struct SubtreeProperty {
  DisplayListSubtreePropertyOpType type;
  union Data {
    float transform[16];
    float opacity;
    struct {
      int32_t type;
      float amount;
    } filter;
  } data;
} SubtreeProperty;
```

Like `DisplayListItem`, its size and offsets are ABI-checked.

## 3. Build Flow

`DisplayListBuilder` exposes the fluent recording API used by fragments:

```cpp
DisplayListBuilder builder(render_offset_x, render_offset_y);
builder.Begin(id, type, x, y, width, height)
    .Fill(color, clip_index)
    .DrawText(text_id, box_index)
    .End();
DisplayList list = builder.Build();
```

Each content method initializes one zero-filled `DisplayListItem`, writes the
typed payload, and appends that item exactly once. Gradient and background-image
helpers follow the same rule while also retaining their trailing data or image
resource.

Subtree property methods append only to `subtree_properties_`; they do not add
content items.

## 4. Reading a Display List

Native consumers use `DisplayListReader`:

```cpp
DisplayListReader reader(list);
while (reader.HasNext()) {
  const DisplayListItem& item = reader.Next();
  switch (item.type) {
    case DisplayListOpType::kFill:
      ApplyFill(item.payload.fill.color, item.payload.fill.clip_index);
      break;
    default:
      break;
  }
}
```

For gradients, `DisplayListReader::Colors()` and `Stops()` resolve the offsets
from the item against the trailing data buffer.

## 5. Platform Integration

### 5.1 Darwin

`LynxDisplayListApplier` owns a `DisplayListReader` and reads typed payload
fields directly from the C++ display list. No content serialization or
parallel-array reconstruction is performed.

`PlatformRendererDarwin::OnUpdateDisplayList` reads the first `kBegin` item to
update the host frame, stores the display list, and passes it to the renderer.

### 5.2 Android

Android exposes two read-only direct byte buffers through
`PlatformRendererContext`:

- `getDisplayListItemsBuffer(id)` for the fixed-size items.
- `getDisplayListDataBuffer(id)` for variable-length trailing data.

The JNI bridge validates the Java-provided item stride against
`sizeof(DisplayListItem)` before returning the items buffer.

`Renderer` passes both buffers to `DisplayListApplier`. The applier:

1. Applies native byte order.
2. Rejects an items buffer whose capacity is smaller than or not divisible by
   the 56-byte item stride.
3. Advances by one item per operation.
4. Reads typed fields at ABI-defined offsets.
5. Resolves gradient colors and stops from the optional data buffer.

The C++ Android renderer reads the first typed `kBegin` item to update the
platform renderer frame.

## 6. Update and Lifetime Rules

- A built `DisplayList` is move-only.
- Platform renderers retain non-empty display lists because direct buffers and
  native readers reference their owned storage.
- Content item and trailing-data buffers must remain stable for the duration of
  platform consumption.
- `Clear()` removes recorded content and subtree properties while preserving
  reusable content-buffer capacity.
- `DisplayListItem` or `SubtreeProperty` layout changes must be synchronized
  with every platform reader and covered by ABI tests.

## 7. Validation

Relevant coverage includes:

- C++ `DisplayList` and `DisplayListBuilder` unit tests.
- Fragment drawing tests that inspect typed items with `DisplayListReader`.
- Android `DisplayListApplier` tests that obtain valid item/data buffers from
  the production C++ `DisplayListBuilder` through a test-only JNI wrapper.
- Android native-to-Java ABI tests that compare C++ generated items with Java
  field offsets.

When adding an operation:

1. Add the enum value and typed payload.
2. Add size/offset assertions where needed.
3. Record it once in the builder.
4. Update native, Android, and Darwin readers.
5. Add typed-buffer and behavior tests.
