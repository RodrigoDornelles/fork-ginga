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

#ifndef GINGA_H
#define GINGA_H

#ifdef  __cplusplus
# define GINGA_BEGIN_DECLS extern "C" {/*}*/
# define GINGA_END_DECLS            /*{*/}
#else
# define GINGA_BEGIN_DECLS
# define GINGA_END_DECLS
#endif

GINGA_BEGIN_DECLS
#include <cairo.h>
#include <glib.h>
#include <gdk/gdk.h>
GINGA_END_DECLS

#include <string>

// Aliases.
typedef GdkRGBA GingaColor;
typedef GdkRectangle GingaRect;

class Ginga
{
 public:
  Ginga (int, char **, int, int, bool);
  ~Ginga ();

  void start (const std::string &);
  void stop ();

  void redraw (cairo_t *);
  void send_key (const std::string &, bool);
  void send_tick (guint64, guint64, guint64);

 private:
  bool started;
  void *_scheduler;
  void *_display;
};

#endif // GINGA_H
