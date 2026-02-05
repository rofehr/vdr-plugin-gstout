/*
 * gstsetup.c: Setup menu for GStreamer output
 */

#include "gstsetup.h"
#include "gstout.h"

// --- cGstoutSetupPage ------------------------------------------------------

cGstoutSetupPage::cGstoutSetupPage(void)
{
  // Copy current config values
  useHardwareDecoding = GstoutConfig.useHardwareDecoding;
  deinterlace = GstoutConfig.deinterlace;
  audioBufferSize = GstoutConfig.audioBufferSize;
  videoBufferSize = GstoutConfig.videoBufferSize;
  
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
    if (strcmp(audioSinkNames[i], GstoutConfig.audioSink) == 0) {
      audioSinkIndex = i;
      break;
    }
  }
  
  videoSinkIndex = 0;
  for (int i = 0; videoSinkNames[i]; i++) {
    if (strcmp(videoSinkNames[i], GstoutConfig.videoSink) == 0) {
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
  Add(new cMenuEditBoolItem(tr("Hardware Decoding"), &useHardwareDecoding));
  Add(new cMenuEditBoolItem(tr("Deinterlace"), &deinterlace));
  Add(new cMenuEditIntItem(tr("Audio Buffer (KB)"), &audioBufferSize, 50, 1000));
  Add(new cMenuEditIntItem(tr("Video Buffer (KB)"), &videoBufferSize, 100, 2000));
  
  SetCurrent(Get(current));
  Display();
}

void cGstoutSetupPage::Store(void)
{
  // Copy selected sink names to config
  strncpy(GstoutConfig.audioSink, audioSinkNames[audioSinkIndex], sizeof(GstoutConfig.audioSink) - 1);
  GstoutConfig.audioSink[sizeof(GstoutConfig.audioSink) - 1] = '\0';
  
  strncpy(GstoutConfig.videoSink, videoSinkNames[videoSinkIndex], sizeof(GstoutConfig.videoSink) - 1);
  GstoutConfig.videoSink[sizeof(GstoutConfig.videoSink) - 1] = '\0';
  
  GstoutConfig.useHardwareDecoding = useHardwareDecoding;
  GstoutConfig.deinterlace = deinterlace;
  GstoutConfig.audioBufferSize = audioBufferSize;
  GstoutConfig.videoBufferSize = videoBufferSize;
  
  SetupStore("UseHardwareDecoding", GstoutConfig.useHardwareDecoding);
  SetupStore("Deinterlace", GstoutConfig.deinterlace);
  SetupStore("AudioBufferSize", GstoutConfig.audioBufferSize);
  SetupStore("VideoBufferSize", GstoutConfig.videoBufferSize);
  SetupStore("AudioSink", GstoutConfig.audioSink);
  SetupStore("VideoSink", GstoutConfig.videoSink);
}
