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

#ifndef _NCLSectionProcessor_H_
#define _NCLSectionProcessor_H_

#include "INCLSectionProcessor.h"

#include "system/SystemCompat.h"
using namespace ::ginga::system;


GINGA_DATAPROC_BEGIN

	class NCLSectionProcessor : public INCLSectionProcessor {
		private:
			INCLMetadata* metadata;
			vector<StreamData*>* dataToProcess;

		public:
			NCLSectionProcessor();
			~NCLSectionProcessor();

			string getRootUri();

		private:
			void clear();
			void clearMetadata();
			void clearDataToProcess();

			void createDirectory(string newDir);
			void addDataToProcess(char* stream, int streamSize);
			void processDataFile(char* stream, int streamSize);

		public:
			//receiving stream
			void process(char* stream, int streamSize);
			void mount();
			bool isConsolidated();

			INCLMetadata* getMetadata();

			//creating stream
			vector<StreamData*>* createNCLSections(
					string componentTag,
					string name,
					string baseUri,
					vector<string>* files,
					map<int, string>* eventMap);
	};

GINGA_DATAPROC_END

#endif //_NCLSectionProcessor_H_
