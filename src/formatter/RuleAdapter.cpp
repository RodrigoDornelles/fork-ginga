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
#include "RuleAdapter.h"

GINGA_FORMATTER_BEGIN

RuleAdapter::RuleAdapter (Settings *settings)
{
  this->_settings = settings;
}

RuleAdapter::~RuleAdapter ()
{
  reset ();
}

void
RuleAdapter::reset ()
{
  for (auto i : _ruleListenMap)
    {
      vector<Rule *> *rules = i.second;
      if (rules != nullptr)
        {
          delete rules;
        }
    }
  _ruleListenMap.clear ();

  map<Rule *, vector<ExecutionObjectSwitch *> *>::iterator j;
  vector<ExecutionObjectSwitch *> *objects;

  for (auto i : _entityListenMap)
    {
      objects = i.second;
      if (objects != nullptr)
        {
          delete objects;
        }

    }
  _entityListenMap.clear ();
}

Settings *
RuleAdapter::getSettings ()
{
  return this->_settings;
}

void
RuleAdapter::adapt (ExecutionObjectContext *compositeObject, bool force)
{
  map<string, ExecutionObject *> *objs =
      compositeObject->getExecutionObjects ();

  if (objs != nullptr)
    {
      for (auto i : *objs)
        {
          ExecutionObject *obj = i.second;
          if (instanceof (ExecutionObjectSwitch *, obj))
            {
              obj = ((ExecutionObjectSwitch *)obj)->getSelectedObject ();
            }

          if (instanceof (ExecutionObjectContext *, obj))
            {
              adapt ((ExecutionObjectContext *)obj, force);
            }
        }
      delete objs;
    }
}

void
RuleAdapter::initializeAttributeRuleRelation (Rule *topRule, Rule *rule)
{
  vector<Rule *> *ruleVector = nullptr;
  vector<Rule *>::iterator rules;

  SimpleRule *simpleRule = cast (SimpleRule *, rule);
  CompositeRule *compositeRule = cast (CompositeRule *, rule);
  if (simpleRule)
    {
      for (auto i : _ruleListenMap)
        {
          if (simpleRule->getAttribute () == i.first)
            {
              ruleVector = i.second;
              break;
            }
        }

      if (ruleVector == nullptr)
        {
          ruleVector = new vector<Rule *>;
          _ruleListenMap[simpleRule->getAttribute ()] = ruleVector;
        }
      ruleVector->push_back (topRule);
    }
  else
    {
      const vector<Rule *> *vec = compositeRule->getRules ();
      for (auto rule: *vec)
        {
          initializeAttributeRuleRelation (topRule, rule);
          ++rules;
        }
    }
}

Node *
RuleAdapter::adaptSwitch (Switch *swtch)
{
  const vector<Node *> *nodes = swtch->getNodes ();
  const vector<Rule *> *rules = swtch->getRules ();

  for (size_t i = 0; i < rules->size (); i++)
    if (evaluateRule (rules->at (i)))
      return nodes->at (i);

  return swtch->getDefaultNode ();
}

bool
RuleAdapter::evaluateRule (Rule *rule)
{
  if (instanceof (SimpleRule *, rule))
    {
      return evaluateSimpleRule ((SimpleRule *)rule);
    }
  else if (instanceof (CompositeRule *, rule))
    {
      return evaluateCompositeRule ((CompositeRule *)rule);
    }
  else
    {
      return false;
    }
}

bool
RuleAdapter::evaluateCompositeRule (CompositeRule *rule)
{
  if (rule->isConjunction ())
    {
      for (auto child: *rule->getRules ())
        if (!evaluateRule (child))
          return false;
      return true;
    }
  else
    {
      for (auto child: *rule->getRules ())
        if (evaluateRule (child))
          return true;
      return false;
    }
  g_assert_not_reached ();
}

bool
RuleAdapter::evaluateSimpleRule (SimpleRule *rule)
{
  string attr = rule->getAttribute ();
  string ruleValue = rule->getValue ();
  string attrValue = _settings->get (attr);
  string op = rule->getOperator ();

  return ginga_eval_comparator (op, attrValue, ruleValue);
}

GINGA_FORMATTER_END
