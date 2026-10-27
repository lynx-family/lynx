import {
  HeadlessLynxView,
  HeadlessLynxViewOptions,
  LoadTemplateOptions,
} from './headless-lynx-view';
import { LynxEnv } from './lynx-env';
import { fetchPublicOpenCardSource } from './open-card-source';
import {
  WindowedLynxView,
  WindowedLynxViewOptions,
} from './windowed-lynx-view';

export type OpenCardState = 'loading' | 'loaded' | 'failed' | 'closed';
export type HeadlessOpenCardState = OpenCardState;

type OpenCardView = {
  loadTemplate(input: Buffer, options?: LoadTemplateOptions): Promise<void>;
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

type OpenCardViewOptions = { timeoutMs?: number };

interface OpenCardManagerOptions<
  TView extends OpenCardView,
  TViewOptions extends OpenCardViewOptions
> {
  view?: TViewOptions;
  load?: Omit<LoadTemplateOptions, 'url'>;
  /**
   * Let `install()` open any URL DebugRouter sends, including local files and
   * loopback or private hosts. By default only public http(s) URLs are opened
   * and every redirect is re-checked. `open()` is not affected.
   */
  allowAnySource?: boolean;
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
  TViewOptions extends OpenCardViewOptions
> {
  private currentCard?: OpenCard<TView>;
  private nextId = 1;
  private installed = false;

  constructor(
    private readonly options: OpenCardManagerOptions<TView, TViewOptions> = {},
    private readonly createView: (options?: TViewOptions) => TView
  ) {}

  install(): void {
    if (this.installed) {
      return;
    }
    LynxEnv.setOpenCardCallback((url) => {
      const opening = this.options.allowAnySource
        ? this.open(url)
        : this.openPublicSource(url);
      void opening.catch(() => undefined);
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

  open(url: string): Promise<OpenCard<TView>> {
    return this.openWith(url, (view) =>
      view.loadTemplateFromUrl(url, this.options.load)
    );
  }

  private openPublicSource(url: string): Promise<OpenCard<TView>> {
    return this.openWith(url, async (view) => {
      const template = await fetchPublicOpenCardSource(
        url,
        this.options.view?.timeoutMs
      );
      await view.loadTemplate(template, { ...this.options.load, url });
    });
  }

  private async openWith(
    url: string,
    load: (view: TView) => Promise<void>
  ): Promise<OpenCard<TView>> {
    if (!url) {
      throw new Error('open card url must not be empty');
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
      await load(view);
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
