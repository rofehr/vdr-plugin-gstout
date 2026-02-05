/*
 * gstout.h: VDR plugin for GStreamer-based audio/video output
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef __GSTOUT_H
#define __GSTOUT_H

#include <vdr/plugin.h>
#include <vdr/player.h>
#include <vdr/thread.h>
#include "gstoutput.h"

static const char *VERSION        = "0.1.0";
static const char *DESCRIPTION    = "GStreamer-based Audio/Video Output";

// Plugin configuration
class cGstoutConfig {
public:
  int audioDevice;
  int videoDevice;
  bool useHardwareDecoding;
  bool deinterlace;
  int audioBufferSize;
  int videoBufferSize;
  char audioSink[256];
  char videoSink[256];
  
  cGstoutConfig(void);
};

extern cGstoutConfig GstoutConfig;

class cPluginGstout : public cPlugin {
private:
  cGstOutput *output;
  
public:
  cPluginGstout(void);
  virtual ~cPluginGstout();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return DESCRIPTION; }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
  virtual void MainThreadHook(void);
  virtual cString Active(void);
  virtual time_t WakeupTime(void);
  virtual const char *MainMenuEntry(void) { return NULL; }
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
  virtual bool Service(const char *Id, void *Data = NULL);
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
};

#endif // __GSTOUT_H
