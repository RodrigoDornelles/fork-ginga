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

#ifndef LINK_H
#define LINK_H

#include "Connector.h"
#include "Bind.h"

GINGA_NCL_BEGIN

class Context;
class Link : public Entity
{
public:
  Link (NclDocument *, const string &, Context *, Connector *);
  virtual ~Link ();

  Connector *getConnector ();
  Context *getContext ();

  void addParameter (Parameter *);
  const vector<Parameter *> *getParameters ();
  Parameter *getParameter (const string &);

  void addBind (Bind *);
  const vector<Bind *> *getBinds ();
  vector<Bind *> getBinds (Role *);
  bool contains (Node *, bool);

private:
  Context *_context;
  Connector *_connector;
  vector<Bind *> _binds;
  vector<Parameter *> _parameters;
};

GINGA_NCL_END

#endif // LINK_H
