# Harmony Lynx Library Plugin

Generates the global Harmony Autolink Registry HAR from npm packages that
declare `platforms.harmony` in `lynx.lib.json`.

Run the prebuild command before `ohpm install` so the generated HAR and source
Library modules are present when Hvigor creates the project model:

```bash
lynx-harmony-autolink --project-root .
ohpm install
```

The same generator can be registered in a HAP module's `hvigorfile.ts`:

```ts
import { hapTasks } from '@ohos/hvigor-ohos-plugin'
import { harmonyLynxLibraryPlugin } from '@lynx/lynx-library-plugin'

export default {
  system: hapTasks,
  plugins: [harmonyLynxLibraryPlugin()],
}
```

The generated package is named `@lynx/lynx_library_registry` and exposes only
`setupGlobal()`. The application imports and calls it once before creating a
Lynx runtime.
