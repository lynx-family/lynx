# AGENTS.md

## Scope

This directory contains the cross-platform Textra text layout entry used by Lynx.
The main entry is `text_layout_textra.cc`.

Textra is responsible for:
- building paragraph content and style from Lynx text elements
- measuring text with TTText/Textra
- creating a `Page*` after layout
- attaching the page pointer to the text element as a text bundle for platform rendering

Textra is not responsible for:
- platform display list dispatch
- platform-specific drawing APIs
- old legacy text renderer paths

## Key Flow

1. `TextLayoutTextra::Measure(...)`
2. `api_->MeasureParagraph(...)`
3. `api_->GetPage(paragraph)`
4. `text_element->SetTextBundle(reinterpret_cast<intptr_t>(page))`
5. later, fragment painting updates the platform painting context with the text bundle
6. platform renderer consumes the page and draws it

## Important Concepts

- `Paragraph`: layout input object
- `Page`: drawable layout result after measurement/layout
- `Text bundle`: platform-facing handle that currently carries `Page*`
- `TextLayoutAPI`: abstraction implemented by platform/service-backed text engines

## Lifetime And Teardown

- `TextLayoutTextra` owns `TextLayoutAPI` and is responsible for releasing it.
- `TextLayoutTextra` teardown must destroy any remaining `ParagraphBuilder` and
  `Paragraph` objects before releasing `TextLayoutAPI`.
- Do not assume `TextElement::~TextElement()` will always trigger
  `DestroyText(element)` during page teardown. Page destroy may set
  `will_destroy=true` first and skip the per-element cleanup path.
- Async font callbacks may still arrive while page teardown is in progress, so
  `Paragraph` state must not outlive the `TextLayoutAPI` it depends on.

## Platform Split

- Android:
  page is consumed by `LynxTextService` and drawn through Android canvas helpers
- iOS:
  page is consumed by `LynxTextService` and drawn through `LynxTextraLayer` + CoreGraphics
  inline images/views in TextService mode are treated like `InlineView`s so
  Textra drives platform node measure/alignment instead of using generic image
  placeholder loading

## Editing Guidance

- If the task is about layout, style mapping, paragraph creation, or page creation, start here.
- If the task is about actual drawing on Android/iOS, do not stop here; continue into the platform service implementation.
- Keep cross-platform behavior here; do not add platform UI logic in this directory.
- When changing teardown behavior, review both paragraph ownership and API
  ownership together. Fixes that only move one side often create stale
  `Paragraph`, `Page`, or callback state.
- On iOS TextService, standalone image nodes are still the real render owner for
  inline images; the Textra entry only needs to wire placeholder/inline-view
  layout behavior.

## Related Files

- `lynx/core/renderer/ui_wrapper/layout/textra/text_layout_textra.cc`
- `lynx/core/renderer/ui_wrapper/layout/textra/text_layout_api.h`
- `service_impls/text_service/text_service_impl.cc`
- `service_impls/text_service/text_service_page.cc`
