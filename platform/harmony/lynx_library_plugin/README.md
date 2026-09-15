<!-- cspell:ignore hvigorconfig -->

# @lynx-js/lynx-library-plugin

Hvigor configuration plugin for HarmonyOS Lynx library Autolink.

Add the plugin to the project root `hvigor/hvigor-config.json5`:

```json5
{
  "modelVersion": "5.0.0",
  "dependencies": {
    "@lynx-js/lynx-library-plugin": "^0.1.0",
  },
}
```

Enable it once in the project root `hvigorconfig.ts`:

```ts
import * as hvigorApi from '@ohos/hvigor';
import { enableHarmonyLynxAutolink } from '@lynx-js/lynx-library-plugin';

enableHarmonyLynxAutolink(hvigorApi, { moduleName: 'entry' });
```

`moduleName` can be omitted when exactly one entry or feature HAP module depends
on `@lynx/lynx`.

Before Hvigor creates the module graph, the plugin discovers installed npm
packages that declare `platforms.harmony` in `lynx.lib.json`, generates a
Registry HAR under the project's ignored
`.hvigor/lynx-autolink/<moduleName>` cache directory, and includes the Registry
and library HAR nodes through the Hvigor config API. JSON5 project metadata is
parsed by Hvigor's public `parseJsonFile` API, so the plugin has no runtime npm
dependencies. After the target HAP is evaluated, the plugin adds the generated
dependency, resource directory, and AppStartup profile through HAP model APIs.
A generated Hvigor task restores the HAP-local AppStartup sources after `clean`
and before each target's `PreBuild`.
Application source files and checked-in build profiles are not modified.

The Harmony manifest entry supports both platform providers and Node-API
addons:

```json
{
  "platforms": {
    "harmony": {
      "packageDir": "harmony",
      "providerExportName": null,
      "nodeApiAddons": [
        {
          "name": "DemoModule",
          "libraryName": "DemoModule",
          "initializerExportName": "initializeNodeApiAddon",
          "required": true
        }
      ]
    }
  }
}
```

- Omit `providerExportName` to import the legacy
  `LynxLibraryProviderImpl` export.
- Set `providerExportName` to `null` for a Node-API-only HAR.
- `initializerExportName` names an ArkTS function exported by the HAR. The
  generated AppStartup registry calls every initializer before
  `LynxLibraryRegistry.setupGlobal`.
- A required initializer error fails AppStartup. An optional initializer error
  is logged and provider registration continues.

## Publishing

The plugin is published to npm independently of the Harmony SDK HAR packages.
Update `version` in this package's `package.json` and merge to `develop` to
publish a new version. The `harmony_library_plugin_publish` workflow checks
the public registry, runs the tests, and installs the tarball in a temporary
consumer to check CommonJS and ESM imports and TypeScript declarations. It
publishes only versions that are not already available.
Registry errors other than a missing version fail the workflow.

The workflow can also be run manually on `develop` with an npm tag. The default
tag is `latest`; prerelease versions require a different tag, such as `next`.
Rerunning an already published version does not change its tags.

Publishing uses npm trusted publishing from the GitHub `npm` environment.
Before enabling automated publishing, an npm maintainer with access to the
`@lynx-js` scope must configure its trusted publisher with these values:

- Organization: `lynx-family`
- Repository: `lynx`
- Workflow filename: `harmony-library-plugin-publish.yml`
- Environment: `npm`
- Allowed action: `npm publish`

After configuring the publisher, merge a version update or run the workflow
manually to publish subsequent versions without an npm token.
