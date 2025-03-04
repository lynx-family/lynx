import { defineConfig } from '@rsbuild/core';
import { pluginNodePolyfill } from '@rsbuild/plugin-node-polyfill';
import { pluginLess } from '@rsbuild/plugin-less';
import { pluginReact } from '@rsbuild/plugin-react';

export default defineConfig({
  source: {
    entry: {
      index: './src/app.tsx',
    },
  },
  output: {
    target: 'web',
    cssModules: {
      auto: /\.less$/i,
    },
    overrideBrowserslist: ['ios >= 10', 'android >= 5'],
    assetPrefix: './',
    minify: {
      jsOptions: {
        minimizerOptions: {
          mangle: {
            // When using the default mangle configuration, there is an error
            // in the iOS 15 environment: 'SyntaxError: Duplicate parameter...'
            keep_fnames: true,
          },
        },
      },
    },
  },
  server: {
    //@ts-expect-error
    port: process.env.PORT,
    base: '/',
  },
  plugins: [
    pluginNodePolyfill(),
    pluginReact(),
    pluginLess({
      lessLoaderOptions: {
        lessOptions: {
          javascriptEnabled: true,
        },
      },
    }),
  ],
  html: {
    title: 'Lynx LogBox',
    meta: {
      viewport: 'width=device-width,initial-scale=1.0,maximum-scale=1.0,user-scalable=no',
      'apple-mobile-web-app-capable': 'yes',
      'apple-mobile-web-app-status-bar-style': 'default',
      'screen-orientation': 'portrait',
      'format-detection': 'telephone=no',
      'x5-orientation': 'portrait',
    },
  },
});
