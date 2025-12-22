const sharedPreset = require('../../../shared-web-ui/tailwind.preset.js');

/** @type {import('tailwindcss').Config} */
module.exports = {
  presets: [sharedPreset],
  content: [
    "./src/**/*.{js,jsx,ts,tsx}",
    "./scripts/**/*.{js,jsx}",
    "./index.html",
    "../../../shared-web-ui/src/**/*.{js,jsx,ts,tsx}",
    "../../../shared-web-ui/styles/**/*.css",
  ],
  safelist: [
    'trigger-indicator',
    'triggered',
  ],
  theme: {
    extend: {
      fontFamily: {
        'brutal': ['ui-monospace', 'SFMono-Regular', 'Menlo', 'Monaco', 'Consolas', 'monospace'],
        'sans': ['"Josefin Sans"', '-apple-system', 'BlinkMacSystemFont', 'system-ui', 'sans-serif'],
      },
      borderWidth: {
        'brutal': '4px',
      },
      colors: {
        'brutal-black': '#000000',
        'brutal-white': '#FFFFFF',
        'brutal-yellow': '#FFFF00',
      },
    },
  },
  plugins: [],
}



