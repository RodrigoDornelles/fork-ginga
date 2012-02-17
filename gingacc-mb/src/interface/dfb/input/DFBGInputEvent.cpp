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

#include "mb/LocalScreenManager.h"

#include "mb/interface/dfb/input/DFBGInputEvent.h"

/* macro for a safe call to DirectFB functions */
#ifndef DFBCHECK
#define DFBCHECK(x...)                                            \
{                                                                 \
	DFBResult err = x;                                            \
	if (err != DFB_OK) {                                          \
		fprintf( stderr, "%s <%d>: \n\t", __FILE__, __LINE__ );   \
		DirectFBError( #x, err );                                 \
	}                                                             \
}
#endif /*DFBCHECK*/

#include "mb/interface/CodeMap.h"

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace core {
namespace mb {
	DFBGInputEvent::DFBGInputEvent(void* event) {
		this->event = (DFBEvent*)event;

		x = 0;
		y = 0;
	}

	DFBGInputEvent::DFBGInputEvent(const int keyCode) {
		event = (DFBEvent*)(new DFBInputEvent);
		event->clazz = (DFBEventClass)DFEC_INPUT;
		((DFBInputEvent*)event)->type = DIET_KEYPRESS;
		((DFBInputEvent*)event)->flags = DIEF_KEYSYMBOL;
		((DFBInputEvent*)event)->key_symbol = (DFBInputDeviceKeySymbol)keyCode;

		x = 0;
		y = 0;
	}

	DFBGInputEvent::DFBGInputEvent(int type, void* data) {
		event = (DFBEvent*)(new DFBUserEvent);
		event->clazz = (DFBEventClass)DFEC_USER;
		((DFBUserEvent*)event)->type = type;
		((DFBUserEvent*)event)->data = data;

		x = 0;
		y = 0;
	}

	DFBGInputEvent::~DFBGInputEvent() {
		if (event != NULL) {
			delete event;
			event = NULL;
		}
	}

	void DFBGInputEvent::clearContent() {
		event = NULL;
	}

	void* DFBGInputEvent::getContent() {
		return event;
	}

	void DFBGInputEvent::setKeyCode(GingaScreenID screenId, const int keyCode) {
		int dfbCode;
		if (event != NULL && event->clazz == DFEC_INPUT) {
			dfbCode = LocalScreenManager::getInstance()->fromGingaToMB(
					screenId, keyCode);

			((DFBInputEvent*)event)->key_symbol = (DFBInputDeviceKeySymbol)(
					dfbCode);
		}
	}

	const int DFBGInputEvent::getKeyCode(GingaScreenID screenId) {
		int result = CodeMap::KEY_NULL;

		if (event != NULL && event->clazz == DFEC_INPUT) {
			if (((DFBInputEvent*)event)->type == DIET_BUTTONPRESS) {
				clog << "DFBGInputEvent::getKeyCode is returning KEY_TAP ";
				clog << "when keyCode was '";
				clog << ((DFBInputEvent*)event)->key_symbol << "'" << endl;
				return CodeMap::KEY_TAP;
			}

			result = ((DFBInputEvent*)event)->key_symbol;
			result = LocalScreenManager::getInstance()->fromMBToGinga(
					screenId, result);

			//Mapping between keyboard and remote control
			if (result == CodeMap::KEY_F1) {
				result = CodeMap::KEY_RED;

			} else if (result == CodeMap::KEY_F2) {
				result = CodeMap::KEY_GREEN;

			} else if (result == CodeMap::KEY_F3) {
				result = CodeMap::KEY_YELLOW;
		
			} else if (result == CodeMap::KEY_F4) {
				result = CodeMap::KEY_BLUE;

			} else if (result == CodeMap::KEY_F5) {
				result = CodeMap::KEY_MENU;

			} else if (result == CodeMap::KEY_F6) {
				result = CodeMap::KEY_INFO;

			} else if (result == CodeMap::KEY_F7) {
				result = CodeMap::KEY_EPG;

			} else if (result == CodeMap::KEY_PLUS_SIGN) {
				result = CodeMap::KEY_VOLUME_UP;

			} else if (result == CodeMap::KEY_MINUS_SIGN) {
				result = CodeMap::KEY_VOLUME_DOWN;

			} else if (result == CodeMap::KEY_PAGE_UP) {
				result = CodeMap::KEY_CHANNEL_UP;
	
			} else if (result == CodeMap::KEY_PAGE_DOWN) {
				result = CodeMap::KEY_CHANNEL_DOWN;

			} else if (result == CodeMap::KEY_BACKSPACE) {
				result = CodeMap::KEY_BACK;

			} else if (result == CodeMap::KEY_ESCAPE) {
				result = CodeMap::KEY_EXIT;
			}
		}

		return result;
	}

	/*void DFBGInputEvent::setType(unsigned int type) {
		if (event != NULL) {
			if (event->clazz == DFEC_INPUT) {
				((DFBInputEvent*)event)->type = (DFBInputEventType)type;

			} else if (event->clazz == DFEC_USER) {
				((DFBUserEvent*)event)->type = (DFBInputEventType)type;
			}
		}
	}*/

	void* DFBGInputEvent::getApplicationData() {
		if (event != NULL) {
			if (event->clazz == DFEC_USER) {
				return ((DFBUserEvent*)event)->data;
			}
		}
		return NULL;
	}

	unsigned int DFBGInputEvent::getType() {
		if (event != NULL) {
			if (event->clazz == DFEC_INPUT) {
				return ((DFBInputEvent*)event)->type;

			} else if (event->clazz == DFEC_USER) {
				return ((DFBUserEvent*)event)->type;
			}
		}
		return CodeMap::KEY_NULL;
	}

	bool DFBGInputEvent::isButtonPressType() {
		if (event != NULL && event->clazz == DFEC_INPUT) {
			return (((DFBInputEvent*)event)->type == DIET_BUTTONPRESS);
		}
		return false;
	}

	bool DFBGInputEvent::isMotionType() {
		if (event != NULL && event->clazz == DFEC_INPUT) {
			return (((DFBInputEvent*)event)->type == DIET_AXISMOTION);
		}
		return false;
	}

	bool DFBGInputEvent::isPressedType() {
		if (event != NULL && event->clazz == DFEC_INPUT) {
			return (((DFBInputEvent*)event)->type == DIET_KEYPRESS);
		}
		return false;
	}

	bool DFBGInputEvent::isKeyType() {
		if (event != NULL && event->clazz == DFEC_INPUT) {
			return (((DFBInputEvent*)event)->type == DIET_KEYPRESS ||
					((DFBInputEvent*)event)->type == DIET_KEYRELEASE);
		}
		return false;
	}

	bool DFBGInputEvent::isApplicationType() {
		if (event != NULL) {
			return (event->clazz == DFEC_USER);
		}
		return false;
	}

	void DFBGInputEvent::setAxisValue(int x, int y, int z) {
		this->x = x;
		this->y = y;
	}

	void DFBGInputEvent::getAxisValue(int* x, int* y, int* z) {
		if (event != NULL && ((DFBInputEvent*)event)->type == DIET_AXISMOTION) {
			if (((DFBInputEvent*)event)->flags & DIEF_AXISABS) {
				switch (((DFBInputEvent*)event)->axis) {
					case DIAI_X:
						*x = ((DFBInputEvent*)event)->axisabs;
						break;

					case DIAI_Y:
						*y = ((DFBInputEvent*)event)->axisabs;
						break;

					case DIAI_Z:
						if (z != NULL) {
							*z = ((DFBInputEvent*)event)->axisabs;
						}
						break;

					default:
						break;
				}

			} else if (((DFBInputEvent*)event)->flags & DIEF_AXISREL) {
				switch (((DFBInputEvent*)event)->axis) {
					case DIAI_X:
						*x += ((DFBInputEvent*)event)->axisrel;
						if (*x < 0) {
							*x = 0;
						}
						break;

					case DIAI_Y:
						*y += ((DFBInputEvent*)event)->axisrel;
						if (*y < 0) {
							*y = 0;
						}
						break;

					case DIAI_Z:
						if (z != NULL) {
							*z += ((DFBInputEvent*)event)->axisrel;
							if (*z < 0) {
								*z = 0;
							}
						}
						break;

					default:
						break;
				}
			}

		} else if (isButtonPressType()) {
			*x = this->x;
			*y = this->y;
		}
	}
}
}
}
}
}
}
