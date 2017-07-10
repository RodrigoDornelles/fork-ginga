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
#include "AssessmentStatement.h"

GINGA_NCL_BEGIN

AssessmentStatement::AssessmentStatement (const string &comp) : Statement ()
{
  _comparator = comp;
  _mainAssessment = NULL;
  _otherAssessment = NULL;
}

AssessmentStatement::~AssessmentStatement ()
{
  if (_mainAssessment != NULL)
    delete _mainAssessment;
  if (_otherAssessment != NULL)
    delete _otherAssessment;
}

AttributeAssessment *
AssessmentStatement::getMainAssessment ()
{
  return _mainAssessment;
}

void
AssessmentStatement::setMainAssessment (AttributeAssessment *assessment)
{
  this->_mainAssessment = assessment;
}

Assessment *
AssessmentStatement::getOtherAssessment ()
{
  return _otherAssessment;
}

void
AssessmentStatement::setOtherAssessment (Assessment *assessment)
{
  this->_otherAssessment = assessment;
}

string
AssessmentStatement::getComparator ()
{
  return _comparator;
}

vector<Role *> *
AssessmentStatement::getRoles ()
{
  vector<Role *> *roles;

  roles = new vector<Role *>;
  roles->push_back (_mainAssessment);
  if (instanceof (AttributeAssessment *, _otherAssessment))
    {
      roles->push_back ((AttributeAssessment *)_otherAssessment);
    }
  return roles;
}

GINGA_NCL_END
