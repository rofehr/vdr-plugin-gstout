/*
 * gstoutput.h: GStreamer output engine
 */

#ifndef __GSTOUTPUT_H
#define __GSTOUTPUT_H

#include <vdr/thread.h>
#include <vdr/ringbuffer.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

// Forward declarations
class cGstAudioOutput;
class cGstVideoOutput;

// Main GStreamer output class
class cGstOutput : public cThread {
private:
  cGstAudioOutput *audioOutput;
  cGstVideoOutput *videoOutput;
  bool initialized;
  cMutex mutex;
  
  static gboolean BusCallback(GstBus *bus, GstMessage *msg, gpointer data);
  
protected:
  virtual void Action(void);
  
public:
  cGstOutput(void);
  virtual ~cGstOutput();
  
  bool Initialize(void);
  void Start(void);
  void Stop(void);
  void Reset(void);
  void MainThreadHook(void);
  
  cString GetStatistics(void);
  
  // Audio/Video data input
  bool PlayAudio(const uchar *Data, int Length);
  bool PlayVideo(const uchar *Data, int Length);
  
  // Flush buffers
  void Clear(void);
};

// --- cGstAudioOutput -------------------------------------------------------

class cGstAudioOutput {
private:
  GstElement *pipeline;
  GstElement *source;
  GstElement *decoder;
  GstElement *converter;
  GstElement *resampler;
  GstElement *sink;
  GstBus *bus;
  
  cRingBufferLinear *buffer;
  cMutex mutex;
  bool playing;
  
  static void NeedDataCallback(GstElement *source, guint size, gpointer data);
  static void EnoughDataCallback(GstElement *source, gpointer data);
  
public:
  cGstAudioOutput(void);
  virtual ~cGstAudioOutput();
  
  bool Initialize(void);
  void Start(void);
  void Stop(void);
  void Reset(void);
  
  bool Play(const uchar *Data, int Length);
  void Clear(void);
  
  cString GetStatistics(void);
};

// --- cGstVideoOutput -------------------------------------------------------

class cGstVideoOutput {
private:
  GstElement *pipeline;
  GstElement *source;
  GstElement *decoder;
  GstElement *deinterlace;
  GstElement *converter;
  GstElement *scaler;
  GstElement *sink;
  GstBus *bus;
  
  cRingBufferLinear *buffer;
  cMutex mutex;
  bool playing;
  
  static void NeedDataCallback(GstElement *source, guint size, gpointer data);
  static void EnoughDataCallback(GstElement *source, gpointer data);
  
public:
  cGstVideoOutput(void);
  virtual ~cGstVideoOutput();
  
  bool Initialize(void);
  void Start(void);
  void Stop(void);
  void Reset(void);
  
  bool Play(const uchar *Data, int Length);
  void Clear(void);
  
  cString GetStatistics(void);
};

#endif // __GSTOUTPUT_H
