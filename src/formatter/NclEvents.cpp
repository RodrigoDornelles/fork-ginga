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
#include "NclEvents.h"

#include "ExecutionObjectContext.h"

GINGA_FORMATTER_BEGIN

set<NclEvent *> NclEvent::_instances;

NclEvent::NclEvent (const string &id, ExecutionObject *exeObj)
{
  this->_id = id;
  _state = EventState::SLEEPING;
  _occurrences = 0;
  _exeObj = exeObj;
  _type = EventType::UNKNOWN;

  _instances.insert (this);
}

NclEvent::~NclEvent ()
{
  _instances.erase (this);
  _listeners.clear ();
}

bool
NclEvent::hasInstance (NclEvent *evt, bool remove)
{
  bool has = _instances.find(evt) != _instances.end();

  if (has && remove)
    {
      _instances.erase (evt);
    }
  return has;
}

bool
NclEvent::hasNcmId (NclEvent *evt, const string &anchorId)
{
  Anchor *anchor;
  string anchorName = " ";

  if (auto anchorEvt = cast (AnchorEvent *, evt))
    {
      anchor = anchorEvt->getAnchor ();
      if (anchor != nullptr)
        {
          if (instanceof (IntervalAnchor *, anchor))
            {
              anchorName = anchor->getId ();
            }
          else if (auto labeledAnchor = cast (LabeledAnchor *, anchor))
            {
              anchorName = labeledAnchor->getLabel ();
            }
          else if (instanceof (LambdaAnchor *, anchor))
            {
              anchorName = "";
            }

          if (anchorName == anchorId
              && !(instanceof (SelectionEvent *, evt)))
            {
              return true;
            }
        }
    }
  else if (auto attrEvt = cast (AttributionEvent *, evt))
    {
      anchor = attrEvt->getAnchor ();
      if (anchor != nullptr)
        {
          auto propAnchor = cast (PropertyAnchor *, anchor);
          g_assert_nonnull (propAnchor);
          anchorName = propAnchor->getName ();
          if (anchorName == anchorId)
            {
              return true;
            }
        }
    }

  return false;
}

void
NclEvent::addListener (INclEventListener *listener)
{
  this->_listeners.insert (listener);
}

void
NclEvent::removeListener (INclEventListener *listener)
{
  _listeners.erase (listener);
}

EventStateTransition
NclEvent::getTransition (EventState newState)
{
  return EventUtil::getTransition (_state, newState);
}

bool
NclEvent::abort ()
{
  if (_state == EventState::OCCURRING || _state == EventState::PAUSED)
    return changeState (EventState::SLEEPING, EventStateTransition::ABORTS);
  else
    return false;
}

bool
NclEvent::start ()
{
  if (_state == EventState::SLEEPING)
    return changeState (EventState::OCCURRING, EventStateTransition::STARTS);
  else
    return false;
}

bool
NclEvent::stop ()
{
  if (_state == EventState::OCCURRING || _state == EventState::PAUSED)
    return changeState (EventState::SLEEPING, EventStateTransition::STOPS);
  else
    return false;
}

bool
NclEvent::pause ()
{
  if (_state == EventState::OCCURRING)
    return changeState (EventState::PAUSED, EventStateTransition::PAUSES);
  else
    return false;
}

bool
NclEvent::resume ()
{
  if (_state == EventState::PAUSED)
    return changeState (EventState::OCCURRING, EventStateTransition::RESUMES);
  else
    return false;
}

void
NclEvent::setState (EventState newState)
{
  _previousState = _state;
  _state = newState;
}

bool
NclEvent::changeState (EventState newState,
                       EventStateTransition transition)
{
  if (transition == EventStateTransition::STOPS)
    {
      _occurrences++;
    }

  _previousState = _state;
  _state = newState;

  set<INclEventListener *> clone (_listeners);

  for (INclEventListener *listener: clone)
    {
      listener->eventStateChanged (this, transition, _previousState);
    }

  return true;
}

// AnchorEvent
AnchorEvent::AnchorEvent (const string &id,
                          ExecutionObject *executionObject,
                          ContentAnchor *anchor)
  : NclEvent (id, executionObject)
{
  this->_anchor = anchor;
}

// PresentationEvent
PresentationEvent::PresentationEvent (const string &id,
                                      ExecutionObject *exeObj,
                                      ContentAnchor *anchor)
  : AnchorEvent (id, exeObj, anchor)
{
  _numPresentations = 1;
  _repetitionInterval = 0;

  auto intervalAnchor = cast (IntervalAnchor *, anchor);
  if (intervalAnchor)
    {
      _begin = intervalAnchor->getBegin ();
      _end = intervalAnchor->getEnd ();
    }
  else
    {
      _begin = 0;
      _end = GINGA_TIME_NONE;
    }
}

bool
PresentationEvent::stop ()
{
  if (_state == EventState::OCCURRING && _numPresentations > 1)
    {
      _numPresentations--;
    }

  return NclEvent::stop ();
}

GingaTime
PresentationEvent::getDuration ()
{
  if (!GINGA_TIME_IS_VALID (this->_end))
    return GINGA_TIME_NONE;
  return this->_end - this->_begin;
}

int
PresentationEvent::getRepetitions ()
{
  return (_numPresentations - 1);
}

void
PresentationEvent::setRepetitionSettings (int repetitions,
                                          GingaTime repetitionInterval)
{
  if (repetitions >= 0)
    {
      this->_numPresentations = repetitions + 1;
    }
  else
    {
      this->_numPresentations = 1;
    }

  this->_repetitionInterval = repetitionInterval;
}

void
PresentationEvent::incOccurrences ()
{
  _occurrences++;
}

// SelectionEvent
SelectionEvent::SelectionEvent (const string &id,
                                ExecutionObject *exeObj,
                                ContentAnchor *anchor)
  : AnchorEvent (id, exeObj, anchor)
{
  _selCode.assign("NO_CODE");
}

bool
SelectionEvent::start ()
{
  if (AnchorEvent::start ())
    return AnchorEvent::stop ();
  else
    return false;
}

// AttributionEvent
AttributionEvent::AttributionEvent (const string &id,
                                    ExecutionObject *exeObj,
                                    PropertyAnchor *anchor,
                                    Settings *settings)
  : NclEvent (id, exeObj)
{
  Entity *entity;
  NodeEntity *dataObject;

  this->_anchor = anchor;
  this->_player = nullptr;
  this->_settingsNode = false;
  this->_settings = settings;

  dataObject = cast (NodeEntity *, exeObj->getDataObject ());

  auto contentNode = cast (ContentNode *, dataObject);
  if (contentNode
      && contentNode->isSettingNode ())
    {
      _settingsNode = true;
    }

  auto referNode = cast (ReferNode *, dataObject);
  if (referNode)
    {
      if (referNode->getInstanceType () == "instSame")
        {
          entity = referNode->getDataEntity ();
          auto contentNode = cast (ContentNode *, entity);
          if (contentNode
              && contentNode->isSettingNode ())
            {
              _settingsNode = true;
            }
        }
    }
}

AttributionEvent::~AttributionEvent ()
{
  _assessments.clear ();
}

string
AttributionEvent::getCurrentValue ()
{
  string propName;
  string value = "";

  if (unlikely (_anchor == nullptr))
    {
      ERROR ("Trying to set a null property anchor of object '%s'.",
             _id.c_str ());
    }

  if (_settingsNode)
    {
      propName = _anchor->getName ();
      if (propName != "")
        {
          value = _settings->get (propName);
        }
    }
  else
    {
      if (_player)
        {
          value = _player->getProperty (this->getAnchor ()->getName ());
        }

      if (value == "")
        {
          value = _anchor->getValue ();
        }
    }

  return value;
}

bool
AttributionEvent::setValue (const string &newValue)
{
  if (_anchor->getValue () != newValue)
    {
      _anchor->setValue (newValue);
      return true;
    }
  return false;
}

void
AttributionEvent::setImplicitRefAssessmentEvent (
    const string &roleId, NclEvent *event)
{
  _assessments[roleId] = event;
}

NclEvent *
AttributionEvent::getImplicitRefAssessmentEvent (const string &id)
{
  return (_assessments.count (id) > 0) ? _assessments[id] : nullptr;
}

string
AttributionEvent::solveImplicitRefAssessment (const string &val)
{
  AttributionEvent *evt;

  if (val.substr (0, 1) != "$")
    return val;

  evt = cast (AttributionEvent *,
    this->getImplicitRefAssessmentEvent (val.substr (1, val.length ())));
  return (evt != nullptr) ? evt->getCurrentValue () : "";
}

// SwitchEvent
SwitchEvent::SwitchEvent (const string &id,
                          ExecutionObject *exeObjSwitch,
                          InterfacePoint *interface,
                          EventType type, const string &key)
  : NclEvent (id, exeObjSwitch)
{
  this->_interface = interface;
  this->_type = type;
  this->_key = key;
  this->_mappedEvent = nullptr;
}

SwitchEvent::~SwitchEvent ()
{
  if (NclEvent::hasInstance (_mappedEvent, false))
    {
      _mappedEvent->removeListener (this);
    }
}

void
SwitchEvent::setMappedEvent (NclEvent *evt)
{
  if (_mappedEvent != nullptr)
    {
      _mappedEvent->removeListener (this);
    }

  _mappedEvent = evt;
  if (_mappedEvent != nullptr)
    {
      _mappedEvent->addListener (this);
    }
}

void
SwitchEvent::eventStateChanged (
    arg_unused (NclEvent *evt),
    EventStateTransition trans,
    arg_unused (EventState prevState))
{
  changeState (EventUtil::getNextState (trans), trans);
}

EventTransition::EventTransition (GingaTime time,
                                  PresentationEvent *evt)
{
  this->_time = time;
  this->_evt = evt;
}

BeginEventTransition::BeginEventTransition (
    GingaTime t, PresentationEvent *evt)
  : EventTransition (t, evt)
{

}

EndEventTransition::EndEventTransition (GingaTime t,
                                        PresentationEvent *evt,
                                        BeginEventTransition *trans)
  : EventTransition (t, evt)
{
  _beginTrans = trans;
  _beginTrans->setEndTransition (this);
}

GINGA_FORMATTER_END
