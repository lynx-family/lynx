import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import test from "node:test";
import vm from "node:vm";

const bootstrapPath = new URL(
  "./lynx_recorder_load_script_bootstrap.js",
  import.meta.url,
);

function executeReplay(scripts) {
  const replayContext = {
    version: 1,
    config: { viewport: { width: 390, height: 844 } },
    actions: [{ "Function Name": "example" }],
    invokedMethods: [{ "Module Name": "example" }],
    callbacks: { 1: { value: "recorded" } },
    scripts,
  };
  const source = readFileSync(bootstrapPath, "utf8").replace(
    "__LYNX_RECORDER_REPLAY_CONTEXT__",
    JSON.stringify(replayContext),
  );
  const nativeLoads = [];
  const context = {
    __events: [],
    loadScript(url) {
      nativeLoads.push(String(url));
      return true;
    },
    setTimeout(callback) {
      callback();
      return 1;
    },
  };
  context.globalThis = context;
  vm.runInNewContext(source, context, { filename: bootstrapPath.pathname });
  return { context, nativeLoads, replayContext };
}

test("loads recorded adapters in URL order and chains their hooks", () => {
  const adapterA = `
    globalThis.__LYNX_RECORDER_REGISTER_REPLAY_ADAPTER__({
      id: "a",
      version: 1,
      beforeReplay(runtime) {
        globalThis.__events.push("before:a");
        globalThis.__contextSeen = runtime.context;
      },
      transformScript(url, source) {
        globalThis.__events.push("transform:a:" + url);
        return source.replace("base", "base-a");
      },
      afterScript(url) {
        globalThis.__events.push("after:a:" + url);
      },
      start(runtime) {
        globalThis.__events.push("start:a");
        runtime.loadScript("app.js");
      },
    });
  `;
  const adapterB = `
    globalThis.__LYNX_RECORDER_REGISTER_REPLAY_ADAPTER__({
      id: "b",
      version: 1,
      beforeReplay() { globalThis.__events.push("before:b"); },
      transformScript(url, source) {
        globalThis.__events.push("transform:b:" + url);
        return source.replace("base-a", "base-a-b");
      },
      afterScript(url) { globalThis.__events.push("after:b:" + url); },
      start() { globalThis.__events.push("start:b"); },
    });
  `;
  const { context, nativeLoads, replayContext } = executeReplay({
    "lynx-recorder://replay-adapter/b.js": adapterB,
    "app.js": "globalThis.__appValue = 'base';",
    "lynx-recorder://replay-adapter/a.js": adapterA,
  });

  assert.deepEqual(context.__events, [
    "before:a",
    "before:b",
    "start:a",
    "transform:a:app.js",
    "transform:b:app.js",
    "after:a:app.js",
    "after:b:app.js",
    "start:b",
  ]);
  assert.equal(context.__appValue, "base-a-b");
  assert.deepEqual(
    JSON.parse(JSON.stringify(context.__contextSeen)),
    replayContext,
  );
  assert.deepEqual(nativeLoads, []);
});

test("keeps legacy no-adapter script loading and native fallback", () => {
  const { context, nativeLoads } = executeReplay({
    "ttfile:///recorded.js": "globalThis.__recordedLoaded = true;",
  });

  assert.equal(context.loadScript("ttfile:///recorded.js"), true);
  assert.equal(context.__recordedLoaded, true);
  assert.equal(context.loadScript("native-only.js"), true);
  assert.deepEqual(nativeLoads, ["native-only.js"]);
});

test("isolates adapter errors and reports the failing stage", () => {
  const { context } = executeReplay({
    "lynx-recorder://replay-adapter/broken.js": `
      globalThis.__LYNX_RECORDER_REGISTER_REPLAY_ADAPTER__({
        id: "broken",
        version: 1,
        beforeReplay() { throw new Error("broken beforeReplay"); },
        start() { globalThis.__startedAfterError = true; },
      });
    `,
  });

  assert.equal(context.__startedAfterError, true);
  assert.equal(context.__LYNX_RECORDER_REPLAY_ERRORS__.length, 1);
  assert.equal(context.__LYNX_RECORDER_REPLAY_ERRORS__[0].adapter, "broken");
  assert.equal(context.__LYNX_RECORDER_REPLAY_ERRORS__[0].stage, "beforeReplay");
});

test("lets an adapter execute a recorded script with its own module runtime", () => {
  const { context } = executeReplay({
    "lynx-recorder://replay-adapter/modules.js": `
      globalThis.__LYNX_RECORDER_REGISTER_REPLAY_ADAPTER__({
        id: "modules",
        version: 1,
        executeScript(url, source) {
          globalThis.__events.push("execute:" + url);
          Function(source.replace("native", "adapter"))();
          return true;
        },
        start(runtime) { runtime.loadScript("module.js"); },
      });
    `,
    "module.js": "globalThis.__executionOwner = 'native';",
  });

  assert.equal(context.__executionOwner, "adapter");
  assert.deepEqual(context.__events, ["execute:module.js"]);
});

test("lets an adapter satisfy an intentionally missing runtime module", () => {
  const { context, nativeLoads } = executeReplay({
    "lynx-recorder://replay-adapter/missing.js": `
      globalThis.__LYNX_RECORDER_REGISTER_REPLAY_ADAPTER__({
        id: "missing",
        version: 1,
        handleMissingScript(url) {
          if (url !== "virtual.js") return false;
          globalThis.__virtualModuleReady = true;
          return true;
        },
      });
    `,
  });

  assert.equal(context.loadScript("virtual.js"), true);
  assert.equal(context.__virtualModuleReady, true);
  assert.deepEqual(nativeLoads, []);
});
