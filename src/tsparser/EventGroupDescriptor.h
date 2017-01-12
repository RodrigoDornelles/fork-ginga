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

#ifndef EVENTGROUPDESCRIPTOR_H_
#define EVENTGROUPDESCRIPTOR_H_

#include "IMpegDescriptor.h"
using namespace br::pucrio::telemidia::ginga::core::tsparser;

#include <vector>
using namespace std;

struct Event{
	unsigned short serviceId;
	unsigned short eventId;
};
struct MultipleEvent{
	unsigned short originalNetworkId;
	unsigned short transportStreamId;
	struct Event* event;
};

#include <vector>
using namespace std;

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace core {
namespace tsparser {
namespace si {
namespace descriptors {
	class EventGroupDescriptor : public IMpegDescriptor {

		protected:
			unsigned char groupType;
			unsigned char eventCount;
			vector<struct Event*>* events;
			vector<struct MultipleEvent*>* multiEvents;

		public:
			EventGroupDescriptor();
			~EventGroupDescriptor();
			unsigned char getDescriptorTag();
			unsigned int getDescriptorLength();
			void print();
			size_t process(char*data, size_t pos);
	};

}

}

}

}

}

}

}

}

#endif /* EVENTGROUPDESCRIPTOR_H_ */