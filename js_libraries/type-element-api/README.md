# `@lynx-js/type-element-api`

TypeScript declarations for the Element APIs provided by Lynx. This package contains no runtime implementation.

## API levels

Choose the narrowest entry point that contains the APIs your framework needs:

| Entry point                              | Intended use                                 | Included declarations                   |
| ---------------------------------------- | -------------------------------------------- | --------------------------------------- |
| `@lynx-js/type-element-api/stable`       | Production integrations                      | Stable APIs                             |
| `@lynx-js/type-element-api/experimental` | Integrations evaluating APIs that may change | Stable and experimental APIs            |
| `@lynx-js/type-element-api/internal`     | Lynx-owned infrastructure                    | Stable, experimental, and internal APIs |
| `@lynx-js/type-element-api`              | Production integrations                      | Stable APIs, equivalent to `stable`     |

Experimental declarations can change before they are promoted to stable. Internal declarations have no compatibility guarantee and are not intended for product code. The package root selects the stable contract; integrations that previously relied on the package's complete declaration surface should explicitly select `internal` or the narrower `experimental` entry point.

Stable declarations follow the package's semantic-versioning policy. While the package remains below `1.0.0`, an incompatible stable change requires a minor release; after `1.0.0`, it requires a major release. Experimental declarations may change incompatibly in a minor release, while internal declarations may change in any release.

These levels describe the TypeScript compatibility contract, not runtime availability. APIs gated by a Lynx version, platform, or feature switch still require the corresponding runtime capability.

## Installation

Framework packages should declare this package as a peer dependency:

```json
{
  "peerDependencies": {
    "@lynx-js/type-element-api": "^0.1.0"
  }
}
```

Applications can install it as a development dependency:

```json
{
  "devDependencies": {
    "@lynx-js/type-element-api": "^0.1.0"
  }
}
```

## Usage

Use a type-only import to load the selected global declarations without producing a runtime import:

```typescript
import type { ElementRef } from '@lynx-js/type-element-api/stable';

const view: ElementRef = __CreateView(10);
```

To evaluate experimental APIs, change the import to `@lynx-js/type-element-api/experimental`. Use the `internal` entry point only for Lynx-owned infrastructure that requires the complete legacy declaration set.
