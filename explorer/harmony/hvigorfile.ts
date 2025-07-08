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
          if (node.getNodeName() == 'entry') {
            node.registerTask({
              name: 'gn',
              run() {
                console.log('---------------gn start---------------');

                console.time('gn');
                const skipGn = hvigor.getParameter().getExtParam('skipGn');
                console.log('skipGn:', skipGn);
                if (skipGn) {
                  console.log('---skip-gn---');
                } else {
                  console.log('---gn-build---');
                  execSync(
                    'source ../../../tools/envsetup.sh --target harmony ' +
                      `&& python3 ./script/build.py ${
                        appContext.getBuildMode() === 'debug'
                          ? '--is_debug'
                          : ''
                      } --verbose`,
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
