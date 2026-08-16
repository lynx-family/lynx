const fs = require('fs');
const path = require('path');

const PACKAGE_ROOT = path.resolve(__dirname, '..');
const FIXTURES_ROOT = path.join(PACKAGE_ROOT, 'fixtures');

function fixtureDirectories() {
  return fs
    .readdirSync(FIXTURES_ROOT, { withFileTypes: true })
    .filter((entry) => entry.isDirectory())
    .map((entry) => path.join(FIXTURES_ROOT, entry.name))
    .sort();
}

function readFixtures(suite = 'all') {
  const fixtures = fixtureDirectories().map((directory) => {
    const metadataPath = path.join(directory, 'fixture.json');
    const metadata = JSON.parse(fs.readFileSync(metadataPath, 'utf8'));
    for (const field of ['name', 'suite', 'expectation', 'viewport']) {
      if (metadata[field] === undefined) {
        throw new Error(`${metadataPath} is missing ${field}`);
      }
    }
    if (!['calibration', 'grid-lanes'].includes(metadata.suite)) {
      throw new Error(`${metadataPath} has invalid suite ${metadata.suite}`);
    }
    if (!['pass', 'fail'].includes(metadata.expectation)) {
      throw new Error(
        `${metadataPath} has invalid expectation ${metadata.expectation}`
      );
    }
    return {
      ...metadata,
      directory,
      bundlePath: path.join(directory, 'dist', 'main.lynx.bundle'),
      oraclePath: path.join(directory, 'oracle.html'),
    };
  });
  fixtures.sort((left, right) => {
    const suiteOrder = { calibration: 0, 'grid-lanes': 1 };
    return (
      suiteOrder[left.suite] - suiteOrder[right.suite] ||
      left.name.localeCompare(right.name)
    );
  });
  return suite === 'all'
    ? fixtures
    : fixtures.filter((fixture) => fixture.suite === suite);
}

module.exports = {
  FIXTURES_ROOT,
  PACKAGE_ROOT,
  readFixtures,
};
