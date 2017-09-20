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
#include "ExecutionObjectContext.h"

GINGA_FORMATTER_BEGIN

ExecutionObjectContext::ExecutionObjectContext (GingaState *ginga,
                                                const string &id,
                                                Node *dataObject,
                                                INclActionListener *seListener)
  : ExecutionObject (ginga, id, dataObject, seListener)
{
  Context *context;
  Entity *entity;

  _execObjList.clear ();
  _links.clear ();
  _uncompiledLinks.clear ();
  _runningEvents.clear ();
  _pausedEvents.clear ();
  _pendingLinks.clear ();

  entity = cast (Entity *, dataObject);
  g_assert_nonnull (entity);

  if (!instanceof (Context *, entity))
    return;                     // switch, nothing to do

  context = cast (Context *, entity);
  g_assert_nonnull (context);

  g_assert_nonnull (context->getLinks ());
  for (auto link: *context->getLinks ())
    _uncompiledLinks.insert (link);
}

ExecutionObjectContext::~ExecutionObjectContext ()
{
  // ExecutionObject *object;
  NclFormatterLink *link;
  set<NclFormatterLink *>::iterator i;
  map<string, ExecutionObject *>::iterator j;

  _runningEvents.clear ();
  _pausedEvents.clear ();
  _pendingLinks.clear ();

  i = _links.begin ();
  while (i != _links.end ())
    {
      link = *i;
      if (link != NULL)
        {
          delete link;
          link = NULL;
        }
      ++i;
    }
  _links.clear ();
  _uncompiledLinks.clear ();
  _execObjList.clear ();
}

ExecutionObjectContext *
ExecutionObjectContext::getParentFromDataObject (Node *dataObject)
{
  ExecutionObject *object;
  Node *parentDataObject;
  map<string, ExecutionObject *>::iterator i;

  parentDataObject = (Node *)(dataObject->getParent ());

  if (parentDataObject != NULL)
    {
      i = _execObjList.begin ();
      while (i != _execObjList.end ())
        {
          object = i->second;
          if (object->getNode () == parentDataObject)
            {
              return (ExecutionObjectContext *)object;
            }
          ++i;
        }
    }
  return NULL;
}

void
ExecutionObjectContext::suspendLinkEvaluation (bool suspend)
{
  for (NclFormatterLink *link : _links)
    {
      link->suspendLinkEvaluation (suspend);
    }
}

bool
ExecutionObjectContext::addExecutionObject (ExecutionObject *obj)
{
  string objId;

  if (obj == NULL)
    {
      return false;
    }

  objId = obj->getId ();
  if (_execObjList.count (objId) != 0)
    {
      WARNING ("Trying to add the same obj twice: '%s'.", objId.c_str ());
      return false;
    }

  _execObjList[objId] = obj;

  obj->addParentObject (this, getNode ());
  return true;
}

bool
ExecutionObjectContext::containsExecutionObject (const string &execObjId)
{
  if (getExecutionObject (execObjId) != NULL)
    {
      return true;
    }
  else
    {
      return false;
    }
}

ExecutionObject *
ExecutionObjectContext::getExecutionObject (const string &id)
{
  map<string, ExecutionObject *>::iterator i;
  ExecutionObject *execObj;

  if (_execObjList.empty ())
    {
      return NULL;
    }

  i = _execObjList.find (id);
  if (i != _execObjList.end ())
    {
      execObj = i->second;
      return execObj;
    }

  return NULL;
}

map<string, ExecutionObject *> *
ExecutionObjectContext::getExecutionObjects ()
{
  return &_execObjList;
}

bool
ExecutionObjectContext::removeExecutionObject (ExecutionObject *obj)
{
  map<string, ExecutionObject *>::iterator i;

  i = _execObjList.find (obj->getId ());
  if (i != _execObjList.end ())
    {
      _execObjList.erase (i);
      return true;
    }
  return false;
}

set<Link *> *
ExecutionObjectContext::getUncompiledLinks ()
{
  clog << "ExecutionObjectContext::getUncompiledLinks '" << getId ();
  clog << "' has '" << _uncompiledLinks.size () << "' uncompiled links";
  clog << endl;

  set<Link *> *uLinks = new set<Link *> (_uncompiledLinks);
  return uLinks;
}

bool
ExecutionObjectContext::containsUncompiledLink (Link *dataLink)
{
  if (_uncompiledLinks.count (dataLink) != 0)
    return true;
  return false;
}

void
ExecutionObjectContext::removeLinkUncompiled (Link *ncmLink)
{
  set<Link *>::iterator i;

  clog << "ExecutionObjectContext::removeLinkUncompiled '";
  clog << ncmLink->getId () << "'" << endl;
  i = _uncompiledLinks.find (ncmLink);
  if (i != _uncompiledLinks.end ())
    {
      _uncompiledLinks.erase (i);
      return;
    }
}

void
ExecutionObjectContext::setLinkCompiled (NclFormatterLink *link)
{
  if (link == NULL)
    {
      clog << "ExecutionObjectContext::setLinkCompiled Warning! ";
      clog << "trying to compile a NULL link" << endl;
      return;
    }

  Link *compiledLink;
  compiledLink = link->getNcmLink ();

  if (compiledLink == NULL)
    {
      clog << "ExecutionObjectContext::setLinkCompiled Warning! ";
      clog << "formatterLink has returned a NULL ncmLink" << endl;
      return;
    }

  _links.insert (link);
}

void
ExecutionObjectContext::setParentsAsListeners ()
{
  map<Node *, ExecutionObjectContext *>::iterator i;

  i = _parentTable.begin ();
  while (i != _parentTable.end ())
    {
      _wholeContent->addListener (i->second);
      ++i;
    }
}

void
ExecutionObjectContext::eventStateChanged (
    NclEvent *event,
    EventStateTransition transition,
    EventState previousState)
{
  set<NclEvent *>::iterator i;

  if (!instanceof (PresentationEvent *, event))
    return;

  switch (transition)
    {
    case EventStateTransition::STARTS:
      if (_runningEvents.empty () && _pausedEvents.empty ())
        {
          setParentsAsListeners ();
          _wholeContent->start ();
        }

      _runningEvents.insert (event);
      break;

    case EventStateTransition::ABORTS:
      lastTransition = transition;
      if (previousState == EventState::OCCURRING)
        {
          i = _runningEvents.find (event);
          if (i != _runningEvents.end ())
            {
              _runningEvents.erase (i);
            }
        }
      else if (previousState == EventState::PAUSED)
        {
          i = _pausedEvents.find (event);
          if (i != _pausedEvents.end ())
            {
              _pausedEvents.erase (i);
            }
        }

      if (_runningEvents.empty () && _pausedEvents.empty ()
          && _pendingLinks.empty ())
        {
          _wholeContent->abort ();
        }
      break;

    case EventStateTransition::STOPS:
      if (((PresentationEvent *)event)->getRepetitions () == 0)
        {
          lastTransition = transition;
          if (previousState == EventState::OCCURRING)
            {
              i = _runningEvents.find (event);
              if (i != _runningEvents.end ())
                {
                  _runningEvents.erase (i);
                }
            }
          else if (previousState == EventState::PAUSED)
            {
              i = _pausedEvents.find (event);
              if (i != _pausedEvents.end ())
                {
                  _pausedEvents.erase (i);
                }
            }

          if (_runningEvents.empty () && _pausedEvents.empty ()
              && _pendingLinks.empty ())
            {
              checkLinkConditions ();
            }
        }
      break;

    case EventStateTransition::PAUSES:
      i = _runningEvents.find (event);
      if (i != _runningEvents.end ())
        {
          _runningEvents.erase (i);
        }

      _pausedEvents.insert (event);
      if (_runningEvents.empty ())
        {
          _wholeContent->pause ();
        }
      break;

    case EventStateTransition::RESUMES:
      i = _pausedEvents.find (event);
      if (i != _pausedEvents.end ())
        {
          _pausedEvents.erase (i);
        }

      _runningEvents.insert (event);
      if (_runningEvents.size () == 1)
        {
          _wholeContent->resume ();
        }
      break;

    default:
      g_assert_not_reached ();
    }
}

void
ExecutionObjectContext::linkEvaluationStarted (NclFormatterLink *link)
{
  int linkNumber = 0;
  NclFormatterLink *evalLink;

  evalLink = link;
  if (_pendingLinks.count (evalLink) != 0)
    {
      linkNumber = _pendingLinks[evalLink];
    }
  _pendingLinks[evalLink] = linkNumber + 1;
}

void
ExecutionObjectContext::linkEvaluationFinished (
    NclFormatterLink *link, bool start)
{
  int linkNumber;
  NclFormatterLink *finishedLink;
  map<NclFormatterLink *, int>::iterator i;

  finishedLink = link;
  i = _pendingLinks.find (finishedLink);
  if (i != _pendingLinks.end ())
    {
      linkNumber = i->second;
      if (linkNumber == 1)
        {
          _pendingLinks.erase (i);
          if (_runningEvents.empty () && _pausedEvents.empty ()
              && _pendingLinks.empty ())
            {
              if (lastTransition == EventStateTransition::STOPS)
                {
                  checkLinkConditions ();
                }
              else if (!start)
                {
                  _wholeContent->abort ();
                }
            }
        }
      else
        {
          _pendingLinks[finishedLink] = linkNumber - 1;
        }
    }
}

void
ExecutionObjectContext::checkLinkConditions ()
{
  if ((_runningEvents.empty () && _pausedEvents.empty ()
       && _pendingLinks.empty ()))
    {
      if (_wholeContent != NULL)
        {
          _wholeContent->stop ();
          if (this->getParentObject () == nullptr)
            TRACE ("*** ALL DONE ***");
        }
    }
}


GINGA_FORMATTER_END
