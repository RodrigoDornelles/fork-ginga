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

#include "ncl/navigation/KeyNavigation.h"

namespace br {
namespace pucrio {
namespace telemidia {
namespace ncl {
namespace navigation {
	KeyNavigation::KeyNavigation() {
		focusIndex = "";
		moveUp = "";
		moveDown = "";
		moveLeft = "";
		moveRight = "";
	}

	string KeyNavigation::getFocusIndex() {
		return focusIndex;
	}

	void KeyNavigation::setFocusIndex(string index) {
		focusIndex = index;
	}

	string KeyNavigation::getMoveUp() {
		return moveUp;
	}

	void KeyNavigation::setMoveUp(string index) {
		moveUp = index;
	}

	string KeyNavigation::getMoveDown() {
		return moveDown;
	}

	void KeyNavigation::setMoveDown(string index) {
		moveDown = index;
	}

	string KeyNavigation::getMoveRight() {
		return moveRight;
	}

	void KeyNavigation::setMoveRight(string index) {
		moveRight = index;
	}

	string KeyNavigation::getMoveLeft() {
		return moveLeft;
	}

	void KeyNavigation::setMoveLeft(string index) {
		moveLeft = index;
	}
}
}
}
}
}
