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

#include "DataProcessor.h"
#include "dsmcc/StreamEvent.h"
#include "dsmcc/DSMCCSectionPayload.h"
#include "tsparser/AIT.h"

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace core {
namespace dataprocessing {
	DataProcessor::DataProcessor() : Thread() {
		filterManager   = new FilterManager();
		nptProcessor    = NULL;
		sdl             = NULL;
		removeOCFilter  = false;
		demux           = NULL;
		ait             = NULL;
		nptPrinter      = false;
		string tempDir;

		startThread();

		tempDir = SystemCompat::getTemporaryDir() + "ginga";
		SystemCompat::makeDir(tempDir.c_str(), 0777);
		tempDir += SystemCompat::getIUriD() + "carousel";
		SystemCompat::makeDir(tempDir.c_str(), 0777);
		tempDir += SystemCompat::getIUriD() + "modules";
		SystemCompat::makeDir(tempDir.c_str(), 0777);

		//TODO: remove all garbage from epg processor before start using it
		//epgProcessor = EPGProcessor::getInstance();
		//epgProcessor->setDataProcessor(this);
	}

	DataProcessor::~DataProcessor() {
		running = false;

		map<string, set<IStreamEventListener*>*>::iterator it;
		set<IStreamEventListener*>::iterator its;
		it = eventListeners.begin();
		while (it != eventListeners.end()) {
			its = it->second->begin();
			while (its != it->second->end()) {
				delete *its;
				++its;
			}
			delete it->second;
			++it;
		}
		eventListeners.clear();
		objectListeners.clear();

		if (filterManager != NULL) {
			delete filterManager;
			filterManager = NULL;
		}

		processors.clear();
		processedIds.clear();

		if (nptProcessor != NULL) {
			delete nptProcessor;
			nptProcessor = NULL;
		}

		sections.clear();

		deleteAIT();

		//TODO: remove all garbage from epg processor before start using it
		/*if (epgProcessor != NULL) {
			epgProcessor->release();
			epgProcessor = NULL;
		}*/
	}

	void DataProcessor::deleteAIT() {
		if (ait != NULL) {
			delete ait;
			ait = NULL;
		}
	}

	void DataProcessor::setNptPrinter(bool nptPrinter) {
		this->nptPrinter = nptPrinter;

		if (nptProcessor != NULL) {
			nptProcessor->setNptPrinter(nptPrinter);

		} else {
			cout << "DataProcessor::printNpt Warning! NULL nptProcessor";
			cout << endl;
		}
	}

	bool DataProcessor::applicationInfoMounted(IAIT* ait) {
		if (sdl != NULL) {
			return sdl->applicationInfoMounted(ait);

		} else {
			deleteAIT();
		}

		return false;
	}

	void DataProcessor::serviceDomainMounted(
			string mountPoint,
			map<string, string>* names,
			map<string, string>* paths) {

		if (removeOCFilter && demux != NULL) {
			clog << "DataProcessor::run requesting ";
			clog << "OC filter destroy" << endl;
			filterManager->destroyFilter(demux, STREAM_TYPE_DSMCC_TYPE_B);
		}

		if (sdl != NULL) {
			sdl->serviceDomainMounted(mountPoint, names, paths);
		}

	}

	void DataProcessor::setDemuxer(IDemuxer* demux) {
		this->demux = demux;
	}

	void DataProcessor::removeOCFilterAfterMount(bool removeIt) {
		removeOCFilter = removeIt;
	}

	void DataProcessor::setSTCProvider(ISTCProvider* stcProvider) {
		assert(nptProcessor == NULL);

		nptProcessor = new NPTProcessor(stcProvider);
	}

	ITimeBaseProvider* DataProcessor::getNPTProvider() {
		return nptProcessor;
	}

	void DataProcessor::createStreamTypeSectionFilter(short streamType) {
		filterManager->createStreamTypeSectionFilter(streamType, demux, this);
	}

	void DataProcessor::createPidSectionFilter(int pid) {
		filterManager->createPidSectionFilter(pid, demux, this);
	}

	void DataProcessor::addSEListener(
			string eventType, IStreamEventListener* listener) {

		map<string, set<IStreamEventListener*>*>::iterator i;
		set<IStreamEventListener*>* listeners;

		clog << "DataProcessor::addSEListener" << endl;
		i = eventListeners.find(eventType);
		if (i != eventListeners.end()) {
			listeners = i->second;
			if (listeners == NULL) {
				listeners = new set<IStreamEventListener*>;
				eventListeners[eventType] = listeners;
			}

			listeners->insert(listener);

		} else {
			listeners = new set<IStreamEventListener*>;
			listeners->insert(listener);
			eventListeners[eventType] = listeners;
		}
	}

	void DataProcessor::removeSEListener(
			string eventType, IStreamEventListener* listener) {

		map<string, set<IStreamEventListener*>*>::iterator i;
		set<IStreamEventListener*>::iterator j;
		set<IStreamEventListener*>* listeners;

		i = eventListeners.find(eventType);
		if (i != eventListeners.end()) {
			listeners = i->second;
			if (listeners != NULL) {
				j = listeners->find(listener);
				if (j != listeners->end()) {
					listeners->erase(j);
				}
			}
		}
	}

	void DataProcessor::setServiceDomainListener(
			IServiceDomainListener* listener) {

		sdl = listener;
	}

	void DataProcessor::addObjectListener(IObjectListener* l) {
		objectListeners.insert(l);
	}

	void DataProcessor::removeObjectListener(IObjectListener* l) {
		set<IObjectListener*>::iterator i;

		i = objectListeners.find(l);
		if (i != objectListeners.end()) {
			objectListeners.erase(i);
		}
	}

	void* DataProcessor::notifySEListener(void* ptr) {
		struct notifyData* data;
		IStreamEventListener* listener;
		IStreamEvent* se;

		data = (struct notifyData*)ptr;
		se = data->se;
		listener = data->listener;

		delete data;
		data = NULL;

		listener->receiveStreamEvent(se);
		return NULL;
	}

	void DataProcessor::notifySEListeners(IStreamEvent* se) {
		map<string, set<IStreamEventListener*>*>::iterator i;
		set<IStreamEventListener*>* listeners;
		set<IStreamEventListener*>::iterator j;
		pthread_t notifyThreadId_;
		struct notifyData* data = NULL;
		string eventName = se->getEventName();

		clog << "DataProcessor::notifySEListeners for eventName '";
		clog << eventName << "'" << endl;
		if (eventListeners.count(eventName) != 0) {
			listeners = eventListeners[eventName];
			j = listeners->begin();
			while (j != listeners->end()) {
				data = new struct notifyData;
				data->listener = *j;
				data->se = se;

				pthread_create(
						&notifyThreadId_,
						0, DataProcessor::notifySEListener, (void*)data);

				pthread_detach(notifyThreadId_);

				++j;
			}
		}
	}

	void DataProcessor::receiveSection(ITransportSection* section) {
		IStreamEvent* se;
		string sectionName;
		set<unsigned int>::iterator i;
		char* payload;
		short tableId;

		//TODO: clean this mess
		DSMCCSectionPayload* dsmccSection;

		assert(section != NULL);

		tableId = section->getTableId();

		//stream event
		if (tableId == DDE_TID) {
			//filterManager->addProcessedSection(section->getSectionName());

			payload = (char*)(section->getPayload());

			/*clog << "DataProcessor::receiveSection DSM-CC descriptor";
			clog << "tag = '" << (payload[0] & 0xFF) << "'" << endl;*/

			if ((payload[0] & 0xFF) == IMpegDescriptor::STR_EVENT_TAG ||
					(payload[0] & 0xFF) == 0x1a) {
				static char lastSeVer = -1;
				if (lastSeVer == section->getVersionNumber()) {
					delete section;
					return;
				}
				lastSeVer = section->getVersionNumber();
				se = new StreamEvent(
						section->getPayload(), section->getPayloadSize());

				//i = processedIds.find(se->getId());
				//if (i == processedIds.end()) {377
					//clog << "DataProcessor::receiveSection STE" << endl;

					processedIds.insert(se->getId());
					//TODO: get stream event object from oc
					se->setEventName("gingaEditingCommands");

					// notify event listeners
					notifySEListeners(se);

				//} else {
					//delete se;
				//}

			} else if ((payload[0] & 0xFF) ==
							IMpegDescriptor::NPT_REFERENCE_TAG ||
					(payload[0] & 0xFF) == IMpegDescriptor::NPT_ENDPOINT_TAG) {

				if (nptPrinter) {
					cout << "FOUND NEW NPT DSM-CC SECTION" << endl;
				}

				//TODO: we have to organize this mess
				dsmccSection = new DSMCCSectionPayload(
						payload, section->getPayloadSize());

				nptProcessor->decodeDescriptors(
						dsmccSection->getDsmccDescritorList());

				delete dsmccSection;
			}

			delete section;
			section = NULL;

		//object carousel 0x3B = MSG, 0x3C = DDB
		} else if (tableId == OCI_TID || tableId == OCD_TID) {
			lock();
			//clog << "DataProcessor::receiveSection OC" << endl;
			sections.push_back(section);
			unlock();
			unlockConditionSatisfied();

		//AIT
		} else if (tableId == AIT_TID) {

			if (ait != NULL) {
				if (ait->getSectionName() == section->getSectionName()) {
					delete section;
					section = NULL;

					return;
				}

				delete ait;
			}

			ait = new AIT();

			ait->setSectionName(section->getSectionName());
			ait->setApplicationType(section->getExtensionId());
			clog << "DataProcessor::receiveSection AIT calling process";
			clog << endl;
			ait->process(section->getPayload(), section->getPayloadSize());

			applicationInfoMounted(ait);
		//SDT
		} else if (tableId == SDT_TID) {
			//clog << "DataProcessor::receiveSection SDT" << endl;
			//TODO: remove all garbage from epg processor before start using it
			//epgProcessor->decodeSdtSection(section);
			delete section;
			section = NULL;

		// EIT present/following and schedule
		} else if ( tableId == EIT_TID || //EIT present/following in actual TS
					(tableId >= 0x50 && tableId <= 0x5F)) { //EIT schedule in actual TS
				
			/*TODO: TS files don't have EITs p/f and sched in other TS.
			 tableId == 0x4F (p/f) and tableId >= 0x60 && tableId <= 0x6F
			 (schedule)
			*/

			//TODO: remove all garbage from epg processor before start using it
			//epgProcessor->decodeEitSection(section);
			delete section;
			section = NULL;

		//CDT
		} else if (tableId == CDT_TID) {
			clog << "DataProcessor::receiveSection CDT" << endl;
			sectionName = section->getSectionName();
			//TODO: TS files don't have any CDT sections.

		} else if (tableId == 0x73) {
			clog << "DataProcessor::receiveSection TOT FOUND!!!" << endl;
			//TODO: remove all garbage from epg processor before start using it
			//epgProcessor->decodeTot(section);

		} else if (nptPrinter) {
			cout << "NPT PRINTER WARNING! FOUND '" << tableId << "TABLE ID! ";
			cout << "EXPECTING SECTION WITH TABLE ID '";
			cout << DDE_TID << "'! ";
		}
	}

	void DataProcessor::updateChannelStatus(short newStatus, IChannel* channel) {
		if (newStatus == TS_LOOP_DETECTED) {
			deleteAIT();
		}
	}

	bool DataProcessor::isReady() {
		return true;
	}

	void DataProcessor::run() {
		int pid;
		ITransportSection* section;
		DsmccMessageHeader* message;
		ServiceDomain* sd = NULL;
		string sectionName;
		MessageProcessor* processor;
		bool hasSection;

		running = true;
		while (running) {
			//clog << "DataProcessor::run checking tasks" << endl;
			lock();
			if (sections.empty()) {
				unlock();
				waitForUnlockCondition();

			} else {
				section = *(sections.begin());
				sections.erase(sections.begin());
				unlock();

				do {
					//clog << "DataProcessor::run call FM->processSec" << endl;

					//we must to acquire pid and section name before process
					//the section
					pid         = section->getESId();
					sectionName = section->getSectionName();

					if (filterManager->processSection(section)) {
						message = new DsmccMessageHeader();
						if (message->readMessageFromFile(sectionName, pid) == 0) {
							if (processors.count(pid) == 0) {
								processor = new MessageProcessor(pid);
								processors[pid] = processor;

							} else {
								processor = processors[pid];
							}
							sd = processor->pushMessage(message);
							if (sd != NULL) {
								sd->setObjectsListeners(&objectListeners);
								sd->setServiceDomainListener(this);
								processor->checkTasks();
								filterManager->setInfo(sd->getInfo());
								filterManager->setBlockSize(sd->getBlockSize());
							}

						}
					}

					lock();
					hasSection = !sections.empty();
					if (hasSection) {
						section = *(sections.begin());
						sections.erase(sections.begin());
					}
					unlock();

				} while (hasSection);
			}
		}

		clog << "DataProcessor::run all done!" << endl;
	}
}
}
}
}
}
}

extern "C" ::br::pucrio::telemidia::ginga::core::dataprocessing::IDataProcessor*
		createDP() {

	return (new ::br::pucrio::telemidia::ginga::core::dataprocessing::
			DataProcessor());
}

extern "C" void destroyDP(::br::pucrio::telemidia::ginga::core::
		dataprocessing::IDataProcessor* dp) {

	delete dp;
}
