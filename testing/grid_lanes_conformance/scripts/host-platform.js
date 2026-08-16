const fs = require('fs');
const { spawnSync } = require('child_process');

function aptHasPackage(packageName) {
  const result = spawnSync('apt-cache', ['show', packageName], {
    stdio: 'ignore',
  });
  return result.status === 0;
}

function configurePlaywrightHostPlatform(environment = process.env) {
  if (
    process.platform !== 'linux' ||
    environment.PLAYWRIGHT_HOST_PLATFORM_OVERRIDE
  ) {
    return environment;
  }
  const osRelease = fs.existsSync('/etc/os-release')
    ? fs.readFileSync('/etc/os-release', 'utf8')
    : '';
  const debianVersion = fs.existsSync('/etc/debian_version')
    ? fs.readFileSync('/etc/debian_version', 'utf8').trim()
    : '';
  const isDebian12Compatible =
    debianVersion.startsWith('12') ||
    (aptHasPackage('libicu72') && !aptHasPackage('libicu74'));
  if (!/^ID=debian$/m.test(osRelease) && isDebian12Compatible) {
    environment.PLAYWRIGHT_HOST_PLATFORM_OVERRIDE =
      process.arch === 'arm64' ? 'debian12-arm64' : 'debian12-x64';
  }
  return environment;
}

module.exports = { configurePlaywrightHostPlatform };
