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

#ifndef SIMPLE_ACTION_H
#define SIMPLE_ACTION_H

#include "Action.h"
#include "CompoundAction.h"
#include "Role.h"

GINGA_NCL_BEGIN

class SimpleAction : public Action, public Role
{
public:
  SimpleAction (EventType, EventStateTransition, const string &,
                const string &, const string &, const string &,
                const string &, const string &, const string &);
  virtual ~SimpleAction ();

  EventStateTransition getActionType ();
  string getDelay ();
  string getRepeat ();
  string getRepeatDelay ();
  string getValue ();
  string getDuration ();
  string getBy ();

  static bool isReserved (const string &, EventType *,
                          EventStateTransition *);
private:
  EventStateTransition _actionType;
  string _delay;
  string _repeat;
  string _repeatDelay;
  string _value;
  string _duration;
  string _by;
};

GINGA_NCL_END

#endif // SIMPLE_ACTION_H
