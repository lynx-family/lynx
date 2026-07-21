# Lynx API Docs - Engine AI Context Bundle

## Install Into Another Project

Use the npm CLI package to vendor this engine docs bundle into another project and inject a managed reference block into that project's root `AGENTS.md`. The package is intended to be consumed by upstream DSL framework skills or agents rather than directly by end developers.

```bash
npx @lynx-js/lynx-api-docs install
```

Optional flags:

- `--project <path>`: target another project root
- `--dest <path>`: override the default install location `.ai/lynx-api-docs`
- `--dry-run`: preview file changes without writing
- `--no-link`: skip linking to project-local agent skill directories
- `--link-claude`: link the skill to `<project>/.claude/skills/`
- `--link-codex`: link the skill to `<project>/.codex/skills/`
- `--link-trae`: link the skill to `<project>/.trae/skills/`
- `--link-all`: link the skill to all known project-local agent directories (default)
- `--skills-dir <path>`: link the skill to a custom directory

The CLI copies this engine context bundle into the target project and adds a compact `AGENTS.md` pointer so agents can retrieve the detailed references on demand. By default, it also links the installed skill into `.claude/skills/`, `.codex/skills/`, and `.trae/skills/`; use `--no-link` to skip those links.

This package is an AI context bundle, not a traditional JavaScript or native runtime API reference. It documents the public Lynx surface shipped in this repository.

This is the Lynx engine documentation entry point for upstream DSL framework skills and agents. It contains public documentation for CSS, layout, elements, patterns, examples, and best practices.

## Public Surface

This package documents only the public Lynx surface. Use the public element references included in the package instead of relying on undocumented tags or host-specific behavior.

## Quick Start

- [Quick reference](./skills/using-lynx-api-docs/quick-reference.md) - Frequently used CSS properties and value types
- [Best practices](./skills/using-lynx-api-docs/best-practices.md) - Performance guidance and development recommendations

## Layout Systems

Lynx provides four layout systems:

1. **[Linear layout](./skills/using-lynx-api-docs/layout/linear-layout.md)** - The default and most efficient layout, suitable for simple lists
2. **[Flex layout](./skills/using-lynx-api-docs/layout/flex-layout.md)** - CSS Flexbox, suitable for complex flexible layouts
3. **[Grid layout](./skills/using-lynx-api-docs/layout/grid-layout.md)** - A subset of CSS Grid, suitable for two-dimensional layouts
4. **[Relative layout](./skills/using-lynx-api-docs/layout/relative-layout.md)** - A Lynx-specific system for layouts based on relative constraints

## CSS Reference

- [Supported CSS properties](./skills/using-lynx-api-docs/css/supported-properties.md) - Complete property list
- [Selectors](./skills/using-lynx-api-docs/css/selectors.md) - Supported selector types
- [Values and units](./skills/using-lynx-api-docs/css/values-and-units.md) - Supported value and unit types
- [Pseudo-classes and pseudo-elements](./skills/using-lynx-api-docs/css/pseudo-classes.md) - Supported pseudo-classes

## Lynx-Specific Features

- [Lynx and Web CSS differences](./skills/using-lynx-api-docs/lynx-vs-web/css-differences.md) - Differences from standard CSS
- [Unsupported features](./skills/using-lynx-api-docs/lynx-vs-web/unsupported-features.md) - CSS features that Lynx explicitly does not support
- [Migration guide](./skills/using-lynx-api-docs/lynx-vs-web/migration-guide.md) - Migrating from the Web platform to Lynx

## Elements

- [page element](./skills/using-lynx-api-docs/elements/page.md) - Page root, CSS `rem` reference root, and layout root
- [view element](./skills/using-lynx-api-docs/elements/view.md) - General-purpose container for layout wrappers and visual styles
- [text element](./skills/using-lynx-api-docs/elements/text.md) - Text rendering, nested text composition, and truncation
- [image element](./skills/using-lynx-api-docs/elements/image.md) - Bitmap loading, placeholders, and animated-image playback control
- [list element](./skills/using-lynx-api-docs/elements/list.md) - Large data sets, `list-item` structure, and scroll-threshold events
- [input element](./skills/using-lynx-api-docs/elements/input.md) - Single-line input, text selection, and confirm actions
- [scroll-view element](./skills/using-lynx-api-docs/elements/scroll-view.md) - General-purpose scroll container, scroll events, and scrolling methods
- [scroll-coordinator element](./skills/using-lynx-api-docs/elements/scroll-coordinator.md) - Public coordinated header/content scrolling, collapse events, and scrolling methods
- [svg element](./skills/using-lynx-api-docs/elements/svg.md) - Native SVG rendering and Serval-mode considerations
- [textarea element](./skills/using-lynx-api-docs/elements/textarea.md) - Multiline input, row limits, and text methods
- [blur-view element](./skills/using-lynx-api-docs/elements/blur-view.md) - Public blur container, iOS material effects, and Android automatic-update control
- [refresh element](./skills/using-lynx-api-docs/elements/refresh.md) - Public pull-to-refresh container, refresh header, and refresh-state events
- [viewpager element](./skills/using-lynx-api-docs/elements/viewpager.md) - Public paging container, page-change events, and `selectTab`
- [overlay element](./skills/using-lynx-api-docs/elements/overlay.md) - Public overlay container, show/dismiss events, and platform overlay capabilities
- [webview element](./skills/using-lynx-api-docs/elements/webview.md) - Native WebView loading, message bridging, and method-based control

## Styling Patterns

- [Theming](./skills/using-lynx-api-docs/patterns/theming.md) - CSS custom properties and theme switching
- [Responsive design](./skills/using-lynx-api-docs/patterns/responsive.md) - Adapting to different screen sizes
- [Animation](./skills/using-lynx-api-docs/patterns/animation.md) - Animations and transitions

## Layout Examples

- [Card list](./skills/using-lynx-api-docs/examples/card-list.md) - A common card-list layout
- [Sticky header](./skills/using-lynx-api-docs/examples/sticky-header.md) - A header that remains fixed while scrolling
- [Bottom navigation](./skills/using-lynx-api-docs/examples/bottom-nav.md) - Navigation fixed to the bottom edge
- [Sidebar layout](./skills/using-lynx-api-docs/examples/sidebar-layout.md) - Sidebar and main content
- [Masonry-style layout](./skills/using-lynx-api-docs/examples/waterfall.md) - A two-column Grid approximation of masonry placement

## Usage Recommendations

### As Context for an AI Coding Agent

```typescript
const loadContext = (intent: string) => {
  const base = load('quick-reference.md');

  switch (intent) {
    case 'layout':
      return base + load('layout/' + layoutType + '.md');
    case 'element':
      return base + load('elements/' + elementName + '.md');
    case 'migration':
      return base + load('lynx-vs-web/migration-guide.md');
    default:
      return base + load('best-practices.md');
  }
};
```

### Context Size

- **Quick reference**: ~8 KB - Frequently used value types and properties
- **One layout system**: ~8 KB - Detailed layout guidance and examples
- **Complete context**: All documentation, loaded on demand

## Key Points

1. **Layout selection**: Use Linear for simple lists, Flex for complex layouts, and Grid for two-dimensional layouts.
2. **Default behavior**: An element with no `display` declaration uses Linear Layout by default, and text content must be placed in a `<text>` element.
3. **Box model**: The default is `border-box`, and margins do not collapse.
4. **Recommended units**: Use `rem` and `vw` for screen adaptation. `rpx` is Lynx-specific and fully supported, but is not Web-compatible.
5. **Performance first**: Minimize nesting, use CSS classes, and avoid dynamic layout changes.
6. **Element selection**: Read the corresponding element reference before using platform-specific capabilities.

## Feedback and Updates

This context bundle evolves with Lynx releases and developer feedback.
