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

#include "cm/ComponentManager.h"
#include "cm/parser/IComponentParser.h"

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace core {
namespace cm {
	ComponentManager::ComponentManager() {
		this->components           = NULL;
		this->symbols              = NULL;
		this->parentObjects        = NULL;
		this->unsolvedDependencies = NULL;
		this->canUnload            = true;

		Thread::mutexInit(&mapMutex, NULL);
	}

	ComponentManager::~ComponentManager() {
		if (_instance != NULL) {
			_instance->release();
		}
	}

	void ComponentManager::setUnloadComponents(bool allowUnload) {
		canUnload = allowUnload;
	}

	void ComponentManager::release() {
		set<string>* childs;
		map<string, set<string>*>::iterator i;

		Thread::mutexLock(&mapMutex);
		if (parentObjects != NULL) {
			i = parentObjects->begin();
			while (i != parentObjects->end()) {
				childs = i->second;
				delete childs;
				++i;
			}
			delete parentObjects;
			parentObjects = NULL;
		}

		if (components != NULL) {
			//TODO: release components
			delete components;
			components = NULL;
		}

		if (symbols != NULL) {
			delete symbols;
			symbols = NULL;
		}

		if (unsolvedDependencies != NULL) {
			i = unsolvedDependencies->begin();
			while (i != unsolvedDependencies->end()) {
				childs = i->second;
				delete childs;
				++i;
			}
			delete unsolvedDependencies;
			unsolvedDependencies = NULL;
		}

		Thread::mutexUnlock(&mapMutex);
		Thread::mutexDestroy(&mapMutex);
	}

	ComponentManager* ComponentManager::_instance = NULL;

	ComponentManager* ComponentManager::getInstance() {
		if (ComponentManager::_instance == NULL) {
			ComponentManager::_instance = new ComponentManager();
		}
		return ComponentManager::_instance;
	}

	void* ComponentManager::getObject(string objectName) {
		IComponent* c;
		void* comp;
		void* symbol;

		Thread::mutexLock(&mapMutex);
		if (symbols == NULL || components == NULL) {
			refreshComponentDescription();
		}

		if (symbols->count(objectName) == 0) {
			clog << "ComponentManager::getObject warning! '" << objectName;
			clog << "' symbol not found!" << endl;

			Thread::mutexUnlock(&mapMutex);
			return NULL;
		}

		c = (*symbols)[objectName];
		Thread::mutexUnlock(&mapMutex);

		if (c != NULL) {
			symbol = SystemCompat::loadComponent(
					c->getName(), &comp, c->getCreatorSymbol(objectName));

			if (comp != NULL) {
				c->setComponent(comp);
				return symbol;
			}

		} else {
			clog << "ComponentManager::getObject warning! Component for '";
			clog << objectName << "' was not described" << endl;
			return NULL;
		}

		clog << "ComponentManager::getObject warning! Can't create component '";
		clog << c->getName() << "'" << endl;
		return NULL;
	}

	set<string>* ComponentManager::getObjectsFromInterface(
			string interfaceName) {

		if (parentObjects->count(interfaceName) != 0) {
			return (*parentObjects)[interfaceName];
		}

		return NULL;
	}

	map<string, set<string>*>* ComponentManager::getUnsolvedDependencies() {
		return unsolvedDependencies;
	}

	bool ComponentManager::releaseComponentFromObject(string objName) {
		IComponent* c;
		void* comp;

		if (symbols->count(objName) == 0) {
			clog << "ComponentManager::releaseComponentFromaObject";
			clog << " warning! '" << objName;
			clog << "' symbol not found!" << endl;
			return false;
		}

		c = (*symbols)[objName];
		if (c != NULL) {
			comp = c->getComponent();
			if (comp != NULL) {
				clog << "ComponentManager::releaseComponentFromObject '";
				clog << objName << "'" << endl;
				if (releaseComponent(comp)) {
					return true;
				}
				c->setComponent(NULL);
				clog << "ComponentManager::releaseComponentFromObject ";
				clog << "Warning! object '" << objName;
				clog << "' was not released." << endl;
			}
		}

		return false;
	}

	bool ComponentManager::releaseComponent(void* component) {
		if (canUnload) {
			return SystemCompat::releaseComponent(component);
		}

		return canUnload;
	}

	void ComponentManager::refreshComponentDescription() {
		IComponentParser* cp;
		void* component;
		void* symbol;
		string compUri;

		string lName = "libgingacccmparser";
		string sName = "createComponentParser";

		if (components != NULL && !components->empty()) {
			clog << "ComponentManager::refreshComponentDescription already ";
			clog << " up to date." << endl;
			return;
		}

		symbol = SystemCompat::loadComponent(lName, &component, sName);
		if (symbol == NULL) {
			clog << "ComponentManager::refreshComponentDescription Warning!";
			clog << "Can't find symbol '" << sName << "' using library '";
			clog << lName << "'" << endl;
			return;
		}

		cp = ((ComponentParserCreator*)symbol)();

		compUri = SystemCompat::appendGingaFilesPrefix(
				"componentDescription.xml");

		cp->parse(compUri);

		this->components           = cp->getComponents();
		this->symbols              = cp->getSymbols();
		this->parentObjects        = cp->getParentObjects();
		this->unsolvedDependencies = cp->getUnsolvedDependencies();

		delete cp;
		releaseComponent(component);
	}

	map<string, IComponent*>* ComponentManager::getComponentDescription() {
		map<string, IComponent*>* compdesc;

		Thread::mutexLock(&mapMutex);
		compdesc = new map<string,IComponent*>(*components);
		Thread::mutexUnlock(&mapMutex);

		return compdesc;
	}

	bool ComponentManager::isAvailable(string objName) {
		IComponent* c;
		string url;

		Thread::mutexLock(&mapMutex);
		if (symbols->count(objName) == 0) {
			Thread::mutexUnlock(&mapMutex);
			clog << "ComponentManager::isAvailable Can't find symbol for '";
			clog << objName << "'! Returning false" << endl;
			return false;
		}

		c = (*symbols)[objName];
		if (c != NULL) {
			url = SystemCompat::updatePath(
					c->getLocation() + SystemCompat::getIUriD() + c->getName());

			if (access(url.c_str(), (int)F_OK) == 0) {
				Thread::mutexUnlock(&mapMutex);
				return true;

			} else {
				clog << "ComponentManager::isAvailable ";
				clog << "File not found: '" << url << "'! Returning false";
				clog << endl;
			}

		} else {
			clog << "ComponentManager::isAvailable Null Component for symbol '";
			clog << objName << "'! Returgning false" << endl;
		}

		Thread::mutexUnlock(&mapMutex);
		return false;
	}
}
}
}
}
}
}

extern "C" ::br::pucrio::telemidia::ginga::core::cm::IComponentManager*
		createCM() {

	return ::br::pucrio::telemidia::ginga::core::cm::ComponentManager::
			getInstance();
}
