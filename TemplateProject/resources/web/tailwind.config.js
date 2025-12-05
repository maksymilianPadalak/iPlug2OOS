/** @type {import('tailwindcss').Config} */
module.exports = {
  content: [
    "./src/**/*.{js,jsx,ts,tsx}",
    "./scripts/**/*.{js,jsx}",
    "./index.html",
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



