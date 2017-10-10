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

#ifndef CONVERTER_H
#define CONVERTER_H

#include "ExecutionObject.h"
#include "ExecutionObjectContext.h"
#include "ExecutionObjectSettings.h"
#include "ExecutionObjectSwitch.h"
#include "GingaInternal.h"
#include "NclAction.h"
#include "NclCondition.h"
#include "NclEvents.h"
#include "NclLink.h"
#include "RuleAdapter.h"

#include "ncl/Ncl.h"
using namespace ::ginga::ncl;

class GingaInternal;

GINGA_FORMATTER_BEGIN

class Scheduler;
class Converter : public INclEventListener
{
public:
  explicit Converter (GingaInternal *, INclActionListener *, RuleAdapter *);
  virtual ~Converter ();

  void setHandlingStatus (bool handling);

  ExecutionObject *getExecutionObjectFromPerspective
  (NclNodeNesting *);

  NclEvent *getEvent (ExecutionObject *exeObj,
                      Anchor *interfacePoint,
                      EventType ncmEventType,
                      const string &key);

  ExecutionObject *
  processExecutionObjectSwitch (ExecutionObjectSwitch *switchObject);

  NclEvent *insertContext (NclNodeNesting *contextPerspective,
                           Port *port);

  RuleAdapter *getRuleAdapter ();


private:
  GingaInternal *_ginga;
  Scheduler *_scheduler;

  set<NclEvent *> _listening;
  INclActionListener *_actionListener;
  RuleAdapter *_ruleAdapter;
  bool _handling;

  void addExecutionObject (ExecutionObject *,
                           ExecutionObjectContext *);



  ExecutionObjectContext *
  addSameInstance (ExecutionObject *, Refer *);

  ExecutionObjectContext *getParentExecutionObject (NclNodeNesting *);

  ExecutionObject *
  createExecutionObject (const string &, NclNodeNesting *);

  void compileExecutionObjectLinks (ExecutionObject *, Node *,
                                    ExecutionObjectContext *);

  void processLink (Link *,
                    Node *,
                    ExecutionObject *,
                    ExecutionObjectContext *);

  void resolveSwitchEvents (ExecutionObjectSwitch *switchObject);

  NclEvent *insertNode (NclNodeNesting *perspective,
                        Anchor *interfacePoint);

  void eventStateChanged (NclEvent *someEvent,
                          EventStateTransition transition,
                          EventState previousState) override;

  static bool hasDescriptorPropName (const string &name);

  bool getBindKey (Bind *, string *);


  NclEvent *createEvent (Bind *, ExecutionObjectContext *);

  // INSANITY ABOVE --------------------------------------------------------

  ExecutionObject *obtainExecutionObject (const string &, Node *);
  NclLink *createLink (Link *, ExecutionObjectContext *);
  NclCondition *createCondition (Condition *, Bind *, ExecutionObjectContext *);
  NclAction *createAction (Action *, Bind *, ExecutionObjectContext *);
};

GINGA_FORMATTER_END

#endif /*CONVERTER_H_*/
