# Android Explorer Sparkling container routing (phase 1)

Phase 1 keeps the Legacy Explorer container as the default for raw `http://`, `https://`, `assets://`, and `file://lynx?local://` bundle URLs while adding an explicit Sparkling route.

## LaunchDescriptor contract

`LaunchDescriptor` is the semantic model shared by Explorer route entry points. It preserves the original launch URL and normalized resource URL, initial data, launch/global props, page name, viewport (`width`, `height`, `density`), presentation (`fullscreen`, `orientation`), query parameters, and requested container type (`LEGACY` or `SPARKLING`).

Parsers:

- Legacy parser: raw HTTP/HTTPS/local bundle URLs and existing `file://lynx?...` wrapper URLs become a `LaunchDescriptor` that defaults to `LEGACY` for normal Open.
- Sparkling canonical parser: `sparkling://`, `sparkling-lynx://`, and `lynx-sparkling://` URLs always request `SPARKLING` and never fall back to Legacy when Sparkling navigation fails.

## Central RouteCoordinator entry points

All Android Explorer launches go through `RouteCoordinator`:

- manual/homepage `ExplorerModule.openSchema`
- explicit `ExplorerModule.openSchemaWithSparkling`
- `QRScanActivity`
- `LynxModuleAdapter.startFromUrl`
- `LynxModuleAdapter.startFromUrlSingleTop`
- `DebugBridgeActivity`
- existing `TemplateDispatcher.dispatchUrl` callers
- Sparkling `router.open`, via `SparklingNavigationRegistrar`

## Routing matrix

| Input | Action | Container |
| --- | --- | --- |
| raw bundle URL | Open | Legacy |
| raw bundle URL | Open with Sparkling | Sparkling |
| legacy wrapper URL | Open | Legacy |
| legacy wrapper URL | Open with Sparkling | Sparkling |
| canonical Sparkling scheme | Open or Open with Sparkling | Sparkling |
| legacy wrapper nesting canonical Sparkling URL | Open or Open with Sparkling | Sparkling |
| malformed/unsupported Sparkling URL | Error, no Legacy fallback |

## Legacy parameter to SparklingContext mapping

| Legacy parameter | LaunchDescriptor field | Sparkling mapping |
| --- | --- | --- |
| bundle URL | `resourceUrl` | `SparklingContext.scheme` and public `url`/`resourceUrl` setters when available |
| `page` / `page_name` | `pageName` | public `pageName` setter when available |
| query params | `queryParameters` | camel-cased page global props |
| `fullscreen` | `fullscreen` | page global prop |
| `width`, `height`, `density` | viewport fields | page global props and Sparkling public setters when available |
| `orientation` | `orientation` | page global prop |

Global prop precedence is: Explorer common props < Sparkling stable/container props < launch/page props. Legacy LynxView pages report `sparklingAvailable=false` and `sparklingNavigation=false`; only real Sparkling containers report Sparkling navigation capability.
