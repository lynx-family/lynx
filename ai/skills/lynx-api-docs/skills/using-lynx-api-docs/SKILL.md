---
name: using-lynx-api-docs
description: Authoritative Lynx engine references for generating, reviewing, migrating, or debugging Lynx TSX/TTML pages, CSS, layouts, selectors, and elements. Use for Lynx-specific styling and Web-to-Lynx differences; always retrieve the relevant installed docs before changing code.

---

# Using Lynx API Docs

**MUST read the relevant API docs before writing or editing Lynx code.** Lynx elements and CSS differ from the Web; do not fill gaps with HTML or browser assumptions.

**For repair tasks, diagnose and route each symptom before reading element documentation.** For layout, sizing, wrapping, or stacking symptoms, read the CSS differences index first. Do not infer element-prop defects from visual differences alone.

## Route the Task

| Task | Read first |
| --- | --- |
| **Generate or edit Lynx CSS; migrate Web HTML/CSS; debug a Lynx/Web style difference** | **`lynx-vs-web/css-differences.md`**, then only the linked topic files needed for the diagnosed task |
| Check a CSS property, value, unit, or function | `css/supported-properties.md` and `css/values-and-units.md`; also check the CSS differences index when Web compatibility matters |
| Write or debug selectors | `css/selectors.md` and `css/pseudo-classes.md` |
| Choose or debug a layout | `layout/linear-layout.md` (default), `layout/flex-layout.md`, `layout/grid-layout.md`, or `layout/relative-layout.md` |
| Use or debug an element | `elements/<element-name>.md` |
| Apply theming, responsive design, or animation | `patterns/theming.md`, `patterns/responsive.md`, or `patterns/animation.md` |
| Look for a complete example or performance guidance | `examples/`, `quick-reference.md`, or `best-practices.md` as needed |

For CSS work, the differences index is the generation entry point, not only a migration reference. Follow its exact topic links instead of loading the entire documentation bundle.

## Generation-Critical Defaults

- Use `<view>`, `<text>`, and `<image>`, not HTML elements; wrap text in `<text>`.
- Lynx defaults to Linear layout, not Web block flow.
- `box-sizing` initially uses `auto`, normally with border-box sizing; set an explicit value when compatibility settings may differ.
- Margins do not collapse.

These defaults are shortcuts, not a substitute for the task-specific references above.

## Repair Scope and Output

For debugging and visual-repair tasks:

- Diagnose before expanding retrieval. Read only topics tied to the observed defect; do not survey unrelated element, animation, or visual-effect documentation.
- Start from the observed defect and the smallest affected element or selector. Change only what is required for that defect; do not repair speculative differences or normalize unrelated code.
- Preserve unrelated styles, component props and behavior, content, assets, fonts, and routing. Do not apply a relevant rule, such as `box-sizing`, to other elements unless they share the diagnosed defect.
- Before answering, compare the result with the original and remove every edit not required by the diagnosis.
- Follow the user's output contract exactly. When complete files only are requested, return those files without retrieval notes, status updates, explanations, or other prose.
