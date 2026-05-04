#pragma once
typedef void (*snd_filler_t)(float *, unsigned);

static void snd_init(snd_filler_t fn);
static void snd_deinit();

#ifdef __APPLE__
#include <AudioToolbox/AudioComponent.h>
#include <AudioToolbox/AudioOutputUnit.h>
#include <AudioToolbox/AudioUnitProperties.h>


static AudioComponentInstance snd_tone_unit;

static OSStatus render(
    void * ref, AudioUnitRenderActionFlags * /*flags*/,
    const AudioTimeStamp * /*timestamp*/,
    UInt32 /*bus_number*/, UInt32 number_frames,
    AudioBufferList *data) {
  float * f = (float *)data->mBuffers[0].mData;
  ((snd_filler_t) ref)(f, number_frames);
  return noErr;
}

static void snd_init(snd_filler_t fn) {
  AudioComponentDescription acd = {0};
  acd.componentType         = kAudioUnitType_Output;
  acd.componentManufacturer = kAudioUnitManufacturer_Apple;

#if TARGET_OS_OSX
  acd.componentSubType = kAudioUnitSubType_DefaultOutput;
#else
  acd.componentSubType = kAudioUnitSubType_RemoteIO;
#endif

  AudioComponent ac = AudioComponentFindNext(nullptr, &acd);
  if (ac == nullptr) return;

  if (AudioComponentInstanceNew(ac, &snd_tone_unit) != noErr) return;

  AURenderCallbackStruct rcs;
  rcs.inputProc       = render;
  rcs.inputProcRefCon = (void *)fn;
  if (noErr != AudioUnitSetProperty(snd_tone_unit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &rcs, sizeof(rcs))) return;

  AudioStreamBasicDescription sbd;
  sbd.mSampleRate       = 44100;
  sbd.mFormatID         = kAudioFormatLinearPCM;
  sbd.mFormatFlags      =
    (unsigned)kAudioFormatFlagsNativeFloatPacked |
    kAudioFormatFlagIsNonInterleaved;
  sbd.mBytesPerPacket   = sizeof(float);
  sbd.mFramesPerPacket  = 1;
  sbd.mBytesPerFrame    = sizeof(float) / 1;
  sbd.mChannelsPerFrame = 1;
  sbd.mBitsPerChannel   = sizeof(float) * 8;
  if (noErr != AudioUnitSetProperty(snd_tone_unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &sbd, sizeof(sbd))) return;

  if (noErr != AudioUnitInitialize(snd_tone_unit)) return;

  AudioOutputUnitStart(snd_tone_unit);
}
static void snd_deinit() {
  if (!snd_tone_unit) return;

  AudioOutputUnitStop(snd_tone_unit);
  AudioUnitUninitialize(snd_tone_unit);
  AudioComponentInstanceDispose(snd_tone_unit);
}
#elif _WIN32
#endif
