/*
 * gstout.c: VDR plugin for GStreamer-based audio/video output
 *
 * See the README file for copyright information and how to reach the author.
 */

#include "gstout.h"
#include "gstsetup.h"
#include <vdr/plugin.h>
#include <getopt.h>

// Global configuration
cGstoutConfig GstoutConfig;

// --- cGstoutConfig ---------------------------------------------------------

cGstoutConfig::cGstoutConfig(void)
{
  audioDevice = 0;
  videoDevice = 0;
  useHardwareDecoding = true;
  deinterlace = true;
  audioBufferSize = 200;
  videoBufferSize = 200;
  strcpy(audioSink, "autoaudiosink");
  strcpy(videoSink, "autovideosink");
  osdBlending = true;
}

// --- cPluginGstout ---------------------------------------------------------

cPluginGstout::cPluginGstout(void)
{
  // Initialize any member variables here.
  output = NULL;
  osdProvider = NULL;
}

cPluginGstout::~cPluginGstout()
{
  // Clean up after yourself!
  delete output;
  delete osdProvider;
}

const char *cPluginGstout::CommandLineHelp(void)
{
  return "  -a SINK,  --audio=SINK   GStreamer audio sink (default: autoaudiosink)\n"
         "  -v SINK,  --video=SINK   GStreamer video sink (default: autovideosink)\n"
         "  -d,       --hwdec        Enable hardware decoding (default: yes)\n"
         "  -D,       --no-hwdec     Disable hardware decoding\n";
}

bool cPluginGstout::ProcessArgs(int argc, char *argv[])
{
  static struct option long_options[] = {
    { "audio",      required_argument, NULL, 'a' },
    { "video",      required_argument, NULL, 'v' },
    { "hwdec",      no_argument,       NULL, 'd' },
    { "no-hwdec",   no_argument,       NULL, 'D' },
    { NULL, 0, NULL, 0 }
  };

  int c;
  while ((c = getopt_long(argc, argv, "a:v:dD", long_options, NULL)) != -1) {
    switch (c) {
      case 'a':
        strncpy(GstoutConfig.audioSink, optarg, sizeof(GstoutConfig.audioSink) - 1);
        GstoutConfig.audioSink[sizeof(GstoutConfig.audioSink) - 1] = '\0';
        break;
      case 'v':
        strncpy(GstoutConfig.videoSink, optarg, sizeof(GstoutConfig.videoSink) - 1);
        GstoutConfig.videoSink[sizeof(GstoutConfig.videoSink) - 1] = '\0';
        break;
      case 'd':
        GstoutConfig.useHardwareDecoding = true;
        break;
      case 'D':
        GstoutConfig.useHardwareDecoding = false;
        break;
      default:
        return false;
    }
  }
  return true;
}

bool cPluginGstout::Initialize(void)
{
  // Initialize GStreamer output
  output = new cGstOutput();
  
  if (!output->Initialize()) {
    esyslog("gstout: Failed to initialize GStreamer output");
    delete output;
    output = NULL;
    return false;
  }
  
  // Create and register OSD provider
  osdProvider = new cGstOsdProvider();
  //cOsdProvider::SetOsdProvider(osdProvider);
  isyslog("gstout: OSD provider registered");
  
  // Link OSD provider to video output
  if (output)
    output->SetOsdProvider(osdProvider);
  
  return true;
}

bool cPluginGstout::Start(void)
{
  // Start any background activities the plugin needs
  if (output)
    output->Start();
  
  return true;
}

void cPluginGstout::Stop(void)
{
  // Stop plugin activity
  if (output)
    output->Stop();
}

void cPluginGstout::Housekeeping(void)
{
  // Perform any cleanup or regular tasks
}

void cPluginGstout::MainThreadHook(void)
{
  // Gets called once per second
  if (output)
    output->MainThreadHook();
}

cString cPluginGstout::Active(void)
{
  // Return message if plugin is currently active
  return NULL;
}

time_t cPluginGstout::WakeupTime(void)
{
  // Return custom wakeup time
  return 0;
}

cOsdObject *cPluginGstout::MainMenuAction(void)
{
  // Return a menu object when selected from main menu
  return NULL;
}

cMenuSetupPage *cPluginGstout::SetupMenu(void)
{
  // Return a setup menu
  return new cGstoutSetupPage();
}

bool cPluginGstout::SetupParse(const char *Name, const char *Value)
{
  // Parse setup parameters
  if      (!strcasecmp(Name, "AudioDevice"))        GstoutConfig.audioDevice = atoi(Value);
  else if (!strcasecmp(Name, "VideoDevice"))        GstoutConfig.videoDevice = atoi(Value);
  else if (!strcasecmp(Name, "UseHardwareDecoding")) GstoutConfig.useHardwareDecoding = atoi(Value);
  else if (!strcasecmp(Name, "Deinterlace"))        GstoutConfig.deinterlace = atoi(Value);
  else if (!strcasecmp(Name, "AudioBufferSize"))    GstoutConfig.audioBufferSize = atoi(Value);
  else if (!strcasecmp(Name, "VideoBufferSize"))    GstoutConfig.videoBufferSize = atoi(Value);
  else if (!strcasecmp(Name, "AudioSink"))          strn0cpy(GstoutConfig.audioSink, Value, sizeof(GstoutConfig.audioSink));
  else if (!strcasecmp(Name, "VideoSink"))          strn0cpy(GstoutConfig.videoSink, Value, sizeof(GstoutConfig.videoSink));
  else if (!strcasecmp(Name, "OsdBlending"))        GstoutConfig.osdBlending = atoi(Value);
  else
    return false;
  
  return true;
}

bool cPluginGstout::Service(const char *Id, void *Data)
{
  // Handle custom service requests from other plugins
  return false;
}

const char **cPluginGstout::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands
  static const char *HelpPages[] = {
    "STAT\n"
    "    Print GStreamer pipeline statistics.",
    "RSET\n"
    "    Reset GStreamer pipeline.",
    NULL
  };
  return HelpPages;
}

cString cPluginGstout::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  // Process SVDRP commands
  if (strcasecmp(Command, "STAT") == 0) {
    if (output)
      return output->GetStatistics();
    else
      return "GStreamer output not initialized";
  }
  else if (strcasecmp(Command, "RSET") == 0) {
    if (output) {
      output->Reset();
      return "GStreamer pipeline reset";
    }
    else
      return "GStreamer output not initialized";
  }
  
  return NULL;
}

VDRPLUGINCREATOR(cPluginGstout); // Don't touch this!
