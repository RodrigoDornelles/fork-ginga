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

#ifndef EXECUTION_OBJECT_SETTINGS_H
#define EXECUTION_OBJECT_SETTINGS_H

#include "ExecutionObject.h"

GINGA_FORMATTER_BEGIN

class ExecutionObject;
class ExecutionObjectSettings : public ExecutionObject
{
public:
  ExecutionObjectSettings (GingaPrivate *,
                           const string &, Node *, INclActionListener *);
  virtual ~ExecutionObjectSettings () {};

  void setProperty (const string &, const string &,
                    const string &, GingaTime);

  void updateCurrentFocus (const string &);
  void scheduleFocusUpdate (const string &);

  // From IEventListener.
  virtual void handleKeyEvent (const string &, bool) override;
  virtual void handleTickEvent (GingaTime, GingaTime, int) override;

 private:
  string _nextFocus;            // next focus index
  bool _hasNextFocus;           // true if a focus update is scheduled

};

GINGA_FORMATTER_END

#endif
