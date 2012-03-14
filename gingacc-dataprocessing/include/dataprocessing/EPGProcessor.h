/******************************************************************************
Este arquivo eh parte da implementacao do ambiente declarativo do middleware
Ginga (Ginga-NCL).

Direitos Autorais Reservados (c) 1989-2007 PUC-Rio/Laboratorio TeleMidia

Este programa eh software livre; voce pode redistribui-lo e/ou modificah-lo sob
os termos da Licenca Publica Geral GNU versao 2 conforme publicada pela Free
Software Foundation.

Este programa eh distribuido na expectativa de que seja util, porem, SEM
NENHUMA GARANTIA; nem mesmo a garantia implicita de COMERCIABILIDADE OU
ADEQUACAO A UMA FINALIDADE ESPECIFICA. Consulte a Licenca Publica Geral do
GNU versao 2 para mais detalhes.

Voce deve ter recebido uma copia da Licenca Publica Geral do GNU versao 2 junto
com este programa; se nao, escreva para a Free Software Foundation, Inc., no
endereco 59 Temple Street, Suite 330, Boston, MA 02111-1307 USA.

Para maiores informacoes:
ncl @ telemidia.puc-rio.br
http://www.ncl.org.br
http://www.ginga.org.br
http://www.telemidia.puc-rio.br
******************************************************************************
This file is part of the declarative environment of middleware Ginga (Ginga-NCL)

Copyright: 1989-2007 PUC-RIO/LABORATORIO TELEMIDIA, All Rights Reserved.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License version 2 as published by
the Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License version 2 for more
details.

You should have received a copy of the GNU General Public License version 2
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

For further information contact:
ncl @ telemidia.puc-rio.br
http://www.ncl.org.br
http://www.ginga.org.br
http://www.telemidia.puc-rio.br
*******************************************************************************/

#ifndef EPGPROCESSOR_H_
#define EPGPROCESSOR_H_

#ifdef __cplusplus
extern "C" {
#endif
	#include <sys/stat.h>
	#include <stdio.h>
	#include <stdio.h>
#ifdef __cplusplus
}
#endif

#include "util/functions.h"
using namespace ::br::pucrio::telemidia::util;

#include "tsparser/IServiceInfo.h"
#include "tsparser/IEventInfo.h"
#include "tsparser/ITOT.h"
using namespace ::br::pucrio::telemidia::ginga::core::tsparser::si;

#include "IDataProcessor.h"
#include "IEPGProcessor.h"

#include <set>
#include <vector>
#include <map>
#include <string>
using namespace std;

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace core {
namespace dataprocessing {
namespace epg {
	class EPGProcessor : public IEPGProcessor {
		private:
			set<string>* processedSections;
			bool presentMapReady;
			bool scheduleMapReady;
			bool serviceMapReady;
			bool timeMapReady;

		protected:
			//TODO: link service id from sdt to eit and cdt
			static EPGProcessor* _instance;
			IServiceInfo* service;
			map<unsigned int, IEventInfo*>* eventPresent;
			map<unsigned int, IEventInfo*>* eventSchedule;
			ITOT* tot;
			set<string>* cdt;
			int files;
			set<IEPGListener*>* epgListeners;
			set<IEPGListener*>* serviceListeners;
			set<IEPGListener*>* timeListeners;
			IDataProcessor* dataProcessor;

		private:
			virtual ~EPGProcessor();

		public:
			static EPGProcessor* getInstance();
			void release();
			void setDataProcessor(IDataProcessor* dataProcessor);
			void decodeSdt(string fileName);
			void decodeSdtSection(ITransportSection* section);
			set<IEventInfo*>* decodeEit(string fileName);
			void decodeTot(ITransportSection* section);
			void decodeCdt(string fileName);
			void decodeEitSection(ITransportSection* section);
			void addEPGListener(IEPGListener* listener, string request, unsigned char type);
			map<string, struct SIField> createMap();
			void removeEPGListener(IEPGListener * listener);

			//void generatePresentMap();

		private:
			EPGProcessor();
			struct SIField* handleFieldStr(string str);
			int savePNG(char* data, int pngSize);
			void generateEitMap(map<unsigned int, IEventInfo*>* actualMap);
			void generateSdtMap(IServiceInfo* si);
			void generateTotMap(ITOT* tot);
			void addProcessedSection(ITransportSection* section);
			void callMapGenerator(unsigned int tableId);
			bool checkProcessedSections(ITransportSection* section);
			void printFieldMap(map<string, struct SIField>* fieldMap);

	};
}
}
}
}
}
}
}

#endif /*EPGPROCESSOR_H_*/
