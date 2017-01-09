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

#include "ic/InteractiveChannelManager.h"
#include "ic/IInteractiveChannel.h"

#include "config.h"

#if HAVE_CURL
#include "ic/curlic/CurlInteractiveChannel.h"
#endif //HAVE_CURL

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace core {
namespace ic {
	InteractiveChannelManager* InteractiveChannelManager::_instance = NULL;
	InteractiveChannelManager::InteractiveChannelManager() {
		ics     = new set<IInteractiveChannel*>;
		urisIcs = new map<string, IInteractiveChannel*>;
	}

	InteractiveChannelManager::~InteractiveChannelManager() {
		if (ics != NULL) {
			delete ics;
		}

		delete _instance;
	}

	bool InteractiveChannelManager::hasInteractiveChannel() {
		IInteractiveChannel* ic;
		set<string>* objects;
		string symbol;
		set<string>::iterator i;

#if HAVE_CURL
		ic = new CurlInteractiveChannel();
		if (ic->hasConnection()) {
			delete ic;
			return true;
		}
		delete ic;
#endif
		clog << "InteractiveChannelManager::hasInteractiveChannel";
		clog << " return false" << endl;
		return false;
	}

	InteractiveChannelManager* InteractiveChannelManager::getInstance() {
		if (_instance == NULL) {
			_instance = new InteractiveChannelManager();
		}
		return _instance;
	}

	set<IInteractiveChannel*>*
	InteractiveChannelManager::getInteractiveChannels() {
		return ics;
	}

	void InteractiveChannelManager::clearInteractiveChannelManager() {
		releaseInteractiveChannels();
	}

	void InteractiveChannelManager::releaseInteractiveChannel(
			IInteractiveChannel* ic) {

		set<IInteractiveChannel*>::iterator i;
		map<string, IInteractiveChannel*>::iterator j;

		i = ics->find(ic);
		if (ics->end() != i) {
			(*i)->releaseUrl();
			delete *i;
			ics->erase(i);

			j = urisIcs->begin();
			while (j != urisIcs->end()) {
				if (j->second == ic) {
					urisIcs->erase(j);
					return;
				}
				++j;
			}
		}
	}

	IInteractiveChannel* InteractiveChannelManager::createInteractiveChannel(
			string rUri) {

		IInteractiveChannel* ic = NULL;

		if (rUri.length() > 7 && rUri.substr(0, 7) == "http://") {
#if HAVE_CURL
			ic = new CurlInteractiveChannel();
#endif
		}

		if (ic != NULL) {
			ics->insert(ic);
			(*urisIcs)[rUri] = ic;
		}

		return ic;
	}

	IInteractiveChannel* InteractiveChannelManager::getInteractiveChannel(
			string remoteUri) {

		if (urisIcs->count(remoteUri) != 0) {
			return (*urisIcs)[remoteUri];
		}

		return NULL;
	}

	void InteractiveChannelManager::releaseInteractiveChannels() {
		set<IInteractiveChannel*>::iterator i;
		urisIcs->clear();
		i = ics->begin();
		while (i != ics->end()) {
			delete *i;
			++i;
		}
		ics->clear();
	}

	void InteractiveChannelManager::performPendingUrls() {
		pthread_t id1;

		clog << "InteractiveChannelManager::performPendingUrls for '";
		clog << ics->size() << "' urls" << endl;

		if (!ics->empty()) {
			pthread_create(
					&id1, 0, InteractiveChannelManager::asyncPerform, this);

			pthread_detach(id1);
		}
	}

	void* InteractiveChannelManager::asyncPerform(void* thiz) {
		InteractiveChannelManager* icm;
		set<IInteractiveChannel*>* icSet;
		set<IInteractiveChannel*>::iterator i;

		icm   = (InteractiveChannelManager*)thiz;
		icSet = icm->getInteractiveChannels();

		i = icSet->begin();
		while (i != icSet->end()) {
			if ((*i)->performUrl()) {
				++i;
			}
		}

		icm->releaseInteractiveChannels();

		return NULL;
	}
}
}
}
}
}
}

extern "C" ::br::pucrio::telemidia::ginga::core::ic::IInteractiveChannelManager*
		createICM() {

	return ::br::pucrio::telemidia::ginga::core::ic::
			InteractiveChannelManager::getInstance();
}

extern "C" void destroyICM(
		::br::pucrio::telemidia::ginga::core::ic::IInteractiveChannelManager*
				icm) {

	delete icm;
}
