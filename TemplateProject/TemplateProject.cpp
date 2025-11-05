#include "TemplateProject.h"
#include "IPlug_include_in_plug_src.h"
#include "LFO.h"

TemplateProject::TemplateProject(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kParamGain)->InitDouble("Gain", 100., 0., 100.0, 0.01, "%");
  GetParam(kParamNoteGlideTime)->InitMilliseconds("Note Glide Time", 0., 0.0, 30.);
  GetParam(kParamAttack)->InitDouble("Attack", 10., 1., 1000., 0.1, "ms", IParam::kFlagsNone, "ADSR", IParam::ShapePowCurve(3.));
  GetParam(kParamDecay)->InitDouble("Decay", 10., 1., 1000., 0.1, "ms", IParam::kFlagsNone, "ADSR", IParam::ShapePowCurve(3.));
  GetParam(kParamSustain)->InitDouble("Sustain", 50., 0., 100., 1, "%", IParam::kFlagsNone, "ADSR");
  GetParam(kParamRelease)->InitDouble("Release", 10., 2., 1000., 0.1, "ms", IParam::kFlagsNone, "ADSR");
  GetParam(kParamLFOShape)->InitEnum("LFO Shape", LFO<>::kTriangle, {LFO_SHAPE_VALIST});
  GetParam(kParamLFORateHz)->InitFrequency("LFO Rate", 1., 0.01, 40.);
  GetParam(kParamLFORateTempo)->InitEnum("LFO Rate", LFO<>::k1, {LFO_TEMPODIV_VALIST});
  GetParam(kParamLFORateMode)->InitBool("LFO Sync", true);
  GetParam(kParamLFODepth)->InitPercentage("LFO Depth");
    
  // Oscillator parameters
  GetParam(kParamOsc1Mix)->InitPercentage("Osc1 Mix", 100.);
  GetParam(kParamOsc2Mix)->InitPercentage("Osc2 Mix", 0.);
  GetParam(kParamOsc3Mix)->InitPercentage("Osc3 Mix", 0.);
  GetParam(kParamOsc1Detune)->InitDouble("Osc1 Detune", 0., -50., 50., 0.1, "cents");
  GetParam(kParamOsc2Detune)->InitDouble("Osc2 Detune", 0., -50., 50., 0.1, "cents");
  GetParam(kParamOsc3Detune)->InitDouble("Osc3 Detune", 0., -50., 50., 0.1, "cents");
  GetParam(kParamOsc1Octave)->InitInt("Osc1 Octave", 0, -2, 2, "");
  GetParam(kParamOsc2Octave)->InitInt("Osc2 Octave", 0, -2, 2, "");
  GetParam(kParamOsc3Octave)->InitInt("Osc3 Octave", 0, -2, 2, "");
  GetParam(kParamOsc1Wave)->InitEnum("Osc1 Wave", 0, {"Sine", "Saw", "Square", "Triangle"});
  GetParam(kParamOsc2Wave)->InitEnum("Osc2 Wave", 0, {"Sine", "Saw", "Square", "Triangle"});
  GetParam(kParamOsc3Wave)->InitEnum("Osc3 Wave", 0, {"Sine", "Saw", "Square", "Triangle"});
  
  // Filter parameters
  GetParam(kParamFilterCutoff)->InitFrequency("Filter Cutoff", 1000., 20., 20000.);
  GetParam(kParamFilterResonance)->InitDouble("Filter Resonance", 1., 0.1, 10., 0.1, "");
  GetParam(kParamFilterEnvAmount)->InitDouble("Filter Env Amount", 0., -5000., 5000., 1., "Hz");
  GetParam(kParamFilterKeytrack)->InitDouble("Filter Keytrack", 0., 0., 100., 0.1, "Hz/key");
  GetParam(kParamFilterAttack)->InitDouble("Filter Attack", 10., 1., 1000., 0.1, "ms", IParam::kFlagsNone, "Filter", IParam::ShapePowCurve(3.));
  GetParam(kParamFilterDecay)->InitDouble("Filter Decay", 10., 1., 1000., 0.1, "ms", IParam::kFlagsNone, "Filter", IParam::ShapePowCurve(3.));
  GetParam(kParamFilterSustain)->InitDouble("Filter Sustain", 50., 0., 100., 1, "%", IParam::kFlagsNone, "Filter");
  GetParam(kParamFilterRelease)->InitDouble("Filter Release", 10., 2., 1000., 0.1, "ms", IParam::kFlagsNone, "Filter");
  
  // LFO2 parameters
  GetParam(kParamLFO2Shape)->InitEnum("LFO2 Shape", LFO<>::kTriangle, {LFO_SHAPE_VALIST});
  GetParam(kParamLFO2RateHz)->InitFrequency("LFO2 Rate", 1., 0.01, 40.);
  GetParam(kParamLFO2RateTempo)->InitEnum("LFO2 Rate", LFO<>::k1, {LFO_TEMPODIV_VALIST});
  GetParam(kParamLFO2RateMode)->InitBool("LFO2 Sync", true);
  GetParam(kParamLFO2Depth)->InitPercentage("LFO2 Depth");
  
  // Delay parameters
  GetParam(kParamDelayTime)->InitMilliseconds("Delay Time", 250., 1., 2000.);
  GetParam(kParamDelayFeedback)->InitPercentage("Delay Feedback", 0.);
  GetParam(kParamDelayDry)->InitPercentage("Delay Dry", 100.);
  GetParam(kParamDelayWet)->InitPercentage("Delay Wet", 0.);
  
  // Sync parameters
  GetParam(kParamOscSync)->InitBool("Osc Sync", false);
  GetParam(kParamOscSyncRatio)->InitDouble("Sync Ratio", 1., 0.125, 8., 0.125, "");
  
  // Reverb parameters
  GetParam(kParamReverbRoomSize)->InitDouble("Reverb Room Size", 0.5, 0.3, 0.99, 0.01, "");
  GetParam(kParamReverbDamp)->InitPercentage("Reverb Damp", 50.);
  GetParam(kParamReverbWidth)->InitDouble("Reverb Width", 0.5, 0., 1., 0.01, "");
  GetParam(kParamReverbDry)->InitPercentage("Reverb Dry", 100.);
  GetParam(kParamReverbWet)->InitPercentage("Reverb Wet", 0.);
    
#if IPLUG_EDITOR
#if defined(WEBVIEW_EDITOR_DELEGATE)
  // WebView editor delegate setup (for AU/VST3)
  SetCustomUrlScheme("iplug2");
  SetEnableDevTools(true);
  
  mEditorInitFunc = [&]() {
    LoadIndexHtml(__FILE__, GetBundleID());
    EnableScroll(false);
  };
#else
  // IGraphics editor delegate setup (for NanoVG-based UI)
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, 1.0f);
  };

  mLayoutFunc = [&](IGraphics* pGraphics) {
    // Berlin Brutalist UI Style - Black background, white text, zero rounded corners, bold borders
    const IColor BG_BLACK = IColor(255, 10, 10, 10);
    const IColor WHITE = IColor(255, 255, 255, 255);
    const IColor MUTED = IColor(153, 255, 255, 255);

    // Brutalist style: no shadows, no roundness, bold borders
    const IVStyle BRUTALIST_STYLE = DEFAULT_STYLE
      .WithColor(kBG, BG_BLACK)
      .WithColor(kFG, WHITE)
      .WithColor(kPR, IColor(255, 40, 40, 40))
      .WithColor(kFR, WHITE)
      .WithColor(kHL, IColor(255, 200, 200, 200))
      .WithColor(kSH, BG_BLACK)
      .WithColor(kX1, IColor(102, 26, 26, 26))
      .WithColor(kX2, WHITE)
      .WithRoundness(0.f)
      .WithFrameThickness(2.f)
      .WithDrawShadows(false)
      .WithDrawFrame(true)
      .WithLabelText(IText(12.f, WHITE))
      .WithValueText(IText(10.f, MUTED));

    pGraphics->AttachPanelBackground(BG_BLACK);
    pGraphics->EnableMouseOver(true);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);

#ifdef OS_WEB
    pGraphics->AttachPopupMenuControl();
#endif

    const IRECT b = pGraphics->GetBounds();
    const IRECT container = b.GetPadded(-30.f);

    // Title
    const IRECT titleArea = container.GetFromTop(60.f);
    pGraphics->AttachControl(new ITextControl(titleArea, "TEMPLATE SYNTH",
      IText(48.f, WHITE)));

    // Main content area
    const IRECT workArea = container.GetReducedFromTop(80.f);

    // Bottom: Keyboard + Wheels
    const IRECT keyboardArea = workArea.GetFromBottom(120.f);
    const IRECT wheelsBounds = keyboardArea.GetFromLeft(80.f).GetPadded(-5.f);
    const IRECT keyboardBounds = keyboardArea.GetReducedFromLeft(85.f);

    pGraphics->AttachControl(new IWheelControl(wheelsBounds.FracRectVertical(0.5f, true), IMidiMsg::EControlChangeMsg::kModWheel));
    pGraphics->AttachControl(new IWheelControl(wheelsBounds.FracRectVertical(0.5f, false)), kCtrlTagBender);
    pGraphics->AttachControl(new IVKeyboardControl(keyboardBounds, 36, 84), kCtrlTagKeyboard);

    // Content area (above keyboard)
    const IRECT contentArea = workArea.GetReducedFromBottom(130.f);

    // Layout: 3 sections (ADSR | Main | LFO)
    // Split into 3 equal columns
    const float colW = contentArea.W() / 3.0f;
    const IRECT adsrSection = contentArea.GetFromLeft(colW).GetPadded(-10.f);
    const IRECT mainSection = contentArea.GetReducedFromLeft(colW).GetFromLeft(colW).GetPadded(-10.f);
    const IRECT lfoSection = contentArea.GetFromRight(colW).GetPadded(-10.f);

    // ADSR SECTION
    const IRECT adsrTitle = adsrSection.GetFromTop(30.f);
    pGraphics->AttachControl(new ITextControl(adsrTitle, "ADSR",
      IText(32.f, WHITE)));

    const IRECT adsrControls = adsrSection.GetReducedFromTop(40.f);
    const float sliderW = adsrControls.W() / 4.0f;
    const float sliderGap = 5.f;

    pGraphics->AttachControl(new IVSliderControl(
      IRECT(adsrControls.L, adsrControls.T, adsrControls.L + sliderW - sliderGap, adsrControls.B),
      kParamAttack, "A", BRUTALIST_STYLE.WithWidgetFrac(0.8f), false, EDirection::Vertical));

    pGraphics->AttachControl(new IVSliderControl(
      IRECT(adsrControls.L + sliderW, adsrControls.T, adsrControls.L + sliderW * 2 - sliderGap, adsrControls.B),
      kParamDecay, "D", BRUTALIST_STYLE.WithWidgetFrac(0.8f), false, EDirection::Vertical));

    pGraphics->AttachControl(new IVSliderControl(
      IRECT(adsrControls.L + sliderW * 2, adsrControls.T, adsrControls.L + sliderW * 3 - sliderGap, adsrControls.B),
      kParamSustain, "S", BRUTALIST_STYLE.WithWidgetFrac(0.8f), false, EDirection::Vertical));

    pGraphics->AttachControl(new IVSliderControl(
      IRECT(adsrControls.L + sliderW * 3, adsrControls.T, adsrControls.R, adsrControls.B),
      kParamRelease, "R", BRUTALIST_STYLE.WithWidgetFrac(0.8f), false, EDirection::Vertical));

    // MAIN CONTROLS SECTION
    const IRECT mainTitle = mainSection.GetFromTop(30.f);
    pGraphics->AttachControl(new ITextControl(mainTitle, "MAIN",
      IText(32.f, WHITE)));

    const IRECT mainControls = mainSection.GetReducedFromTop(40.f);
    const float knobH = mainControls.H() * 0.48f;
    const IRECT gainKnobRect = mainControls.GetFromTop(knobH);
    const IRECT glideKnobRect = mainControls.GetFromBottom(knobH);

    pGraphics->AttachControl(new IVKnobControl(gainKnobRect, kParamGain, "GAIN",
      BRUTALIST_STYLE.WithWidgetFrac(0.7f)));
    pGraphics->AttachControl(new IVKnobControl(glideKnobRect, kParamNoteGlideTime, "GLIDE",
      BRUTALIST_STYLE.WithWidgetFrac(0.7f)));

    // LFO SECTION
    const IRECT lfoTitle = lfoSection.GetFromTop(30.f);
    pGraphics->AttachControl(new ITextControl(lfoTitle, "LFO",
      IText(32.f, WHITE)));

    const IRECT lfoControls = lfoSection.GetReducedFromTop(40.f);

    // Simple 2x2 grid layout for LFO knobs
    const float knobRowH = lfoControls.H() * 0.5f;
    const float knobColW = lfoControls.W() * 0.5f;

    // Row 1: Shape and Depth
    const IRECT lfoShapeRect = IRECT(lfoControls.L, lfoControls.T,
                                      lfoControls.L + knobColW, lfoControls.T + knobRowH).GetPadded(-8.f);
    const IRECT lfoDepthRect = IRECT(lfoControls.L + knobColW, lfoControls.T,
                                      lfoControls.R, lfoControls.T + knobRowH).GetPadded(-8.f);

    pGraphics->AttachControl(new IVKnobControl(lfoShapeRect, kParamLFOShape, "SHAPE",
      BRUTALIST_STYLE.WithWidgetFrac(0.6f)))->DisablePrompt(false);
    pGraphics->AttachControl(new IVKnobControl(lfoDepthRect, kParamLFODepth, "DEPTH",
      BRUTALIST_STYLE.WithWidgetFrac(0.6f)));

    // Row 2: Rate and Visualizer
    const IRECT lfoRateRect = IRECT(lfoControls.L, lfoControls.T + knobRowH,
                                     lfoControls.L + knobColW, lfoControls.B).GetPadded(-8.f);
    const IRECT lfoVisRect = IRECT(lfoControls.L + knobColW, lfoControls.T + knobRowH,
                                    lfoControls.R, lfoControls.B).GetPadded(-8.f);

    // Rate knob (Hz or Tempo based on sync) - both occupy same space
    pGraphics->AttachControl(new IVKnobControl(lfoRateRect, kParamLFORateHz, "RATE HZ",
      BRUTALIST_STYLE.WithWidgetFrac(0.6f)), kParamLFORateHz)->Hide(true);
    pGraphics->AttachControl(new IVKnobControl(lfoRateRect, kParamLFORateTempo, "RATE",
      BRUTALIST_STYLE.WithWidgetFrac(0.6f)), kParamLFORateTempo)->DisablePrompt(false);

    // Sync toggle (small button at top of rate knob area)
    const IRECT lfoSyncRect = IRECT(lfoControls.L + 5, lfoControls.T + knobRowH + 5,
                                     lfoControls.L + knobColW - 5, lfoControls.T + knobRowH + 22);
    pGraphics->AttachControl(new IVSlideSwitchControl(lfoSyncRect, kParamLFORateMode, "SYNC",
      BRUTALIST_STYLE.WithShowValue(false).WithWidgetFrac(0.4f).WithLabelText(IText(9.f, WHITE)), false));

    // Visualizer - LFO waveform display (white line on black background)
    pGraphics->AttachControl(new IVDisplayControl(lfoVisRect, "", BRUTALIST_STYLE.WithColor(kFG, WHITE).WithColor(kBG, BG_BLACK).WithColor(kX1, WHITE), EDirection::Horizontal, 0.f, 1.f, 0.5f, 512, 3.f), kCtrlTagLFOVis);

    // QWERTY keyboard handler for playing notes
    pGraphics->SetQwertyMidiKeyHandlerFunc([pGraphics](const IMidiMsg& msg) {
      pGraphics->GetControlWithTag(kCtrlTagKeyboard)->As<IVKeyboardControl>()->SetNoteFromMidi(msg.NoteNumber(), msg.StatusMsg() == IMidiMsg::kNoteOn);
    });
  };
#endif
#endif
}

#if IPLUG_DSP
void TemplateProject::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  mDSP.ProcessBlock(nullptr, outputs, 2, nFrames, mTimeInfo.mPPQPos, mTimeInfo.mTransportIsRunning);
  mLFOVisSender.PushData({kCtrlTagLFOVis, {float(mDSP.mLFO.GetLastOutput())}});
  mMeterSender.ProcessBlock(outputs, nFrames, kCtrlTagMeter);
}

void TemplateProject::OnIdle()
{
  mLFOVisSender.TransmitData(*this);
  mMeterSender.TransmitData(*this);
}

void TemplateProject::OnReset()
{
  mDSP.Reset(GetSampleRate(), GetBlockSize());
  mMeterSender.Reset(GetSampleRate());
}

void TemplateProject::ProcessMidiMsg(const IMidiMsg& msg)
{
  TRACE;
  
  int status = msg.StatusMsg();
  
  switch (status)
  {
    case IMidiMsg::kNoteOn:
    case IMidiMsg::kNoteOff:
    case IMidiMsg::kPolyAftertouch:
    case IMidiMsg::kControlChange:
    case IMidiMsg::kProgramChange:
    case IMidiMsg::kChannelAftertouch:
    case IMidiMsg::kPitchWheel:
    {
      goto handle;
    }
    default:
      return;
  }
  
handle:
  mDSP.ProcessMidiMsg(msg);
  SendMidiMsg(msg);
}

void TemplateProject::OnParamChange(int paramIdx)
{
  mDSP.SetParam(paramIdx, GetParam(paramIdx)->Value());
}

void TemplateProject::OnParamChangeUI(int paramIdx, EParamSource source)
{
  #if IPLUG_EDITOR
  #ifndef NO_IGRAPHICS
  if (auto pGraphics = GetUI())
  {
    if (paramIdx == kParamLFORateMode)
    {
      const auto sync = GetParam(kParamLFORateMode)->Bool();
      pGraphics->HideControl(kParamLFORateHz, sync);
      pGraphics->HideControl(kParamLFORateTempo, !sync);
    }
  }
  #endif
  #endif
}

bool TemplateProject::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
#if IPLUG_EDITOR
#ifndef NO_IGRAPHICS
  if(ctrlTag == kCtrlTagBender && msgTag == IWheelControl::kMessageTagSetPitchBendRange)
  {
    const int bendRange = *static_cast<const int*>(pData);
    mDSP.mSynth.SetPitchBendRange(bendRange);
  }
#endif
#endif

  return false;
}
#endif
