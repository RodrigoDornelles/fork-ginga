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

#ifndef IMAGE_PLAYER_H
#define IMAGE_PLAYER_H

#include "Player.h"
#include "mb/Display.h"
using namespace ginga::mb;

GINGA_PLAYER_BEGIN

class ImagePlayer : public Player
{
private:
  GINGA_MUTEX_DEFN ();
  GINGA_COND_DEFN (DisplayJob);

  static bool displayJobCallbackWrapper (DisplayJob *,
                                         SDL_Renderer *, void *);
  bool displayJobCallback (DisplayJob *, SDL_Renderer *);
  SDL_Surface* decodeSVG(int width, int height);

public:
  ImagePlayer (const string &mrl);
  ~ImagePlayer (void);
  bool play (void);
};

GINGA_PLAYER_END

#endif /* IMAGE_PLAYER_H */
