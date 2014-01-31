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

#include "network/TcpSocketService.h"

#include <pthread.h>
#include <map>
#include <vector>
#include <string>
using namespace std;

#include "util/functions.h"
using namespace ::br::pucrio::telemidia::util;

#include "system/compat/SystemCompat.h"
using namespace ::br::pucrio::telemidia::ginga::core::system::compat;

#include "contextmanager/IContextManager.h"
using namespace ::br::pucrio::telemidia::ginga::core::contextmanager;

#ifndef REMOTEEVENTSERVICE_H_
#define REMOTEEVENTSERVICE_H_

using namespace std;

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace core {
namespace multidevice {
	class RemoteEventService : public IRemoteDeviceListener {
		private:
			static RemoteEventService* _instance;
			pthread_mutex_t groupsMutex;
			map<int,TcpSocketService*> groups;
			static const int DEFAULT_PORT = 22222;
			static IContextManager* contextManager;
			string base_device_ncl_path;

		public:
			RemoteEventService();
			virtual ~RemoteEventService();

			static RemoteEventService* getInstance();

			void addDeviceClass(unsigned int id);
			void setBaseDeviceNCLPath(string base_device_ncl_path);
			void addDevice(
					unsigned int device_class,
					unsigned int device_id,
					char* addr,
					int srvPort,
					bool isLocalConnection);

			void addDocument(unsigned int device_class, char* name, char* body);
			void startDocument(unsigned int device_class, char* name);
			void stopDocument(unsigned int device_class, char* name);

			bool newDeviceConnected(int newDevClass, int w, int h);
			void connectedToBaseDevice(unsigned int domainAddr);

			bool receiveRemoteContent(
					int remoteDevClass,
					string contentUri);

			bool receiveRemoteContent(
					int remoteDevClass,
					char *stream, int streamSize);

			bool receiveRemoteContentInfo(
					string contentId, string contentUri);

			bool receiveRemoteEvent(
					int remoteDevClass,
					int eventType,
					string eventContent);

	};
}
}
}
}
}
}

#endif /* REMOTEEVENTSERVICE_H_ */
