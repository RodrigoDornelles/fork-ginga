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

#ifndef OBJECTPROCESSOR_H_
#define OBJECTPROCESSOR_H_

#include "util/functions.h"
using namespace ::ginga::util;

#include "system/SystemCompat.h"
using namespace ::ginga::system;

#include "Binding.h"
#include "Object.h"
#include "IObjectListener.h"


BR_PUCRIO_TELEMIDIA_GINGA_CORE_DATAPROCESSING_CAROUSEL_BEGIN

	class ObjectProcessor {
		private:
			//mapping object location to known name
			map<string, string> objectNames;

			//mapping object location to known path
			map<string, string> objectPaths;

			//mapping object location in Object
			map<string, Object*> objects;

			set<IObjectListener*> listeners;

			unsigned short pid;

		public:
			ObjectProcessor(unsigned short pid);
			virtual ~ObjectProcessor();

			void setObjectsListeners(set<IObjectListener*>* l);
			void pushObject(Object* object);
			bool hasObjects();
			map<string, string>* getSDNames();
			map<string, string>* getSDPaths();

		private:
			bool mountObject(Object* object);
			void notifyObjectListeners(Object* obj);
	};

BR_PUCRIO_TELEMIDIA_GINGA_CORE_DATAPROCESSING_CAROUSEL_END
#endif /*OBJECTPROCESSOR_H_*/
