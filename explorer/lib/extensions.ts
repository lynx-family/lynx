// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { openSchema, supportsSparklingContainer } from './navigation';
import type { ContainerRequest } from './navigation';

export const SPARKLING_GO_EXTENSION_ID = 'sparkling-go';
export const SPARKLING_GO_ROOT_SCHEME =
  'hybrid://lynxview_page?bundle=.%2FResource%2Fextensions%2Fsparkling-go%2Fmain.lynx.bundle&hide_status_bar=1&hide_nav_bar=1';

export interface ExplorerExtension {
  readonly id: string;
  readonly title: string;
  readonly description: string;
  readonly accentColor: string;
  readonly rootScheme: string;
  readonly version?: string;
}

export interface ExplorerExtensionCapabilities {
  readonly supportsSparklingContainer: boolean;
}

type ExtensionRouteOpener = (
  url: string,
  container: ContainerRequest,
  source?: 'showcase'
) => Promise<void>;

const SPARKLING_GO_EXTENSION: ExplorerExtension = {
  id: SPARKLING_GO_EXTENSION_ID,
  title: 'Sparkling Go',
  description: 'Sparkling playground and examples',
  accentColor: '#e10543',
  rootScheme: SPARKLING_GO_ROOT_SCHEME,
};

function hostCapabilities(): ExplorerExtensionCapabilities {
  return {
    supportsSparklingContainer: supportsSparklingContainer(),
  };
}

function hostSparklingVersion(): string | undefined {
  try {
    const value = ((lynx.__globalProps as unknown) as
      | Record<string, unknown>
      | undefined)?.explorerSparklingVersion;
    return typeof value === 'string' && value.length > 0 ? value : undefined;
  } catch {
    return undefined;
  }
}

export function getAvailableExtensions(
  capabilities: ExplorerExtensionCapabilities = hostCapabilities()
): readonly ExplorerExtension[] {
  return capabilities.supportsSparklingContainer
    ? [{ ...SPARKLING_GO_EXTENSION, version: hostSparklingVersion() }]
    : [];
}

export function launchExtension(
  extensionID: string,
  capabilities: ExplorerExtensionCapabilities = hostCapabilities(),
  open: ExtensionRouteOpener = openSchema
): Promise<void> {
  'background only';
  const extension = getAvailableExtensions(capabilities).find(
    ({ id }) => id === extensionID
  );
  if (!extension) {
    return Promise.reject(
      new Error(`[extensions] ${extensionID} is unavailable on this host.`)
    );
  }
  return open(extension.rootScheme, 'sparkling', 'showcase');
}
