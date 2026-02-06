/*
 * gstoutput.c: GStreamer output engine
 */

#include "gstoutput.h"
#include "gstout.h"
#include <vdr/tools.h>

// --- cGstOutput ------------------------------------------------------------

cGstOutput::cGstOutput(void)
:cThread("GStreamer Output")
{
  audioOutput = NULL;
  videoOutput = NULL;
  osdProvider = NULL;
  initialized = false;
}

cGstOutput::~cGstOutput()
{
  Stop();
  delete audioOutput;
  delete videoOutput;
  
  if (initialized)
    gst_deinit();
}

bool cGstOutput::Initialize(void)
{
  cMutexLock lock(&mutex);
  
  // Initialize GStreamer
  GError *error = NULL;
  if (!gst_init_check(NULL, NULL, &error)) {
    esyslog("gstout: Failed to initialize GStreamer: %s", error ? error->message : "unknown error");
    if (error)
      g_error_free(error);
    return false;
  }
  
  initialized = true;
  isyslog("gstout: GStreamer %s initialized", gst_version_string());
  
  // Create audio and video outputs
  audioOutput = new cGstAudioOutput();
  if (!audioOutput->Initialize()) {
    esyslog("gstout: Failed to initialize audio output");
    return false;
  }
  
  videoOutput = new cGstVideoOutput();
  if (!videoOutput->Initialize()) {
    esyslog("gstout: Failed to initialize video output");
    return false;
  }
  
  return true;
}

void cGstOutput::Start(void)
{
  cMutexLock lock(&mutex);
  
  if (audioOutput)
    audioOutput->Start();
  if (videoOutput)
    videoOutput->Start();
  
  cThread::Start();
}

void cGstOutput::Stop(void)
{
  cMutexLock lock(&mutex);
  
  if (audioOutput)
    audioOutput->Stop();
  if (videoOutput)
    videoOutput->Stop();
  
  if (Running())
    Cancel(3);
}

void cGstOutput::Reset(void)
{
  cMutexLock lock(&mutex);
  
  if (audioOutput)
    audioOutput->Reset();
  if (videoOutput)
    videoOutput->Reset();
}

void cGstOutput::MainThreadHook(void)
{
  // Process any pending events
}

void cGstOutput::Action(void)
{
  while (Running()) {
    // Process GStreamer events
    cCondWait::SleepMs(100);
  }
}

bool cGstOutput::PlayAudio(const uchar *Data, int Length)
{
  if (audioOutput)
    return audioOutput->Play(Data, Length);
  return false;
}

bool cGstOutput::PlayVideo(const uchar *Data, int Length)
{
  if (videoOutput)
    return videoOutput->Play(Data, Length);
  return false;
}

void cGstOutput::Clear(void)
{
  if (audioOutput)
    audioOutput->Clear();
  if (videoOutput)
    videoOutput->Clear();
}

cString cGstOutput::GetStatistics(void)
{
  cString audio = audioOutput ? audioOutput->GetStatistics() : "Audio: N/A";
  cString video = videoOutput ? videoOutput->GetStatistics() : "Video: N/A";
  
  return cString::sprintf("%s\n%s", *audio, *video);
}

gboolean cGstOutput::BusCallback(GstBus *bus, GstMessage *msg, gpointer data)
{
  switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR: {
      GError *err;
      gchar *debug;
      gst_message_parse_error(msg, &err, &debug);
      esyslog("gstout: GStreamer error: %s", err->message);
      if (debug)
        dsyslog("gstout: Debug info: %s", debug);
      g_error_free(err);
      g_free(debug);
      break;
    }
    case GST_MESSAGE_WARNING: {
      GError *err;
      gchar *debug;
      gst_message_parse_warning(msg, &err, &debug);
      isyslog("gstout: GStreamer warning: %s", err->message);
      g_error_free(err);
      g_free(debug);
      break;
    }
    case GST_MESSAGE_EOS:
      dsyslog("gstout: End of stream");
      break;
    case GST_MESSAGE_STATE_CHANGED: {
      GstState old_state, new_state;
      gst_message_parse_state_changed(msg, &old_state, &new_state, NULL);
      dsyslog("gstout: State changed from %s to %s",
              gst_element_state_get_name(old_state),
              gst_element_state_get_name(new_state));
      break;
    }
    default:
      break;
  }
  
  return TRUE;
}

// --- cGstAudioOutput -------------------------------------------------------

cGstAudioOutput::cGstAudioOutput(void)
{
  pipeline = NULL;
  source = NULL;
  decoder = NULL;
  converter = NULL;
  resampler = NULL;
  sink = NULL;
  bus = NULL;
  buffer = NULL;
  playing = false;
}

cGstAudioOutput::~cGstAudioOutput()
{
  Stop();
  
  if (pipeline) {
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
  }
  
  delete buffer;
}

bool cGstAudioOutput::Initialize(void)
{
  cMutexLock lock(&mutex);
  
  // Create ring buffer for audio data
  buffer = new cRingBufferLinear(GstoutConfig.audioBufferSize * 1024, 0, false);
  if (!buffer) {
    esyslog("gstout: Failed to create audio buffer");
    return false;
  }
  
  // Create pipeline elements
  source = gst_element_factory_make("appsrc", "audio-source");
  decoder = gst_element_factory_make("decodebin", "audio-decoder");
  converter = gst_element_factory_make("audioconvert", "audio-converter");
  resampler = gst_element_factory_make("audioresample", "audio-resampler");
  sink = gst_element_factory_make(GstoutConfig.audioSink, "audio-sink");
  
  if (!source || !decoder || !converter || !resampler || !sink) {
    esyslog("gstout: Failed to create audio pipeline elements");
    return false;
  }
  
  // Create pipeline
  pipeline = gst_pipeline_new("audio-pipeline");
  if (!pipeline) {
    esyslog("gstout: Failed to create audio pipeline");
    return false;
  }
  
  // Add elements to pipeline
  gst_bin_add_many(GST_BIN(pipeline), source, decoder, converter, resampler, sink, NULL);
  
  // Link elements (decoder will be linked dynamically via pad-added signal)
  if (!gst_element_link(source, decoder)) {
    esyslog("gstout: Failed to link audio source and decoder");
    return false;
  }
  
  if (!gst_element_link_many(converter, resampler, sink, NULL)) {
    esyslog("gstout: Failed to link audio converter, resampler, and sink");
    return false;
  }
  
  // Connect decoder pad-added signal
  g_signal_connect(decoder, "pad-added", G_CALLBACK(+[](GstElement *src, GstPad *new_pad, gpointer data) {
    GstElement *conv = (GstElement *)data;
    GstPad *sink_pad = gst_element_get_static_pad(conv, "sink");
    if (!gst_pad_is_linked(sink_pad)) {
      gst_pad_link(new_pad, sink_pad);
    }
    gst_object_unref(sink_pad);
  }), converter);
  
  // Configure appsrc
  g_object_set(G_OBJECT(source),
               "stream-type", GST_APP_STREAM_TYPE_STREAM,
               "format", GST_FORMAT_TIME,
               "is-live", TRUE,
               NULL);
  
  // Connect appsrc callbacks
  g_signal_connect(source, "need-data", G_CALLBACK(NeedDataCallback), this);
  g_signal_connect(source, "enough-data", G_CALLBACK(EnoughDataCallback), this);
  
  // Set up bus
  bus = gst_element_get_bus(pipeline);
  gst_bus_add_watch(bus, cGstOutput::BusCallback, this);
  
  isyslog("gstout: Audio pipeline created (sink: %s)", GstoutConfig.audioSink);
  
  return true;
}

void cGstAudioOutput::Start(void)
{
  cMutexLock lock(&mutex);
  
  if (pipeline) {
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    playing = true;
    isyslog("gstout: Audio pipeline started");
  }
}

void cGstAudioOutput::Stop(void)
{
  cMutexLock lock(&mutex);
  
  if (pipeline) {
    gst_element_set_state(pipeline, GST_STATE_NULL);
    playing = false;
    isyslog("gstout: Audio pipeline stopped");
  }
}

void cGstAudioOutput::Reset(void)
{
  cMutexLock lock(&mutex);
  
  Clear();
  
  if (pipeline) {
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
  }
}

bool cGstAudioOutput::Play(const uchar *Data, int Length)
{
  cMutexLock lock(&mutex);
  
  if (!buffer || !playing)
    return false;
  
  int free = buffer->Free();
  if (free < Length)
    return false;
  
  int written = buffer->Put(Data, Length);
  return written == Length;
}

void cGstAudioOutput::Clear(void)
{
  cMutexLock lock(&mutex);
  
  if (buffer)
    buffer->Clear();
}

void cGstAudioOutput::NeedDataCallback(GstElement *source, guint size, gpointer data)
{
  cGstAudioOutput *self = (cGstAudioOutput *)data;
  cMutexLock lock(&self->mutex);
  
  if (!self->buffer)
    return;
  
  int available = self->buffer->Available();
  if (available > 0) {
    uchar *readData;
    int count = 0;
	readData = self->buffer->Get(count);
    if (count > 0) {
      GstBuffer *gstBuffer = gst_buffer_new_allocate(NULL, count, NULL);
      GstMapInfo map;
      gst_buffer_map(gstBuffer, &map, GST_MAP_WRITE);
      memcpy(map.data, readData, count);
      gst_buffer_unmap(gstBuffer, &map);
      
      GstFlowReturn ret;
      g_signal_emit_by_name(source, "push-buffer", gstBuffer, &ret);
      gst_buffer_unref(gstBuffer);
      
      self->buffer->Del(count);
    }
  }
}

void cGstAudioOutput::EnoughDataCallback(GstElement *source, gpointer data)
{
  // Appsrc has enough data, we can pause feeding
}

cString cGstAudioOutput::GetStatistics(void)
{
  cMutexLock lock(&mutex);
  
  int available = buffer ? buffer->Available() : 0;
  int free = buffer ? buffer->Free() : 0;
  
  GstState state = GST_STATE_NULL;
  if (pipeline)
    gst_element_get_state(pipeline, &state, NULL, GST_CLOCK_TIME_NONE);
  
  return cString::sprintf("Audio: %s, Buffer: %d/%d KB",
                         gst_element_state_get_name(state),
                         available / 1024,
                         (available + free) / 1024);
}

// --- cGstVideoOutput -------------------------------------------------------

cGstVideoOutput::cGstVideoOutput(void)
{
  pipeline = NULL;
  source = NULL;
  decoder = NULL;
  deinterlace = NULL;
  converter = NULL;
  scaler = NULL;
  sink = NULL;
  bus = NULL;
  buffer = NULL;
  playing = false;
}

cGstVideoOutput::~cGstVideoOutput()
{
  Stop();
  
  if (pipeline) {
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
  }
  
  delete buffer;
}

bool cGstVideoOutput::Initialize(void)
{
  cMutexLock lock(&mutex);
  
  // Create ring buffer for video data
  buffer = new cRingBufferLinear(GstoutConfig.videoBufferSize * 1024, 0, false);
  if (!buffer) {
    esyslog("gstout: Failed to create video buffer");
    return false;
  }
  
  // Create pipeline elements
  source = gst_element_factory_make("appsrc", "video-source");
  
  // Use hardware decoder if available and configured
  if (GstoutConfig.useHardwareDecoding) {
    decoder = gst_element_factory_make("vaapidecodebin", "video-decoder");
    if (!decoder) {
      isyslog("gstout: Hardware decoder not available, using software decoder");
      decoder = gst_element_factory_make("decodebin", "video-decoder");
    }
  } else {
    decoder = gst_element_factory_make("decodebin", "video-decoder");
  }
  
  if (GstoutConfig.deinterlace)
    deinterlace = gst_element_factory_make("deinterlace", "deinterlacer");
  
  converter = gst_element_factory_make("videoconvert", "video-converter");
  scaler = gst_element_factory_make("videoscale", "video-scaler");
  sink = gst_element_factory_make(GstoutConfig.videoSink, "video-sink");
  
  if (!source || !decoder || !converter || !scaler || !sink) {
    esyslog("gstout: Failed to create video pipeline elements");
    return false;
  }
  
  // Create pipeline
  pipeline = gst_pipeline_new("video-pipeline");
  if (!pipeline) {
    esyslog("gstout: Failed to create video pipeline");
    return false;
  }
  
  // Add elements to pipeline
  gst_bin_add_many(GST_BIN(pipeline), source, decoder, NULL);
  if (deinterlace)
    gst_bin_add(GST_BIN(pipeline), deinterlace);
  gst_bin_add_many(GST_BIN(pipeline), converter, scaler, sink, NULL);
  
  // Link elements
  if (!gst_element_link(source, decoder)) {
    esyslog("gstout: Failed to link video source and decoder");
    return false;
  }
  
  GstElement *linkTarget = deinterlace ? deinterlace : converter;
  
  // Connect decoder pad-added signal
  g_signal_connect(decoder, "pad-added", G_CALLBACK(+[](GstElement *src, GstPad *new_pad, gpointer data) {
    GstElement *target = (GstElement *)data;
    GstPad *sink_pad = gst_element_get_static_pad(target, "sink");
    if (!gst_pad_is_linked(sink_pad)) {
      gst_pad_link(new_pad, sink_pad);
    }
    gst_object_unref(sink_pad);
  }), linkTarget);
  
  if (deinterlace) {
    if (!gst_element_link_many(deinterlace, converter, scaler, sink, NULL)) {
      esyslog("gstout: Failed to link video pipeline with deinterlace");
      return false;
    }
  } else {
    if (!gst_element_link_many(converter, scaler, sink, NULL)) {
      esyslog("gstout: Failed to link video pipeline");
      return false;
    }
  }
  
  // Configure appsrc
  g_object_set(G_OBJECT(source),
               "stream-type", GST_APP_STREAM_TYPE_STREAM,
               "format", GST_FORMAT_TIME,
               "is-live", TRUE,
               NULL);
  
  // Connect appsrc callbacks
  g_signal_connect(source, "need-data", G_CALLBACK(NeedDataCallback), this);
  g_signal_connect(source, "enough-data", G_CALLBACK(EnoughDataCallback), this);
  
  // Set up bus
  bus = gst_element_get_bus(pipeline);
  gst_bus_add_watch(bus, cGstOutput::BusCallback, this);
  
  isyslog("gstout: Video pipeline created (sink: %s, hwdec: %s, deinterlace: %s)",
          GstoutConfig.videoSink,
          GstoutConfig.useHardwareDecoding ? "yes" : "no",
          GstoutConfig.deinterlace ? "yes" : "no");
  
  return true;
}

void cGstVideoOutput::Start(void)
{
  cMutexLock lock(&mutex);
  
  if (pipeline) {
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    playing = true;
    isyslog("gstout: Video pipeline started");
  }
}

void cGstVideoOutput::Stop(void)
{
  cMutexLock lock(&mutex);
  
  if (pipeline) {
    gst_element_set_state(pipeline, GST_STATE_NULL);
    playing = false;
    isyslog("gstout: Video pipeline stopped");
  }
}

void cGstVideoOutput::Reset(void)
{
  cMutexLock lock(&mutex);
  
  Clear();
  
  if (pipeline) {
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
  }
}

bool cGstVideoOutput::Play(const uchar *Data, int Length)
{
  cMutexLock lock(&mutex);
  
  if (!buffer || !playing)
    return false;
  
  int free = buffer->Free();
  if (free < Length)
    return false;
  
  int written = buffer->Put(Data, Length);
  return written == Length;
}

void cGstVideoOutput::Clear(void)
{
  cMutexLock lock(&mutex);
  
  if (buffer)
    buffer->Clear();
}

void cGstVideoOutput::NeedDataCallback(GstElement *source, guint size, gpointer data)
{
  cGstVideoOutput *self = (cGstVideoOutput *)data;
  cMutexLock lock(&self->mutex);
  
  if (!self->buffer)
    return;
  
  int available = self->buffer->Available();
  if (available > 0) {
    uchar *readData;
    int count = 0;
	readData = self->buffer->Get(count);
    if (count > 0) {
      GstBuffer *gstBuffer = gst_buffer_new_allocate(NULL, count, NULL);
      GstMapInfo map;
      gst_buffer_map(gstBuffer, &map, GST_MAP_WRITE);
      memcpy(map.data, readData, count);
      gst_buffer_unmap(gstBuffer, &map);
      
      GstFlowReturn ret;
      g_signal_emit_by_name(source, "push-buffer", gstBuffer, &ret);
      gst_buffer_unref(gstBuffer);
      
      self->buffer->Del(count);
    }
  }
}

void cGstVideoOutput::EnoughDataCallback(GstElement *source, gpointer data)
{
  // Appsrc has enough data, we can pause feeding
}

cString cGstVideoOutput::GetStatistics(void)
{
  cMutexLock lock(&mutex);
  
  int available = buffer ? buffer->Available() : 0;
  int free = buffer ? buffer->Free() : 0;
  
  GstState state = GST_STATE_NULL;
  if (pipeline)
    gst_element_get_state(pipeline, &state, NULL, GST_CLOCK_TIME_NONE);
  
  return cString::sprintf("Video: %s, Buffer: %d/%d KB",
                         gst_element_state_get_name(state),
                         available / 1024,
                         (available + free) / 1024);
}
