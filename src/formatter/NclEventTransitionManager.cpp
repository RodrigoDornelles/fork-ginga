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
#include "NclEventTransitionManager.h"

GINGA_FORMATTER_BEGIN

NclEventTransitionManager::NclEventTransitionManager ()
{
}

NclEventTransitionManager::~NclEventTransitionManager ()
{
  vector<NclEventTransition *>::iterator i;

  i = transTable.begin ();
  while (i != transTable.end ())
    delete *i;

  transTable.clear ();
}

vector<NclEventTransition *> *
NclEventTransitionManager::getTransitionEvents ()
{
  return &this->transTable;
}

void
NclEventTransitionManager::addEventTransition (NclEventTransition *transition)
{
  size_t beg, end, pos;
  NclEventTransition *auxTransition;
  vector<NclEventTransition *> *transitionEvents = &this->transTable;

  // binary search
  beg = 0;
  if (transitionEvents->size () == 0)
    goto done;
  end = transitionEvents->size () - 1;
  while (beg <= end)
    {
      pos = (beg + end) / 2;
      auxTransition = (*transitionEvents)[pos];
      switch (transition->compareTo (auxTransition))
        {
        case 0:
          return;
        case -1:
          end = pos - 1;
          break;
        case 1:
          beg = pos + 1;
          break;
        default:
          g_assert_not_reached ();
        }
    }
 done:
  transitionEvents->insert (transitionEvents->begin () + (int) beg, transition);
}

void
NclEventTransitionManager::removeEventTransition (
    NclPresentationEvent *event)
{
  size_t i, size;
  vector<NclEventTransition *>::iterator j;
  NclEventTransition *transition;
  NclEventTransition *endTransition;
  vector<NclEventTransition *> *transitionEvents;

  transitionEvents = getTransitionEvents ();
  if (transitionEvents == NULL)
    return;                     // nothing to do

  size = transitionEvents->size ();
  for (i = 0; i < size; i++)
    {
      transition = (*transitionEvents)[i];
      if (transition->getEvent () == event)
        {
          if (transition->instanceOf ("NclBeginEventTransition")
              && ((NclBeginEventTransition *)transition)
                         ->getEndTransition ()
                     != NULL)
            {
              endTransition = ((NclBeginEventTransition *)transition)
                                  ->getEndTransition ();

              for (j = transitionEvents->begin ();
                   j != transitionEvents->end (); ++j)
                {
                  if (*j == endTransition)
                    {
                      transitionEvents->erase (j);
                      size = transitionEvents->size ();
                      i = 0;
                      break;
                    }
                }
            }

          for (j = transitionEvents->begin ();
               j != transitionEvents->end (); ++j)
            {
              if (*j == transition)
                {
                  transitionEvents->erase (j);
                  size = transitionEvents->size ();
                  i = 0;
                  break;
                }
            }
        }
    }
}

void
NclEventTransitionManager::resetTimeIndex ()
{
  currentTransitionIndex = startTransitionIndex;
}

void
NclEventTransitionManager::prepare (bool wholeContent, GingaTime startTime)
{
  vector<NclEventTransition *> *transitionEvents;
  NclEventTransition *transition;
  size_t transIx, size;

  if (wholeContent && startTime == 0)
    {
      startTransitionIndex = 0;
    }
  else
    {
      transitionEvents = getTransitionEvents ();
      size = transitionEvents->size ();
      transIx = 0;
      startTransitionIndex = transIx;
      while (transIx < size)
        {
          transition = (*transitionEvents)[transIx];
          if (transition->getTime () >= startTime)
            {
              break;
            }

          if (transition->instanceOf ("NclBeginEventTransition"))
            {
              transition->getEvent ()->setState (
                  EventState::OCCURRING);
            }
          else
            {
              clog << "NclEventTransitionManager::prepare set '";
              clog << transition->getEvent ()->getId ();
              clog << "' to SLEEP" << endl;

              transition->getEvent ()->setState (
                  EventState::SLEEPING);

              transition->getEvent ()->incOccurrences ();
            }
          transIx++;
          startTransitionIndex = transIx;
        }
    }

  resetTimeIndex ();
}

void
NclEventTransitionManager::start (GingaTime offsetTime)
{
  vector<NclEventTransition *> *transitionEvents;
  NclEventTransition *transition;
  size_t transIx, size;

  transitionEvents = getTransitionEvents ();
  size = transitionEvents->size ();

  transIx = currentTransitionIndex;

  while (transIx < size)
    {
      transition = (*transitionEvents)[transIx];
      if (transition->getTime () <= offsetTime)
        {
          if (transition->instanceOf ("NclBeginEventTransition"))
            {
              transition->getEvent ()->start ();
            }
          transIx++;
          currentTransitionIndex = transIx;
        }
      else
        {
          break;
        }
    }
}

void
NclEventTransitionManager::stop (GingaTime endTime, bool applicationType)
{
  vector<NclEventTransition *> *transitionEvents;
  vector<NclEventTransition *>::iterator i;
  NclEventTransition *transition;
  NclFormatterEvent *fev;

  transitionEvents = getTransitionEvents ();

  i = transitionEvents->begin ();
  while (i != transitionEvents->end ())
    {
      transition = *i;
      if (!applicationType
          || (applicationType && GINGA_TIME_IS_VALID (transition->getTime ())))
        {
          fev = transition->getEvent ();
          if (!GINGA_TIME_IS_VALID (endTime) || transition->getTime () > endTime)
            {
              fev->setState (EventState::SLEEPING);
            }
          else if (transition->instanceOf ("NclEndEventTransition"))
            {
              fev->stop ();
            }
        }
      ++i;
    }
}

void
NclEventTransitionManager::abort (GingaTime endTime, bool applicationType)
{
  vector<NclEventTransition *> *transitionEvents;
  size_t transIx, i, size;
  NclEventTransition *transition;
  NclFormatterEvent *fev;

  transIx = currentTransitionIndex;
  transitionEvents = getTransitionEvents ();
  size = transitionEvents->size ();

  for (i = transIx; i < size; i++)
    {
      transition = (*transitionEvents)[i];
      if (!applicationType
          || (applicationType && !isinf (transition->getTime ())))
        {
          fev = transition->getEvent ();
          if (transition->getTime () > endTime)
            {
              fev->setState (EventState::SLEEPING);
            }
          else if (transition->instanceOf ("NclEndEventTransition"))
            {
              fev->abort ();
            }
        }
    }
}

void
NclEventTransitionManager::addPresentationEvent (
    NclPresentationEvent *event)
{
  GingaTime begin, end;
  NclBeginEventTransition *beginTransition;
  NclEndEventTransition *endTransition;
  vector<NclEventTransition *> *transitionEvents;
  NclEventTransition *lastTransition = NULL;
  vector<NclEventTransition *>::iterator i;
  GingaTime lTime;

  transitionEvents = getTransitionEvents ();

  if ((event->getAnchor ())->instanceOf ("LambdaAnchor"))
    {
      beginTransition = new NclBeginEventTransition (0, event);
      transitionEvents->insert (transitionEvents->begin (),
                                beginTransition);
      if (event->getEnd () != GINGA_TIME_NONE)
        {
          endTransition = new NclEndEventTransition (
              event->getEnd (), event, beginTransition);

          i = transitionEvents->begin ();
          while (i != transitionEvents->end ())
            {
              lastTransition = *i;
              lTime = lastTransition->getTime ();
              if (!GINGA_TIME_IS_VALID (lTime)
                  || endTransition->getTime () < lTime)
                {
                  transitionEvents->insert (i, endTransition);
                  break;
                }

              ++i;

              if (i == transitionEvents->end ())
                {
                  transitionEvents->push_back (endTransition);
                  break;
                }
            }
        }
    }
  else
    {
      begin = event->getBegin ();
      beginTransition = new NclBeginEventTransition (begin, event);
      addEventTransition (beginTransition);
      end = event->getEnd ();

      endTransition
        = new NclEndEventTransition (end, event, beginTransition);
      addEventTransition (endTransition);
    }
}

void
NclEventTransitionManager::updateTransitionTable (
    GingaTime value, Player *player, NclFormatterEvent *mainEvent)
{
  NclEventTransition *transition;
  NclFormatterEvent *ev;
  vector<NclEventTransition *> *transitionEvents;
  size_t currentIx;

  transitionEvents = &this->transTable;
  currentIx = currentTransitionIndex;

  while (currentIx < transitionEvents->size ())
    {
      transition = (*transitionEvents)[currentIx];

      if (transition->getTime () <= value)
        {
          ev = transition->getEvent ();
          if (transition->instanceOf ("NclBeginEventTransition"))
            {
              clog << "NclEventTransitionManager::updateTransitionTable ";
              clog << "starting event '" << ev->getId () << "' ";
              clog << "current state '" << (int) ev->getCurrentState ();
              clog << "'" << endl;

              ev->start ();
            }
          else
            {
              if (ev == mainEvent && player != NULL)
                {
                  player->stop ();
                }

              clog << "NclEventTransitionManager::updateTransitionTable ";
              clog << "stopping event '" << ev->getId () << "' ";
              clog << "current state '" << (int) ev->getCurrentState ();
              clog << "'" << endl;

              ev->stop ();
            }

          currentIx++;
          currentTransitionIndex = currentIx;
        }
      else
        {
          break;
        }
    }
}

set<GingaTime> *
NclEventTransitionManager::getTransitionsValues ()
{
  set<GingaTime> *transValues;
  size_t currentIx, ix;
  vector<NclEventTransition *> *transitionEvents;
  vector<NclEventTransition *>::iterator i;

  currentTransitionIndex = startTransitionIndex;
  transitionEvents = getTransitionEvents ();
  transValues = new set<GingaTime>;

  currentIx = currentTransitionIndex;

  ix = 0;
  i = transitionEvents->begin ();
  while (i != transitionEvents->end ())
    {
      if (ix >= currentIx)
        {
          transValues->insert ((*i)->getTime ());
        }
      ++ix;
      ++i;
    }

  return transValues;
}

NclEventTransition *
NclEventTransitionManager::getNextTransition (NclFormatterEvent *mainEvent)
{
  NclEventTransition *transition;
  vector<NclEventTransition *> *transitionEvents;
  size_t currentIx;
  GingaTime transTime;
  GingaTime eventEnd;

  transitionEvents = &transTable;
  currentIx = currentTransitionIndex;

  if (currentIx < transitionEvents->size ())
    {
      transition = transitionEvents->at (currentIx);

      eventEnd = ((NclPresentationEvent *)mainEvent)->getEnd ();
      transTime = transition->getTime ();

      if (!GINGA_TIME_IS_VALID (eventEnd)
          || transTime <= eventEnd)
        {
          return transition;
        }
    }

  return NULL;
}

GINGA_FORMATTER_END
