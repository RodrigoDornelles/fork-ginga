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

#ifndef VIDEO_PLAYER_H
#define VIDEO_PLAYER_H

#include "Player.h"

GINGA_PLAYER_BEGIN

class VideoPlayer : public Player
{
public:
  VideoPlayer (GingaState *, const string &, const string &);
  virtual ~VideoPlayer ();
  void start () override;
  void stop () override;
  void pause () override;
  void resume () override;
  void redraw (cairo_t *) override;
  void setProperty (const string &, const string &) override;

private:
  GstElement *_playbin;           // pipeline
  GstElement *_capsfilter;        // video filter
  GstElement *_app_videosink;     // video sink
  GstElement *_volumefilter;      // volume filter
  GstElement *_balancefilter;     // balance filter
  GstElement *_app_audiosink;     // audio sink
  int _sample_flag;               // true if new sample is available
  GstAppSinkCallbacks _callbacks; // appsink callback data

  // Properties.
  double _volume;               // sound level
  bool _mute;                   // true if mute is on
  float _balance;               // balance level

  // Callbacks.
  static gboolean cb_Bus (GstBus *, GstMessage *, VideoPlayer *);
  static GstFlowReturn cb_NewSample (GstAppSink *, gpointer);
};

GINGA_PLAYER_END

#endif // VIDEO_PLAYER_H
