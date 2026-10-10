import {
  HeadlessLynxView,
  HeadlessLynxViewOptions,
  LoadTemplateOptions,
} from './headless-lynx-view';
import { LynxEnv } from './lynx-env';
import { TemplateAllowlist } from './template-allowlist';
import { REQUIRE_TEMPLATE_ALLOWLIST } from './open-card-defaults';
import {
  WindowedLynxView,
  WindowedLynxViewOptions,
} from './windowed-lynx-view';

export type OpenCardState = 'loading' | 'loaded' | 'failed' | 'closed';
export type HeadlessOpenCardState = OpenCardState;

type OpenCardView = {
  loadTemplate(template: Buffer, options?: LoadTemplateOptions): Promise<void>;
  loadTemplateFromUrl(
    url: string,
    options?: Omit<LoadTemplateOptions, 'url'>
  ): Promise<void>;
  destroy(): void;
};

export interface OpenCard<TView extends OpenCardView> {
  id: number;
  url: string;
  view: TView;
  state: OpenCardState;
  createdAt: number;
  error?: Error;
}

export type HeadlessOpenCard = OpenCard<HeadlessLynxView>;
export type WindowedOpenCard = OpenCard<WindowedLynxView>;

interface OpenCardManagerOptions<TView extends OpenCardView, TViewOptions> {
  /** Allowed template directories and HTTP(S) URL path prefixes. */
  templateAllowlist?: readonly string[];
  /** Explicitly allow OpenCard to read any template URL or local file. */
  unsafeAllowAnyTemplate?: boolean;
  view?: TViewOptions;
  load?: Omit<LoadTemplateOptions, 'url'>;
  onCardLoaded?: (card: OpenCard<TView>) => void;
  onCardError?: (error: Error, card: OpenCard<TView>) => void;
}

export type HeadlessOpenCardManagerOptions = OpenCardManagerOptions<
  HeadlessLynxView,
  HeadlessLynxViewOptions
>;

export type WindowedOpenCardManagerOptions = OpenCardManagerOptions<
  WindowedLynxView,
  WindowedLynxViewOptions
>;

function normalizeError(error: unknown): Error {
  return error instanceof Error ? error : new Error(String(error));
}

class OpenCardManager<
  TView extends OpenCardView,
  TViewOptions extends { timeoutMs?: number }
> {
  private currentCard?: OpenCard<TView>;
  private nextId = 1;
  private installed = false;
  private readonly templateAccess?: TemplateAllowlist;

  constructor(
    private readonly options: OpenCardManagerOptions<TView, TViewOptions> = {},
    private readonly createView: (options?: TViewOptions) => TView
  ) {
    if (
      options.unsafeAllowAnyTemplate &&
      options.templateAllowlist !== undefined
    ) {
      throw new Error(
        'unsafeAllowAnyTemplate cannot be combined with templateAllowlist'
      );
    }
    if (
      !options.unsafeAllowAnyTemplate &&
      (REQUIRE_TEMPLATE_ALLOWLIST || options.templateAllowlist !== undefined)
    ) {
      this.templateAccess = new TemplateAllowlist(
        options.templateAllowlist ?? [],
        options.view?.timeoutMs
      );
    }
  }

  install(): void {
    if (this.installed) {
      return;
    }
    LynxEnv.setOpenCardCallback((url) => {
      void this.open(url).catch(() => undefined);
    });
    this.installed = true;
  }

  uninstall(): void {
    if (!this.installed) {
      return;
    }
    LynxEnv.clearOpenCardCallback();
    this.installed = false;
  }

  dispose(): void {
    this.uninstall();
    this.closeAll();
  }

  async open(url: string): Promise<OpenCard<TView>> {
    if (!url) {
      throw new Error('open card url must not be empty');
    }
    try {
      this.templateAccess?.assertAllowed(url);
    } catch (error) {
      // No card exists for this rejection; keep the current card intact.
      console.error(normalizeError(error).message);
      throw error;
    }
    this.closeCurrentCard();

    const view = this.createView(this.options.view);
    const card: OpenCard<TView> = {
      id: this.nextId++,
      url,
      view,
      state: 'loading',
      createdAt: Date.now(),
    };
    this.currentCard = card;

    try {
      if (this.templateAccess) {
        await view.loadTemplate(await this.templateAccess.read(url), {
          ...this.options.load,
          url,
        });
      } else {
        await view.loadTemplateFromUrl(url, this.options.load);
      }
      if (this.currentCard === card && card.state !== 'closed') {
        card.state = 'loaded';
        this.options.onCardLoaded?.(card);
      }
      return card;
    } catch (error) {
      const normalizedError = normalizeError(error);
      if (this.currentCard !== card || card.state === 'closed') {
        return card;
      }
      card.state = 'failed';
      card.error = normalizedError;
      this.currentCard = undefined;
      view.destroy();
      this.options.onCardError?.(normalizedError, card);
      throw normalizedError;
    }
  }

  close(id: number): void {
    if (this.currentCard?.id !== id) {
      return;
    }
    this.closeCurrentCard();
  }

  closeAll(): void {
    this.closeCurrentCard();
  }

  list(): Array<OpenCard<TView>> {
    return this.currentCard ? [this.currentCard] : [];
  }

  private closeCurrentCard(): void {
    const card = this.currentCard;
    if (!card) {
      return;
    }
    card.state = 'closed';
    this.currentCard = undefined;
    card.view.destroy();
  }
}

export class HeadlessOpenCardManager extends OpenCardManager<
  HeadlessLynxView,
  HeadlessLynxViewOptions
> {
  constructor(options: HeadlessOpenCardManagerOptions = {}) {
    super(options, (viewOptions) => new HeadlessLynxView(viewOptions));
  }
}

export class WindowedOpenCardManager extends OpenCardManager<
  WindowedLynxView,
  WindowedLynxViewOptions
> {
  constructor(options: WindowedOpenCardManagerOptions = {}) {
    super(options, (viewOptions) => new WindowedLynxView(viewOptions));
  }
}
