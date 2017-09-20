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

#include "ginga.h"
#include "ginga-internal.h"
#include "GingaState.h"

Ginga::Ginga (int, char **, GingaOptions *)
{
}

Ginga::~Ginga ()
{
}


// Class methods.

/**
 * @brief Creates a new Ginga state.
 * @param argc Number arguments passed to main.
 * @param argv Arguments passed to main.
 * @param opts State options.
 * @return A new formatter handle.
 */
Ginga *
Ginga::create (int argc, char **argv, GingaOptions *opts)
{
  return new GingaState (argc, argv, opts);
}

/**
 * @brief Gets Ginga version string.
 * @return libginga version string.
 */
string
Ginga::version ()
{
  return PACKAGE_VERSION;
}
