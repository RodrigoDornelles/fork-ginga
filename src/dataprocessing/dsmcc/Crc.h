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

#ifndef _header_dsmcc_crc_
#define _header_dsmcc_crc_

#include "system/SystemCompat.h"
using namespace ::br::pucrio::telemidia::ginga::core::system::compat;

#include <stdio.h>
#include <string.h>

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace core {
namespace dataprocessing {
namespace dsmcc {
namespace npt {

class Crc32 {
	private:

	protected:
		unsigned int dsmcc_crc_tab[0x100];

	public:
		Crc32();
		~Crc32();

		unsigned int crc (char *data, unsigned int len);

};

}
}
}
}
}
}
}
}

#endif
