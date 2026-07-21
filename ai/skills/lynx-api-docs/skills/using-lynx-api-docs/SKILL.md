---
name: using-lynx-api-docs
description: Provides authoritative guidance for Lynx development involving pages, templates, components, CSS, layouts, elements, web migration, rendering or styling bugs, and code review. Applies to *.ttml, Lynx *.tsx, and all Lynx-specific files, and requires searching the installed API docs before acting.

---

# Using Lynx API Docs

## Overview

**Pre-training knowledge is insufficient for Lynx.** Lynx is a rendering platform (like a browser) with its own elements, non-standard CSS behavior, unique layout systems, and web-incompatible defaults. Always retrieve from the installed API docs before writing or modifying Lynx page code.

## Core Rule

**MUST ALWAYS read the relevant API docs BEFORE writing or editing any Lynx code.**

Do not rely on web CSS knowledge. Do not assume standard HTML behavior. Do not guess element APIs. Lynx is a distinct platform with its own rules.

## Why Web Knowledge Fails

Lynx is **not** a web browser. Web assumptions produce broken Lynx code:

- **Elements**: `<view>`, `<text>`, `<image>` — not `<div>`, `<span>`, `<img>`
- **CSS defaults**: `border-box`, no margin collapsing, Linear layout (not Flow)
- **Properties**: Many CSS properties are unsupported or behave differently — always check `css/` docs

**Every web assumption is a potential bug.**

## How to Use the Docs

1. Identify what you're working on (layout, CSS, element, migration, pattern)
2. Look up the relevant doc file from the table below
3. **Read the doc BEFORE writing or editing code**
4. Apply the rules and constraints from the doc
5. If uncertain, search the `elements/` or `css/` directories

## Quick Reference

| Task | Read These Files First |
|------|------------------------|
| Choose layout | `layout/linear-layout.md` (default), `layout/flex-layout.md`, `layout/grid-layout.md`, `layout/relative-layout.md` |
| CSS properties/units | `css/supported-properties.md`, `css/values-and-units.md` |
| CSS selectors | `css/selectors.md`, `css/pseudo-classes.md` |
| Use an element | `elements/<element-name>.md` |
| Migrate from web | `lynx-vs-web/migration-guide.md`, `lynx-vs-web/css-differences.md` |
| Theming/animation/responsive | `patterns/theming.md`, `patterns/animation.md`, `patterns/responsive.md` |
| General lookup | `quick-reference.md`, `best-practices.md` |

## Core Rules

The compact layout decision rules and key CSS constraints are documented in `quick-reference.md`. Most critical:

- **Text** must use `<text>` component
- **Default box-sizing** is `border-box` (not `content-box`)
- **No margin collapsing**
- **Use `rem` + `vw`** for screen adaptation

## Red Flags — Stop and Read Docs

- Adding a `<div>` or `<span>` → Lynx uses `<view>`, `<text>`
- Using `margin` without checking if margin collapsing applies → It doesn't
- Assuming `content-box` → Default is `border-box`
- Using web CSS properties without checking `css/supported-properties.md`
- Guessing element attributes → Read `elements/<name>.md`
- Choosing `rpx` for web-portable code → `rpx` is Lynx-specific; use `rem` + `vw` for web compatibility
- Writing plain text without `<text>` wrapper → Invalid in Lynx
- Using web pseudo-classes without checking `css/pseudo-classes.md`
- Assuming standard HTML flow layout → Lynx uses linear layout by default
