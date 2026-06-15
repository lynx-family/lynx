#!/usr/bin/env node

import * as fs from 'fs';

import { runNodeLynxCli } from './cli-main';

export { runNodeLynxCli };

runNodeLynxCli().catch((error: unknown) => {
  fs.writeSync(
    process.stderr.fd,
    `${error instanceof Error ? error.message : String(error)}\n`
  );
  process.exit(1);
});
