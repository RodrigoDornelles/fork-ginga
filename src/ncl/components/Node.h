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

#ifndef NODE_H_
#define NODE_H_

#include <vector>
using namespace std;

#include "../Entity.h"
using namespace ::br::pucrio::telemidia::ncl;

#include "../interfaces/Anchor.h"
#include "../interfaces/PropertyAnchor.h"
using namespace ::br::pucrio::telemidia::ncl::interfaces;

namespace br {
namespace pucrio {
namespace telemidia {
namespace ncl {
namespace components {
	class Node : public Entity {
		private:
			void* parentNode;

		protected:
			vector<Anchor*> anchorList;
			vector<PropertyAnchor*> originalPAnchors;

		public:
			Node(string id);
			virtual ~Node();

		private:
			bool hasProperty(string propName);

		public:
			void copyProperties(Node* node);
			void setParentComposition(void* composition);
			void* getParentComposition();
	  		vector<Node*>* getPerspective();
	  		virtual bool addAnchor(Anchor *anchor);
			virtual bool addAnchor(int index, Anchor *anchor);
			Anchor* getAnchor(string anchorId);
			Anchor* getAnchor(int index);

			vector<Anchor*>* getAnchors();
			vector<PropertyAnchor*>* getOriginalPropertyAnchors();

			PropertyAnchor* getPropertyAnchor(string propertyName);
			int getNumAnchors();
			int indexOfAnchor(Anchor *anchor);
			virtual bool removeAnchor(int index);
			virtual bool removeAnchor(Anchor *anchor);
	};
}
}
}
}
}

#endif /*NODE_H_*/