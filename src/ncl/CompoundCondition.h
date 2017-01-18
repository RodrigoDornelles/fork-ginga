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

#ifndef _COMPOUNDCONDITION_H_
#define _COMPOUNDCONDITION_H_

#include "util/functions.h"
using namespace ::ginga::util;

#include "AssessmentStatement.h"
#include "CompoundStatement.h"
#include "ConditionExpression.h"
#include "TriggerExpression.h"
#include "SimpleCondition.h"
#include "Role.h"

GINGA_NCL_BEGIN

class CompoundCondition : public TriggerExpression
{
public:
  static const short OP_AND = 0;
  static const short OP_OR = 1;

private:
  vector<ConditionExpression *> *expressions;
  short myOperator;

public:
  CompoundCondition ();
  CompoundCondition (ConditionExpression *c1, ConditionExpression *c2,
                     short op);

  virtual ~CompoundCondition ();
  void setOperator (short op);
  short getOperator ();
  vector<ConditionExpression *> *getConditions ();
  void addConditionExpression (ConditionExpression *condition);
  void removeConditionExpression (ConditionExpression *condition);
  vector<Role *> *getRoles ();
  bool
  instanceOf (string type)
  {
    return TriggerExpression::instanceOf (type);
  }
};

GINGA_NCL_END

#endif //_COMPOUNDCONDITION_H_
