const {
  HeadlessOpenCardManager,
  LynxEnv,
  WindowedOpenCardManager,
} = require('../dist/index.js');
const {
  runOpenCardManagerTests,
} = require('./helpers/open-card-cases.js');
const {
  getHelloWorldTemplateUrl,
} = require('./helpers/remote-templates.js');

async function main() {
  await runOpenCardManagerTests({
    HeadlessOpenCardManager,
    LynxEnv,
    WindowedOpenCardManager,
    fixtureUrl: getHelloWorldTemplateUrl(),
  });
}

main()
  .then(() => {
    console.log('open card manager tests finished successfully.');
    process.exitCode = 0;
  })
  .catch((error) => {
    console.error('open card manager tests failed:', error);
    process.exitCode = 1;
  });
