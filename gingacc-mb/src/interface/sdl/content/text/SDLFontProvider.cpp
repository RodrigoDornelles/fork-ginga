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
#include "mb/interface/sdl/SDLDeviceScreen.h"
#include "mb/interface/sdl/content/text/SDLFontProvider.h"
#include "mb/interface/sdl/output/SDLWindow.h"

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace core {
namespace mb {
	const short SDLFontProvider::A_LEFT          = 0;
	const short SDLFontProvider::A_CENTER        = 1;
	const short SDLFontProvider::A_RIGHT         = 2;

	const short SDLFontProvider::A_TOP           = 3;
	const short SDLFontProvider::A_TOP_CENTER    = 4;
	const short SDLFontProvider::A_TOP_LEFT      = 5;
	const short SDLFontProvider::A_TOP_RIGHT     = 6;

	const short SDLFontProvider::A_BOTTOM        = 7;
	const short SDLFontProvider::A_BOTTOM_CENTER = 8;
	const short SDLFontProvider::A_BOTTOM_LEFT   = 9;
	const short SDLFontProvider::A_BOTTOM_RIGHT  = 10;

	bool SDLFontProvider::initialized = false;
	short SDLFontProvider::fontRefs   = 0;

	SDLFontProvider::SDLFontProvider(
			GingaScreenID screenId, const char* fontUri, int heightInPixel) {

		isWaiting = false;
		pthread_mutex_init(&cMutex, NULL);
		pthread_cond_init(&cond, NULL);

		pthread_mutex_init(&pMutex, NULL);

		fontRefs++;

		this->fontUri   = "";
		this->myScreen  = screenId;
		this->height    = heightInPixel;
		this->font      = NULL;
		this->content   = NULL;
		this->plainText = "";
		this->coordX    = 0;
		this->coordY    = 0;
		this->align     = A_TOP_LEFT;
		this->fontUri.assign(fontUri);
	}

	SDLFontProvider::~SDLFontProvider() {
		fontRefs--;

		content = NULL;

		isWaiting = false;
		pthread_mutex_destroy(&cMutex);
		pthread_cond_destroy(&cond);

		pthread_mutex_destroy(&pMutex);

		if (font != NULL) {
			TTF_CloseFont(font);
		}

		if (fontRefs == 0) {
			TTF_Quit();
			initialized = false;
		}
	}

	void* SDLFontProvider::getContent() {
		return (void*)content;
	}

	int SDLFontProvider::getStringWidth(const char* text, int textLength) {
		return FP_AUTO_WORDWRAP;
	}

	int SDLFontProvider::getHeight() {
		int fontHeight = 0;

		return fontHeight;
	}

	void SDLFontProvider::playOver(
			ISurface* surface, const char* text, int x, int y, short align) {

		pthread_mutex_lock(&pMutex);
		plainText   = text;
		coordX      = x;
		coordY      = y;
		this->align = align;

		playOver(surface);

		pthread_mutex_unlock(&pMutex);
	}

	void SDLFontProvider::playOver(ISurface* surface) {
		this->content = surface;

		SDLDeviceScreen::addDMPToRendererList(this);
		waitNTSRenderer();
	}

	void SDLFontProvider::ntsPlayOver() {
		SDLWindow* parent;
		SDL_Surface* renderedSurface;
		IColor* fontColor = NULL;
		SDL_Color sdlColor;

		if (!initialized) {
			initialized = true;
			if (TTF_Init() < 0) {
				clog << "SDLFontProvider::SDLFontProvider ";
				clog << "Couldn't initialize TTF: " << SDL_GetError();
				clog << endl;
			}
		}

		if (content != NULL && LocalScreenManager::getInstance()->hasSurface(
				myScreen, content)) {

			fontColor = content->getColor();

			if (fontColor != NULL) {
				sdlColor.r = fontColor->getR();
				sdlColor.g = fontColor->getG();
				sdlColor.b = fontColor->getB();

			} else {
				sdlColor.r = 0x00;
				sdlColor.g = 0x00;
				sdlColor.b = 0x00;
			}

			if (font == NULL) {
				font = TTF_OpenFont(fontUri.c_str(), height);
			}

			renderedSurface = TTF_RenderText_Solid(
					font,
					plainText.c_str(),
					sdlColor);

			content->setContent((void*)renderedSurface);

			parent = (SDLWindow*)(content->getParent());
			if (parent != NULL) {
	//			parent->renderFrom(content);
				parent->setRenderedSurface(renderedSurface);
			}
		}

		ntsRenderer();
	}

	void SDLFontProvider::waitNTSRenderer() {
		isWaiting = true;
		pthread_mutex_lock(&cMutex);
		pthread_cond_wait(&cond, &cMutex);
		isWaiting = false;
		pthread_mutex_unlock(&cMutex);
	}

	bool SDLFontProvider::ntsRenderer() {
		if (isWaiting) {
			pthread_cond_signal(&cond);
			return true;
		}
		return false;
	}
}
}
}
}
}
}

extern "C" ::br::pucrio::telemidia::ginga::core::mb::IFontProvider*
		createSDLFontProvider(
				GingaScreenID screenId,
				const char* fontUri,
				int heightInPixel) {

	return (new ::br::pucrio::telemidia::ginga::core::mb::
			SDLFontProvider(screenId, fontUri, heightInPixel));
}

extern "C" void destroySDLFontProvider(
		::br::pucrio::telemidia::ginga::core::mb::IFontProvider* fp) {

	delete fp;
}
