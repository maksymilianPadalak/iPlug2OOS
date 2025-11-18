# Complete Plugin Build Flow Explained

## Overview
This document explains the complete flow from writing TypeScript code to running the plugin in a DAW.

---

## 📁 SOURCE CODE STRUCTURE

### What Gets Committed to Git:
```
resources/web/
├── src/                          # TypeScript source files (✅ in git)
│   ├── config/
│   │   └── constants.ts         # Parameter enums, message types
│   ├── types/
│   │   ├── iplug.d.ts           # iPlug2 communication types
│   │   └── wam.d.ts             # WAM controller types
│   ├── utils/
│   │   ├── environment.ts       # WebView vs WAM detection
│   │   └── parameter.ts         # Parameter conversion utilities
│   ├── communication/
│   │   ├── iplug-bridge.ts      # UI → Processor messages
│   │   └── callbacks.ts         # Processor → UI callbacks
│   ├── audio/
│   │   ├── wam-controller.ts    # WAM initialization
│   │   └── midi.ts              # MIDI keyboard handling
│   ├── ui/
│   │   ├── parameters.ts        # Parameter UI management
│   │   ├── meters.ts            # Audio meter display
│   │   └── lfo-visualizer.ts    # LFO waveform canvas
│   └── index.ts                 # Main entry point
├── index.html                   # HTML template (✅ in git)
├── package.json                # Build config (✅ in git)
├── tsconfig.json               # TypeScript config (✅ in git)
└── .gitignore                  # Excludes build artifacts (✅ in git)
```

### What's NOT in Git (Build Artifacts):
```
resources/web/
└── scripts/
    ├── **/*.js                 # Compiled JS files (❌ excluded)
    ├── **/*.js.map             # Source maps (❌ excluded)
    ├── index.bundle.js          # Bundled output (❌ excluded)
    └── node_modules/           # Dependencies (❌ excluded)
```

---

## 🔨 BUILD PROCESS

### Step 1: TypeScript Compilation (`npm run build`)

When you run `npm run build` in `resources/web/`, it executes:

```bash
tsc && esbuild scripts/index.js --bundle --format=iife --global-name=TemplateProjectUI --outfile=scripts/index.bundle.js
```

**What happens:**

1. **`tsc`** (TypeScript Compiler):
   - Reads `tsconfig.json`
   - Compiles all `.ts` files from `src/` → `scripts/`
   - Outputs:
     - `scripts/index.js` (with ES6 imports)
     - `scripts/audio/midi.js`
     - `scripts/communication/callbacks.js`
     - etc. (one JS file per TS file)
   - Generates `.js.map` files for debugging

2. **`esbuild`** (Bundler):
   - Takes `scripts/index.js` (entry point)
   - Bundles all modules into a single file
   - Converts ES6 modules to IIFE format (works in browsers)
   - Outputs: `scripts/index.bundle.js` (single file, ~25KB)

**Why bundle?**
- Browsers don't support ES6 modules directly (without type="module")
- Single file is faster to load
- Works in WebView (AU/VST3) without module loader

---

### Step 2: Web Build Script (`makedist-web.sh`)

When you run `./scripts/makedist-web.sh`, it:

#### Phase 1: Build Web Distribution (`public/plugin/`)
```bash
# 1. Create build directory
BUILD_WEB_DIR = ../../public/plugin

# 2. Copy WAM files (AudioWorklet processors)
cp scripts/TemplateProject-awn.js
cp scripts/TemplateProject-awp.js
cp scripts/wam-controller.js
cp scripts/audioworklet.js

# 3. Copy TypeScript bundle (if exists)
cp resources/web/scripts/index.bundle.js → scripts/

# 4. Copy HTML (your custom index.html)
cp resources/web/index.html → index.html

# 5. Copy CSS, fonts, images
cp styles/style.css
cp fonts.js, etc.
```

**Result:** `public/plugin/` contains everything needed for web testing.

#### Phase 2: Build Plugin Resources (`resources/web/`)
```bash
# 1. Build TypeScript FIRST
cd resources/web
npm install --silent
npm run build  # ← Compiles TypeScript + bundles

# 2. Copy files to plugin resources
cp index.html → resources/web/
cp styles/style.css → resources/web/styles/
cp scripts/index.bundle.js → resources/web/scripts/
cp scripts/webview-wam-adapter.js → resources/web/scripts/
```

**Result:** `resources/web/` contains files that get bundled into the AU plugin.

---

### Step 3: Plugin Build (`makedist-mac.sh`)

When building the AU plugin:

1. **Xcode Build:**
   - Compiles C++ code (`TemplateProject.cpp`)
   - Links frameworks (WebKit, Accelerate)
   - Copies `resources/web/` → `TemplateProject.component/Contents/Resources/web/`

2. **Plugin Bundle Structure:**
```
TemplateProject.component/
├── Contents/
│   ├── MacOS/
│   │   └── TemplateProject      # Compiled C++ binary
│   └── Resources/
│       └── web/                 # ← Your UI files here!
│           ├── index.html
│           ├── scripts/
│           │   ├── index.bundle.js
│           │   └── webview-wam-adapter.js
│           └── styles/
│               └── style.css
```

---

## 🚀 RUNTIME EXECUTION

### When Plugin Opens in DAW:

#### 1. Plugin Initialization (C++)
```cpp
// TemplateProject.cpp line 26-29
mEditorInitFunc = [&]() {
  LoadIndexHtml(__FILE__, GetBundleID());  // ← Loads web/index.html
  EnableScroll(false);
};
```

**What `LoadIndexHtml()` does:**
- Finds `web/index.html` in plugin bundle
- Creates a WebView (WKWebView on macOS)
- Loads the HTML file
- Injects `IPlugSendMsg()` function into JavaScript context

#### 2. HTML Loading

Browser loads `index.html`:
```html
<head>
  <!-- 1. AudioWorklet polyfill -->
  <script src="scripts/audioworklet.js"></script>
  
  <!-- 2. WebView-to-WAM adapter -->
  <script src="scripts/webview-wam-adapter.js"></script>
  
  <!-- 3. Your TypeScript bundle -->
  <script src="scripts/index.bundle.js"></script>
</head>
```

#### 3. JavaScript Execution

`index.bundle.js` executes:

1. **Environment Detection** (`environment.ts`):
   ```typescript
   detectEnvironment() {
     // Checks if IPlugSendMsg exists (WebView mode)
     if (window.IPlugSendMsg) return 'webview';
     // Otherwise WAM mode (browser)
     return 'wam';
   }
   ```

2. **Setup Callbacks** (`callbacks.ts`):
   ```typescript
   window.SPVFD = (paramIdx, value) => {
     // Updates UI when processor changes parameter
     updateParameterFromProcessor(paramIdx, value);
   };
   ```

3. **Initialize UI** (`index.ts`):
   ```typescript
   initKeyboard();          // Creates piano keyboard
   initLFOWaveform();       // Sets up canvas
   setupKeyboardHandlers(); // QWERTY key mapping
   ```

#### 4. User Interaction

When user moves a slider:
```typescript
// HTML: <input oninput="updateParam(2, this.value)">
window.updateParam(2, 0.5) {
  sendParameterValue(EParams.kParamAttack, 0.5);
}

sendParameterValue() {
  window.IPlugSendMsg({
    msg: "SPVFUI",
    paramIdx: 2,
    value: 0.5
  });
}
```

**`IPlugSendMsg()` is injected by WebView:**
- In WebView mode: Calls native C++ code via `webkit.messageHandlers`
- In WAM mode: Routes through `webview-wam-adapter.js` → WAM controller

#### 5. Processor Updates UI

When C++ processor changes a parameter:
```cpp
// TemplateProject.cpp
SendParameterValueFromDelegate(kParamAttack, 0.5);
```

This calls JavaScript:
```javascript
window.SPVFD(2, 0.5);  // ← Defined in callbacks.ts
```

UI updates automatically!

---

## 🔄 TWO MODES OF OPERATION

### Mode 1: WebView (AU/VST3 Plugin)
- **Location:** Runs inside DAW
- **WebView:** WKWebView (macOS) or equivalent
- **Communication:** Direct native bridge (`IPlugSendMsg`)
- **Audio:** C++ processor handles audio
- **UI:** Only HTML/CSS/JS (no AudioWorklet needed)

### Mode 2: WAM (Web Browser)
- **Location:** Runs in browser
- **Environment:** Standard web page
- **Communication:** WebView adapter bridges to WAM
- **Audio:** AudioWorklet processor handles audio
- **UI:** Same HTML/CSS/JS (with "Start web audio!" button)

**Key Difference:** Same UI code, different audio backend!

---

## 📊 COMPLETE FLOW DIAGRAM

```
┌─────────────────────────────────────────────────────────────┐
│                    YOU WRITE CODE                            │
│  Edit TypeScript files in resources/web/src/                 │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│              BUILD TYPESCRIPT (npm run build)               │
│  1. tsc compiles: src/**/*.ts → scripts/**/*.js             │
│  2. esbuild bundles: scripts/index.js → index.bundle.js     │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│         BUILD WEB DISTRIBUTION (makedist-web.sh)            │
│  1. Copy bundle to public/plugin/scripts/                    │
│  2. Copy HTML, CSS, scripts to public/plugin/               │
│  3. Copy to resources/web/ for plugin build                 │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│           BUILD PLUGIN (makedist-mac.sh)                    │
│  1. Xcode compiles C++ code                                 │
│  2. Copies resources/web/ → Plugin.component/Resources/web/ │
│  3. Links WebKit framework                                  │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│              DAW LOADS PLUGIN                               │
│  1. LoadIndexHtml() finds web/index.html in bundle           │
│  2. Creates WebView, loads HTML                              │
│  3. Injects IPlugSendMsg() function                         │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│              BROWSER/WEBVIEW LOADS HTML                      │
│  1. Loads scripts/audioworklet.js                           │
│  2. Loads scripts/webview-wam-adapter.js                     │
│  3. Loads scripts/index.bundle.js (your code)               │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│              JAVASCRIPT EXECUTES                            │
│  1. detectEnvironment() → 'webview'                         │
│  2. setupCallbacks() → Registers SPVFD, SCVFD, etc.        │
│  3. initKeyboard() → Creates piano keys                     │
│  4. Ready for user interaction!                             │
└─────────────────────────────────────────────────────────────┘
```

---

## 🎯 KEY POINTS

1. **Source code is TypeScript** - Clean, type-safe, modular
2. **Build artifacts are generated** - Not committed to git
3. **Single UI codebase** - Works in both WebView and browser
4. **Bidirectional communication** - UI ↔ Processor via callbacks
5. **Automatic builds** - Scripts handle compilation and bundling

---

## 🔧 MAINTENANCE

### To add a new parameter:
1. Add enum to `src/config/constants.ts` (EParams)
2. Add UI element to `index.html`
3. Add handler in `src/ui/parameters.ts`
4. Build automatically includes it!

### To modify UI:
1. Edit TypeScript files in `src/`
2. Run `npm run build` (or let build script do it)
3. Test in browser or rebuild plugin

### To debug:
- **WebView mode:** Use `SetEnableDevTools(true)` in C++ code
- **Browser mode:** Use browser DevTools
- **TypeScript:** Source maps are generated (`.js.map` files)

---

This architecture gives you:
- ✅ Clean, maintainable TypeScript code
- ✅ Automatic builds
- ✅ Single codebase for both plugin and web
- ✅ Type safety throughout
- ✅ No magic numbers or hardcoded values

