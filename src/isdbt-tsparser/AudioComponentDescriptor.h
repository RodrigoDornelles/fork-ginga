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
#ifndef AUDIOCOMPONENTDESCRIPTOR_H_
#define AUDIOCOMPONENTDESCRIPTOR_H_


#include "IMpegDescriptor.h"
using namespace ::br::pucrio::telemidia::ginga::core::tsparser;

BR_PUCRIO_TELEMIDIA_GINGA_CORE_TSPARSER_SI_DESCRIPTORS_BEGIN

	class AudioComponentDescriptor : public IMpegDescriptor {
		protected:
			unsigned char streamContent;
			unsigned char componentType;
			unsigned char componentTag;
			unsigned char streamType;
			unsigned char simulcastGroupTag;
			bool ESMultiLingualFlag;
			bool mainComponentFlag;
			unsigned char qualityIndicator;
			unsigned char samplingRate;
			char languageCode[3];//static: always used
			char* languageCode2;//dinamic: not always used, when used length is 3.
			unsigned char textLength;//not sent in TS, so it is calculated
			char* textChar;

		public:
			AudioComponentDescriptor();
			~AudioComponentDescriptor();
			unsigned char getDescriptorTag();
			unsigned char getStreamContent();
			unsigned char getComponentTag();
			unsigned char getComponentType();
			unsigned char getStreamType();
			unsigned char getSimulcastGroupTag();
			bool getESMultiLingualFlag();
			bool getMainComponentFlag();
			unsigned char getQualityIndicator();
			unsigned char getSamplingRate();
			string getLanguageCode();
			string getLanguageCode2();
			string getTextChar();
			void print();
			unsigned int getDescriptorLength();
			size_t process (char* data, size_t pos);
	};


BR_PUCRIO_TELEMIDIA_GINGA_CORE_TSPARSER_SI_DESCRIPTORS_END
#endif /* AUDIOCOMPONENTDESCRIPTOR_H_ */
