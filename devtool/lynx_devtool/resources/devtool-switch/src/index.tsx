import { root } from '@lynx-js/react';
import App from './App.jsx';

root.render(<App />);

if (import.meta.webpackHot) {
  import.meta.webpackHot.accept();
}
