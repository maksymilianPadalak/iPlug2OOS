
#include <TargetConditionals.h>
#if TARGET_OS_IOS == 1
#import <UIKit/UIKit.h>
#else
#import <Cocoa/Cocoa.h>
#endif

#define IPLUG_AUVIEWCONTROLLER IPlugAUViewController_vPluginInstance
#define IPLUG_AUAUDIOUNIT IPlugAUAudioUnit_vPluginInstance
#import <PluginInstanceAU/IPlugAUViewController.h>
#import <PluginInstanceAU/IPlugAUAudioUnit.h>

//! Project version number for PluginInstanceAU.
FOUNDATION_EXPORT double PluginInstanceAUVersionNumber;

//! Project version string for PluginInstanceAU.
FOUNDATION_EXPORT const unsigned char PluginInstanceAUVersionString[];

@class IPlugAUViewController_vPluginInstance;
