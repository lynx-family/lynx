// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { appTasks, OhosPluginId, Target } from '@ohos/hvigor-ohos-plugin';
import { getNode, HvigorNode, HvigorTask, hvigor } from '@ohos/hvigor';
import { exec, execSync } from 'child_process';

function gnPlugin(): HvigorPlugin {
  return {
    pluginId: 'gnPlugin',
    apply(node: HvigorNode) {
      const entryTasks = hvigor.getCommandEntryTask() as string[];
      if (entryTasks.length === 1 && entryTasks[0] === 'clean') {
        console.log('clean');
        return;
      }

      const appContext = hvigor
        .getRootNode()
        .getContext(OhosPluginId.OHOS_APP_PLUGIN) as OhosAppContext;

      hvigor.nodesEvaluated(() => {
        node.subNodes((node: HvigorNode) => {
          if (node.getNodeName() == 'lynx_explorer') {
            node.registerTask({
              name: 'install',
              run() {
                console.log('---------------install start---------------');

                console.time('install');
                execSync(
                  'pushd ../../platform/harmony && ohpm install && popd',
                  { stdio: 'inherit' }
                );
                console.timeEnd('install');
              },
              postDependencies: ['init'],
            });
            node.registerTask({
              name: 'gn',
              run() {
                console.log('---------------gn start---------------');

                console.time('gn');
                const skipGn = hvigor.getParameter().getExtParam('skipGn');
                const mode = appContext.getBuildMode();

                if (skipGn) {
                  console.log('---skip-gn---');
                } else {
                  console.log('---gn-build---');
                  execSync(
                    'source ../../tools/envsetup.sh' +
                      `&& python3 ./script/build.py ${
                        mode === 'skipBundle'
                          ? ''
                          : '--build_lynx_core --build_bundle'
                      } ${mode === 'release' ? '' : '--debug'} --dev --verbose`,
                    { stdio: 'inherit' }
                  );
                }
                console.timeEnd('gn');
              },
              postDependencies: ['assembleHap'],
            });
          }
        });
      });
    },
  };
}

export default {
  system: appTasks,
  plugins: [gnPlugin()],
};
