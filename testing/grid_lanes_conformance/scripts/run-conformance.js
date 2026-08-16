const fs = require('fs');
const path = require('path');

const { compareGeometry, formatDifference } = require('./geometry');
const { PACKAGE_ROOT, readFixtures } = require('./fixtures');
const { renderNativeFixture } = require('./native-runner');
const { ORACLES, renderOracleFixture } = require('./oracle-runner');

function parseSuite() {
  const index = process.argv.indexOf('--suite');
  if (index === -1) {
    return 'all';
  }
  const suite = process.argv[index + 1];
  if (!['all', 'calibration', 'grid-lanes'].includes(suite)) {
    throw new Error(`invalid suite: ${suite}`);
  }
  return suite;
}

function percent(passed, total) {
  return total === 0 ? 0 : Number(((passed / total) * 100).toFixed(2));
}

async function runFixture(fixture) {
  const native = await renderNativeFixture(fixture);
  const oracleResults = {};
  for (const oracle of ORACLES) {
    oracleResults[oracle] = await renderOracleFixture(oracle, fixture);
  }

  const oracleDifferences = compareGeometry(
    oracleResults.chromium.geometry,
    oracleResults.webkit.geometry
  );
  if (oracleDifferences.length > 0 && !fixture.oracleDisagreement) {
    throw new Error(
      `${fixture.name}: Chromium and WebKit disagree without an oracleDisagreement annotation\n` +
        oracleDifferences.slice(0, 20).map(formatDifference).join('\n')
    );
  }
  if (oracleDifferences.length === 0 && fixture.oracleDisagreement) {
    throw new Error(
      `${fixture.name}: stale oracleDisagreement annotation; browser oracles now agree`
    );
  }

  const comparisons = {};
  for (const oracle of ORACLES) {
    const differences = compareGeometry(
      oracleResults[oracle].geometry,
      native.geometry
    );
    comparisons[oracle] = {
      passed: differences.length === 0,
      differences,
    };
  }
  const passed = ORACLES.every((oracle) => comparisons[oracle].passed);
  return {
    name: fixture.name,
    suite: fixture.suite,
    expectation: fixture.expectation,
    passed,
    expectedOutcome: fixture.expectation === (passed ? 'pass' : 'fail'),
    viewport: fixture.viewport,
    oracleDisagreement: fixture.oracleDisagreement || null,
    nativeErrors: native.errors,
    oracles: Object.fromEntries(
      ORACLES.map((oracle) => [
        oracle,
        {
          browserVersion: oracleResults[oracle].browserVersion,
          supportsGridLanes: oracleResults[oracle].supportsGridLanes,
          passed: comparisons[oracle].passed,
          differences: comparisons[oracle].differences,
        },
      ])
    ),
  };
}

function buildSummary(results) {
  const suites = {};
  for (const suite of ['calibration', 'grid-lanes', 'all']) {
    const cases =
      suite === 'all'
        ? results
        : results.filter((result) => result.suite === suite);
    suites[suite] = {
      total: cases.length,
      passed: cases.filter((result) => result.passed).length,
      conformancePercent: percent(
        cases.filter((result) => result.passed).length,
        cases.length
      ),
      expectedOutcomes: cases.filter((result) => result.expectedOutcome).length,
    };
  }
  return suites;
}

function table(results, summary) {
  const lines = [
    '| Case | Suite | Chromium | WebKit | Expected |',
    '| --- | --- | ---: | ---: | ---: |',
  ];
  for (const result of results) {
    lines.push(
      `| ${result.name} | ${result.suite} | ${
        result.oracles.chromium.passed ? 'PASS' : 'FAIL'
      } | ${result.oracles.webkit.passed ? 'PASS' : 'FAIL'} | ${
        result.expectedOutcome ? 'YES' : 'NO'
      } |`
    );
  }
  lines.push('');
  lines.push(
    `Calibration: ${summary.calibration.passed}/${summary.calibration.total} ` +
      `(${summary.calibration.conformancePercent}%)`
  );
  lines.push(
    `Grid lanes: ${summary['grid-lanes'].passed}/${summary['grid-lanes'].total} ` +
      `(${summary['grid-lanes'].conformancePercent}%)`
  );
  lines.push(
    `Overall: ${summary.all.passed}/${summary.all.total} ` +
      `(${summary.all.conformancePercent}%)`
  );
  return lines.join('\n');
}

function printDifferences(results) {
  for (const result of results) {
    for (const oracle of ORACLES) {
      const differences = result.oracles[oracle].differences;
      if (differences.length === 0) {
        continue;
      }
      console.log(`\n${result.name} vs ${oracle}:`);
      for (const difference of differences.slice(0, 20)) {
        console.log(`  ${formatDifference(difference)}`);
      }
      if (differences.length > 20) {
        console.log(`  ... ${differences.length - 20} more differences`);
      }
    }
  }
}

async function main() {
  const suite = parseSuite();
  const fixtures = readFixtures(suite);
  if (fixtures.length === 0) {
    throw new Error(`no fixtures selected for ${suite}`);
  }

  const results = [];
  for (const fixture of fixtures) {
    process.stdout.write(`running ${fixture.name} ... `);
    const result = await runFixture(fixture);
    results.push(result);
    console.log(result.expectedOutcome ? 'expected' : 'UNEXPECTED');
    if (fixture.suite === 'calibration' && !result.passed) {
      throw new Error(
        `${fixture.name}: calibration failed; grid-lanes scoring was not run`
      );
    }
  }

  const summary = buildSummary(results);
  const report = {
    schemaVersion: 1,
    generatedAt: new Date().toISOString(),
    epsilon: 0.01,
    results,
    summary,
  };
  const outputDirectory = path.resolve(
    PACKAGE_ROOT,
    '..',
    '..',
    'out',
    'grid-lanes-conformance'
  );
  fs.mkdirSync(outputDirectory, { recursive: true });
  const outputPath = path.join(outputDirectory, 'results.json');
  fs.writeFileSync(outputPath, `${JSON.stringify(report, null, 2)}\n`);

  const summaryTable = table(results, summary);
  console.log(`\n${summaryTable}`);
  printDifferences(results);
  console.log(`\nJSON: ${outputPath}`);

  if (process.env.GITHUB_STEP_SUMMARY) {
    fs.appendFileSync(
      process.env.GITHUB_STEP_SUMMARY,
      `## Grid Lanes Conformance\n\n${summaryTable}\n`
    );
  }

  if (results.some((result) => !result.expectedOutcome)) {
    process.exitCode = 1;
  }
}

main().catch((error) => {
  console.error(error.stack || error.message || String(error));
  process.exit(1);
});
