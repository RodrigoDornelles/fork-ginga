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

#ifndef EXECUTION_OBJECT_CONTEXT_H
#define EXECUTION_OBJECT_CONTEXT_H

#include "ExecutionObject.h"
#include "NclEvents.h"
#include "NclFormatterLink.h"
#include "NclLinkListener.h"
#include "NclNodeNesting.h"

#include "ncl/Ncl.h"
using namespace ::ginga::ncl;

GINGA_FORMATTER_BEGIN

class ExecutionObjectContext : public ExecutionObject,
    public NclLinkListener,
    public INclEventListener
{
public:
  ExecutionObjectContext (const string &id,
                          Node *dataObject,
                          INclActionListener *seListener);

  virtual ~ExecutionObjectContext ();

  ExecutionObjectContext *getParentFromDataObject (Node *dataObject);
  void suspendLinkEvaluation (bool suspend);
  bool addExecutionObject (ExecutionObject *execObj);
  bool containsExecutionObject (const string &execObjId);
  ExecutionObject *getExecutionObject (const string &execObjId);
  map<string, ExecutionObject *> *getExecutionObjects ();
  bool removeExecutionObject (ExecutionObject *execObj);
  set<Link *> *getUncompiledLinks ();
  bool containsUncompiledLink (Link *dataLink);
  void removeLinkUncompiled (Link *ncmLink);
  void setLinkCompiled (NclFormatterLink *formatterLink);
  void setParentsAsListeners ();
  void unsetParentsAsListeners () override;
  void eventStateChanged (NclEvent *event,
                          EventStateTransition transition,
                          EventState previousState) override;

  void linkEvaluationStarted (NclFormatterLink *link) override;
  void linkEvaluationFinished (NclFormatterLink *, bool) override;

private:
  static const short _mSleepTime = 800;
  set<NclFormatterLink *> _links;
  set<Link *> _uncompiledLinks;

  set<NclEvent *> _runningEvents; // child events occurring
  set<NclEvent *> _pausedEvents;  // child events paused
  EventStateTransition lastTransition;

  map<NclFormatterLink *, int> _pendingLinks;

  map<string, ExecutionObject *> _execObjList;

  void checkLinkConditions ();
};

GINGA_FORMATTER_END

#endif // EXECUTION_OBJECT_CONTEXT_H
