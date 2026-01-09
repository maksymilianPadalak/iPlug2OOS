#define PLUG_NAME "GoldenEffect"
#define PLUG_MFR "BMI"
#define PLUG_VERSION_HEX 0x00010000
#define PLUG_VERSION_STR "1.0.0"
#define PLUG_UNIQUE_ID 'GEfx'
#define PLUG_MFR_ID 'Acme'
#define PLUG_URL_STR "https://iplug2.github.io"
#define PLUG_EMAIL_STR "spam@me.com"
#define PLUG_COPYRIGHT_STR "Copyright 2025 Acme Inc"
#define PLUG_CLASS_NAME PluginInstance

#define BUNDLE_NAME "GoldenEffect"
#define BUNDLE_MFR "BMI"
#define BUNDLE_DOMAIN "com"

#define SHARED_RESOURCES_SUBPATH "GoldenEffect"

#define PLUG_CHANNEL_IO "1-1 2-2"

#define PLUG_LATENCY 0
#define PLUG_TYPE 0
#define PLUG_DOES_MIDI_IN 0
#define PLUG_DOES_MIDI_OUT 0
#define PLUG_DOES_MPE 0
#define PLUG_DOES_STATE_CHUNKS 0
#define PLUG_HAS_UI 1
#define PLUG_WIDTH 950
#define PLUG_HEIGHT 525
#define PLUG_FPS 60
#define PLUG_SHARED_RESOURCES 0
#define PLUG_HOST_RESIZE 1
#define PLUG_MIN_WIDTH 256
#define PLUG_MIN_HEIGHT 256
#define PLUG_MAX_WIDTH 8192
#define PLUG_MAX_HEIGHT 8192

// AUV2_* defines MUST be unique per plugin to avoid Objective-C class name collisions
// when multiple plugins are loaded simultaneously in a host (e.g. Logic Pro)
#define AUV2_ENTRY GoldenEffect_Entry
#define AUV2_ENTRY_STR "GoldenEffect_Entry"
#define AUV2_FACTORY GoldenEffect_Factory
#define AUV2_VIEW_CLASS GoldenEffect_View
#define AUV2_VIEW_CLASS_STR "GoldenEffect_View"

#define AAX_TYPE_IDS 'GEf1'
#define AAX_TYPE_IDS_AUDIOSUITE 'GEfA'
#define AAX_PLUG_MFR_STR "Acme"
#define AAX_PLUG_NAME_STR "GoldenEffect\nIPEF"
#define AAX_PLUG_CATEGORY_STR "Effect"
#define AAX_DOES_AUDIOSUITE 1

#define VST3_SUBCATEGORY "Fx"

#define CLAP_MANUAL_URL "https://iplug2.github.io/manuals/example_manual.pdf"
#define CLAP_SUPPORT_URL "https://github.com/iPlug2/iPlug2/wiki"
#define CLAP_DESCRIPTION "Reverb and Delay effect using Q DSP library"
#define CLAP_FEATURES "audio-effect", "reverb", "delay"

#define APP_NUM_CHANNELS 2
#define APP_N_VECTOR_WAIT 0
#define APP_MULT 1
#define APP_COPY_AUV3 0
#define APP_SIGNAL_VECTOR_SIZE 64

#define ROBOTO_FN "Roboto-Regular.ttf"
