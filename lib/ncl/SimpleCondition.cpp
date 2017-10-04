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
#include "SimpleCondition.h"

GINGA_NCL_BEGIN


// Public.

/**
 * @brief Creates a new simple condition.
 * @param type Event type.
 * @param transition Condition type.
 * @param label Role.
 * @param conj True if multiple binds are to be interpreted as conjunctions.
 * @param delay Delay.
 * @param key Key.
 */
SimpleCondition::SimpleCondition (EventType type,
                                  EventStateTransition transition,
                                  const string &label,
                                  bool conj,
                                  const string &delay,
                                  const string &key)
  : TriggerExpression (), Role (type, label)
{
  _transition = transition;
  _conjunction = conj;
  _delay = delay;
  _key = key;
}

/**
 * @brief Gets transition.
 * @return Transition value.
 */
EventStateTransition
SimpleCondition::getTransition ()
{
  return _transition;
}

/**
 * @brief Tests whether simple condition is a conjunction.
 * @return True if successful, or false otherwise.
 */
bool
SimpleCondition::isConjunction ()
{
  return _conjunction;
}

/**
 * @brief Gets delay.
 * @return Delay value.
 */
string
SimpleCondition::getDelay ()
{
  return _delay;
}

/**
 * @brief Gets key.
 * @return Key value.
 */
string
SimpleCondition::getKey ()
{
  return _key;
}


// Public: Static.

/**
 * @brief Tests whether role is a reserved condition role.
 * @param type Address of variable to store role type.
 * @param trans Address of variable to store role transition.
 * @return True if successful, or false otherwise.
 */
bool
SimpleCondition::isReserved (const string &role,
                             EventType *type,
                             EventStateTransition *trans)
{
  static map<string, pair<int,int>> reserved =
    {
     {"onBegin",
      {(int) EventType::PRESENTATION,
       (int) EventStateTransition::START}},
     {"onEnd",
      {(int) EventType::PRESENTATION,
       (int) EventStateTransition::STOP}},
     {"onAbort",
      {(int) EventType::PRESENTATION,
       (int) EventStateTransition::ABORT}},
     {"onPause",
      {(int) EventType::PRESENTATION,
       (int) EventStateTransition::PAUSE}},
     {"onResumes",
      {(int) EventType::PRESENTATION,
       (int) EventStateTransition::RESUME}},
     {"onBeginAttribution",
      {(int) EventType::ATTRIBUTION,
       (int) EventStateTransition::START}},
     {"onEndAttribution",
      {(int) EventType::SELECTION,
       (int) EventStateTransition::STOP}},
     {"onSelection",
      {(int) EventType::SELECTION,
       (int) EventStateTransition::START}},
    };
  map<string, pair<int,int>>::iterator it;
  if ((it = reserved.find (role)) == reserved.end ())
    return false;
  tryset (type, (EventType) it->second.first);
  tryset (trans, (EventStateTransition) it->second.second);
  return true;
}

GINGA_NCL_END
