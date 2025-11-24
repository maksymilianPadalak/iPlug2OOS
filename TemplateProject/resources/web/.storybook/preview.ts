import type { Preview } from '@storybook/react-vite';
import '../styles/style.css';

const preview: Preview = {
  parameters: {
    controls: {
      matchers: {
        color: /(background|color)$/i,
        date: /Date$/i,
      },
    },
    backgrounds: {
      default: 'plugin',
      values: [
        { name: 'plugin', value: '#0a0a0a' },
        { name: 'dark', value: '#1c1917' },
        { name: 'light', value: '#fafaf9' },
      ],
    },
    layout: 'centered',
  },
};

export default preview;
