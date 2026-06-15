const path = require('path');

const {
  runDebugRouterProtocolSmoke,
} = require('./helpers/debug-router.js');
const {
  getHelloWorldTemplateUrl,
} = require('./helpers/remote-templates.js');

const PACKAGE_ROOT = path.resolve(__dirname, '..');
const CLI_PATH = path.join(PACKAGE_ROOT, 'dist', 'cli.js');

runDebugRouterProtocolSmoke({
  packageRoot: PACKAGE_ROOT,
  cliPath: CLI_PATH,
  templateUrl: getHelloWorldTemplateUrl(),
})
  .then(() => {
    console.log('node-lynx DebugRouter protocol smoke finished successfully.');
    process.exitCode = 0;
  })
  .catch((error) => {
    console.error(error instanceof Error ? error.stack || error.message : String(error));
    process.exit(1);
  });
