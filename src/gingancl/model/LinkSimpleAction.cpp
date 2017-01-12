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

#include "LinkSimpleAction.h"
#include "LinkAssignmentAction.h"

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace ncl {
namespace model {
namespace link {
	LinkSimpleAction::LinkSimpleAction(
			FormatterEvent* event, short type) : LinkAction() {

		this->event = event;
		actionType  = type;
		listener    = NULL;

		typeSet.insert("LinkSimpleAction");
	}

	LinkSimpleAction::~LinkSimpleAction() {
		clog << "LinkSimpleAction::~LinkSimpleAction" << endl;
		if (listener != NULL) {
			listener->removeAction(this);
		}

		listener = NULL;
		event    = NULL;
	}

	FormatterEvent* LinkSimpleAction::getEvent() {
		return event;
	}

	short LinkSimpleAction::getType() {
		return actionType;
	}

	void LinkSimpleAction::setSimpleActionListener(
		    ILinkActionListener* listener) {

		if (listener != NULL) {
			listener->addAction(this);
		}

		this->listener = listener;
	}

	vector<FormatterEvent*>* LinkSimpleAction::getEvents() {
		if (event == NULL)
			return NULL;

		vector<FormatterEvent*>* events;
		events = new vector<FormatterEvent*>;

		events->push_back(event);
		return events;
	}

	vector<LinkAction*>* LinkSimpleAction::getImplicitRefRoleActions() {
		vector<LinkAction*>* actions;
		string attVal = "", durVal = "", byVal = "";
		Animation* anim;

		actions = new vector<LinkAction*>;

		if (this->instanceOf("LinkAssignmentAction")) {
			attVal = ((LinkAssignmentAction*)this)->getValue();
			anim   = ((LinkAssignmentAction*)this)->getAnimation();

			if (anim != NULL) {
				durVal = anim->getDuration();
				byVal  = anim->getBy();
			}

			if ((byVal != "" && byVal.substr(0, 1) == "$") ||
					(durVal != "" && durVal.substr(0, 1) == "$") ||
					(attVal != "" && attVal.substr(0, 1) == "$")) {

				if (event->instanceOf("AttributionEvent")) {
					actions->push_back((LinkAction*)this);
				}
			}
		}

		if (actions->empty()) {
			delete actions;
			return NULL;
		}

		return actions;
	}

	void LinkSimpleAction::run() {
		LinkAction::run();

		if (listener != NULL) {
			listener->scheduleAction(satisfiedCondition, (void*)this);
		}

		if (actionType == SimpleAction::ACT_START) {
			/*clog << "LinkSimpleAction::run notify action INIT ";
			if (event != NULL) {
				clog << "'" << event->getId() << "'";
			}
			clog << endl;*/
			notifyProgressionListeners(true);

		} else {
			/*clog << "LinkSimpleAction::run notify action END ";
			if (event != NULL) {
				clog << "'" << event->getId() << "'";
			}
			clog << endl;*/
			notifyProgressionListeners(false);
		}
	}

}
}
}
}
}
}
}