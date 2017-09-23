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

#include "ginga-internal.h"
#include "RegionBase.h"

GINGA_NCL_BEGIN

/**
 * @brief Creates a new region base.
 * @param ncl Parent document.
 * @param id Base id.
 */
RegionBase::RegionBase (NclDocument *ncl, const string &id) : Base (ncl, id)
{
}

/**
 * @brief Destroys region base.
 */
RegionBase::~RegionBase ()
{
}

/**
 * @brief Adds region to base.
 * @param region Region.
 */
void
RegionBase::addRegion (Region *region)
{
  Base::addEntity (region);
}

/**
 * @brief Gets region.
 * @param id Region id.
 * @return Region if successful, or null if not found.
 */
Region *
RegionBase::getRegion (const string &id)
{
  return cast (Region *, Base::getEntity (id));
}

GINGA_NCL_END
