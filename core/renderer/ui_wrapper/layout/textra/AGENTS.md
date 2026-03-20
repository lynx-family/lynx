# Textra AGENTS Guide

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

## Platform Split

- Android:
  page is consumed by `LynxTextService` and drawn through Android canvas helpers
- iOS:
  page is consumed by `LynxTextService` and drawn through `LynxTextraLayer` + CoreGraphics

## Editing Guidance

- If the task is about layout, style mapping, paragraph creation, or page creation, start here.
- If the task is about actual drawing on Android/iOS, do not stop here; continue into the platform service implementation.
- Keep cross-platform behavior here; do not add platform UI logic in this directory.

## Related Files

- `lynx/core/renderer/ui_wrapper/layout/textra/text_layout_textra.cc`
- `lynx/core/renderer/ui_wrapper/layout/textra/text_layout_api.h`
- `service_impls/text_service/text_service_impl.cc`
- `service_impls/text_service/text_service_page.cc`
