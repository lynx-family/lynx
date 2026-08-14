# Lynx API Docs Index

Use this file only to choose the smallest relevant reference. The installed package version defines the documentation baseline. Search [topics.jsonl](./topics.jsonl) by an exact property, element, symptom, or task keyword.

Do not load every file in a group. Start with one canonical document and follow its related links only when the task requires more context.

## Core

| Topic | Read when | Platforms |
| --- | --- | --- |
| [Best practices](./best-practices.md) | Use for performance, maintainability, accessibility, and general Lynx page authoring guidance. | all |

## CSS reference

| Topic | Read when | Platforms |
| --- | --- | --- |
| [Supported properties](./css/supported-properties.md) | Check whether a CSS property is registered and find the canonical compatibility topic for behavioral limits. | all |
| [Selectors](./css/selectors.md) | Check selector syntax and matching support. | all |
| [Values and units](./css/values-and-units.md) | Check Lynx CSS units, functions, value parsing, and timing-function syntax. | all |
| [Pseudo-classes and pseudo-elements](./css/pseudo-classes.md) | Check which pseudo-classes and pseudo-elements are parsed and matched. | all |

## CSS compatibility

| Topic | Read when | Platforms |
| --- | --- | --- |
| [Box model and compatibility mode](./css/compatibility/box-model.md) | Use for box-sizing defaults, auto behavior, and default versus W3C-aligned page modes. | all |
| [Transforms and stacking contexts](./css/compatibility/transforms-and-stacking.md) | Use for transform function support and transform-created stacking boundaries. | all |
| [Filters](./css/compatibility/filters.md) | Use for supported filter functions, unsupported Web filter forms, and alternatives. | all |
| [Text CSS compatibility](./css/compatibility/text.md) | Use for line-height, word-spacing, text-decoration, text-shadow, and cross-platform text painting limits. | android, ios, harmony |
| [Computed-style CSS text](./css/compatibility/computed-style.md) | Use when a computed-style query is empty, lossy, or unsuitable for feature detection. | all |
| [Invalid declarations and cascade](./css/compatibility/invalid-declarations.md) | Use when generated or merged styles contain a later invalid value for the same property. | all |
| [Background and border painting](./css/compatibility/backgrounds-and-borders.md) | Use for background origin and clip, cover sizing, repeat-y, transparent borders, and patterned-border geometry. | android, ios, harmony, fragment-layer, clay |

## Layout

| Topic | Read when | Platforms |
| --- | --- | --- |
| [Linear layout](./layout/linear-layout.md) | Use for the default Lynx sequential layout algorithm. | all |
| [Flex layout](./layout/flex-layout.md) | Use for Lynx Flexbox sizing, alignment, wrapping, and shrink behavior. | all |
| [Grid layout](./layout/grid-layout.md) | Use for the supported Lynx CSS Grid subset and placement rules. | all |
| [Relative layout](./layout/relative-layout.md) | Use for Lynx relative constraints and relative-id relationships. | all |

## Web migration

| Topic | Read when | Platforms |
| --- | --- | --- |
| [Lynx and Web CSS differences](./lynx-vs-web/css-differences.md) | Use for broad Web-to-Lynx CSS migration after a more specific compatibility topic is not available. | all |
| [Migration guide](./lynx-vs-web/migration-guide.md) | Use for end-to-end conversion of Web markup, styles, events, images, lists, and forms. | all |
| [Unsupported Web features](./lynx-vs-web/unsupported-features.md) | Use as a negative index of Web features that are absent or only partially implemented. | all |

## Elements

| Topic | Read when | Platforms |
| --- | --- | --- |
| [page element](./elements/page.md) | Use for the page root, rem reference root, and layout root. | all |
| [view element](./elements/view.md) | Use for the general-purpose Lynx container. | all |
| [text element](./elements/text.md) | Use for text nodes, nested text composition, props, and truncation. | all |
| [image element](./elements/image.md) | Use for bitmap loading, placeholders, modes, events, and animated images. | all |
| [list element](./elements/list.md) | Use for large data sets, list-item structure, and list scrolling events. | all |
| [input element](./elements/input.md) | Use for single-line input, focus, selection, and confirm actions. | all |
| [scroll-view element](./elements/scroll-view.md) | Use for general scrolling, scroll events, and imperative scrolling methods. | all |
| [scroll-coordinator element](./elements/scroll-coordinator.md) | Use for coordinated header and content scrolling. | all |
| [svg element](./elements/svg.md) | Use for native SVG rendering and Serval-mode constraints. | all |
| [textarea element](./elements/textarea.md) | Use for multiline input, row limits, events, and text methods. | all |
| [blur-view element](./elements/blur-view.md) | Use for the public blur container and platform-specific blur behavior. | android, ios, harmony |
| [refresh element](./elements/refresh.md) | Use for pull-to-refresh structure, state, events, and methods. | all |
| [viewpager element](./elements/viewpager.md) | Use for page swiping, page-change events, and selectTab. | all |
| [overlay element](./elements/overlay.md) | Use for overlay presentation, dismissal, events, and platform capabilities. | all |
| [webview element](./elements/webview.md) | Use for native WebView loading, messaging, and methods. | all |

## Patterns

| Topic | Read when | Platforms |
| --- | --- | --- |
| [Theming](./patterns/theming.md) | Use for CSS custom properties and theme switching. | all |
| [Responsive design](./patterns/responsive.md) | Use for viewport adaptation and responsive units. | all |
| [Animation](./patterns/animation.md) | Use for transitions, keyframes, and animation patterns. | all |

## Examples

| Topic | Read when | Platforms |
| --- | --- | --- |
| [Card list](./examples/card-list.md) | Use for a basic card-list composition example. | all |
| [Sticky header](./examples/sticky-header.md) | Use for a header that remains visible while content scrolls. | all |
| [Bottom navigation](./examples/bottom-nav.md) | Use for navigation fixed to the bottom edge. | all |
| [Sidebar layout](./examples/sidebar-layout.md) | Use for a sidebar and main-content composition. | all |
| [Masonry-style layout](./examples/waterfall.md) | Use for a two-column Grid approximation of masonry placement. | all |

## Deterministic lookup

Search `topics.jsonl` before searching prose. Each `canonical_for` term has exactly one owner, and every reference Markdown file is represented by one topic entry.
