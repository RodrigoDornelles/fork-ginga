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

#include "isdbt-tsparser/ServiceInfo.h"

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace core {
namespace tsparser {
namespace si {
	ServiceInfo::ServiceInfo() {
		descriptorsLoopLength   = 0;
		serviceId               = 0;
		runningStatus           = 0;
		eitPresentFollowingFlag = false;
		eitScheduleFlag         = false;
		descriptors             = new vector<IMpegDescriptor*>;

	}

	ServiceInfo::~ServiceInfo() {
		vector<IMpegDescriptor*>::iterator i;
		if (descriptors != NULL) {
			i = descriptors->begin();
			while (i != descriptors->end()) {
				delete (*i);
				++i;
			}
			delete descriptors;
			descriptors = NULL;
		}
	}

	size_t ServiceInfo::getSize() {
		return (descriptorsLoopLength + 5);
	}

	unsigned short ServiceInfo::getServiceId() {
		return serviceId;
	}

	bool ServiceInfo::getEitScheduleFlag() {
		return eitScheduleFlag;
	}

	bool ServiceInfo::getEitPresentFollowingFlag() {
		return eitPresentFollowingFlag;
	}

	unsigned char ServiceInfo::getRunningStatus() {
		return runningStatus;
	}

	string ServiceInfo::getRunningStatusDescription () {
		switch (runningStatus) {
			case 0:
				return "Undefined";
				break;

			case 1:
				return "Off";
				break;

			case 2:
				return "Start within few minutes";
				break;

			case 3:
				return "Paused";
				break;

			case 4:
				return "Running";
				break;
			//5-7 are reserved for future used
		}

		return "";
	}

	unsigned char ServiceInfo::getFreeCAMode() {
		return freeCAMode;
	}

	unsigned short ServiceInfo::getDescriptorsLoopLength() {
		return descriptorsLoopLength;
	}

	void ServiceInfo::insertDescriptor(IMpegDescriptor* descriptor) {
		size_t count = 0;

		descriptors->push_back(descriptor);

		count += (descriptor->getDescriptorLength()+ 2);
		descriptorsLoopLength = count;
	}

	vector<IMpegDescriptor*>* ServiceInfo::getDescriptors() {
		return descriptors;
	}

	void ServiceInfo::print() {
		vector<IMpegDescriptor*>::iterator i;

		clog << "ServiceInfo::print printing..." << endl;
		clog << " -serviceId: "               << serviceId        << endl;
		clog << " -eitScheduleFlag: "         << eitScheduleFlag  << endl;
		clog << " -eitPresentFlag:"           << eitPresentFollowingFlag;
		clog << endl;
		clog << " -runningStatusDesc: "       << getRunningStatusDescription();
		clog << endl;

		for (i =  descriptors->begin(); i!= descriptors->end(); ++i) {
			(*i)->print();
		}
	}
	size_t ServiceInfo::process(char* data, size_t pos) {
		IMpegDescriptor*  descriptor;
		size_t localpos;
		unsigned char remainingBytesDescriptor, value;

		cout <<"ServiceInfo::process with pos " << pos << endl;

		serviceId = ((((data[pos] << 8) & 0xFF00) |
								(data[pos+1] & 0xFF)));

		//cout <<"ServiceId = " << serviceId << endl;
		pos += 2;
		//jumping reserved_future_use
		eitScheduleFlag = ((data[pos] & 0x02) >> 1);
		eitPresentFollowingFlag = (data[pos] & 0x01);
		pos++;

		runningStatus = ((data[pos] & 0xE0) >> 5);
		freeCAMode = ((data[pos] & 0x10) >> 4);
		descriptorsLoopLength = (
				(((data[pos] & 0x0F) << 8) & 0xFF00) |
				(data[pos+1] & 0xFF));

		pos += 2;

		//clog << "DescriptorsLoopLength = " << descriptorsLoopLength << endl;
		remainingBytesDescriptor = descriptorsLoopLength;

		while (remainingBytesDescriptor) {//there's at least one descriptor
			value = data[pos+1] + 2;
			remainingBytesDescriptor -= value;

			switch (data[pos]) {
				case LOGO_TRANMISSION:
					descriptor = new LogoTransmissionDescriptor();
					localpos = descriptor->process(data, pos);
					pos += value;
					descriptors->push_back(descriptor);
					break;

				case SERVICE:
					descriptor = new ServiceDescriptor();
					localpos = descriptor->process(data, pos);
					pos += value;
					descriptors->push_back(descriptor);
					break;

				default: //Unrecognized Descriptor
					clog << "ServiceInfo:: process default descriptor with tag: ";
					clog << hex << (unsigned int)data[pos] << dec ;
					clog << " with length = " << (unsigned int)data[pos+1];
					clog << " and with pos = " << pos << endl;
					pos += value;
					break;
			}
		}
		return pos;
	}

}
}
}
}
}
}
}

extern "C" ::br::pucrio::telemidia::ginga::core::tsparser::si::IServiceInfo*
		createServiceInfo() {

	return (new
			::br::pucrio::telemidia::ginga::core::tsparser::si::ServiceInfo());
}

extern "C" void destroyServiceInfo(
		::br::pucrio::telemidia::ginga::core::tsparser::si::IServiceInfo* si) {

	delete si;
}
