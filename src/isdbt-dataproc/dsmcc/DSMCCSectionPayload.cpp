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

#include "config.h"
#include "DSMCCSectionPayload.h"

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace core {
namespace dataprocessing {
namespace dsmcc {
namespace npt {
DSMCCSectionPayload::DSMCCSectionPayload(char* data, unsigned int length) {
	payload            = data;
	payloadSize        = length;
	privateDataByte    = NULL;
	dsmccDescritorList = new vector<MpegDescriptor*>;
	privateDataLength  = 0;
	checksum           = 0;

	processSectionPayload();
}

DSMCCSectionPayload::~DSMCCSectionPayload() {
	clearDsmccDescritor();
	if (dsmccDescritorList != NULL) {
		delete (dsmccDescritorList);
	}
}

int DSMCCSectionPayload::processSectionPayload() {
	unsigned int pos;
	unsigned char descriptorTag;
	unsigned short descriptorSize;
	NPTReference* nptRef;
	NPTEndpoint* epRef;
	StreamMode* strMode;

	pos = 0;

	//dsmccDescriptorList()
	while (pos < payloadSize) {
		descriptorTag  = payload[pos] & 0xFF;
		descriptorSize = (payload[pos + 1] & 0xFF) + 2;

		switch (descriptorTag) {
			case 0x01: // NPT Reference
				nptRef = new NPTReference();
				nptRef->addData(payload + pos, descriptorSize);
				addDsmccDescriptor(nptRef);
				break;

			case 0x02: // NPT Endpoint
				epRef = new NPTEndpoint();
				epRef->addData(payload + pos, descriptorSize);
				addDsmccDescriptor(epRef);
				break;

			case 0x03: // Stream Mode
				strMode = new StreamMode();
				strMode->addData(payload + pos, descriptorSize);
				addDsmccDescriptor(strMode);
				break;

			case 0x04: // Stream Event
				clog << "DSMCCSectionPayload::processSectionPayload";
				clog << " stream event." << endl;
				break;

			default:
				clog << "DSMCCSectionPayload::processSectionPayload";
				clog << "MpegDescriptor unrecognized. ";
				clog << (descriptorTag & 0xFF) << endl;
				break;
		}
		pos = pos + descriptorSize;
	}

	return pos;
}

int DSMCCSectionPayload::updateStream() {
	/*int pos;

	if (sectionSyntaxIndicator) {
		privateIndicator = 0x00;
	} else {
		privateIndicator = 0x01;
	}

	pos = PrivateSection::updateStream();

	MpegDescriptor* desc;
	int streamLen;
	char* dataStream;

	if (tableId == 0x3A) {
		//LLCSNAP()
	} else if (tableId == 0x3B) {
		//userNetworkMessage()
	} else if (tableId == 0x3C) {
		//downloadDataMessage()
	} else if (tableId == 0x3D) {
		vector<MpegDescriptor*>::iterator i;
		if ((dsmccDescritorList != NULL) &&
				(!dsmccDescritorList->empty())) {
			i = dsmccDescritorList->begin();
			while (i != dsmccDescritorList->end()) {
				desc = *i;
				streamLen = desc->getStream(&dataStream);
				if ((pos + streamLen + 4) <= MAX_SECTION_SIZE) {
					memcpy(stream + pos, dataStream, streamLen);
					pos += streamLen;
				} else {
					break;
				}
				++i;
			}
		}
	} else if (tableId == 0x3E) {
		//private_data_byte
	}

	if (!sectionSyntaxIndicator) {
		//TODO: checksum
		stream[pos++] = (checksum >> 24) & 0xFF;
		stream[pos++] = (checksum >> 16) & 0xFF;
		stream[pos++] = (checksum >> 8) & 0xFF;
		stream[pos++] = checksum & 0xFF;
	} else {
		//crc32
		Crc32 crc;
		unsigned int value = crc.crc(stream, pos);
		crc32 = value;
		stream[pos++] = (crc32 >> 24) & 0xFF;
		stream[pos++] = (crc32 >> 16) & 0xFF;
		stream[pos++] = (crc32 >> 8) & 0xFF;
		stream[pos++] = crc32 & 0xFF;
	}
	return pos;*/
	return 0;
}

int DSMCCSectionPayload::calculateSectionSize() {
	/*unsigned int pos = PrivateSection::calculateSectionSize();
	MpegDescriptor* desc;
	int streamLen;
	if (tableId == 0x3A) {
		//LLCSNAP()
	} else if (tableId == 0x3B) {
		//userNetworkMessage()
	} else if (tableId == 0x3C) {
		//downloadDataMessage()
	} else if (tableId == 0x3D) {
		vector<MpegDescriptor*>::iterator i;
		if ((dsmccDescritorList != NULL) &&
				(!dsmccDescritorList->empty())) {
			i = dsmccDescritorList->begin();
			while (i != dsmccDescritorList->end()) {
				desc = *i;
				streamLen = desc->getStreamSize();
				if ((pos + streamLen + 4) <= MAX_SECTION_SIZE) {
					pos += streamLen;
				} else {
					break;
				}
				++i;
			}
		}
	} else if (tableId == 0x3E) {
		//private_data_byte
	}
	return pos + 4;*/
	return 0;
}

vector<MpegDescriptor*>* DSMCCSectionPayload::getDsmccDescritorList() {
	return dsmccDescritorList;
}

unsigned int DSMCCSectionPayload::getChecksum() {
	return checksum;
}

void DSMCCSectionPayload::setChecksum(unsigned int cs) {
	checksum = cs;
}

int DSMCCSectionPayload::getPrivateDataByte(char** dataStream) {
	if (privateDataByte != NULL) {
		*dataStream = privateDataByte;
		return privateDataLength;
	} else {
		return 0;
	}
}

int DSMCCSectionPayload::setPrivateDataByte(char* data, unsigned short length) {
	if (privateDataByte != NULL) {
		delete (privateDataByte);
	}
	try {
		privateDataByte = new char[length];
	} catch(...) {
		return -1;
	}

	memcpy((void*)privateDataByte, (void*)data, (::size_t)length);
	privateDataLength = length;
	return privateDataLength;
}

void DSMCCSectionPayload::addDsmccDescriptor(MpegDescriptor* d) {
	dsmccDescritorList->push_back(d);
}

void DSMCCSectionPayload::removeDsmccDescriptor(unsigned char descriptorTag) {
	MpegDescriptor* desc;
	vector<MpegDescriptor*>::iterator i;
	if ((dsmccDescritorList != NULL) && (!dsmccDescritorList->empty())) {
		i = dsmccDescritorList->begin();
		while (i != dsmccDescritorList->end()) {
			desc = *i;
			if (desc->getDescriptorTag() == descriptorTag) {
				delete (desc);
				dsmccDescritorList->erase(i);
				break;
			}
			++i;
		}
	}
}

void DSMCCSectionPayload::clearDsmccDescritor() {
	MpegDescriptor* desc;
	vector<MpegDescriptor*>::iterator i;
	if ((dsmccDescritorList != NULL) && (!dsmccDescritorList->empty())) {
		i = dsmccDescritorList->begin();
		while (i != dsmccDescritorList->end()) {
			desc = *i;
			delete (desc);
			++i;
		}
		dsmccDescritorList->clear();
	}
}

}
}
}
}
}
}
}
}
