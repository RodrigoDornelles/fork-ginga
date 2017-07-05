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

#ifndef _DESCRIPTOR_H_
#define _DESCRIPTOR_H_

#include "Entity.h"
#include "Parameter.h"

#include "LayoutRegion.h"

#include "FocusDecoration.h"
#include "KeyNavigation.h"

#include "Transition.h"

#include "Descriptor.h"

GINGA_NCL_BEGIN

class Descriptor : public Entity
{
public:
  static const short FIT_FILL = 0;
  static const short FIT_HIDDEN = 1;
  static const short FIT_MEET = 2;
  static const short FIT_MEETBEST = 3;
  static const short FIT_SLICE = 4;

  static const short SCROLL_NONE = 0;
  static const short SCROLL_HORIZONTAL = 1;
  static const short SCROLL_VERTICAL = 2;
  static const short SCROLL_BOTH = 3;
  static const short SCROLL_AUTOMATIC = 4;

  Descriptor (const string &_id);
  virtual ~Descriptor ();
  GingaTime getExplicitDuration ();

  LayoutRegion *getRegion ();
  int getRepetitions ();
  void setExplicitDuration (GingaTime);
  void setPlayerName (const string &name);
  void setRegion (LayoutRegion *_region);
  void setRepetitions (int r);
  void addParameter (Parameter *parameter);
  vector<Parameter *> *getParameters ();
  Parameter *getParameter (const string &paramName);
  void removeParameter (Parameter *parameter);
  KeyNavigation *getKeyNavigation ();
  void setKeyNavigation (KeyNavigation *keyNav);
  FocusDecoration *getFocusDecoration ();
  void setFocusDecoration (FocusDecoration *focusDec);
  vector<Transition *> *getInputTransitions ();
  bool addInputTransition (Transition *transition, int pos);
  void removeInputTransition (Transition *transition);
  void removeAllInputTransitions ();
  vector<Transition *> *getOutputTransitions ();
  bool addOutputTransition (Transition *transition, int pos);
  void removeOutputTransition (Transition *transition);
  void removeAllOutputTransitions ();

protected:
  GingaTime _explicitDuration;
  string _presentationTool;
  int _repetitions;
  LayoutRegion *_region;
  map<string, Parameter *> _parameters;

  KeyNavigation *_keyNavigation;
  FocusDecoration *_focusDecoration;
  vector<Transition *> _inputTransitions;
  vector<Transition *> _outputTransitions;
};

GINGA_NCL_END

#endif //_DESCRIPTOR_H_
