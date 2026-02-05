/*
 * gstsetup.h: Setup menu for GStreamer output
 */

#ifndef __GSTSETUP_H
#define __GSTSETUP_H

#include <vdr/menuitems.h>

class cGstoutSetupPage : public cMenuSetupPage {
private:
  cGstoutConfig data;
  
  const char *audioSinkNames[10];
  const char *videoSinkNames[10];
  int audioSinkIndex;
  int videoSinkIndex;
  
  void Setup(void);
  
protected:
  virtual void Store(void);
  
public:
  cGstoutSetupPage(void);
};

#endif // __GSTSETUP_H
