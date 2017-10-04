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
#include "Connector.h"

#include "AssessmentStatement.h"
#include "CompoundAction.h"
#include "CompoundCondition.h"
#include "SimpleAction.h"
#include "SimpleCondition.h"

GINGA_NCL_BEGIN


// Public.

/**
 * @brief Creates a new connector.
 * @param ncl Parent document.
 * @param id Connector id.
 */
Connector::Connector (NclDocument *ncl, const string &id) : Entity (ncl, id)
{
  _condition = nullptr;
  _action = nullptr;
}

/**
 * Destroys connector.
 */
Connector::~Connector ()
{
  //_parameters.clear ();
  delete _condition;
  delete _action;
}

/**
 * @brief Gets connector condition.
 */
Condition *
Connector::getCondition ()
{
  return _condition;
}

/**
 * @brief Initializes connector condition.
 */
void
Connector::initCondition (Condition *condition)
{
  g_assert_nonnull (condition);
  g_assert_null (_condition);
  _condition = condition;
}

/**
 * @brief Gets connector action.
 */
Action *
Connector::getAction ()
{
  return _action;
}

/**
 * @brief Initializes connector action.
 */
void
Connector::initAction (Action *action)
{
  g_assert_nonnull (action);
  g_assert_null (_action);
  _action = action;
}

/**
 * @brief Gets connector role.
 * @param label Role label.
 * @return Connector role if successful, or null otherwise.
 */
Role *
Connector::getRole (const string &label)
{
  Role *role = this->searchRole (_condition, label);
  if (role != nullptr)
    return role;
  return this->searchRole (_action, label);
}


// Private.

Role *
Connector::searchRole (Condition *cond, const string &label)
{
  Role *role;

  g_assert (instanceof (Condition *, cond));

  if (instanceof (SimpleCondition *, cond)
      || instanceof (AttributeAssessment *, cond))
    {
      role = cast (Role *, cond);
      g_assert_nonnull (role);
      if (role->getLabel () == label)
        return role;
    }
  else if (instanceof (CompoundCondition *, cond))
    {
      CompoundCondition *parent = cast (CompoundCondition *, cond);
      g_assert_nonnull (parent);
      for (auto child: *parent->getConditions ())
        {
          role = this->searchRole (child, label);
          if (role != nullptr)
            return role;
        }
    }
  else if (instanceof (AssessmentStatement *, cond))
    {
      AssessmentStatement *parent = cast (AssessmentStatement *, cond);
      g_assert_nonnull (parent);

      role = cast (Role *, parent->getMainAssessment ());
      g_assert_nonnull (role);
      if (role->getLabel () == label)
        return role;

      Assessment *other = parent->getOtherAssessment ();
      if (instanceof (Role *, other))
        {
          role = cast (Role *, other);
          g_assert_nonnull (other);
          if (role->getLabel () == label)
            return role;
        }
    }
  else if (instanceof (CompoundStatement *, cond))
    {
      CompoundStatement *parent = cast (CompoundStatement *, cond);
      g_assert_nonnull (parent);
      for (auto child: *parent->getStatements ())
        {
          role = this->searchRole (child, label);
          if (role != nullptr)
            return role;
        }
    }
  else
    {
      g_assert_not_reached ();
    }

  return nullptr;               // not found
}

Role *
Connector::searchRole (Action *act, const string &label)
{
  Role *role;

  g_assert (instanceof (Action *, act));

  if (instanceof (SimpleAction *, act))
    {
      role = cast (Role *, act);
      g_assert_nonnull (role);
      if (role->getLabel () == label)
        return role;
    }
  else if (instanceof (CompoundAction *, act))
    {
      CompoundAction *parent = cast (CompoundAction *, act);
      for (auto child: *parent->getActions ())
        {
          role = this->searchRole (child, label);
          if (role != nullptr)
            return role;
        }
    }
  else
    {
      g_assert_not_reached ();
    }

  return nullptr;               // not found
}

GINGA_NCL_END
