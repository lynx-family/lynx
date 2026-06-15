import * as fs from 'fs';
import * as path from 'path';
import { fileURLToPath, pathToFileURL } from 'url';
import {
  HeadlessOpenCardManager,
  WindowedOpenCardManager,
} from './open-card-manager';
import { HeadlessLynxView } from './headless-lynx-view';
import { LynxEnv } from './lynx-env';
import { WindowedLynxView } from './windowed-lynx-view';

type TemplateKind = 'path' | 'url';
type TemplateInput = {
  templateSource: string;
  templateKind: TemplateKind;
};
type LynxView = HeadlessLynxView | WindowedLynxView;

type CliOptions = {
  mode: 'render' | 'preview';
  initialTemplate?: TemplateInput;
  outputPath: string;
  width: number;
  height: number;
  devicePixelRatio: number;
  timeoutMs: number;
  screenshotDelayMs: number;
  title: string;
  debugRouter: boolean;
  debugRouterSchema?: string;
};

const DEFAULT_WIDTH = 390;
const DEFAULT_HEIGHT = 844;
const DEFAULT_DPR = 2;
const DEFAULT_TIMEOUT_MS = 10000;
const DEFAULT_SCREENSHOT_DELAY_MS = 100;
const DEBUG_ROUTER_APP_INFO_KEYS = ['App', 'AppVersion'];
const DEBUG_ROUTER_APP_INFO_VALUES = ['NodeLynxCLI', '0.1.0'];
const URL_SCHEME_PATTERN = /^[A-Za-z][A-Za-z0-9+.-]*:/;

function printUsage(): void {
  console.log(`Usage:
  node-lynx [bundle|bundle-url] [--output <screenshot.png>] [--width 390] [--height 844]
  node-lynx --template <bundle> [--output <screenshot.png>] [--width 390] [--height 844]
  node-lynx --url <bundle-url> [--output <screenshot.png>] [--width 390] [--height 844]
  node-lynx render [bundle|bundle-url] [--output <screenshot.png>] [--width 390] [--height 844]
  node-lynx preview [bundle|bundle-url] [--width 390] [--height 844]

Options:
  -t, --template <path>            Local Lynx bundle path.
  -u, --url <url>                  Remote http(s) Lynx bundle URL.
  -o, --output <path>              PNG output path. Defaults to screenshot.png.
  -w, --width <number>             View width in CSS px. Defaults to ${DEFAULT_WIDTH}.
  --height <number>                View height in CSS px. Defaults to ${DEFAULT_HEIGHT}.
  --dpr, --device-pixel-ratio <n>  Device pixel ratio. Defaults to ${DEFAULT_DPR}.
  --title <text>                   Preview window title.
  --debug-router-schema <schema>   Connect debug-router with the given schema.
  --no-debug-router                Disable preview debug-router startup. Requires a template input.
  --timeout <ms>                   Load and frame timeout. Defaults to ${DEFAULT_TIMEOUT_MS}.
  --screenshot-delay <ms>          Delay after first frame before screenshot. Defaults to ${DEFAULT_SCREENSHOT_DELAY_MS}.
  -h, --help                       Show this help message.`);
}

function writeStdoutLine(message: string): Promise<void> {
  fs.writeSync(process.stdout.fd, `${message}\n`);
  return Promise.resolve();
}

function readValue(args: string[], index: number, name: string): string {
  const value = args[index + 1];
  if (!value || value.startsWith('-')) {
    throw new Error(`missing value for ${name}`);
  }
  return value;
}

function readOptionValue(
  args: string[],
  index: number,
  name: string
): { value: string; consumed: boolean } {
  const equalsIndex = name.indexOf('=');
  if (equalsIndex >= 0) {
    const optionName = name.slice(0, equalsIndex);
    const value = name.slice(equalsIndex + 1);
    if (!value || value.startsWith('-')) {
      throw new Error(`missing value for ${optionName}`);
    }
    return { value, consumed: false };
  }
  return { value: readValue(args, index, name), consumed: true };
}

function parsePositiveNumber(value: string, name: string): number {
  const parsed = Number(value);
  if (!Number.isFinite(parsed) || parsed <= 0) {
    throw new Error(`${name} must be a positive number`);
  }
  return parsed;
}

function parseNonNegativeNumber(value: string, name: string): number {
  const parsed = Number(value);
  if (!Number.isFinite(parsed) || parsed < 0) {
    throw new Error(`${name} must be a non-negative number`);
  }
  return parsed;
}

function isHttpUrl(value: string): boolean {
  return value.startsWith('http://') || value.startsWith('https://');
}

function parseArgs(argv: string[]): CliOptions | undefined {
  const args = [...argv];
  if (args.includes('-h') || args.includes('--help')) {
    printUsage();
    return undefined;
  }

  let mode: 'render' | 'preview' = 'render';
  if (args[0] === 'render' || args[0] === 'preview') {
    mode = args[0];
    args.shift();
  }

  let initialTemplate: TemplateInput | undefined;
  let outputPath = 'screenshot.png';
  let width = DEFAULT_WIDTH;
  let height = DEFAULT_HEIGHT;
  let devicePixelRatio = DEFAULT_DPR;
  let timeoutMs = DEFAULT_TIMEOUT_MS;
  let screenshotDelayMs = DEFAULT_SCREENSHOT_DELAY_MS;
  let title = 'Node Lynx Preview';
  let debugRouter = true;
  let debugRouterSchema: string | undefined;

  const setTemplate = (value: string, kind: TemplateKind): void => {
    if (initialTemplate) {
      throw new Error('template input was provided more than once');
    }
    initialTemplate = { templateSource: value, templateKind: kind };
  };

  for (let index = 0; index < args.length; index++) {
    const arg = args[index];
    if (arg === '-t' || arg === '--template' || arg.startsWith('--template=')) {
      const result = readOptionValue(args, index, arg);
      setTemplate(result.value, isHttpUrl(result.value) ? 'url' : 'path');
      if (result.consumed) {
        index++;
      }
    } else if (arg === '-u' || arg === '--url' || arg.startsWith('--url=')) {
      const result = readOptionValue(args, index, arg);
      if (!isHttpUrl(result.value)) {
        throw new Error('--url must be an http(s) URL');
      }
      setTemplate(result.value, 'url');
      if (result.consumed) {
        index++;
      }
    } else if (
      arg === '-o' ||
      arg === '--output' ||
      arg.startsWith('--output=')
    ) {
      const result = readOptionValue(args, index, arg);
      outputPath = result.value;
      if (result.consumed) {
        index++;
      }
    } else if (
      arg === '-w' ||
      arg === '--width' ||
      arg.startsWith('--width=')
    ) {
      const result = readOptionValue(args, index, arg);
      width = parsePositiveNumber(result.value, 'width');
      if (result.consumed) {
        index++;
      }
    } else if (arg === '--height' || arg.startsWith('--height=')) {
      const result = readOptionValue(args, index, arg);
      height = parsePositiveNumber(result.value, 'height');
      if (result.consumed) {
        index++;
      }
    } else if (
      arg === '--dpr' ||
      arg === '--device-pixel-ratio' ||
      arg.startsWith('--dpr=') ||
      arg.startsWith('--device-pixel-ratio=')
    ) {
      const result = readOptionValue(args, index, arg);
      devicePixelRatio = parsePositiveNumber(result.value, 'devicePixelRatio');
      if (result.consumed) {
        index++;
      }
    } else if (arg === '--title' || arg.startsWith('--title=')) {
      const result = readOptionValue(args, index, arg);
      title = result.value;
      if (result.consumed) {
        index++;
      }
    } else if (
      arg === '--debug-router-schema' ||
      arg.startsWith('--debug-router-schema=')
    ) {
      const result = readOptionValue(args, index, arg);
      debugRouterSchema = result.value;
      if (result.consumed) {
        index++;
      }
    } else if (arg === '--no-debug-router') {
      debugRouter = false;
    } else if (arg === '--timeout' || arg.startsWith('--timeout=')) {
      const result = readOptionValue(args, index, arg);
      timeoutMs = parsePositiveNumber(result.value, 'timeoutMs');
      if (result.consumed) {
        index++;
      }
    } else if (
      arg === '--screenshot-delay' ||
      arg.startsWith('--screenshot-delay=')
    ) {
      const result = readOptionValue(args, index, arg);
      screenshotDelayMs = parseNonNegativeNumber(
        result.value,
        'screenshotDelayMs'
      );
      if (result.consumed) {
        index++;
      }
    } else if (arg.startsWith('-')) {
      throw new Error(`unknown option: ${arg}`);
    } else if (!initialTemplate) {
      setTemplate(arg, isHttpUrl(arg) ? 'url' : 'path');
    } else {
      throw new Error(`unexpected argument: ${arg}`);
    }
  }

  if (!initialTemplate && !debugRouter) {
    throw new Error('missing Lynx bundle path or URL');
  }

  return {
    mode,
    initialTemplate,
    outputPath,
    width,
    height,
    devicePixelRatio,
    timeoutMs,
    screenshotDelayMs,
    title,
    debugRouter,
    debugRouterSchema,
  };
}

function initDebugRouter(schema?: string): void {
  LynxEnv.init();
  LynxEnv.setAppInfo(DEBUG_ROUTER_APP_INFO_KEYS, DEBUG_ROUTER_APP_INFO_VALUES);
  if (schema) {
    const connected = LynxEnv.connectDevtools(schema);
    if (!connected) {
      throw new Error(`failed to connect debug-router schema: ${schema}`);
    }
  }
  console.log('Node Lynx debug-router initialized. Connect to 127.0.0.1:8901.');
}

function waitForExitSignal(extraExitSignal?: Promise<void>): Promise<void> {
  return new Promise((resolve) => {
    const keepAlive = setInterval(() => undefined, 1000);
    let settled = false;
    const cleanup = (): void => {
      clearInterval(keepAlive);
      process.off('SIGINT', onSignal);
      process.off('SIGTERM', onSignal);
    };
    const finish = (message?: string): void => {
      if (settled) {
        return;
      }
      settled = true;
      cleanup();
      if (message) {
        console.log(message);
      }
      resolve();
    };
    const onSignal = (signal: NodeJS.Signals): void => {
      finish(`Node Lynx received ${signal}; exiting.`);
    };
    process.once('SIGINT', onSignal);
    process.once('SIGTERM', onSignal);
    if (extraExitSignal) {
      extraExitSignal.then(
        () => finish('Node Lynx debug-router close requested; exiting.'),
        (error: unknown) =>
          finish(`Node Lynx debug-router close failed: ${formatError(error)}`)
      );
    }
  });
}

function formatError(error: unknown): string {
  return error instanceof Error ? error.message : String(error);
}

function normalizeDebugRouterTemplateInput(
  input: string
): TemplateInput | undefined {
  const source = input.trim();
  if (!source) {
    return undefined;
  }
  if (isHttpUrl(source)) {
    return { templateKind: 'url', templateSource: source };
  }
  if (source.startsWith('file://')) {
    try {
      return { templateKind: 'path', templateSource: fileURLToPath(source) };
    } catch {
      return undefined;
    }
  }
  if (URL_SCHEME_PATTERN.test(source)) {
    return undefined;
  }
  return { templateKind: 'path', templateSource: source };
}

async function loadTemplateSourceIntoView(
  view: LynxView,
  kind: TemplateKind,
  source: string
): Promise<void> {
  if (kind === 'url') {
    await view.loadTemplateFromUrl(source);
    return;
  }
  const templatePath = path.resolve(source);
  const template = await fs.promises.readFile(templatePath);
  await view.loadTemplate(template, {
    url: pathToFileURL(templatePath).toString(),
  });
}

async function loadTemplateIntoView(
  view: LynxView,
  options: CliOptions
): Promise<void> {
  if (!options.initialTemplate) {
    throw new Error('missing Lynx bundle path or URL');
  }
  await loadTemplateSourceIntoView(
    view,
    options.initialTemplate.templateKind,
    options.initialTemplate.templateSource
  );
}

function registerDebugRouterWindowedOpenCardHandlers(
  options: CliOptions,
  closeView: () => void
): () => void {
  const openCards = new WindowedOpenCardManager({
    view: {
      width: options.width,
      height: options.height,
      devicePixelRatio: options.devicePixelRatio,
      timeoutMs: options.timeoutMs,
      title: options.title,
    },
    onCardLoaded(card) {
      console.log(`Node Lynx debug-router opened windowed card: ${card.url}`);
    },
    onCardError(error, card) {
      console.error(
        `Node Lynx debug-router windowed OpenCard failed: ${
          card.url
        }: ${formatError(error)}`
      );
    },
  });
  LynxEnv.setOpenCardCallback((url) => {
    const input = normalizeDebugRouterTemplateInput(url);
    if (!input) {
      console.warn(
        `Node Lynx debug-router OpenCard ignored unsupported URL: ${url}`
      );
      return;
    }
    void openCards.open(toOpenCardLoadUrl(input)).catch(() => undefined);
  });
  LynxEnv.setClosePageCallback(() => {
    try {
      openCards.closeAll();
      closeView();
    } catch (error: unknown) {
      console.error(
        `Node Lynx debug-router closePage failed: ${formatError(error)}`
      );
    }
  });
  return () => {
    openCards.dispose();
    LynxEnv.clearOpenCardCallback();
    LynxEnv.clearClosePageCallback();
  };
}

function toOpenCardLoadUrl(input: TemplateInput): string {
  if (input.templateKind === 'url') {
    return input.templateSource;
  }
  return pathToFileURL(path.resolve(input.templateSource)).toString();
}

function registerDebugRouterHeadlessOpenCardHandlers(
  options: CliOptions,
  closeView: () => void
): () => void {
  const openCards = new HeadlessOpenCardManager({
    view: {
      width: options.width,
      height: options.height,
      devicePixelRatio: options.devicePixelRatio,
      timeoutMs: options.timeoutMs,
    },
    onCardLoaded(card) {
      console.log(`Node Lynx debug-router opened card: ${card.url}`);
    },
    onCardError(error, card) {
      console.error(
        `Node Lynx debug-router OpenCard failed: ${card.url}: ${formatError(
          error
        )}`
      );
    },
  });
  LynxEnv.setOpenCardCallback((url) => {
    const input = normalizeDebugRouterTemplateInput(url);
    if (!input) {
      console.warn(
        `Node Lynx debug-router OpenCard ignored unsupported URL: ${url}`
      );
      return;
    }
    void openCards.open(toOpenCardLoadUrl(input)).catch(() => undefined);
  });
  LynxEnv.setClosePageCallback(() => {
    try {
      openCards.closeAll();
      closeView();
    } catch (error: unknown) {
      console.error(
        `Node Lynx debug-router closePage failed: ${formatError(error)}`
      );
    }
  });
  return () => {
    openCards.dispose();
    LynxEnv.clearOpenCardCallback();
    LynxEnv.clearClosePageCallback();
  };
}

function createDebugRouterCloseHandle(): {
  close: () => void;
  promise: Promise<void>;
} {
  let close: (() => void) | undefined;
  const promise = new Promise<void>((resolve) => {
    close = resolve;
  });
  return {
    close: () => close?.(),
    promise,
  };
}

export async function runNodeLynxCli(): Promise<void> {
  const options = parseArgs(process.argv.slice(2));
  if (!options) {
    return;
  }

  if (options.mode === 'preview') {
    if (options.debugRouter) {
      initDebugRouter(options.debugRouterSchema);
    }
    let cleanupDebugRouter = (): void => undefined;
    if (!options.initialTemplate) {
      const debugRouterClose = createDebugRouterCloseHandle();
      try {
        cleanupDebugRouter = registerDebugRouterWindowedOpenCardHandlers(
          options,
          debugRouterClose.close
        );
        await writeStdoutLine(
          'Node Lynx preview is waiting for DebugRouter OpenCard.'
        );
        await waitForExitSignal(debugRouterClose.promise);
      } finally {
        cleanupDebugRouter();
      }
      return;
    }

    const view = new WindowedLynxView({
      width: options.width,
      height: options.height,
      devicePixelRatio: options.devicePixelRatio,
      timeoutMs: options.timeoutMs,
      title: options.title,
    });
    try {
      if (options.debugRouter) {
        cleanupDebugRouter = registerDebugRouterWindowedOpenCardHandlers(
          options,
          () => view.close()
        );
      }
      await loadTemplateIntoView(view, options);
      await view.waitForFrame();
      console.log('Node Lynx preview window opened. Close the window to exit.');
      await view.waitUntilClosed();
    } finally {
      cleanupDebugRouter();
      view.destroy();
    }
    return;
  }

  if (options.debugRouter) {
    initDebugRouter(options.debugRouterSchema);
  }

  let cleanupDebugRouter = (): void => undefined;
  let debugRouterClose:
    | ReturnType<typeof createDebugRouterCloseHandle>
    | undefined;
  if (options.debugRouter) {
    debugRouterClose = createDebugRouterCloseHandle();
    cleanupDebugRouter = registerDebugRouterHeadlessOpenCardHandlers(
      options,
      debugRouterClose.close
    );
  }

  if (!options.initialTemplate) {
    try {
      await writeStdoutLine(
        'Headless Lynx is waiting for DebugRouter OpenCard.'
      );
      await waitForExitSignal(debugRouterClose?.promise);
    } finally {
      cleanupDebugRouter();
    }
    return;
  }

  const outputPath = path.resolve(options.outputPath);
  const view = new HeadlessLynxView({
    width: options.width,
    height: options.height,
    devicePixelRatio: options.devicePixelRatio,
    timeoutMs: options.timeoutMs,
  });
  try {
    await loadTemplateIntoView(view, options);
    const png = await view.screenshot({ settleMs: options.screenshotDelayMs });
    await fs.promises.mkdir(path.dirname(outputPath), { recursive: true });
    await fs.promises.writeFile(outputPath, png);
    console.log(`Headless Lynx screenshot written to: ${outputPath}`);
    if (options.debugRouter) {
      console.log('Headless Lynx is waiting for SIGINT or SIGTERM to exit.');
      await waitForExitSignal(debugRouterClose?.promise);
    }
  } finally {
    cleanupDebugRouter();
    view.destroy();
  }
}
