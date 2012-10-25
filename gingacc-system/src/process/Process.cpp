/******************************************************************************
Este arquivo eh parte da implementacao do ambiente declarativo do middleware
Ginga (Ginga-NCL).

Direitos Autorais Reservados (c) 1989-2007 PUC-Rio/Laboratorio TeleMidia

Este programa eh software livre; voce pode redistribui-lo e/ou modificah-lo sob
os termos da Licen� Publica Geral GNU versao 2 conforme publicada pela Free
Software Foundation.

Este programa eh distribu�o na expectativa de que seja util, porem, SEM
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

#include "system/process/Process.h"

#include "util/functions.h"
using namespace ::br::pucrio::telemidia::util;

#include <pthread.h>

extern char **environ;

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace core {
namespace system {
namespace process {
	Process::Process(char** argv) {
		this->pid           = -1;
		this->argv          = argv;
		this->envp          = environ;
		this->processUri    = processUri;
		this->processStatus = PST_NULL;
		this->sigListener   = NULL;
		this->reader        = false;
		this->wFd           = NULL;
		this->rFd           = NULL;
		this->hasCom        = false;

		posix_spawnattr_init(&spawnAttr);
		posix_spawn_file_actions_init(&fileActions);

		isCheckingCom = false;
		Thread::mutexInit(&comMutex, NULL);
		Thread::condInit(&comCond, NULL);

		isSpawnedReady = false;
	}

	Process::~Process() {
		clog << "Process::~Process(" << this;
		clog << ")" << endl;

		sigListener = NULL;

		release();

		Thread::mutexDestroy(&comMutex);
		Thread::condDestroy(&comCond);

		posix_spawnattr_destroy(&spawnAttr);
		posix_spawn_file_actions_destroy(&fileActions);
	}

	void Process::release() {
		tryCom();
		forceKill();

		hasCom         = false;
		isSpawnedReady = false;
		processStatus  = PST_NULL;

		fclose(wFd);
		fclose(rFd);

		unlink(wCom.c_str());
		unlink(rCom.c_str());
	}

	void Process::setProcessInfo(string processUri, string objName) {
		pthread_t threadId_;
		string processName;

		this->processUri = processUri;
		this->objName    = objName;

		if (processUri.find(SystemCompat::getIUriD()) != std::string::npos) {
			processName = processUri.substr(
					processUri.find_last_of(SystemCompat::getIUriD()) + 1,
					processUri.length() - processUri.find_last_of(SystemCompat::getIUriD()) + 1);

		} else {
			processName = processUri;
		}

		rCom = ("/tmp/ginga/_" + processName +
				itos((long int)(void*)this) + "_ctop");

		wCom = ("/tmp/ginga/_" + processName +
				itos((long int)(void*)this) + "_ptoc");

		pthread_create(&threadId_, 0, Process::createFiles, this);
		pthread_detach(threadId_);
	}

	void Process::checkCom() {
		if (wFd > 0 && rFd > 0 && isSpawnedReady) {
			return;
		}

		isCheckingCom = true;
		Thread::mutexLock(&comMutex);
		Thread::condWait(&comCond, &comMutex);
		isCheckingCom = false;
		Thread::mutexUnlock(&comMutex);
	}

	void Process::tryCom() {
		if (wFd > 0 && rFd > 0 && isSpawnedReady && isCheckingCom) {
			Thread::condSignal(&comCond);
		}
	}

	bool Process::sendMsg(string msg) {
		return sendMsg(wFd, msg);
	}

	bool Process::sendMsg(FILE* fd, string msg) {
		int rval = -1;

		if (fd != NULL) {

			try {
				rval = fwrite(msg.c_str(), 1, msg.length(), fd);

		    } catch (const char *except) {
		    	clog << "Process::sendMsg catch: " << except << endl;
		    	SystemCompat::uSleep(100000);
		    	rval = fwrite(msg.c_str(), 1, msg.length(), fd);
		    }

			if (rval == msg.length()) {
				return true;
			}
		}
		return false;
	}

	void Process::messageReceived(string msg) {
		clog << "Process::messageReceived '" << msg << "'" << endl;
	}

	string Process::receiveMsg(FILE* fd) {
		int rval = -1;
		char buff[512];
		string msg = "";

		try {
			rval = fread(buff, 1, sizeof(buff), fd);

	    } catch (const char *except) {
	    	clog << "Process::receiveMsg catch: " << except << endl;
	    	SystemCompat::uSleep(100000);
	    	rval = fread(buff, 1, sizeof(buff), fd);
	    }

		if (rval > 0) {
			msg.assign(buff, rval);
		}

		return msg;
	}

	FILE* Process::openW(string wName) {
		FILE* fd = fopen(wName.c_str(), "wb");
		if (fd < 0) {
			clog << "Process::openW Warning! ";
			clog << "can't open '" << wName << "'";
			clog << endl;
		}

		return fd;
	}

	FILE* Process::openR(string rName) {
		FILE* fd = fopen(rName.c_str(), "rb");
		if (fd == NULL) {
			clog << "Process::openR Warning! ";
			clog << "can't open '" << rName << "'";
			clog << endl;
		}

		return fd;
	}

	void Process::setProcessListener(IProcessListener* listener) {
		sigListener = listener;
	}

	void Process::run() {
		pthread_t threadId_;
		pthread_t threadIdR_;
		int rspawn;

		while (!hasCom) {
			SystemCompat::uSleep(10000);
		}

		if (processStatus == PST_NULL) {
			if (argv == NULL) {
				argv = envp;
			}
			argv[0] = (char*)objName.c_str();
			argv[1] = (char*)rCom.c_str();
			argv[2] = (char*)wCom.c_str();

			rspawn  = posix_spawn(
					&pid,
					processUri.c_str(),
					&fileActions,
					&spawnAttr,
					argv,
					envp);

			if (rspawn == 0) {
				clog << "Process::run process '";
				clog << processUri << "' successfully spawned" << endl;
				reader        = true;
				processStatus = PST_RUNNING;
				pthread_create(&threadId_, 0, Process::detachWait, this);
				pthread_detach(threadId_);

				pthread_create(&threadIdR_, 0, Process::detachReceive, this);
				pthread_detach(threadIdR_);

			} else {
				clog << "Process::run Warning! Can't spawn process '";
				clog << processUri << "'" << endl;
			}
		}
	}

	void Process::forceKill() {
		int rkill;

		sigListener = NULL;
		reader      = false;
		rkill       = kill(pid, SIGKILL);
		if (rkill != 0) {
			perror("forceKill");
		}
	}

	void Process::spawnedReady(bool ready) {
		isSpawnedReady = ready;

		tryCom();
	}

	void* Process::createFiles(void* ptr) {
		int rval;
		Process* process = (Process*)ptr;

		clog << "Process::createFiles(" << process << ")";
		clog << " creating wCom: '" << process->wCom << "'" << endl;
		if (fileExists(process->wCom)) {
			clog << "Process::createFiles(" << process << ") File '";
			clog << process->wCom << "' already exists" << endl;
			unlink(process->wCom.c_str());
		}

		rval = mkfifo(process->wCom.c_str(), S_IFIFO);
		if (rval < 0 && !fileExists(process->wCom)) {
			clog << "Process::createFiles Warning! ";
			perror("wCom");
			clog << "can't create wCom pipe '" << process->wCom << "'";
			clog << endl;
			return NULL;
		}

		clog << "Process::createFiles(" << process << ")";
		clog << " creating rCom: '" << process->rCom << "'" << endl;
		if (fileExists(process->rCom)) {
			clog << "Process::createFiles(" << process << ") File '";
			clog << process->rCom << "' already exists" << endl;
			unlink(process->rCom.c_str());

		}

		rval = mkfifo(process->rCom.c_str(), S_IFIFO);
		if (rval < 0) {
			clog << "Process::createFiles Warning! ";
			perror("rCom");
			clog << "can't create rCom pipe '" << process->rCom << "'";
			clog << endl;
			return NULL;
		}

		process->hasCom = true;
		process->wFd = openW(process->wCom);
		if (process->wFd < 0) {
			clog << "Process::createFiles Warning! ";
			clog << "can't open '" << process->wCom << "'";
			clog << endl;
		}

		process->tryCom();
		return NULL;
	}

	void* Process::detachWait(void* ptr) {
		Process* process = (Process*)ptr;
		int status;
		int type;
		int wpid;
		int ppid;
		IProcessListener* listener;

		ppid = process->pid;
		while (true) {
			wpid = waitpid(ppid, &status, WUNTRACED);
			if (wpid == ppid) {
				break;
			}
		}
		listener = process->sigListener;

        if (WIFEXITED(status)) {
        	clog << "Process::detachWait process '" << process->processUri;
        	clog << "' exited with exit status '" << WEXITSTATUS(status);
        	clog << "'" << endl;
        	printTimeStamp();
        	type = IProcessListener::PST_EXIT_OK;

        } else if (WIFSTOPPED(status)) {
        	clog << "Process::detachWait process '" << process->processUri;
        	clog << "' has not terminated correctly: '" << WSTOPSIG(status);
        	clog << endl;
        	printTimeStamp();

        	type = IProcessListener::PST_EXIT_ERROR;

        } else {
        	type = IProcessListener::PST_EXEC_SIGNAL;
        }

		if (listener != NULL) {
        	clog << "Process::detachWait process '" << process->processUri;
        	clog << "' send status '" << WEXITSTATUS(status);
        	clog << "'" << endl;
        	printTimeStamp();
			listener->receiveProcessSignal(type, status, ppid);
		}

		return NULL;
	}

	void* Process::detachReceive(void* ptr) {
		string msg;
		Process* process = (Process*)ptr;

		process->rFd = openR(process->rCom);
		if (process->rFd < 0) {
			clog << "Process::detachReceive Warning! ";
			clog << "can't open '" << process->rCom << "'";
			clog << endl;
			return NULL;
		}

		process->tryCom();

		while (process->reader) {
			msg = receiveMsg(process->rFd);
			if (msg == "ready") {
				process->spawnedReady(true);

			} else if (msg != "") {
				process->messageReceived(msg);
			}
		}

		return NULL;
	}
}
}
}
}
}
}
}
