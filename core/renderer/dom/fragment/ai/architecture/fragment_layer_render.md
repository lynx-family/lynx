# Fragment Layer Render Architecture

## 1. Overview

Fragment Layer Render is the rendering architecture of the Lynx engine, designed to convert layout computation results into platform-native UI. The core design principle is to decouple rendering commands from platform implementations through **DisplayList**, achieving cross-platform consistent rendering.

### Key Features

- **Cross-Platform Abstraction**: DisplayList serves as an intermediate layer, isolating the C++ core from platform implementations
- **Layer Separation**: Content Operations and Subtree Properties are separated to optimize animation performance
- **Lazy Allocation**: Data storage employs lazy allocation strategies to reduce memory overhead
- **Flattened Rendering**: Supports merged rendering for non-layered nodes

---

## 2. Overall Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         C++ Core Layer                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌───────────┐   ┌───────────┐   ┌──────────────────┐   ┌─────────────┐     │
│  │  Element  │──▶│  Fragment │──▶│DisplayListBuilder│──▶│ DisplayList │     │
│  └───────────┘   └─────┬─────┘   └──────────────────┘   └──────┬──────┘     │
│                        │                                       │            │
│                        ▼                                       │            │
│                 ┌────────────────┐                             │            │
│                 │FragmentBehavior│                             │            │
│                 └────────────────┘                             │            │
│                                                                             │
│  ┌───────────────────────────────────────────────────────────────────────┐  │
│  │                    PlatformRenderer (Impl)                            │  │
│  │                                                                       │  │
│  │   ┌───────────┐  ┌───────────┐  ┌───────────┐  ┌───────────┐          │  │
│  │   │ Platform  │  │ Platform  │  │ Platform  │  │ Platform  │          │  │
│  │   │Renderer   │  │Renderer   │  │Renderer   │  │Renderer   │          │  │
│  │   │  Darwin   │  │  Android  │  │   Other   │  │   ...     │          │  │
│  │   └───────────┘  └───────────┘  └───────────┘  └───────────┘          │  │
│  │                                                                       │  │
│  └───────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
                                        │
                                        ▼ JNI/Obj-C Bridge
┌─────────────────────────────────────────────────────────────────────────────┐
│                         Platform Layer                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│    iOS (Darwin)                             Android                         │
│    ┌───────────────────────┐               ┌────────────────────┐           │
│    │ LynxDisplayListApplier│               │ DisplayListApplier │           │
│    │   (Objective-C++)     │               │      (Java)        │           │
│    └─────────┬─────────────┘               └─────────┬──────────┘           │
│              │                                       │                      │
│              ▼                                       ▼                      │
│    ┌────────────────────┐                  ┌────────────────────┐           │
│    │    LynxRenderer    │                  │      Renderer      │           │
│    │   CALayer/UIView   │                  │    Canvas.draw     │           │
│    └────────────────────┘                  └────────────────────┘           │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 3. Core Components

### 3.1 DisplayList - Rendering Command Storage

**File**: `lynx/core/renderer/dom/fragment/display_list.h`

DisplayList is the carrier of rendering commands, employing a compact data layout:

```cpp
struct OpData {
  base::InlineVector<int32_t, 8> ops;        // Operation type array
  base::InlineVector<int32_t, 16> int_data;  // Integer parameter array
  base::InlineVector<float, 16> float_data;  // Float parameter array
};
```

#### Operation Types (DisplayListOpType)

| Type | Value | Description |
|------|-------|-------------|
| kBegin | 0 | Begin a Fragment, record position and size |
| kEnd | 1 | End current Fragment |
| kFill | 2 | Fill background color |
| kDrawView | 3 | Draw child View (nested Layer) |
| kText | 6 | Draw text |
| kImage | 7 | Draw image |
| kCustom | 8 | Custom drawing |
| kBorder | 9 | Draw border |
| kClipRect | 10 | Set clipping region |
| kRecordBox | 11 | Record Box Model (border/padding/content) |
| kLinearGradient | 12 | Draw linear gradient |

#### Subtree Property Types (DisplayListSubtreePropertyOpType)

| Type | Value | Description |
|------|-------|-------------|
| kTransform | 0 | Transformation matrix (4x4) |
| kOpacity | 1 | Opacity |

#### Data Layout Example

A typical `kFill` operation data layout:

```
ops:        [2, ...]           // kFill = 2
int_data:   [2, 0, 0xFFFF0000, 0]  // [int_count=2, float_count=0, color=red, clip_index=0]
float_data: []                   // No float parameters
```

### 3.2 DisplayListBuilder - Rendering Command Builder

**File**: `lynx/core/renderer/dom/fragment/display_list_builder.h`

The Builder pattern is used to construct DisplayList, providing a fluent API:

```cpp
class DisplayListBuilder {
  DisplayListBuilder& Begin(int id, float x, float y, float width, float height);
  DisplayListBuilder& End();
  DisplayListBuilder& Fill(uint32_t color, int32_t clip_index);
  DisplayListBuilder& DrawView(int view_id);
  DisplayListBuilder& Transform(const transforms::Matrix44& matrix);
  DisplayListBuilder& Opacity(float alpha);
  DisplayListBuilder& Border(int32_t out_index, int32_t inner_index, const BordersData& border);
  DisplayListBuilder& ClipRect(const RoundedRectangle& border);
  DisplayListBuilder& RecordBoxModel(const RoundedRectangle& rect, int32_t& index);
  DisplayList Build();
};
```

### 3.3 Fragment - Rendering Node

**File**: `lynx/core/renderer/dom/fragment/fragment.h`

Fragment is the basic unit of rendering, corresponding to an Element's rendering representation:

```cpp
class Fragment : public BaseElementContainer {
  // Core drawing methods
  void Draw();                                    // Generate independent DisplayList
  void Draw(DisplayListBuilder& parent_builder);  // Merge into parent DisplayList
  void OnDraw(DisplayListBuilder& display_list_builder);

  // Box Model definitions
  int32_t DefineBorderBox(DisplayListBuilder& builder);
  int32_t DefinePaddingBox(DisplayListBuilder& builder);
  int32_t DefineContentBox(DisplayListBuilder& builder);

  // Draw individual parts
  void DrawBackground(DisplayListBuilder& builder);
  void DrawBorder(DisplayListBuilder& builder);
  void DrawTransform(DisplayListBuilder& builder);
  void DrawOpacity(DisplayListBuilder& builder);
  void DrawClip(DisplayListBuilder& builder);
};
```

#### Fragment Tree Structure

```
┌────────────────────────────────────────────────────────────┐
│                     Fragment Tree                          │
├────────────────────────────────────────────────────────────┤
│                                                            │
│   Root Fragment (Page)                                     │
│   ├── View Fragment (z-index: 0, non-fixed)                │
│   │   ├── Text Fragment                                    │
│   │   └── Image Fragment                                   │
│   │                                                        │
│   ├── View Fragment (z-index: 1) ─────────────┐            │
│   │                                           │            │
│   └── View Fragment (position: fixed) ────────┤            │
│                                               │            │
└── Stacking Context (z-index != 0) ◄───────────┘            │
    └── View Fragment (position: fixed) ◄────────────────────┘
```

### 3.4 PlatformRenderer - Platform Renderer

**File**: `lynx/core/renderer/ui_wrapper/painting/platform_renderer.h`

```cpp
class PlatformRenderer : public fml::RefCountedThreadSafeStorage {
  virtual void UpdateDisplayList(DisplayList display_list) = 0;
  virtual void UpdateAttributes(const fml::RefPtr<PropBundle>& attributes, bool tends_to_flatten) = 0;
  virtual void AddChild(fml::RefPtr<PlatformRenderer> child) = 0;
  virtual void RemoveFromParent() = 0;
  virtual int GetId() const = 0;
};
```

#### Platform Implementations

- **iOS**: `PlatformRendererDarwin` → `LynxContainerView` + `LynxRenderer`
- **Android**: `PlatformRendererAndroid` → `ContainerRenderer` + `Renderer`

---

## 4. DisplayList Generation Flow

### 4.1 Complete Flow Diagram

```
┌───────────────────────────────────────────────────────┐
│                       Layout Complete                 │
└───────────────────────────────┬───────────────────────┘
                                │
                                ▼
┌───────────────────────────────────────────────────────┐
│                    Fragment::UpdateLayout()           │
│   - Update layout_info_                               │
│   - MarkDirtyState(kNeedRedraw)                       │
└───────────────────────────────┬───────────────────────┘
                                │
                                ▼
┌──────────────────────────────────────────────────────────────────────────┐
│                         Painting                                         │
│   ┌──────────────────────────────────────────────────────────────────┐   │
│   │  Root Fragment                                                   │   │
│   │  └─▶ Fragment::Draw()                                            │   │
│   │       ├─▶ DisplayListBuilder builder{offset_x, offset_y}         │   │
│   │       ├─▶ OnDraw(builder)                                        │   │
│   │       │    ├─▶ builder.Begin(id, x, y, width, height)            │   │
│   │       │    ├─▶ DrawBackground(builder)                           │   │
│   │       │    │    ├─▶ builder.Fill(color, clip_index)              │   │
│   │       │    │    └─▶ builder.LinearGradient(...)                  │   │
│   │       │    ├─▶ DrawBorder(builder)                               │   │
│   │       │    │    └─▶ builder.Border(out_idx, inner_idx, border)   │   │
│   │       │    ├─▶ DrawTransform(builder)                            │   │
│   │       │    │    └─▶ builder.Transform(matrix)                    │   │
│   │       │    ├─▶ DrawOpacity(builder)                              │   │
│   │       │    │    └─▶ builder.Opacity(alpha)                       │   │
│   │       │    ├─▶ DrawClip(builder)                                 │   │
│   │       │    │    └─▶ builder.ClipRect(rounded_rect)               │   │
│   │       │    ├─▶ behavior_->OnDraw(builder)                        │   │
│   │       │    │    └─▶ Text/Image specific drawing                  │   │
│   │       │    ├─▶ DrawChildren(builder)                             │   │
│   │       │    │    └─▶ child->Draw(builder) // Recursive            │   │
│   │       │    └─▶ builder.End()                                     │   │
│   │       └─▶ builder.Build() // Return DisplayList                  │   │
│   └──────────────────────────────────────────────────────────────────┘   │
└───────────────────────────────┬──────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────────────┐
│             NativePaintingContext::UpdateDisplayList()                  │
│   - Update DisplayList to PlatformRenderer                              │
└───────────────────────────────┬─────────────────────────────────────────┘
                                │
                                ▼ JNI/Obj-C Bridge
┌─────────────────────────────────────────────────────────────────────────┐
│                   Receive DisplayList and apply at platform layer       │
└─────────────────────────────────────────────────────────────────────────┘
```

### 4.2 Key Code Paths

```cpp
// Fragment::Draw() - Generate independent DisplayList (for nodes with PlatformRenderer)
void Fragment::Draw() {
  DisplayListBuilder builder{render_offset_[0], render_offset_[1]};
  if (draw_node_capacity_ > 0) {
    builder.Reserve(draw_node_capacity_);
  }
  OnDraw(builder);
  CheckRootIfNeedClipBounds(builder);

  // Pass to platform layer through NativePaintingContext
  painting_context()->impl()->CastToNativeCtx()->UpdateDisplayList(
      id(), builder.Build());
}

// Fragment::Draw(parent_builder) - Merge into parent DisplayList (for flattened nodes)
void Fragment::Draw(DisplayListBuilder& display_list_builder) {
  if (has_platform_renderer_) {
    // Has independent renderer, draw reference and generate own DisplayList
    display_list_builder.DrawView(id());
    Draw();  // Generate independent DisplayList
    return;
  }
  // No independent renderer, draw directly to parent builder
  OnDraw(display_list_builder);
}
```

---

## 5. DisplayList Parsing Flow

### 5.1 iOS Platform Parsing

**File**: `lynx/platform/darwin/ios/lynx/renderer/LynxDisplayListApplier.mm`

The iOS platform parses DisplayList through **direct memory access**. The C++ layer passes the `DisplayList` pointer directly to the Objective-C++ layer, and `LynxDisplayListApplier` operates directly on C++ object memory:

```objc
// LynxDisplayListApplier holds C++ DisplayList pointer directly
- (void)applyDisplayList:(lynx::tasm::DisplayList *)list {
  [self reset];
  list_ = list;  // Store pointer directly, no memory copy
  [self processContentOperations];
}

- (void)processContentOperations {
  while (content_op_index_ < list_->GetContentOpTypesSize()) {
    // Call C++ methods directly to read data
    auto op = static_cast<DisplayListOpType>(list_->GetOpAtIndex(content_op_index_++));
    auto int_count = list_->GetIntAtIndex(content_int_index_++);
    auto float_count = list_->GetIntAtIndex(content_int_index_++);

    switch (op) {
      case DisplayListOpType::kBegin: {
        // Parse position and offset, handle nested levels
        x_stack_.emplace([self nextContentFloat]);
        y_stack_.emplace([self nextContentFloat]);
        break;
      }
      case DisplayListOpType::kEnd: {
        // Restore offset stack
        left_offset_ -= x_stack_.top();
        x_stack_.pop();
        top_offset_ -= y_stack_.top();
        y_stack_.pop();
        break;
      }
      case DisplayListOpType::kFill: {
        auto argb = [self nextContentInt];
        auto clip_index = [self nextContentInt];
        // Create CALayer and set background color
        CALayer *layer = [[CALayer alloc] init];
        layer.backgroundColor = [UIColor colorWithARGB:argb].CGColor;
        [self applyRectToLayer:layer withBoxIndex:clip_index];
        [self insertLayer:layer];
        break;
      }
      case DisplayListOpType::kBorder: {
        // Read border data: outBox, innerBox, 4 colors, 4 styles
        int out_box_index = [self nextContentInt];
        int inner_box_index = [self nextContentInt];
        // Generate border image and create borderLayer
        UIImage *borderImage = [self createBorderImageWithOutBox:...];
        break;
      }
      case DisplayListOpType::kText: {
        auto text_id = [self nextContentInt];
        auto box_index = [self nextContentInt];
        // Get TextRenderer from LynxRendererContext to create LynxTextLayer
        LynxTextLayer *layer = [[LynxTextLayer alloc] initWithLynxTextRenderer:...];
        break;
      }
      case DisplayListOpType::kImage: {
        auto image_id = [self nextContentInt];
        auto box_index = [self nextContentInt];
        // Create UIImageView and load image through LynxImageManager
        UIImageView *imageView = [self createImageView];
        LynxImageManager *imageManager = [self imageManagerForID:image_id];
        [imageManager setTarget:imageView];
        break;
      }
      case DisplayListOpType::kDrawView: {
        // Reference child View, adjust drawing order
        refView = [_view subviews][view_index++];
        _refLayer = refView.layer;
        break;
      }
      case DisplayListOpType::kRecordBox: {
        // Record Box Model (x, y, width, height + 8 radii)
        RoundedRectangle rect;
        rect.SetX([self nextContentFloat]);
        // ... read other fields
        box_array_.emplace_back(std::move(rect));
        break;
      }
      // ... other operations
    }
  }
}
```

**Key Characteristics**:
- **Zero-Copy**: Objective-C++ accesses C++ DisplayList memory directly, no serialization/deserialization overhead
- **Stack Management**: Uses `std::stack` to manage coordinate offsets for nested Fragments
- **Lazy Loading**: Image, Text, etc. obtain actual resources through ID lookup from `LynxRendererContext`

### 5.2 Android Platform Parsing

**File**: `lynx/platform/android/lynx_android/src/main/java/com/lynx/tasm/behavior/render/DisplayListApplier.java`

The Android platform adopts a **two-phase data retrieval** strategy, copying C++ DisplayList data to Java arrays through JNI:

```java
public class DisplayListApplier {
  // DisplayList data is pre-filled into Java arrays through JNI
  private DisplayList mDisplayList;  // Contains ops[], iArgv[], fArgv[]

  public void drawTillNextView(Canvas canvas) {
    if (mDisplayList == null) return;
    processContentOperations(canvas);
  }

  private void processContentOperations(Canvas canvas) {
    final int[] ops = mDisplayList.ops;
    final int[] iArgv = mDisplayList.iArgv;
    final float[] fArgv = mDisplayList.fArgv;

    while (mContentOpIndex < ops.length) {
      int op = ops[mContentOpIndex++];
      int intParamCount = iArgv[mContentIntIndex++];
      int floatParamCount = iArgv[mContentIntIndex++];

      switch (op) {
        case OP_BEGIN:
          // Canvas coordinate transformation
          canvas.save();
          canvas.translate(x, y);
          break;
        case OP_END:
          canvas.restore();
          break;
        case OP_FILL:
          // Use Paint to draw background (supports rounded corner clipping)
          int color = nextContentInt();
          int clipIndex = nextContentInt();
          mPaint.setColor(color);
          if (roundedRectangle.hasBorderRadius()) {
            canvas.drawPath(mReusablePath, mPaint);
          } else {
            canvas.drawRect(roundedRectangle.getRectF(), mPaint);
          }
          break;
        case OP_BORDER:
          // Use BorderDrawingUtil to draw borders
          BorderDrawingUtil.drawBorders(canvas, paint, outBox, innerBox, colors, styles);
          break;
        case OP_LINEAR_GRADIENT:
          // Create LinearGradient Shader to draw gradient
          mPaint.setShader(new LinearGradient(startX, startY, endX, endY, colors, stops, tileMode));
          canvas.drawPath(clipPath, mPaint);
          break;
        case OP_TEXT:
          // Get Layout from TextMeasurer to draw text
          TextUpdateBundle textBundle = mTextMeasurer.takeTextLayout(textId);
          textBundle.getTextLayout().draw(canvas);
          break;
        case OP_IMAGE:
          // Draw image through LynxImageManager
          LynxImageManager imageManager = mContext.getImage(imageId);
          imageManager.onDraw(canvas);
          break;
        case OP_DRAW_VIEW:
          // Stop current drawing, wait for child View's onDraw to be called
          return;
        case OP_RECORD_BOX:
          // Record RoundedRectangle to array for subsequent operations to reference
          mRoundedRectangleArray.add(new RoundedRectangle(rectF, borderRadii));
          break;
        case OP_CLIP_RECT:
          // Set Canvas clipping region
          canvas.clipPath(mReusablePath);  // or clipRect
          break;
      }
    }
  }
}
```

**Key Characteristics**:
- **Canvas API**: Uses Android standard `Canvas.drawXXX()` interfaces
- **Object Reuse**: Uses `mReusablePath`, `mReusableRectF`, etc. to reduce GC pressure
- **Resource Management**: Text, Image obtained through `PlatformRendererContext`

### 5.3 Subtree Property Application

Subtree properties (Transform/Opacity) are updated through independent channels to optimize animation performance.

**Data Structure Definition** (`lynx/core/renderer/dom/fragment/display_list.h`):

```cpp
// SubtreeProperty memory layout (shared between C++ and Java/OC)
struct SubtreeProperty {
  int32_t type;  // 0=Transform, 1=Opacity
  union {
    float matrix[16];  // 4x4 transformation matrix
    float alpha;       // Opacity
  } data;
};
```

**iOS Implementation** - Direct pointer passing:

```cpp
// platform_renderer_darwin.mm
void PlatformRendererDarwin::OnUpdateSubtreeProperties(
    const DisplayList& subtree_properties) {
  size_t count = subtree_properties.GetSubtreePropertiesSize();
  if (count == 0 || _view == nil) return;

  // Get C++ memory pointer directly
  const SubtreeProperty* props = subtree_properties.GetSubtreePropertiesData();

  // Pass directly to Objective-C layer
  LynxRenderer* renderer = [_view getRenderer];
  [renderer applySubtreeProperties:props count:count];
}
```

```objc
// LynxRenderer.mm
- (void)applySubtreeProperties:(const lynx::tasm::SubtreeProperty *)props
                         count:(size_t)count {
  for (size_t i = 0; i < count; i++) {
    const auto& prop = props[i];
    switch (prop.type) {
      case DisplayListSubtreePropertyOpType::kTransform:
        // Apply 4x4 transformation matrix to CALayer
        self.layer.transform = CATransform3DFromMatrix(prop.data.matrix);
        break;
      case DisplayListSubtreePropertyOpType::kOpacity:
        // Apply opacity
        self.layer.opacity = prop.data.alpha;
        break;
    }
  }
}
```

**Android Implementation** - DirectByteBuffer zero-copy:

```cpp
// platform_renderer_context.cc
void PlatformRendererContext::UpdatePlatformRendererSubtreeProperties(
    int32_t id, const SubtreeProperty* properties, size_t count) {
  JNIEnv* env = base::android::AttachCurrentThread();

  // Create DirectByteBuffer (zero-copy, shared C++ memory)
  const size_t total_bytes = count * sizeof(SubtreeProperty);
  void* buffer_data = const_cast<void*>(static_cast<const void*>(properties));
  jobject direct_buffer = env->NewDirectByteBuffer(buffer_data, total_bytes);

  if (direct_buffer != nullptr) {
    // Call Java method
    Java_PlatformRendererContext_updatePlatformRendererSubtreeProperties(
        env, local_ref.Get(), id, direct_buffer, static_cast<jint>(count));
    env->DeleteLocalRef(direct_buffer);
  }
}
```

```java
// PlatformRendererContext.java
public void updatePlatformRendererSubtreeProperties(int id, ByteBuffer buffer, int count) {
  buffer.order(ByteOrder.nativeOrder());  // Ensure byte order consistency
  IntBuffer intBuffer = buffer.asIntBuffer();

  for (int i = 0; i < count; i++) {
    int type = intBuffer.get();  // Read type

    if (type == SUBTREE_OP_TRANSFORM) {
      // Read 16 floats as transformation matrix
      FloatBuffer floatBuffer = buffer.asFloatBuffer();
      floatBuffer.position(intBuffer.position());
      float[] matrix = new float[16];
      floatBuffer.get(matrix);
      intBuffer.position(intBuffer.position() + 16);

      // Apply to View
      applyTransform(view, matrix);
    } else if (type == SUBTREE_OP_OPACITY) {
      // Read alpha
      float alpha = buffer.asFloatBuffer().get(intBuffer.position());
      intBuffer.position(intBuffer.position() + 1);
      view.setAlpha(alpha);
    }
  }
}
```

**Key Differences Summary**:

| Feature | iOS | Android |
|---------|-----|---------|
| Passing Method | Direct C++ pointer | DirectByteBuffer (zero-copy) |
| Memory Access | Direct access to C++ struct | Access through ByteBuffer |
| Byte Order | No conversion needed | Must set nativeOrder() |
| Type Safety | Compile-time checking | Runtime parsing |

---

## 6. Platform Bridge Layer

### 6.1 C++ to Platform Data Transfer Architecture Comparison

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                               C++ Core Layer                                        │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │  DisplayList (ops, int_data, float_data, subtree_properties)                  │  │
│  └───────────────────────────────────────────────────────────────────────────────┘  │
└──────────────────────────────────┬──────────────────────────────────────────────────┘
                                   │
          ┌────────────────────────┴────────────────────────┐
          │ Direct Pointer Access                             │ JNI Bridge (Two-Phase)
          ▼                                                   ▼
┌─────────────────────────────┐               ┌───────────────────────────────────────┐
│       iOS (Darwin)          │               │            Android                    │
│                             │               │                                       │
│  ┌───────────────────────┐  │               │  Step 1: GetDisplayListLengths()      │
│  │ PlatformRendererDarwin│  │               │          ↓                            │
│  │  - Store DL pointer   │──┼──┐            │  Step 2: GetDisplayListData()         │
│  └───────────────────────┘  │  │            │          ↓                            │
│            │                │  │            │  memcpy() to Java arrays              │
│            ▼                │  │            │          ↓                            │
│  ┌───────────────────────┐  │  │            │  DisplayList(ops[], iArgv[], fArgv[]) │
│  │   LynxRenderer        │  │  │            │          ↓                            │
│  │ updateDisplayList:&dl │◀─┘  │            │  DisplayListApplier                   │
│  └───────────────────────┘     │            │                                       │
│            │                   │            └───────────────────────────────────────┘
│            ▼                   │
│  ┌───────────────────────┐     │
│  │ LynxDisplayListApplier│     │
│  │ - Direct C++ mem access   │
│  │ - No serialization    │     │
│  └───────────────────────┘     │
│                                │
│ Subtree Properties:            │
│ direct pointer pass            │
└────────────────────────────────┘
```

### 6.2 iOS Bridge Implementation

iOS adopts a **direct pointer passing** strategy, exposing C++ `DisplayList` objects directly to the Objective-C++ layer through pointers.

**File**: `lynx/core/renderer/ui_wrapper/painting/ios/platform_renderer_darwin.mm`

```cpp
void PlatformRendererDarwin::OnUpdateDisplayList(DisplayList display_list) {
  display_list_ = std::move(display_list);

  if (_view != nil) {
    // Extract frame from DisplayList (first 4 floats: x, y, width, height)
    constexpr int kFrameValueCount = 4;
    if (display_list_.GetContentFloatData() &&
        display_list_.GetContentFloatDataSize() >= kFrameValueCount) {
      float frame[4];
      memcpy(frame, display_list_.GetContentFloatData(), 4 * sizeof(float));

      // Update UIView frame, including render_offset offset
      [_view setFrame:CGRectMake(frame[0] + display_list_.GetRenderOffset()[0],
                                  frame[1] + display_list_.GetRenderOffset()[1],
                                  frame[2], frame[3])];
    }

    // Pass DisplayList pointer directly to LynxRenderer
    [[_view getRenderer] updateDisplayList:&display_list_];
    [_view setNeedsDisplay];  // Trigger redraw
  }
}

void PlatformRendererDarwin::OnUpdateSubtreeProperties(const DisplayList& subtree_props) {
  // Pass SubtreeProperty pointer directly, no copy
  const SubtreeProperty* props = subtree_props.GetSubtreePropertiesData();
  [[_view getRenderer] applySubtreeProperties:props count:count];
}
```

**File**: `lynx/platform/darwin/ios/lynx/renderer/LynxRenderer+Internal.h`

```objc
@interface LynxRenderer (Internal)
// Receive C++ DisplayList pointer directly
- (void)updateDisplayList:(lynx::tasm::DisplayList *)displayList;
// Receive SubtreeProperty pointer array directly
- (void)applySubtreeProperties:(const lynx::tasm::SubtreeProperty *)props
                         count:(size_t)count;
@end
```

**Characteristics**:
- **Zero-Copy**: Objective-C++ accesses C++ memory directly, no serialization overhead
- **Type Safety**: Shared C++ header file definitions (`display_list.h`, `fragment_constants.h`)
- **Synchronous Update**: Frame updates and DisplayList updates complete in the same call stack

### 6.3 Android Bridge Implementation

Android adopts a **two-phase data copy** strategy, copying C++ DisplayList data to Java arrays through JNI.

**File**: `lynx/core/renderer/ui_wrapper/painting/android/platform_renderer_android.cc`

```cpp
void PlatformRendererAndroid::OnUpdateDisplayList(DisplayList display_list) {
  display_list_ = std::move(display_list);

  // Extract frame data
  constexpr int kFrameValueCount = 4;
  if (context_ && display_list_.GetContentFloatData() &&
      display_list_.GetContentFloatDataSize() >= kFrameValueCount) {
    float frame[4];
    memcpy(frame, display_list_.GetContentFloatData(), 4 * sizeof(float));

    // Update platform layer frame through JNI
    context_->UpdatePlatformRendererFrame(
        PlatformRendererImpl::GetId(),
        display_list_.RootNeedClipBounds(),
        frame,
        display_list_.GetRenderOffset());
  }
  // DisplayList data remains in C++ layer, retrieved by Java layer through JNI on demand
}
```

**File**: `lynx/core/renderer/ui_wrapper/painting/android/platform_renderer_context.cc`

```cpp
// JNI: Java layer first gets array lengths for pre-allocating Java arrays
jintArray GetDisplayListLengths(JNIEnv* env, jobject /*jcaller*/,
                                jlong nativePtr, jint id) {
  // ... get renderer ...
  const DisplayList& display_list = renderer->GetDisplayList();

  jint lengths[3] = {
    static_cast<jint>(display_list.GetContentOpTypesSize()),
    static_cast<jint>(display_list.GetContentIntDataSize()),
    static_cast<jint>(display_list.GetContentFloatDataSize())
  };
  // Return length array
}

// JNI: Java layer then gets actual data
void GetDisplayListData(JNIEnv* env, jobject /*jcaller*/, jlong nativePtr,
                        jint id, jintArray ops, jintArray iArgv, jfloatArray fArgv) {
  // ... get renderer and display_list ...

  // Get C++ data pointers
  const int32_t* opsDataSrc = display_list.GetContentOpTypesData();
  const int32_t* iArgvDataSrc = display_list.GetContentIntData();
  const float* fArgvDataSrc = display_list.GetContentFloatData();

  // Get Java array pointers
  jint* opsData = env->GetIntArrayElements(ops, nullptr);
  jint* iArgvData = env->GetIntArrayElements(iArgv, nullptr);
  jfloat* fArgvData = env->GetFloatArrayElements(fArgv, nullptr);

  // Copy data using memcpy
  memcpy(opsData, opsDataSrc, ...);
  memcpy(iArgvData, iArgvDataSrc, ...);
  memcpy(fArgvData, fArgvDataSrc, ...);

  // Release arrays
  env->ReleaseIntArrayElements(ops, opsData, 0);
  env->ReleaseIntArrayElements(iArgv, iArgvData, 0);
  env->ReleaseFloatArrayElements(fArgv, fArgvData, 0);
}

void PlatformRendererContext::UpdatePlatformRendererSubtreeProperties(
    int32_t id, const SubtreeProperty* properties, size_t count) {
  JNIEnv* env = base::android::AttachCurrentThread();

  // Use DirectByteBuffer for zero-copy SubtreeProperty passing
  const size_t total_bytes = count * sizeof(SubtreeProperty);
  void* buffer_data = const_cast<void*>(static_cast<const void*>(properties));
  jobject direct_buffer = env->NewDirectByteBuffer(buffer_data, total_bytes);

  if (direct_buffer != nullptr) {
    Java_PlatformRendererContext_updatePlatformRendererSubtreeProperties(
        env, local_ref.Get(), id, direct_buffer, static_cast<jint>(count));
    env->DeleteLocalRef(direct_buffer);
  }
}
```

**Java Layer Data Retrieval Flow**:

```java
// PlatformRendererContext.java
public DisplayList getDisplayList(int id) {
  // Step 1: Get array lengths
  int[] lengths = nativeGetDisplayListLengths(mNativePtr, id);
  int opSize = lengths[0];
  int intSize = lengths[1];
  int floatSize = lengths[2];

  // Step 2: Pre-allocate Java arrays
  int[] ops = new int[opSize];
  int[] iArgv = new int[intSize];
  float[] fArgv = new float[floatSize];

  // Step 3: Copy data to Java arrays
  nativeGetDisplayListData(mNativePtr, id, ops, iArgv, fArgv);

  // Create DisplayList object
  return new DisplayList(ops, iArgv, fArgv);
}
```

**Characteristics**:
- **Two-Phase Retrieval**: Get lengths first to pre-allocate arrays, then copy data (avoid dynamic resizing)
- **Data Isolation**: Java layer holds independent data copies, C++ layer can safely release
- **SubtreeProperty Zero-Copy**: Uses `DirectByteBuffer` for shared memory

---

## 7. Performance Optimization Design

### 7.1 Content and Subtree Property Separation

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         DisplayList                                     │
│   ┌────────────────────────┐  ┌──────────────────────────────────────┐  │
│   │     Content Data       │  │       Subtree Properties             │  │
│   │       (Stable)         │  │        (Frequently Changing)         │  │
│   │                        │  │                                      │  │
│   │   - Background         │  │   - Transform                        │  │
│   │   - Border             │  │   - Opacity                          │  │
│   │   - Text/Image         │  │                                      │  │
│   │   (High rebuild cost)  │  │   (Independent update, low cost)     │  │
│   └────────────────────────┘  └──────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────┘
```

### 7.2 Lazy Allocation Strategy

```cpp
// auto_create_optional only creates on first access
base::auto_create_optional<OpData> content_data_;
base::auto_create_optional<base::InlineVector<SubtreeProperty, 1>> subtree_properties_;
```

### 7.3 InlineVector Optimization

Uses small pre-allocated arrays on the stack to avoid heap allocation:

```cpp
base::InlineVector<int32_t, 8> ops;        // Pre-allocate 8 int32
base::InlineVector<int32_t, 16> int_data;  // Pre-allocate 16 int32
base::InlineVector<float, 16> float_data;  // Pre-allocate 16 float
```

---

## 8. Key Data Flow Summary

### 8.1 Complete Rendering Data Flow

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              C++ Core Layer                                         │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                     │
│  ┌───────────┐     ┌───────────┐     ┌──────────────────┐     ┌─────────────────┐   │
│  │ Layout    │────▶│ Fragment  │────▶│DisplayListBuilder│────▶│ DisplayList     │   │
│  │ Engine    │     │ Tree      │     │                  │     │ (ops/int/float) │   │
│  └───────────┘     └───────────┘     └──────────────────┘     └────────┬────────┘   │
│                                                                        │            │
│                                    ┌───────────────────────────────────┘            │
│                                    │                                                │
│                                    ▼                                                │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │                    PlatformRenderer::OnUpdateDisplayList()                    │   │
│  │                    PlatformRenderer::OnUpdateSubtreeProperties()              │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
│                                    │                                                │
└────────────────────────────────────┼────────────────────────────────────────────────┘
                                     │ Platform-Specific Bridge
            ┌────────────────────────┼────────────────────────┐
            │ Direct Pointer         │                        │ JNI Bridge
            ▼                        │                        ▼
┌───────────────────────┐           │           ┌───────────────────────────────────┐
│     iOS (Darwin)      │           │           │           Android                 │
│                       │           │           │                                   │
│  ┌─────────────────┐  │           │           │  ┌─────────────────────────────┐  │
│  │PlatformRenderer │  │           │           │  │  PlatformRendererAndroid    │  │
│  │     Darwin      │  │           │           │  │  - Store DisplayList        │  │
│  │                 │  │           │           │  │  - Call JNI UpdateFrame     │  │
│  │ 1. Update frame │  │           │           │  └─────────────────────────────┘  │
│  │ 2. Pass &dl to  │  │           │           │              │                    │
│  │    LynxRenderer │  │           │           │              ▼ JNI                │
│  └─────────────────┘  │           │           │  ┌─────────────────────────────┐  │
│          │            │           │           │  │ GetDisplayListLengths()     │  │
│          ▼            │           │           │  │ GetDisplayListData()        │  │
│  ┌─────────────────┐  │           │           │  │ (memcpy to Java arrays)     │  │
│  │   LynxRenderer  │  │           │           │  └─────────────────────────────┘  │
│  │  updateDisplay  │  │           │           │              │                    │
│  │     List:&dl    │  │           │           │              ▼                    │
│  └─────────────────┘  │           │           │  ┌─────────────────────────────┐  │
│          │            │           │           │  │ DisplayList(ops,iArgv,fArgv)│  │
│          ▼            │           │           │  └─────────────────────────────┘  │
│  ┌─────────────────┐  │           │           │              │                    │
│  │LynxDisplayList  │  │           │           │              ▼                    │
│  │    Applier      │  │           │           │  ┌─────────────────────────────┐  │
│  │                 │  │           │           │  │   DisplayListApplier        │  │
│  │ - Direct C++    │  │           │           │  │   - Canvas API              │  │
│  │   memory access │  │           │           │  │   - Reusable objects        │  │
│  │ - No copy       │  │           │           │  └─────────────────────────────┘  │
│  └─────────────────┘  │           │           │                                   │
└───────────────────────┘           │           └───────────────────────────────────┘
```

### 8.2 Platform Differences Comparison

| Feature | iOS (Darwin) | Android |
|---------|--------------|---------|
| **DisplayList Transfer** | Direct C++ pointer | JNI two-phase copy (length+data) |
| **Memory Copy** | Zero-copy | memcpy to Java arrays |
| **Drawing API** | CALayer/UIView tree | Canvas.drawXXX() |
| **Subtree Properties** | Direct pointer passing | DirectByteBuffer zero-copy |
| **Coordinate System** | Absolute coordinates + offset stack | Canvas.save/restore + translate |
| **Resource Retrieval** | LynxRendererContext (ID lookup) | PlatformRendererContext (ID lookup) |
| **Border Drawing** | Generate UIImage as CALayer contents | BorderDrawingUtil direct Canvas drawing |
| **Text Rendering** | LynxTextLayer (CoreText) | TextLayout.draw(canvas) |
| **Image Rendering** | UIImageView + LynxImageManager | LynxImageManager.onDraw(canvas) |

---

## 9. Related File Index

### C++ Core

- `lynx/core/renderer/dom/fragment/display_list.h` - DisplayList definition, includes SubtreeProperty structure
- `lynx/core/renderer/dom/fragment/display_list_builder.h` - Builder pattern for constructing DisplayList
- `lynx/core/renderer/dom/fragment/fragment.h` - Fragment node, core drawing methods
- `lynx/core/renderer/dom/fragment/fragment_behavior.h` - Fragment behavior abstraction
- `lynx/core/renderer/ui_wrapper/painting/platform_renderer.h` - Platform renderer interface definition
- `lynx/core/renderer/ui_wrapper/painting/platform_renderer_impl.h` - Platform renderer base implementation

### iOS Platform (Darwin)

**C++ Bridge Layer**:
- `lynx/core/renderer/ui_wrapper/painting/ios/platform_renderer_darwin.h/.mm` - iOS platform renderer implementation
- `lynx/core/renderer/ui_wrapper/painting/ios/platform_renderer_context_darwin.h` - iOS rendering context

**Objective-C Platform Layer**:
- `lynx/platform/darwin/ios/lynx/renderer/LynxDisplayListApplier.h` - DisplayList parser (direct memory access)
- `lynx/platform/darwin/ios/lynx/renderer/LynxDisplayListApplier+Internal.h` - Internal interface, receives C++ pointers
- `lynx/platform/darwin/ios/lynx/renderer/LynxRenderer.h` - iOS renderer, manages CALayer tree
- `lynx/platform/darwin/ios/lynx/public/renderer/LynxRendererContext.h` - Rendering context, manages Text/Image resources

### Android Platform

**C++ Bridge Layer**:
- `lynx/core/renderer/ui_wrapper/painting/android/platform_renderer_android.h/.cc` - Android platform renderer implementation
- `lynx/core/renderer/ui_wrapper/painting/android/platform_renderer_context.h/.cc` - Android rendering context, JNI bridge

**Java Platform Layer**:
- `lynx/platform/android/lynx_android/src/main/java/com/lynx/tasm/behavior/render/DisplayListApplier.java` - DisplayList parsing and drawing
- `lynx/platform/android/lynx_android/src/main/java/com/lynx/tasm/behavior/render/DisplayList.java` - Java DisplayList data class
- `lynx/platform/android/lynx_android/src/main/java/com/lynx/tasm/behavior/render/PlatformRendererContext.java` - Platform renderer context
