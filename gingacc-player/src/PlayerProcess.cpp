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

#include "../include/PlayerProcess.h"

#include "util/functions.h"
using namespace ::br::pucrio::telemidia::util;

#include <iostream>
using namespace std;

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace core {
namespace player {
	PlayerProcess::PlayerProcess(const char* objName) :
			Player(""), Process(NULL) {

		msgs = new map<string, string>;

		pthread_mutex_init(&msgMutex, NULL);

		isWaitingAns = false;
		pthread_mutex_init(&ansMutex, NULL);
		pthread_cond_init(&ansCond, NULL);

		init(objName);
		setProcessListener(this);
	}

	PlayerProcess::~PlayerProcess() {
		cout << "PlayerProcess::~PlayerProcess(" << this;
		cout << ") => '" << mrl << "'" << endl;

		reader = false;

		if (isWaitingAns) {
			pthread_cond_signal(&ansCond);
		}

		isWaitingAns = false;
		pthread_mutex_destroy(&ansMutex);
		pthread_cond_destroy(&ansCond);

		pthread_mutex_lock(&msgMutex);
		if (msgs != NULL) {
			delete msgs;
			msgs = NULL;
		}

		pthread_mutex_unlock(&msgMutex);
		pthread_mutex_destroy(&msgMutex);
	}

	void PlayerProcess::init(const char* objName) {
		setProcessInfo(
				"/usr/local/etc/ginga/tools/loaders/players",
				objName);

		run();
		checkCom();
	}

	void PlayerProcess::receiveProcessSignal(int sigType, int pSig, int ppid) {
		notifyListeners(PL_NOTIFY_STOP, itos(pSig), TYPE_SIGNAL);
	}

	void PlayerProcess::setMrl(string mrl, bool visible) {
		string strbool = "true";
		if (!visible) {
			strbool = "false";
		}

		cout << "PlayerProcess::setMrl '" << mrl << "'" << endl;

		sendMsg("createplayer," + mrl + "," + strbool + "::;::");
		Player::setMrl(mrl, visible);
	}

	void PlayerProcess::reset() {
		Process::release();

		init(objName.c_str());
		setProcessListener(this);
		setMrl(mrl, visible);
		setOutWindow(windowId);
	}

	void PlayerProcess::rebase() {
		if (status == PLAY) {
			play();

		} else if (status == PAUSE) {
			play();
			pause();
		}
	}

	string PlayerProcess::getAnswer(string token, int howPatient) {
		string ans = "";
		map<string, string>::iterator i;

		cout << "PlayerProcess::getAnswer '" << token << "'" << endl;
		while (reader) {
			pthread_mutex_lock(&msgMutex);
			i = msgs->find(token);
			if (i != msgs->end()) {
				ans = i->second;
				msgs->erase(i);
				pthread_mutex_unlock(&msgMutex);
				return ans;

			} else {
				pthread_mutex_unlock(&msgMutex);
				waitAnswer(howPatient);
			}
		}

		return ans;
	}

	void PlayerProcess::waitAnswer(int milliseconds) {
		int res;
		struct timeval time;
		struct timespec timeOut;
		long int micro;

		gettimeofday(&time, NULL);
		timeOut.tv_sec = time.tv_sec + (long int)(milliseconds / 1000);
		micro = ((milliseconds%1000) * 1000) + time.tv_usec;
		if (micro > 1000000) {
			timeOut.tv_sec++;
			micro -= 1000000;
		}

		timeOut.tv_nsec = micro * 1000;

		pthread_mutex_lock(&ansMutex);
		isWaitingAns = true;
		pthread_cond_timedwait(
				&ansCond, &ansMutex, (const struct timespec*)(&timeOut));

		isWaitingAns = false;
		pthread_mutex_unlock(&ansMutex);
	}

	void PlayerProcess::messageReceived(string msg) {
		string key, value;
		vector<string>* vMsg;
		vector<string>::iterator i;

		if (msg.find("=") != std::string::npos) {
			key   = msg.substr(0, msg.find_first_of("="));
			value = msg.substr(
					msg.find_first_of("=") + 1,
					msg.length() - msg.find_first_of("=") + 1);

			pthread_mutex_lock(&msgMutex);
			(*msgs)[key] = value;
			pthread_mutex_unlock(&msgMutex);
			if (isWaitingAns) {
				pthread_cond_signal(&ansCond);
			}

		} else if (msg.find(",") != std::string::npos) {
			vMsg = split(msg, ",");
			if ((*vMsg)[0] == "updatestatus" && vMsg->size() == 4) {
				if ((*vMsg)[2] == "NULL") {
					(*vMsg)[2] = "";
				}
				notifyListeners(stof((*vMsg)[1]), (*vMsg)[2], stof((*vMsg)[3]));
			}
		}
	}

	void PlayerProcess::setNotifyContentUpdate(bool notify) {
		Player::setNotifyContentUpdate(notify);
	}

	void PlayerProcess::addListener(IPlayerListener* listener) {
		Player::addListener(listener);
	}

	void PlayerProcess::removeListener(IPlayerListener* listener) {
		Player::removeListener(listener);
	}

	void PlayerProcess::notifyListeners(
			short code, string parameter, short type) {

		Player::notifyListeners(code, parameter, type);
	}

	int64_t PlayerProcess::getVPts() {
		string ans;

		sendMsg("getvpts::;::");

		ans = getAnswer("vpts", 1000);
		if (ans == "") {
			return 0.0;
		}

		return stof(ans);
	}

	double PlayerProcess::getMediaTime() {
		return Player::getMediaTime();
	}

	void PlayerProcess::setMediaTime(double newTime) {
		if (status == PLAY) {
			sendMsg("setmediatime," + itos(newTime + 0.25) + "::;::");

		} else {
			sendMsg("setmediatime," + itos(newTime) + "::;::");
		}
		Player::setMediaTime(newTime);
	}

	bool PlayerProcess::setKeyHandler(bool isHandler) {
		string auxstr = "true";
		if (!isHandler) {
			auxstr = "false";
		}

		sendMsg("setkeyhandler," + auxstr + "::;::");
		auxstr = getAnswer("iskeyhandler", 1000);
		if (auxstr == "") {
			return false;
		}

		return auxstr == "true";
	}

	void PlayerProcess::setScope(
			string scope, short type, double begin, double end) {

		sendMsg(
				"setscope," +
				scope       + "," +
				itos(type)  + "," +
				itos(begin) + "," +
				itos(end)   + "::;::");

		Player::setScope(scope, type, begin, end);
	}

	void PlayerProcess::play() {
		sendMsg("play::;::");
		Player::play();
	}

	void PlayerProcess::stop() {
		sendMsg("stop::;::");
		Player::stop();
	}

	void PlayerProcess::abort() {
		sendMsg("abort::;::");
		Player::abort();
	}

	void PlayerProcess::pause() {
		sendMsg("pause::;::");
		Player::pause();
	}

	void PlayerProcess::resume() {
		sendMsg("resume::;::");
		Player::resume();
	}

	string PlayerProcess::getPropertyValue(string name) {
		string ans;

		sendMsg("getpropertyvalue," + name + "::;::");

		ans = getAnswer("propertyvalue" + name, 1000);
		if (ans == "") {
			return "";
		}

		return ans;
	}

	void PlayerProcess::setPropertyValue(
			string name, string value, double duration, double by) {

		string msg = "setpropertyvalue," + name + "," + value;

		if (duration > 0) {
			msg = msg + "," + itos(duration);
		}

		if (by > 0) {
			msg = msg + "," + itos(by);
		}

		sendMsg(msg + "::;::");
	}

	void PlayerProcess::setReferenceTimePlayer(IPlayer* player) {
		Player::setReferenceTimePlayer(player);
	}

	void PlayerProcess::addTimeReferPlayer(IPlayer* referPlayer) {
		Player::addTimeReferPlayer(referPlayer);
	}

	void PlayerProcess::removeTimeReferPlayer(IPlayer* referPlayer) {
		Player::removeTimeReferPlayer(referPlayer);
	}

	void PlayerProcess::notifyReferPlayers(int transition) {
		Player::notifyReferPlayers(transition);
	}

	void PlayerProcess::timebaseObjectTransitionCallback(int transition) {
		Player::timebaseObjectTransitionCallback(transition);
	}

	void PlayerProcess::setTimeBasePlayer(IPlayer* timeBasePlayer) {
		Player::setTimeBasePlayer(timeBasePlayer);
	}

	bool PlayerProcess::hasPresented() {
		return Player::hasPresented();
	}

	void PlayerProcess::setPresented(bool presented) {
		Player::setPresented(presented);
	}

	bool PlayerProcess::isVisible() {
		return Player::isVisible();
	}

	void PlayerProcess::setVisible(bool visible) {
		Player::setVisible(visible);
	}

	bool PlayerProcess::immediatelyStart() {
		return Player::immediatelyStart();
	}

	void PlayerProcess::setImmediatelyStart(bool immediatelyStartVal) {
		Player::setImmediatelyStart(immediatelyStartVal);
	}

	void PlayerProcess::forceNaturalEnd() {
		Player::forceNaturalEnd();
	}

	bool PlayerProcess::isForcedNaturalEnd() {
		return Player::isForcedNaturalEnd();
	}

	bool PlayerProcess::setOutWindow(int windowId) {
		this->windowId = windowId;
		sendMsg("setoutwindow," + itos(windowId) + "::;::");
		return true;
	}

	IPlayer* PlayerProcess::getSelectedPlayer() {
		return NULL;
	}

	void PlayerProcess::setPlayerMap(map<string, IPlayer*>* objs) {

	}

	map<string, IPlayer*>* PlayerProcess::getPlayerMap() {

	}

	IPlayer* PlayerProcess::getPlayer(string objectId) {

	}

	void PlayerProcess::select(IPlayer* selObject) {

	}

	void PlayerProcess::setCurrentScope(string scopeId) {
		sendMsg("setcurrentscope," + scopeId + "::;::");
	}

	void PlayerProcess::timeShift(string direction) {
		sendMsg("timeshift," + direction + "::;::");
	}
}
}
}
}
}
}

extern "C" ::br::pucrio::telemidia::ginga::core::player::IPlayer*
createPlayerProcess(const char* objectName, bool hasVisual) {
	return new ::br::pucrio::telemidia::ginga::core::player::PlayerProcess(
			objectName);
}

extern "C" void destroyPlayerProcess(
		::br::pucrio::telemidia::ginga::core::player::IPlayer* player) {

	delete player;
}
