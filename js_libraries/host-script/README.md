# Host Script

`@lynx-js/host-script` provides a non-owning reference to the single platform-created
LynxView associated with a Host Script runtime.

```ts
import {
  defineHostScript,
  LynxTemplateData,
  LynxUpdateMeta,
} from '@lynx-js/host-script';

defineHostScript(async ({ lynxView }) => {
  await lynxView.ready;
  const onLoaded = () => {
    lynxView.off('loadSuccess', onLoaded);
    lynxView.updateMetaData(new LynxUpdateMeta({
      updateData: new LynxTemplateData({ message: 'Hello' }),
    }));
  };
  lynxView.on('loadSuccess', onLoaded);
  lynxView.on('error', (code, message) => console.error(code, message));
  if (!lynxView.loadURL('https://example.com/main.lynx.bundle', {
    data: { message: 'Loading' },
    globalProps: { theme: 'light' },
  })) {
    lynxView.off('loadSuccess', onLoaded);
    throw new Error('Load request was not accepted');
  }
});
```

## Acceptance and completion

`available` reports whether a View is bound. `ready` is a lazily created, cached
`Promise<void>` that waits for binding, not page loading. It rejects if the View
is destroyed before binding or the runtime detaches while waiting.

All page operations return a synchronous boolean. `true` means that the platform
accepted the request for execution; it does not mean the page finished loading
or rendering. `false` means the request was not accepted, for example because
there is no current View or the platform queue rejected it. Invalid arguments
and serialization failures throw synchronously with code `INVALID_ARGUMENT`.
Use lifecycle events for page results and asynchronous errors. Do not use
`await lynxView.loadURL(...)` as a page completion signal.

`defineHostScript()` still accepts an asynchronous setup function. Its completion
reports entry initialization, independently of page loading.

## Page API

- `loadURL(url, { data?, globalProps? }?)` loads a URL.
- `loadTemplate({ template, url, initialData?, globalProps?, processor?, readOnly? })`
  loads a non-empty ArrayBuffer. URL-only loads use `loadURL`.
- `loadSSR({ data, url, initialData? })` and `hydrateSSR(...)` retain their existing
  binary input and options.
- `updateMetaData({ updateData?, globalProps? })` requires at least one field;
  supplied fields contain `LynxTemplateData` instances. Omitted fields are not
  updated. `LynxUpdateMeta` is an optional constructor for this structure.
- `setGlobalProps(object)` independently sets global props.
- `reloadTemplate({ data?, globalProps? }?)` retains the existing reload behavior.
- `sendGlobalEvent(name, ...args)` forwards positional arguments. For example,
  `sendGlobalEvent('event', { value: 1 }, [2], '[]')` delivers three arguments;
  the final string is not parsed as JSON.

`LynxTemplateData<T>` holds an object and returns it from `toObject()`. Objects must
be JSON serializable. Data is serialized at the time of each API call, so later
mutations do not change an already submitted request. JSON strings remain private
to the native adapter.

## Events and lifetime

`on(event, listener)` and `off(event, listener)` return the View reference for
chaining. `off` removes the last matching registration and is safe after View
loss. Registration requires an available View. The supported events remain
`ready`, `loadSuccess`, `firstScreen`, `pageUpdate`, `dataUpdated`, `error`, and
`destroyed`. Error listeners receive `(code, message)`; other events have no
arguments. `ready` means binding is complete, not that the page is ready to show.

Destroying the View invalidates the reference. Runtime detachment releases JS
listeners and suppresses queued or late callbacks. Host Script never owns or
creates the platform View.

## Alignment and migration

URL loading, metadata updates, global props, variadic events, and chainable
subscriptions follow the Lynxtron API shape. Host Script does not add a cache for
updates issued before the first page load: calls require a bound View and are
immediately submitted to the platform. Platform SDK behavior still applies.
File loading, pre-decoded template bundles, and desktop window APIs are outside
this package's scope.

Replace `updateData(data)` with `updateMetaData({ updateData: new LynxTemplateData(data) })`,
URL-only `loadTemplate({ url })` with `loadURL(url)`, and
`sendGlobalEvent(name, params)` with `sendGlobalEvent(name, ...params)` when
`params` is an array of positional arguments. Replace unsubscribe functions
returned from `on` with explicit `off(event, listener)` calls.
