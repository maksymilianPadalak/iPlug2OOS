#!/bin/bash

# makedist-web.sh builds a Web version of an iPlug2 project using emscripten
# it copies a template folder from the iPlug2 tree and does a find and replace on various JavaScript and HTML files
# arguments:
# 1st argument : either "on", "off" or "ws" - this specifies whether $EMRUN is used to launch a server and browser after compilation. "ws" builds the project in websocket mode, without the WAM stuff
# 2nd argument : site origin -
# 3rd argument : browser - either "chrome", "safari", "firefox" - if you want to launch a browser other than chrome, you must specify the correct origin for argument #2

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
IPLUG2_ROOT=../../iPlug2
PROJECT_ROOT=$SCRIPT_DIR/..
IPLUG2_ROOT=$SCRIPT_DIR/$IPLUG2_ROOT
FILE_PACKAGER=/opt/homebrew/Cellar/emscripten/4.0.15/libexec/tools/file_packager.py
EMRUN="python3 ${IPLUG2_ROOT}/Scripts/emrun/emrun.py"

PROJECT_NAME=EffectTemplate
BUILD_WEB_DIR=$PROJECT_ROOT/../../public/effect
BUILD_DSP=1
BUILD_EDITOR=1
WEBSOCKET_MODE=0
EMRUN_BROWSER=chrome
LAUNCH_EMRUN=1
EMRUN_SERVER=1
EMRUN_SERVER_PORT=8001
EMRUN_CONTAINER=0
SITE_ORIGIN="/effect/"

cd $PROJECT_ROOT

if [ "$1" = "ws" ]; then
  LAUNCH_EMRUN=0
  BUILD_DSP=0
  WEBSOCKET_MODE=1
elif [ "$1" = "off" ]; then
  LAUNCH_EMRUN=0
elif [ "$1" = "container" ]; then
  EMRUN_CONTAINER=1
fi

if [ "$#" -eq 2 ]; then
  SITE_ORIGIN=${2}
fi

if [ "$#" -eq 3 ]; then
  EMRUN_BROWSER=${3}
fi

# check to see if the build web folder has its own git repo
if [ -d $BUILD_WEB_DIR/.git ]
then
  # if so trash only the scripts
    if [ -d $BUILD_WEB_DIR/scripts ]; then
      if [ "$BUILD_DSP" -eq "1" ]; then
        rm $BUILD_WEB_DIR/scripts/*-wam.js
      fi

      # Skip WEB module cleanup - we're using WebView UI adapter instead
      # if [ "$BUILD_EDITOR" -eq "1" ]; then
      #   rm $BUILD_WEB_DIR/scripts/*-web.*
      # fi
    fi
else
  # otherwise trash the whole build-web folder
  if [ -d $BUILD_WEB_DIR ]; then
    rm -r $BUILD_WEB_DIR
  fi

  mkdir -p $BUILD_WEB_DIR
fi

mkdir -p $BUILD_WEB_DIR/scripts

echo BUNDLING RESOURCES -----------------------------

if [ -f $BUILD_WEB_DIR/imgs.js ]; then rm $BUILD_WEB_DIR/imgs.js; fi
if [ -f $BUILD_WEB_DIR/imgs@2x.js ]; then rm $BUILD_WEB_DIR/imgs@2x.js; fi
if [ -f $BUILD_WEB_DIR/svgs.js ]; then rm $BUILD_WEB_DIR/svgs.js; fi
if [ -f $BUILD_WEB_DIR/fonts.js ]; then rm $BUILD_WEB_DIR/fonts.js; fi

#package fonts
FOUND_FONTS=0
if ls ./resources/fonts/*.ttf 1> /dev/null 2>&1; then
  FOUND_FONTS=1
  python3 $FILE_PACKAGER fonts.data --preload ./resources/fonts/ --exclude *DS_Store --js-output=./fonts.js
fi

#package svgs
FOUND_SVGS=0
if ls ./resources/img/*.svg 1> /dev/null 2>&1; then
  FOUND_SVGS=1
  python3 $FILE_PACKAGER svgs.data --preload ./resources/img/ --exclude *.png --exclude *DS_Store --js-output=./svgs.js
fi

#package @1x pngs
FOUND_PNGS=0
if ls ./resources/img/*.png 1> /dev/null 2>&1; then
  FOUND_PNGS=1
  python3 $FILE_PACKAGER imgs.data --use-preload-plugins --preload ./resources/img/ --use-preload-cache --indexedDB-name="/$PROJECT_NAME_pkg" --exclude *DS_Store --exclude  *@2x.png --exclude  *.svg >> ./imgs.js
fi

# package @2x pngs into separate .data file
FOUND_2XPNGS=0
if ls ./resources/img/*@2x*.png 1> /dev/null 2>&1; then
  FOUND_2XPNGS=1
  mkdir $BUILD_WEB_DIR/2x/
  cp ./resources/img/*@2x* $BUILD_WEB_DIR/2x
  python3 $FILE_PACKAGER imgs@2x.data --use-preload-plugins --preload ./2x@/resources/img/ --use-preload-cache --indexedDB-name="/$PROJECT_NAME_pkg" --exclude *DS_Store >> ./imgs@2x.js
  rm -r $BUILD_WEB_DIR/2x
fi

if [ -f ./imgs.js ]; then mv ./imgs.js $BUILD_WEB_DIR/imgs.js; fi
if [ -f ./imgs@2x.js ]; then mv ./imgs@2x.js $BUILD_WEB_DIR/imgs@2x.js; fi
if [ -f ./svgs.js ]; then mv ./svgs.js $BUILD_WEB_DIR/svgs.js; fi
if [ -f ./fonts.js ]; then mv ./fonts.js $BUILD_WEB_DIR/fonts.js; fi

if [ -f ./imgs.data ]; then mv ./imgs.data $BUILD_WEB_DIR/imgs.data; fi
if [ -f ./imgs@2x.data ]; then mv ./imgs@2x.data $BUILD_WEB_DIR/imgs@2x.data; fi
if [ -f ./svgs.data ]; then mv ./svgs.data $BUILD_WEB_DIR/svgs.data; fi
if [ -f ./fonts.data ]; then mv ./fonts.data $BUILD_WEB_DIR/fonts.data; fi

if [ "$BUILD_DSP" -eq "1" ]; then
  echo MAKING  - WAM WASM MODULE -----------------------------
  cd $PROJECT_ROOT/projects
  emmake make --makefile $PROJECT_NAME-wam-processor.mk

  if [ $? -ne "0" ]; then
    echo IPlugWAM WASM compilation failed
    exit 1
  fi

  cd $BUILD_WEB_DIR/scripts

  # Don't prefix - let emscripten create Module directly
  # Just ensure AudioWorkletGlobalScope.WAM exists at the top
  echo "AudioWorkletGlobalScope.WAM = AudioWorkletGlobalScope.WAM || {}; AudioWorkletGlobalScope.WAM.$PROJECT_NAME = { ENVIRONMENT: 'WEB' };" > $PROJECT_NAME-wam.tmp.js;
  # Read the original file and replace 'var Module' with the AudioWorkletGlobalScope assignment
  sed "s/var Module=typeof Module!=\"undefined\"?Module:{}/var Module=typeof AudioWorkletGlobalScope.WAM.$PROJECT_NAME!==\"undefined\"?AudioWorkletGlobalScope.WAM.$PROJECT_NAME:{}/g" $PROJECT_NAME-wam.js >> $PROJECT_NAME-wam.tmp.js
  mv $PROJECT_NAME-wam.tmp.js $PROJECT_NAME-wam.js

  # copy in WAM SDK and AudioWorklet polyfill scripts
  cp $IPLUG2_ROOT/Dependencies/IPlug/WAM_SDK/wamsdk/*.js .
  cp $IPLUG2_ROOT/Dependencies/IPlug/WAM_AWP/*.js .

  # Assign WAMController to window for global access
  echo "" >> wam-controller.js
  echo "// Assign to window for global access" >> wam-controller.js
  echo "window.WAMController = WAMController;" >> wam-controller.js

  # copy in template scripts
  cp $IPLUG2_ROOT/IPlug/WEB/Template/scripts/IPlugWAM-awn.js $PROJECT_NAME-awn.js
  cp $IPLUG2_ROOT/IPlug/WEB/Template/scripts/IPlugWAM-awp.js $PROJECT_NAME-awp.js

  # copy WebView-to-WAM adapter script
  cp $IPLUG2_ROOT/IPlug/WEB/Template/scripts/webview-wam-adapter.js webview-wam-adapter.js

  # replace NAME_PLACEHOLDER in the template -awn.js and -awp.js scripts
  sed -i.bak s/NAME_PLACEHOLDER/$PROJECT_NAME/g $PROJECT_NAME-awn.js
  sed -i.bak s/NAME_PLACEHOLDER/$PROJECT_NAME/g $PROJECT_NAME-awp.js

  # replace ORIGIN_PLACEHOLDER in the template -awn.js script
  sed -i.bak s,ORIGIN_PLACEHOLDER,$SITE_ORIGIN,g $PROJECT_NAME-awn.js

  # Assign controller class to window for global access
  echo "" >> $PROJECT_NAME-awn.js
  echo "// Assign to window for global access" >> $PROJECT_NAME-awn.js
  echo "window.${PROJECT_NAME}Controller = ${PROJECT_NAME}Controller;" >> $PROJECT_NAME-awn.js

  rm *.bak
else
  echo "WAM not being built, BUILD_DSP = 0"
fi

cd $BUILD_WEB_DIR

# Build TypeScript FIRST before copying (if src directory exists)
PROJECT_RESOURCES_WEB_DIR=$PROJECT_ROOT/resources/web
if [ -d "$PROJECT_RESOURCES_WEB_DIR/src" ] && [ -f "$PROJECT_RESOURCES_WEB_DIR/package.json" ]; then
  echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
  echo "Building TypeScript and Web UI..."
  echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
  cd $PROJECT_RESOURCES_WEB_DIR

  # Install dependencies (show output for debugging)
  echo "📦 Installing npm dependencies..."
  if npm install --silent 2>&1 | grep -q "error\|Error\|ERROR"; then
    echo "⚠️  npm install had warnings/errors (check output above)"
  else
    echo "✅ Dependencies installed"
  fi

  # Clean old build artifacts to force fresh compilation
  echo "🧹 Cleaning old build artifacts..."
  rm -rf scripts/*.js scripts/*.js.map 2>/dev/null || true
  rm -f styles/style.processed.css 2>/dev/null || true
  echo "✅ Cleaned build artifacts"

  # Run build (this includes parameter generation)
  echo "🔨 Running build (includes parameter generation)..."
  echo ""
  BUILD_OUTPUT=$(npm run build 2>&1)
  BUILD_EXIT_CODE=$?

  # Always show the build output
  echo "$BUILD_OUTPUT"
  echo ""

  if [ $BUILD_EXIT_CODE -eq 0 ]; then
    echo "✅ Build completed successfully"
  else
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "❌ Build failed with exit code: $BUILD_EXIT_CODE"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

    # Extract and highlight errors
    echo ""
    echo "🔍 Error Summary:"
    echo "$BUILD_OUTPUT" | grep -i "error\|failed\|❌" | head -20 || echo "   (No specific error patterns found)"
    echo ""

    # Check if bundle was created despite errors
    if [ -f "$PROJECT_RESOURCES_WEB_DIR/scripts/index.bundle.js" ]; then
      echo "⚠️  Warning: Bundle file exists despite build failure"
      echo "   This may indicate a partial build. Proceeding with caution..."
    else
      echo "❌ Bundle file missing - build definitely failed"
      echo "   Expected: $PROJECT_RESOURCES_WEB_DIR/scripts/index.bundle.js"
    fi
    exit $BUILD_EXIT_CODE  # Propagate the error
  fi

  cd $BUILD_WEB_DIR
  echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
fi

# Copy compiled TypeScript bundle if it exists (from resources/web build)
if [ -f "$PROJECT_RESOURCES_WEB_DIR/scripts/index.bundle.js" ]; then
  cp $PROJECT_RESOURCES_WEB_DIR/scripts/index.bundle.js scripts/ 2>/dev/null || true
  BUNDLE_SIZE=$(ls -lh "$PROJECT_RESOURCES_WEB_DIR/scripts/index.bundle.js" | awk '{print $5}')
  echo "✅ React bundle copied to public/effect/scripts/ (size: $BUNDLE_SIZE)"
else
  echo "❌ ERROR: Bundle file not found at $PROJECT_RESOURCES_WEB_DIR/scripts/index.bundle.js"
  echo "   This usually means the TypeScript build failed."
  echo "   Check the build output above for errors."
fi

# copy in the template HTML - comment this out if you have customised the HTML
cp $IPLUG2_ROOT/IPlug/WEB/Template/index.html index.html
sed -i.bak s/NAME_PLACEHOLDER/$PROJECT_NAME/g index.html

# If we have a custom index.html in resources/web, use that instead
if [ -f "$PROJECT_RESOURCES_WEB_DIR/index.html" ]; then
  cp $PROJECT_RESOURCES_WEB_DIR/index.html index.html
fi

if [ $FOUND_FONTS -eq "0" ]; then sed -i.bak s/'<script async src="fonts.js"><\/script>'/'<!--<script async src="fonts.js"><\/script>-->'/g index.html; fi
if [ $FOUND_SVGS -eq "0" ]; then sed -i.bak s/'<script async src="svgs.js"><\/script>'/'<!--<script async src="svgs.js"><\/script>-->'/g index.html; fi
if [ $FOUND_PNGS -eq "0" ]; then sed -i.bak s/'<script async src="imgs.js"><\/script>'/'<!--<script async src="imgs.js"><\/script>-->'/g index.html; fi
if [ $FOUND_2XPNGS -eq "0" ]; then sed -i.bak s/'<script async src="imgs@2x.js"><\/script>'/'<!--<script async src="imgs@2x.js"><\/script>-->'/g index.html; fi
if [ $WEBSOCKET_MODE -eq "1" ]; then
  cp $IPLUG2_ROOT/Dependencies/IPlug/WAM_SDK/wamsdk/wam-controller.js scripts/wam-controller.js
  cp $IPLUG2_ROOT/IPlug/WEB/Template/scripts/websocket.js scripts/websocket.js
  sed -i.bak s/'<script src="scripts\/audioworklet.js"><\/script>'/'<!--<script src="scripts\/audioworklet.js"><\/script>-->'/g index.html;
  sed -i.bak s/'let WEBSOCKET_MODE=false;'/'let WEBSOCKET_MODE=true;'/g index.html;
else
  sed -i.bak s/'<script src="scripts\/websocket.js"><\/script>'/'<!--<script src="scripts\/websocket.js"><\/script>-->'/g index.html;

  # update the i/o details for the AudioWorkletNodeOptions parameter, based on config.h channel io str
  MAXNINPUTS=$(python3 $IPLUG2_ROOT/Scripts/parse_iostr.py "$PROJECT_ROOT" inputs)
  MAXNOUTPUTS=$(python3 $IPLUG2_ROOT/Scripts/parse_iostr.py "$PROJECT_ROOT" outputs)

  if [ $MAXNINPUTS -eq "0" ]; then
    MAXNINPUTS="";
    sed -i.bak '181,203d' index.html; # hack to remove GetUserMedia() from code, and allow WKWebKitView usage for instruments
  fi
  sed -i.bak s/"MAXNINPUTS_PLACEHOLDER"/"$MAXNINPUTS"/g index.html;
  sed -i.bak s/"MAXNOUTPUTS_PLACEHOLDER"/"$MAXNOUTPUTS"/g index.html;
fi

rm *.bak

# copy the style & WAM favicon
mkdir styles
cp $IPLUG2_ROOT/IPlug/WEB/Template/styles/style.css styles/style.css
cp $IPLUG2_ROOT/IPlug/WEB/Template/favicon.ico favicon.ico

# Also copy files to resources/web for plugin builds (AU/VST3)
PROJECT_RESOURCES_WEB_DIR=$PROJECT_ROOT/resources/web
mkdir -p $PROJECT_RESOURCES_WEB_DIR/scripts
mkdir -p $PROJECT_RESOURCES_WEB_DIR/styles

# TypeScript build already happened above, so just copy files
# Copy HTML, CSS, and adapter script
if [ -f "index.html" ]; then
  cp index.html $PROJECT_RESOURCES_WEB_DIR/
fi
# Copy processed CSS if it exists (from resources/web build)
if [ -f "$PROJECT_RESOURCES_WEB_DIR/styles/style.processed.css" ]; then
  cp $PROJECT_RESOURCES_WEB_DIR/styles/style.processed.css styles/style.css 2>/dev/null || true
  echo "✅ Processed CSS copied to styles/style.css"
elif [ -f "$PROJECT_RESOURCES_WEB_DIR/styles/style.css" ]; then
  cp $PROJECT_RESOURCES_WEB_DIR/styles/style.css styles/
fi
if [ -f "favicon.ico" ]; then
  cp favicon.ico $PROJECT_RESOURCES_WEB_DIR/
fi

# Copy scripts needed for WebView (including compiled TypeScript)
cp scripts/webview-wam-adapter.js $PROJECT_RESOURCES_WEB_DIR/scripts/ 2>/dev/null || true
# Copy compiled TypeScript bundle
if [ -f "$PROJECT_RESOURCES_WEB_DIR/scripts/index.bundle.js" ]; then
  cp $PROJECT_RESOURCES_WEB_DIR/scripts/index.bundle.js $PROJECT_RESOURCES_WEB_DIR/scripts/ 2>/dev/null || true
fi

echo "Files copied to $PROJECT_RESOURCES_WEB_DIR for plugin builds"
echo "Note: Add the 'web' folder to Xcode project Resources if not already added"

echo "Skipping WEB WASM module build - using WebView UI instead"
# echo MAKING  - WEB WASM MODULE -----------------------------
# cd $PROJECT_ROOT/projects
# emmake make --makefile $PROJECT_NAME-wam-controller.mk EXTRA_CFLAGS=-DWEBSOCKET_CLIENT=$WEBSOCKET_MODE
# if [ $? -ne "0" ]; then
#   echo IPlugWEB WASM compilation failed
#   exit 1
# fi

cd $BUILD_WEB_DIR

# print payload
echo payload:
find . -maxdepth 2 -mindepth 1 -name .git -type d \! -prune -o \! -name .DS_Store -type f -exec du -hs {} \;

# launch emrun
if [ "$LAUNCH_EMRUN" -eq "1" ]; then
  mkcert 127.0.0.1 localhost
  if [ "$EMRUN_CONTAINER" -eq "1" ]; then
    $EMRUN --no_browser --serve_after_close --serve_after_exit --port=$EMRUN_SERVER_PORT --hostname=0.0.0.0 .
  elif [ "$EMRUN_SERVER" -eq "0" ]; then
    $EMRUN --browser $EMRUN_BROWSER --no_server --port=$EMRUN_SERVER_PORT index.html
  else
    $EMRUN --browser $EMRUN_BROWSER --no_emrun_detect index.html
  fi
else
  echo "Not running emrun"
fi
