#ifndef SND_H
#define SND_H

typedef void (*snd_filler_t)(float *, unsigned);

void snd_init(snd_filler_t fn);
void snd_deinit();

#ifdef SND_IMPLEMENTATION
#ifdef __ANDROID__
#elif __APPLE__
#include <AudioToolbox/AudioComponent.h>
#include <AudioToolbox/AudioOutputUnit.h>
#include <AudioToolbox/AudioUnitProperties.h>

static AudioComponentInstance snd_tone_unit;

static OSStatus render(
    void * ref, AudioUnitRenderActionFlags * flags,
    const AudioTimeStamp * timestamp,
    UInt32 bus_number, UInt32 number_frames,
    AudioBufferList *data) {
  float * f = (float *)data->mBuffers[0].mData;
  ((snd_filler_t) ref)(f, number_frames);
  return noErr;
}

void snd_init(snd_filler_t fn) {
  AudioComponentDescription acd = {0};
  acd.componentType         = kAudioUnitType_Output;
  acd.componentManufacturer = kAudioUnitManufacturer_Apple;

#if TARGET_OS_OSX
  acd.componentSubType = kAudioUnitSubType_DefaultOutput;
#else
  acd.componentSubType = kAudioUnitSubType_RemoteIO;
#endif

  AudioComponent ac = AudioComponentFindNext(NULL, &acd);
  if (!ac) return;

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
void snd_deinit() {
  if (!snd_tone_unit) return;

  AudioOutputUnitStop(snd_tone_unit);
  AudioUnitUninitialize(snd_tone_unit);
  AudioComponentInstanceDispose(snd_tone_unit);
}
#elif _WIN32
#include <xaudio2.h>

static float snd_buffer[44100];

static snd_filler_t snd_fn; 

static IXAudio2               * snd_xa2;
static IXAudio2MasteringVoice * snd_main_voice;
static IXAudio2SourceVoice    * snd_src_voice;

#define SND_COM(obj, method, ...) (obj)->lpVtbl->method(obj, __VA_ARGS__)
#define SND_CHK(obj, method, ...) if (FAILED(SND_COM(obj, method, __VA_ARGS__))) return

static void snd_voice_processing_pass_start(IXAudio2VoiceCallback * self, UINT32 size) {
  if (size > sizeof(snd_buffer)) size = sizeof(snd_buffer);

  snd_fn(snd_buffer, size / sizeof(float));

  XAUDIO2_BUFFER buf = {
    .AudioBytes = size,
    .pAudioData = (BYTE *)snd_buffer,
  };
  SND_COM(snd_src_voice, SubmitSourceBuffer, &buf, NULL);
}

static void snd_dummy_0(IXAudio2VoiceCallback * self, void * ctx) {}
static void snd_dummy_1(IXAudio2VoiceCallback * self) {}
static void snd_dummy_2(IXAudio2VoiceCallback * self, void * ctx, HRESULT err) {}
static IXAudio2VoiceCallbackVtbl snd_vtbl = {
  .OnBufferEnd                = &snd_dummy_0,
  .OnBufferStart              = &snd_dummy_0,
  .OnLoopEnd                  = &snd_dummy_0,
  .OnStreamEnd                = &snd_dummy_1,
  .OnVoiceError               = &snd_dummy_2,
  .OnVoiceProcessingPassEnd   = &snd_dummy_1,
  .OnVoiceProcessingPassStart = &snd_voice_processing_pass_start,
};
static IXAudio2VoiceCallback snd_callback = {
  .lpVtbl = &snd_vtbl,
};

void snd_init(snd_filler_t fn) {
  HRESULT h;
  if (FAILED(h = CoInitializeEx(NULL, COINIT_MULTITHREADED))) return;
  if (FAILED(h = XAudio2Create(&snd_xa2, 0, XAUDIO2_DEFAULT_PROCESSOR))) return;
  SND_CHK(snd_xa2, CreateMasteringVoice, &snd_main_voice, 1, 44100, 0, NULL, NULL, 0);

  snd_fn = fn;

  WAVEFORMATEX wfx = {
    .wFormatTag      = WAVE_FORMAT_IEEE_FLOAT,
    .nChannels       = 1,
    .nSamplesPerSec  = 44100,
    .nAvgBytesPerSec = 44100 * sizeof(float),
    .nBlockAlign     = 4, // channels * bitsPerSample / 8
    .wBitsPerSample  = 32,
  };
  SND_CHK(snd_xa2, CreateSourceVoice, &snd_src_voice, &wfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &snd_callback, NULL, NULL);

  SND_CHK(snd_src_voice, Start, 0, 0);
}

void snd_deinit() {
  if (snd_src_voice ) SND_COM(snd_src_voice,  DestroyVoice);
  if (snd_main_voice) SND_COM(snd_main_voice, DestroyVoice);
  if (snd_xa2       ) SND_COM(snd_xa2,        Release     );
  CoUninitialize();
}

#endif
#endif
#endif
