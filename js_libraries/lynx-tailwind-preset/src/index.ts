import type { Config } from 'tailwindcss';

interface ComponentConfig {
  base: string;
  props: Record<string, any>;
}

interface PluginAPI {
  addComponents: (components: Record<string, Record<string, any>>) => void;
  addUtilities: (utilities: Record<string, Record<string, any>>) => void;
  theme: (path: string) => any;
}

const preset: Config = {
  content: [],
  theme: {
    extend: {
      // Add custom utilities for safe areas
      utilities: {
        '.safe-top': {
          paddingTop: 'env(safe-area-inset-top)',
        },
        '.safe-bottom': {
          paddingBottom: 'env(safe-area-inset-bottom)',
        },
        '.safe-left': {
          paddingLeft: 'env(safe-area-inset-left)',
        },
        '.safe-right': {
          paddingRight: 'env(safe-area-inset-right)',
        },
        '.touch-feedback': {
          transition: 'background-color 0.2s',
        },
        '.optimize-gradient': {
          willChange: 'background-image',
        },
      },
    },
  },
  plugins: [
    // Plugin to handle component mapping
    function({ addComponents }: PluginAPI) {
      const components = {
        'button': {
          base: 'view',
          props: {
            role: 'button',
            className: 'touch-feedback',
          },
        },
        'link': {
          base: 'view',
          props: {
            role: 'link',
            className: 'touch-feedback',
          },
        },
        'a': {
          base: 'view',
          props: {
            role: 'link',
            className: 'touch-feedback',
          },
        },
        'nav': {
          base: 'view',
          props: {
            role: 'navigation',
          },
        },
        'header': {
          base: 'view',
          props: {
            role: 'banner',
          },
        },
        'footer': {
          base: 'view',
          props: {
            role: 'contentinfo',
          },
        },
        'main': {
          base: 'view',
          props: {
            role: 'main',
          },
        },
        'article': {
          base: 'view',
          props: {
            role: 'article',
          },
        },
        'section': {
          base: 'view',
          props: {
            role: 'region',
          },
        },
        'aside': {
          base: 'view',
          props: {
            role: 'complementary',
          },
        },
      } as Record<string, ComponentConfig>;

      Object.entries(components).forEach(([name, config]) => {
        addComponents({
          [`.${name}`]: {
            ...config.props,
          },
        });
      });
    },
    // Plugin to handle scroll behavior
    function({ addUtilities }: PluginAPI) {
      const scroll = {
        'overflow-y-auto': {
          'overflow-y': 'auto',
          'shows-vertical-scroll-indicator': true,
        },
        'overflow-x-auto': {
          'overflow-x': 'auto',
          'shows-horizontal-scroll-indicator': true,
        },
      } as Record<string, Record<string, any>>;

      addUtilities(scroll);
    },
    // Plugin to handle interactive states
    function({ addUtilities }: PluginAPI) {
      const states = {
        'hover': {
          'active:bg-opacity-80': {
            'background-color-opacity': 0.8,
          },
        },
        'active': {
          'active:bg-opacity-90': {
            'background-color-opacity': 0.9,
          },
        },
        'focus': {
          'focus:ring-2': {
            'ring-width': 2,
          },
          'focus:ring-offset-2': {
            'ring-offset-width': 2,
          },
        },
      } as Record<string, Record<string, Record<string, any>>>;

      Object.entries(states).forEach(([_, utilities]) => {
        addUtilities(utilities);
      });
    },
  ],
};

export default preset; 