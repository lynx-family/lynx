<!-- cspell:ignore hvigorconfig -->

# @lynx/lynx-library-plugin

Hvigor configuration plugin for HarmonyOS Lynx library Autolink.

Add the plugin to the project root `hvigor/hvigor-config.json5`:

```json5
{
  "modelVersion": "5.0.0",
  "dependencies": {
    "@lynx/lynx-library-plugin": "^0.1.0",
  },
}
```

Enable it once in the project root `hvigorconfig.ts`:

```ts
import * as hvigorApi from '@ohos/hvigor';
import { enableHarmonyLynxAutolink } from '@lynx/lynx-library-plugin';

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
