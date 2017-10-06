/* Copyright (C) 2006-2017 PUC-Rio/Laboratorio TeleMidia

This file is part of Ginga (Ginga-NCL).

Ginga is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Ginga is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
License for more details.

You should have received a copy of the GNU General Public License
along with Ginga.  If not, see <http://www.gnu.org/licenses/>.  */

#include "aux-ginga.h"
#include "VideoPlayer.h"

#define gstx_element_get_state(elt, st, pend, tout)             \
  g_assert (gst_element_get_state ((elt), (st), (pend), (tout)) \
            != GST_STATE_CHANGE_FAILURE)

#define gstx_element_get_state_sync(elt, st, pend)\
  gstx_element_get_state ((elt), (st), (pend), GST_CLOCK_TIME_NONE)

#define gstx_element_set_state(elt, st)         \
  g_assert (gst_element_set_state ((elt), (st)) \
            != GST_STATE_CHANGE_FAILURE)

#define gstx_element_set_state_sync(elt, st)                    \
  G_STMT_START                                                  \
  {                                                             \
    gstx_element_set_state ((elt), (st));                       \
    gstx_element_get_state_sync ((elt), nullptr, nullptr);      \
  }                                                             \
  G_STMT_END

GINGA_PLAYER_BEGIN

// Public.

VideoPlayer::VideoPlayer (GingaInternal *ginga, const string &id,
                          const string &uri)
  : Player (ginga, id, uri)
{
  GstBus *bus;
  gulong ret;
  char *buf;

  GstPad *pad;
  GstPad *ghost;

  _playbin = nullptr;
  _audio.bin = nullptr;
  _audio.volume = nullptr;
  _audio.pan = nullptr;
  _audio.sink = nullptr;
  _video.bin = nullptr;
  _video.caps = nullptr;
  _video.sink = nullptr;

  if (!gst_is_initialized ())
    {
      GError *error = nullptr;
      if (unlikely (!gst_init_check (nullptr, nullptr, &error)))
        {
          g_assert_nonnull (error);
          ERROR ("%s", error->message);
          g_error_free (error);
        }
    }

  _playbin = gst_element_factory_make ("playbin", "playbin");
  g_assert_nonnull (_playbin);

  bus = gst_pipeline_get_bus (GST_PIPELINE (_playbin));
  g_assert_nonnull (bus);
  ret = gst_bus_add_watch (bus, (GstBusFunc) cb_Bus, this);
  g_assert (ret > 0);
  gst_object_unref (bus);

  buf = gst_filename_to_uri (uri.c_str (), nullptr);
  g_assert_nonnull (buf);

  g_object_set (G_OBJECT (_playbin), "uri", buf, nullptr);
  g_free (buf);

  // Setup audio pipeline.
  _audio.bin = gst_bin_new ("audio.bin");
  g_assert_nonnull (_audio.bin);

  _audio.volume = gst_element_factory_make ("volume", "audio.volume");
  g_assert_nonnull (_audio.volume);

  _audio.pan = gst_element_factory_make ("audiopanorama", "audio.pan");
  g_assert_nonnull (_audio.pan);

  _audio.sink = gst_element_factory_make ("alsasink", "audio.sink");
  if (_audio.sink == nullptr)
    _audio.sink = gst_element_factory_make ("autoaudiosink", "audio.sink");
  g_assert_nonnull (_audio.sink);

  g_assert (gst_bin_add (GST_BIN (_audio.bin), _audio.volume));
  g_assert (gst_bin_add (GST_BIN (_audio.bin), _audio.pan));
  g_assert (gst_bin_add (GST_BIN (_audio.bin), _audio.sink));
  g_assert (gst_element_link (_audio.volume, _audio.pan));
  g_assert (gst_element_link (_audio.pan, _audio.sink));

  pad = gst_element_get_static_pad (_audio.volume, "sink");
  g_assert_nonnull (pad);
  ghost = gst_ghost_pad_new ("sink", pad);
  g_assert_nonnull (ghost);
  g_assert (gst_element_add_pad (_audio.bin, ghost));
  gst_object_unref (pad);
  g_object_set (G_OBJECT (_playbin), "audio-sink", _audio.bin, nullptr);

  // Setup video pipeline.
  _video.bin = gst_bin_new ("video.bin");
  g_assert_nonnull (_video.bin);

  _video.caps = gst_element_factory_make ("capsfilter", "video.filter");
  g_assert_nonnull (_video.caps);

  _video.sink = gst_element_factory_make ("appsink", "video.sink");
  g_assert_nonnull (_video.sink);

  g_assert (gst_bin_add (GST_BIN (_video.bin), _video.caps));
  g_assert (gst_bin_add (GST_BIN (_video.bin), _video.sink));
  g_assert (gst_element_link (_video.caps, _video.sink));

  pad = gst_element_get_static_pad (_video.caps, "sink");
  g_assert_nonnull (pad);
  ghost = gst_ghost_pad_new ("sink", pad);
  g_assert_nonnull (ghost);
  g_assert (gst_element_add_pad (_video.bin, ghost));
  gst_object_unref (pad);
  g_object_set (G_OBJECT (_playbin), "video-sink", _video.bin, nullptr);

  // Callbacks.
  _callbacks.eos = nullptr;
  _callbacks.new_preroll = nullptr;
  _callbacks.new_sample = cb_NewSample;
  gst_app_sink_set_callbacks (GST_APP_SINK (_video.sink),
                              &_callbacks, this, nullptr);

  // Initialize handled properties.
  static set<string> handled = {"balance", "mute", "volume", "freeze"};
  this->resetProperties (&handled);
}

VideoPlayer::~VideoPlayer ()
{
}

void
VideoPlayer::start ()
{
  GstCaps *caps;
  GstStructure *st;
  GstStateChangeReturn ret;
  static const char *format;
#if defined WITH_OPENGL && WITH_OPENGL
  format = "RGBA";
#else
  format = "BGRA";
#endif

  g_assert (_state != OCCURRING);
  TRACE ("starting");

  st = gst_structure_new_empty ("video/x-raw");
  gst_structure_set (st,
                     "format", G_TYPE_STRING, format,
                     nullptr);

  caps = gst_caps_new_full (st, nullptr);
  g_assert_nonnull (caps);

  g_object_set (_video.caps, "caps", caps, nullptr);
  gst_caps_unref (caps);

  Player::setEOS (false);
  g_atomic_int_set (&_sample_flag, 0);

  // Initialize properties.
  g_object_set (_audio.volume,
                "volume", _prop.volume,
                "mute", _prop.mute,
                NULL);

  g_object_set (_audio.pan,
                "panorama", _prop.balance,
                NULL);

  ret = gst_element_set_state (_playbin, GST_STATE_PLAYING);
  if (unlikely (ret == GST_STATE_CHANGE_FAILURE))
    Player::setEOS (true);

  Player::start ();
}

void
VideoPlayer::stop ()
{
  g_assert (_state != SLEEPING);
  TRACE ("stopping");

  gstx_element_set_state_sync (_playbin, GST_STATE_NULL);
  gst_object_unref (_playbin);
  Player::stop ();
}

void
VideoPlayer::pause ()
{
  g_assert (_state != PAUSED && _state != SLEEPING);
  TRACE ("pausing");

  gstx_element_set_state_sync (_playbin, GST_STATE_PAUSED);
  Player::pause ();
}

void
VideoPlayer::resume ()
{
  g_assert (_state == PAUSED);
  TRACE ("resuming");

  gstx_element_set_state_sync (_playbin, GST_STATE_PLAYING);
  Player::resume ();
}

void
VideoPlayer::redraw (cairo_t *cr)
{
  GstSample *sample;
  GstVideoFrame v_frame;
  GstVideoInfo v_info;
  GstBuffer *buf;
  GstCaps *caps;
  guint8 *pixels;
  int width;
  int height;
  int stride;

  static cairo_user_data_key_t key;
  cairo_status_t status;

  g_assert (_state != SLEEPING);

  if (Player::getEOS ())
    goto done;

  if (!g_atomic_int_compare_and_exchange (&_sample_flag, 1, 0))
    goto done;

  sample = gst_app_sink_pull_sample (GST_APP_SINK (_video.sink));
  if (sample == nullptr)
    goto done;

  buf = gst_sample_get_buffer (sample);
  g_assert_nonnull (buf);

  caps = gst_sample_get_caps (sample);
  g_assert_nonnull (caps);

  g_assert (gst_video_info_from_caps (&v_info, caps));
  g_assert (gst_video_frame_map (&v_frame, &v_info, buf, GST_MAP_READ));

  pixels = (guint8 *) GST_VIDEO_FRAME_PLANE_DATA (&v_frame, 0);
  width = GST_VIDEO_FRAME_WIDTH (&v_frame);
  height = GST_VIDEO_FRAME_HEIGHT (&v_frame);
  stride = (int) GST_VIDEO_FRAME_PLANE_STRIDE (&v_frame, 0);

  if (_surface != nullptr)
    {
      cairo_surface_destroy (_surface);
    }

  _surface = cairo_image_surface_create_for_data
    (pixels, CAIRO_FORMAT_ARGB32, width, height, stride);
  g_assert_nonnull (_surface);

  gst_video_frame_unmap (&v_frame);

  status = cairo_surface_set_user_data
    (_surface, &key, (void *) sample,
     (cairo_destroy_func_t) gst_sample_unref);
  g_assert (status == CAIRO_STATUS_SUCCESS);

 done:
  Player::redraw (cr);
}

void
VideoPlayer::redrawGL ()
{
#if !(defined WITH_OPENGL && WITH_OPENGL)
  WARNING_NOT_IMPLEMENTED ("not compiled with OpenGL support");
#else
  GstSample *sample;
  GstVideoFrame v_frame;
  GstVideoInfo v_info;
  GstBuffer *buf;
  GstCaps *caps;
  guint8 *pixels;
  int width;
  int height;

  g_assert (_state != SLEEPING);

  if (Player::getEOS ())
    goto done;

  if (!g_atomic_int_compare_and_exchange (&_sample_flag, 1, 0))
    goto done;

  sample = gst_app_sink_pull_sample (GST_APP_SINK (_video.sink));
  if (sample == nullptr)
    goto done;

  buf = gst_sample_get_buffer (sample);
  g_assert_nonnull (buf);

  caps = gst_sample_get_caps (sample);
  g_assert_nonnull (caps);

  g_assert (gst_video_info_from_caps (&v_info, caps));
  g_assert (gst_video_frame_map (&v_frame, &v_info, buf, GST_MAP_READ));

  pixels = (guint8 *) GST_VIDEO_FRAME_PLANE_DATA (&v_frame, 0);
  width = GST_VIDEO_FRAME_WIDTH (&v_frame);
  height = GST_VIDEO_FRAME_HEIGHT (&v_frame);

  if (_gltexture != (GLuint) -1)
    {
      gl_delete_texture (&_gltexture);
    }

  // fixme: There is no need for recreating the texture to each frame.
  gl_create_texture (&_gltexture, width, height, pixels);

  gst_video_frame_unmap (&v_frame);

 done:
  Player::redrawGL ();
#endif
}


// Protected.

bool
VideoPlayer::doSetProperty (PlayerProperty code,
                            unused (const string &name),
                            const string &value)
{
  switch (code)
    {
      case PROP_BALANCE:
        _prop.balance = xstrtodorpercent (value, nullptr);
        if (_state != SLEEPING)
          g_object_set (_audio.pan, "panorama", _prop.balance, NULL);
        break;
//      case PROP_BASS:
//        _prop.bass = xstrtodorpercent (value, nullptr);
//        if (_state != SLEEPING)
//          //TODO;
//        break;
//      case PROP_FREEZE:
//        _prop.freeze = ginga_parse_bool (value);
//        if (_state != SLEEPING)
//          //TODO;
//        break;
      case PROP_MUTE:
        _prop.mute = ginga_parse_bool (value);
        if (_state != SLEEPING)
          g_object_set (_audio.volume, "mute", _prop.mute, NULL);
        break;
//      case PROP_TREBLE:
//        _prop.treble = xstrtodorpercent (value, nullptr);
//        if (_state != SLEEPING)
//          //TODO;
//        break;
      case PROP_VOLUME:
        _prop.volume = xstrtodorpercent (value, nullptr);
        if (_state != SLEEPING)
          g_object_set (_audio.volume, "volume", _prop.volume, NULL);
        break;
      default:
        return Player::doSetProperty (code, name, value);
    }
  return true;
}

// Private: Static (GStreamer callbacks).

gboolean
VideoPlayer::cb_Bus (GstBus *bus, GstMessage *msg, VideoPlayer *player)
{
  g_assert_nonnull (bus);
  g_assert_nonnull (msg);
  g_assert_nonnull (player);

  switch (GST_MESSAGE_TYPE (msg))
    {
    case GST_MESSAGE_EOS:
      {
        player->setEOS (true);
        TRACE ("EOS");
        break;
      }
    case GST_MESSAGE_ERROR:
    case GST_MESSAGE_WARNING:
      {
        GstObject *obj = nullptr;
        GError *error = nullptr;

        obj = GST_MESSAGE_SRC (msg);
        g_assert_nonnull (obj);

        if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR)
          {
            gst_message_parse_error (msg, &error, nullptr);
            g_assert_nonnull (error);
            ERROR ("%s", error->message);
          }
        else
          {
            gst_message_parse_warning (msg, &error, nullptr);
            g_assert_nonnull (error);
            WARNING ("%s", error->message);
          }
        g_error_free (error);
        break;
      }
    default:
      break;
    }
  return TRUE;
}

GstFlowReturn
VideoPlayer::cb_NewSample (unused (GstAppSink *appsink), gpointer data)
{
  VideoPlayer *player = (VideoPlayer *) data;
  g_assert_nonnull (player);
  g_atomic_int_compare_and_exchange (&player->_sample_flag, 0, 1);
  return GST_FLOW_OK;
}

GINGA_PLAYER_END
