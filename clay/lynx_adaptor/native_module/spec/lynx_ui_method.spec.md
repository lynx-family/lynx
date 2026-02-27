# Lynx UI Method Invocation & Targeting Spec

> Scope: This document describes how Lynx UI methods (`invokeUIMethod`) locate target nodes and what the cross-platform call chains look like. It covers code under `lynx/`, `platform/`, and `oss/clay/`.

---

## 1. Terminology and ID Types

Across layers (DOM / TASM / platform UI tree), "finding a node" involves different IDs:

- **`component_id` (string)**: Component instance id used as the query root (component subtree).
  - JS source: `NodeSelectToken.component_id` (`SelectorQuery`) or `NodeRef._rootComponentId` (`NodeRef`).
- **`element_id` / `unique_id` (number / string)**: Unique element id (often seen as `uid` in event callbacks). It can be used to:
  - Set the search root (`root_unique_id`, higher priority than `component_id`)
  - Select by id (`IdentifierType.UNIQUE_ID` / `NodeSelectOptions::ELEMENT_ID`)
  - References: `lynx/js_libraries/lynx-core/src/modules/selectorQuery/interface.ts`, `lynx/core/renderer/dom/vdom/radon/node_select_options.h`
- **`ui_impl_id` / `sign` (int32)**: The "handle/index" of a node in the platform UI tree (UI method execution typically lands on this).
  - `SelectorQuery/nativeApp.invokeUIMethod` selects nodes first and then converts them to `ui_impl_id`.
  - The Lepus RenderFunction path can call directly with `ui_impl_id`.
  - References: `lynx/core/renderer/page_proxy.cc`, `lynx/core/runtime/bindings/lepus/renderer_functions.cc`
- **`idSelector` (string)**: Platform UI-tree attribute (JS usually writes `#id` in NodeRef id mode, while the stored value is typically `id` without `#`).
  - Android: `LynxBaseUI.setIdSelector` (`lynx/platform/android/.../LynxBaseUI.java`)
  - iOS: `LynxUI.idSelector` (`lynx/platform/darwin/ios/lynx/ui/...`)
  - Harmony: `UIBase::id_selector_` (`lynx/platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.cc`)
  - Clay: `clay::BaseView::id_selector_` (`oss/clay/ui/component/base_view.cc`)
- **`react-ref` (string)**: ReactLynx ref identifier. When `_isCallByRefId` is true, NodeRef uses this as the selector.
  - Android prop name: `PropsConstants.REACT_REF_ID = "react-ref"` (`lynx/platform/android/.../PropsConstants.java`)
  - iOS: `LynxUI.refId`
  - Harmony: `UIBase::react_ref_id_` (`lynx/platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.cc`)
  - Clay: `BaseView::ref_id_selector_` (`oss/clay/ui/component/base_view.*`)

---

## 2. Targeting Modes (How the Target Node Is Located)

`invokeUIMethod` ultimately needs to land on a single UI node. There are three major targeting modes:

### 2.1 Direct `ui_impl_id/sign` (Direct UI Handle)

- Typical entry: **Lepus RenderFunction `InvokeUIMethod`**
  - Accepts element id array or a `FiberElement`, and ends up using `impl_id()` / `ui_impl_id`.
  - References: `lynx/core/runtime/bindings/lepus/renderer_functions.cc`, `lynx/core/renderer/template_assembler.cc`

Use case: internal renderer/runtime already has the target handle; shortest path.

### 2.2 `SelectorQuery` (DOM Selector -> `ui_impl_id` -> UI Invoke)

On the JS side, `SelectorQuery` calls `nativeApp.invokeUIMethod(...)`:

- **Identifier type** (JS: `lynx/js_libraries/lynx-core/src/modules/selectorQuery/interface.ts`)
  - `ID_SELECTOR`: DOM CSS selector
  - `REF_ID`: match by `react-ref` attribute
  - `UNIQUE_ID`: match by `element_id` (`uid` in event callbacks)
- **Root selection**
  - `component_id`: search within the component subtree
  - `root_unique_id`: use an element unique id as root (**higher priority** than `component_id`)
  - References: comment on `NodeSelectToken.root_unique_id`, and `NodeSelectRoot` in `lynx/core/renderer/dom/vdom/radon/node_select_options.h`

JS/C++ identifier mapping (numeric values are aligned):

| JS (`IdentifierType`) | C++ (`NodeSelectOptions::IdentifierType`) | Meaning | Typical `identifier` |
| --- | --- | --- | --- |
| `ID_SELECTOR` (0) | `CSS_SELECTOR` (0) | DOM CSS selector | `"#video"` / `".cls"` |
| `REF_ID` (1) | `REF_ID` (1) | React ref (`react-ref`) | `"myRef"` |
| `UNIQUE_ID` (2) | `ELEMENT_ID` (2) | element unique id (`uid/element_id`) | `"12345"` |

Use case: standard querying and event-callback targeting; best cross-platform consistency.

### 2.3 `NodeRef` (Platform UI-Tree Traversal, Legacy getNodeRef Compatibility)

JS `NodeRef.invoke(...)` calls `nativeLynxUIModule.invokeUIMethod(...)` (it does **not** go through DOM selector):

- Signature: `(componentId, nodes: string[], method, params, callback)`
  - `nodes` is a chained path: each segment searches inside the current node's subtree.
    - Most platform implementations behave like **DFS-first-match per segment**: for each segment, it searches the whole subtree (not necessarily direct children), picks the first match, and then uses it as the next segment's root.
  - `_isCallByRefId` switches the meaning of `nodes`:
    - **`false/undefined`**: `nodes` is a `#id` path (id selector only)
    - **`true`**: `nodes` is a `react-ref` path (refId)
  - Reference: `lynx/js_libraries/lynx-core/src/modules/selectorQuery/nodeRef.ts`

Use case: legacy getNodeRef / historical code paths; capabilities are platform-dependent (see §4.2).

---

## 3. Call Chains (By Entry)

### 3.1 JS: `SelectorQuery` -> `nativeApp.invokeUIMethod` (Main Path)

1. JS builds a token and commits a task
   - `SelectorQuery.select/selectAll/selectReactRef/selectUniqueID` builds `NodeSelectToken`
   - `NodesRef.invoke()` calls `nativeApp.invokeUIMethod(...)`
   - Reference: `lynx/js_libraries/lynx-core/src/modules/selectorQuery/SelectorQuery.ts`

2. JSI host parses args into `NodeSelectRoot` + `NodeSelectOptions`
   - host function `invokeUIMethod` -> `App::InvokeUIMethod(...)`
   - Reference: `lynx/core/runtime/bindings/jsi/js_app.cc`

3. Engine selects nodes and then invokes UI method
   - `RuntimeMediator::InvokeUIMethod` -> `LynxEngine::InvokeUIMethod`
   - `page_proxy()->GetLynxUI(root, options)` selects the target node(s) and produces `ui_impl_id`
   - References: `lynx/core/shell/runtime_mediator.cc`, `lynx/core/shell/lynx_engine.cc`, `lynx/core/renderer/page_proxy.cc`

4. Branch: PaintingContext vs NativeFacade (controlled by `use_invoke_ui_method_func_`)
   - `TasmMediator::InvokeUIMethod`:
     - if `invoke_ui_method_func_` is injected: **PaintingContext branch**
     - else: **NativeFacade branch**
   - Injection is configured by `LynxShellBuilder::SetUseInvokeUIMethodFunction(true)` which binds `PaintingContext::InvokeUIMethod(ui_impl_id, ...)` into `TasmMediator`.
   - References: `lynx/core/shell/tasm_mediator.cc`, `lynx/core/shell/lynx_shell_builder.cc`

5. Platform sinks (illustrative)
   - PaintingContext branch: `PaintingContext*::InvokeUIMethod(...)`
     - Android: `lynx/core/renderer/ui_wrapper/painting/android/painting_context_android.cc`
     - iOS: `lynx/core/renderer/ui_wrapper/painting/ios/painting_context_darwin.mm`
     - Harmony: `lynx/core/renderer/ui_wrapper/painting/harmony/painting_context_harmony.cc`
     - Clay (Desktop): `oss/clay/lynx_adaptor/painting_context_clay.cc`
   - NativeFacade branch: `NativeFacade*::InvokeUIMethod(...)`
     - Android: `lynx/core/shell/android/native_facade_android.cc`
     - iOS: `lynx/core/shell/ios/native_facade_darwin.mm`
     - Harmony: `lynx/core/shell/harmony/native_facade_harmony.cc`
     - Embedder: `lynx/platform/embedder/core/native_facade_impl.cc`

### 3.2 JS: `NodeRef.invoke` -> `nativeLynxUIModule.invokeUIMethod` (Compat Path)

1. JS directly calls the native module
   - Reference: `lynx/js_libraries/lynx-core/src/modules/selectorQuery/nodeRef.ts`

2. Platform module: `LynxUIMethodModule.invokeUIMethod` (platform-specific)
   - Android (Java): `lynx/platform/android/lynx_android/src/main/java/com/lynx/jsbridge/LynxUIMethodModule.java`
   - iOS (ObjC): `lynx/platform/darwin/ios/lynx/module/LynxUIMethodModule.mm`
   - Clay (C++): `oss/clay/lynx_adaptor/native_module/lynx_ui_method_module.cc`
   - Harmony (C++ CAPI): `lynx/platform/harmony/lynx_harmony/src/main/cpp/module/lynx_ui_method_module.cc`

3. Traverse the platform UI tree and invoke the method
   - Android: `LynxUIOwner.invokeUIMethod(componentId, nodes, ...)`
     - Reference: `lynx/platform/android/lynx_android/src/main/java/com/lynx/tasm/behavior/LynxUIOwner.java`
   - iOS: `LynxUIOwner invokeUIMethod:params:callback:fromRoot:toNodes:`
     - Reference: `lynx/platform/darwin/ios/lynx/ui/LynxUIOwner.m`
   - Harmony: `UIOwner::InvokeUIMethod(component_id, node, ...)` -> `UIBase::FindViewById(node, by_ref_id)`
     - References: `lynx/platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.cc`, `lynx/platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.cc`
   - Clay: `ViewContext::FindViewByComponentId` + `FindViewByIdSelector/FindViewByRefIdSelector` + registrar invoke
     - References: `oss/clay/ui/component/view_context.cc`, `oss/clay/ui/component/base_view.cc`, `oss/clay/lynx_adaptor/native_module/lynx_ui_method_module.cc`

### 3.3 Lepus: RenderFunction `InvokeUIMethod` (Direct `ui_impl_id`)

1. Lepus binding parses element ids / `FiberElement.impl_id()`
   - Reference: `lynx/core/runtime/bindings/lepus/renderer_functions.cc`

2. `TemplateAssembler::LepusInvokeUIMethod(...)` -> `Catalyzer::Invoke(ui_impl_id, ...)`
   - References: `lynx/core/renderer/template_assembler.cc`, `lynx/core/renderer/ui_wrapper/painting/catalyzer.cc`

3. `PaintingContext::Invoke(...)` -> platform `PaintingContext*::Invoke(...)`
   - Reference: `lynx/core/renderer/ui_wrapper/painting/painting_context.h` and per-platform implementations

---

## 4. Platform Differences

### 4.1 `use_invoke_ui_method_func_` (Affects Call Chains That Go Through `TasmMediator::InvokeUIMethod`)

This switch determines whether `SelectorQuery/nativeApp.invokeUIMethod` ends up in PaintingContext or NativeFacade:

- **Embedder (macOS/Windows/Linux)**: always enabled (PaintingContext)
  - `lynx/platform/embedder/core/lynx_template_renderer.cc`: `.SetUseInvokeUIMethodFunction(true)`
- **Harmony**: always enabled (PaintingContext)
  - `lynx/platform/harmony/lynx_harmony/src/main/cpp/lynx_template_renderer.cc`: `.SetUseInvokeUIMethodFunction(true)`
- **Android**: decided by `ILynxUIRenderer.useInvokeUIMethod()`
  - JNI builder: `lynx/core/shell/android/lynx_template_render_android.cc`
  - Default `LynxUIRenderer` returns `false` (NativeFacade): `lynx/platform/android/lynx_android/src/main/java/com/lynx/tasm/behavior/LynxUIRenderer.java`
  - `LynxUIRendererClay` returns `true` (PaintingContext): `platform/android/lynx_android/src/main/java/com/lynx/tasm/behavior/LynxUIRendererClay.java`
- **iOS**: decided by `LynxUIRendererProtocol.useInvokeUIMethodFunction`
  - builder: `lynx/platform/darwin/ios/lynx/LynxTemplateRenderHelper.mm`
  - Default `LynxUIRenderer` returns `NO` (NativeFacade): `lynx/platform/darwin/ios/lynx/LynxUIRenderer.mm`
  - `LynxUIRendererClay` returns `YES` (PaintingContext): `platform/darwin/ios/lynx/clay/LynxUIRendererClay.mm`

> Note: `NodeRef.invoke` goes through `LynxUIMethodModule` and is not affected by this switch.

### 4.2 `NodeRef.invoke` Capability Differences (Compat Path)

Even though the JS entry is the same, platform implementations differ:

- **Android (Java UI tree)**: supports chained `nodes[]`; supports `_isCallByRefId` (match by `react-ref`); id mode requires `#...`; traversal skips `component` subtrees.
  - Traversal: `lynx/platform/android/lynx_android/src/main/java/com/lynx/tasm/behavior/LynxUIOwner.java`
  - Storage: `lynx/platform/android/lynx_android/src/main/java/com/lynx/tasm/behavior/ui/LynxBaseUI.java` (`setRefIdSelector`)
- **iOS (ObjC UI tree)**: supports chained `nodes[]`; supports `_isCallByRefId` (match by `refId/react-ref`); traversal skips `LynxUIComponent` subtrees.
  - Traversal: `lynx/platform/darwin/ios/lynx/ui/LynxUIOwner.m`
- **Clay (C++ UI tree; Desktop embedder shares this implementation)**: supports chained `nodes[]`; supports `_isCallByRefId` (match by `react-ref`).
  - Module: `oss/clay/lynx_adaptor/native_module/lynx_ui_method_module.cc`
  - Registration: `clay/lynx_adaptor/desktop_embedder.cc` -> `oss/clay/lynx_adaptor/native_module/lynx_module_factory.cc`
  - Lookup: `oss/clay/ui/component/view_context.cc`
    - Note: current lookup is full-subtree DFS and does **not** skip component subtrees, so NodeRef may "cross component boundaries" compared to Android/iOS/Harmony.
  - Empty `nodes[]` returns `kParamInvalid`; unlike Android/iOS, Clay does not fall through to invoking the method on the root UI.
  - Extra behavior: `EnsureInvokeAfterLayout` defers invocation by one frame when there are dirty layout nodes.
- **Harmony (C++ CAPI)**: only uses `nodes[0]` (no chained `nodes[]`); supports `_isCallByRefId` (match by `react-ref`); traversal skips component subtrees.
  - Module: `lynx/platform/harmony/lynx_harmony/src/main/cpp/module/lynx_ui_method_module.cc` (strips leading `#` if present)
  - Lookup: `lynx/platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.cc` -> `lynx/platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.cc`

---

## 5. Debugging Shortcuts

- To understand **how nodes are selected** (SelectorQuery): start at `LynxEngine::InvokeUIMethod` -> `PageProxy::GetLynxUI`
  - `lynx/core/shell/lynx_engine.cc`, `lynx/core/renderer/page_proxy.cc`
- To determine whether **PaintingContext or NativeFacade** is used: check whether `TasmMediator::InvokeUIMethod` has `invoke_ui_method_func_` installed
  - `lynx/core/shell/tasm_mediator.cc`, `lynx/core/shell/lynx_shell_builder.cc`
- When **NodeRef behaves differently from SelectorQuery**: confirm you are on the `LynxUIMethodModule` compat path and whether the platform supports chaining / `_isCallByRefId`
  - JS entry: `lynx/js_libraries/lynx-core/src/modules/selectorQuery/nodeRef.ts`
