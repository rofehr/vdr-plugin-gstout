/*
 * gstsetup.c: Setup menu for GStreamer output
 */

#include "gstsetup.h"
#include "gstout.h"

// --- cGstoutSetupPage ------------------------------------------------------

cGstoutSetupPage::cGstoutSetupPage(void)
{
  data = GstoutConfig;
  
  // Audio sink options
  audioSinkNames[0] = "autoaudiosink";
  audioSinkNames[1] = "alsasink";
  audioSinkNames[2] = "pulsesink";
  audioSinkNames[3] = "osssink";
  audioSinkNames[4] = "jackaudiosink";
  audioSinkNames[5] = NULL;
  
  // Video sink options
  videoSinkNames[0] = "autovideosink";
  videoSinkNames[1] = "xvimagesink";
  videoSinkNames[2] = "ximagesink";
  videoSinkNames[3] = "vaapisink";
  videoSinkNames[4] = "glimagesink";
  videoSinkNames[5] = "fbdevsink";
  videoSinkNames[6] = NULL;
  
  // Find current selections
  audioSinkIndex = 0;
  for (int i = 0; audioSinkNames[i]; i++) {
    if (strcmp(audioSinkNames[i], data.audioSink) == 0) {
      audioSinkIndex = i;
      break;
    }
  }
  
  videoSinkIndex = 0;
  for (int i = 0; videoSinkNames[i]; i++) {
    if (strcmp(videoSinkNames[i], data.videoSink) == 0) {
      videoSinkIndex = i;
      break;
    }
  }
  
  Setup();
}

void cGstoutSetupPage::Setup(void)
{
  int current = Current();
  Clear();
  
  Add(new cMenuEditStraItem(tr("Audio Sink"), &audioSinkIndex, 5, audioSinkNames));
  Add(new cMenuEditStraItem(tr("Video Sink"), &videoSinkIndex, 6, videoSinkNames));
  Add(new cMenuEditBoolItem(tr("Hardware Decoding"), &data.useHardwareDecoding));
  Add(new cMenuEditBoolItem(tr("Deinterlace"), &data.deinterlace));
  Add(new cMenuEditIntItem(tr("Audio Buffer (KB)"), &data.audioBufferSize, 50, 1000));
  Add(new cMenuEditIntItem(tr("Video Buffer (KB)"), &data.videoBufferSize, 100, 2000));
  
  SetCurrent(Get(current));
  Display();
}

void cGstoutSetupPage::Store(void)
{
  // Copy selected sink names
  strncpy(data.audioSink, audioSinkNames[audioSinkIndex], sizeof(data.audioSink) - 1);
  data.audioSink[sizeof(data.audioSink) - 1] = '\0';
  
  strncpy(data.videoSink, videoSinkNames[videoSinkIndex], sizeof(data.videoSink) - 1);
  data.videoSink[sizeof(data.videoSink) - 1] = '\0';
  
  SetupStore("AudioDevice", GstoutConfig.audioDevice = data.audioDevice);
  SetupStore("VideoDevice", GstoutConfig.videoDevice = data.videoDevice);
  SetupStore("UseHardwareDecoding", GstoutConfig.useHardwareDecoding = data.useHardwareDecoding);
  SetupStore("Deinterlace", GstoutConfig.deinterlace = data.deinterlace);
  SetupStore("AudioBufferSize", GstoutConfig.audioBufferSize = data.audioBufferSize);
  SetupStore("VideoBufferSize", GstoutConfig.videoBufferSize = data.videoBufferSize);
  SetupStore("AudioSink", GstoutConfig.audioSink);
  SetupStore("VideoSink", GstoutConfig.videoSink);
}
