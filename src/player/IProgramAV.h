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

#ifndef IPROGRAMAV_H_
#define IPROGRAMAV_H_

#include "IPlayer.h"

GINGA_PLAYER_BEGIN

class IProgramAV
{
public:
  virtual ~IProgramAV (){};
  virtual void release () = 0;
  virtual void setAVPid (int programPid, int aPid, int vPid) = 0;
  virtual IPlayer *getPlayer (int pid) = 0;
  virtual void setPlayer (int pid, IPlayer *) = 0;
  virtual void setPropertyValue (const string &name, const string &value) = 0;
};

GINGA_PLAYER_END

#endif /*IPROGRAMAV_H_*/
