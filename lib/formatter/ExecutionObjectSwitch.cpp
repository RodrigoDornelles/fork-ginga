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
#include "ExecutionObjectSwitch.h"

GINGA_FORMATTER_BEGIN

ExecutionObjectSwitch::ExecutionObjectSwitch (GingaInternal *ginga,
                                              const string &id,
                                              Node *switchNode)
  : ExecutionObjectContext (ginga, id, switchNode)
{
  _selectedObj = nullptr;
}

void
ExecutionObjectSwitch::select (ExecutionObject *exeObj)
{
  if (exeObj != nullptr)
    {
      g_assert_nonnull (this->getChildById (exeObj->getId ()));
      _selectedObj = exeObj;
    }
  else
    {
      _selectedObj = nullptr;
      // for (auto evt: *(this->getEvents ()))
      //   {
      //     auto switchEvent = cast (SwitchEvent *, evt);
      //     g_assert_nonnull (switchEvent);
      //     switchEvent->setMappedEvent (nullptr);
      //   }
    }
}

GINGA_FORMATTER_END
