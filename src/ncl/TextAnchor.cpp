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

#include "config.h"
#include "ncl/TextAnchor.h"

GINGA_NCL_BEGIN

TextAnchor::TextAnchor (string id, string t, long p) : Anchor (id)
{
  position = p;
  text = t;
  typeSet.insert ("TextAnchor");
}

long
TextAnchor::getPosition ()
{
  return position;
}

string
TextAnchor::getText ()
{
  return text;
}

void
TextAnchor::setPosition (long p)
{
  position = p;
}

void
TextAnchor::setText (string t)
{
  text = t;
}

GINGA_NCL_END
