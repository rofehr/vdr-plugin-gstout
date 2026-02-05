/*
 * gstsetup.h: Setup menu for GStreamer output
 */

#ifndef __GSTSETUP_H
#define __GSTSETUP_H

#include <vdr/menuitems.h>

// Forward declaration to avoid circular dependency
class cGstoutConfig;

class cGstoutSetupPage : public cMenuSetupPage {
private:
  int audioSinkIndex;
  int videoSinkIndex;
  int useHardwareDecoding;
  int deinterlace;
  int audioBufferSize;
  int videoBufferSize;
  
  const char *audioSinkNames[10];
  const char *videoSinkNames[10];
  
  void Setup(void);
  
protected:
  virtual void Store(void);
  
public:
  cGstoutSetupPage(void);
};

#endif // __GSTSETUP_H
