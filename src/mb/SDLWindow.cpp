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

#include "util/Color.h"

#include "LocalScreenManager.h"
#include "SDLWindow.h"
#include "SDLSurface.h"
#include "SDLDeviceScreen.h"
#include "SDLConvert.h"

extern "C" {
#include <stdlib.h>
}

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace core {
namespace mb {
	SDLWindow::SDLWindow(
			GingaWindowID windowID,
			GingaWindowID parentWindowID,
			GingaScreenID screenId,
			int x, int y, int width, int height,
			float z) {

		initialize(
				windowID,
				parentWindowID,
				screenId,
				x, y, width, height,
				z);
	}

	SDLWindow::~SDLWindow() {
		vector<ISurface*>::iterator i;

		lock();
		lockChilds();
		if (childSurface != NULL) {
			childSurface->setParentWindow(NULL);
		}

		unlockChilds();

		lockSurface();
		curSur = NULL;

		releaseWinISur();

		set<IWindow*>::iterator j;
		j = mirrors.begin();
		while (j != mirrors.end()) {
			(*j)->setMirrorSrc(NULL);
			mirrors.erase(j);
			j = mirrors.begin();
		}
		mirrors.clear();

		if (mirrorSrc != NULL) {
			mirrorSrc->removeMirror(this);
			mirrorSrc = NULL;
		}
		unlockSurface();

		releaseBGColor();
		releaseBorderColor();
		releaseWinColor();
		releaseColorKey();

		// release window will delete texture
		ScreenManagerFactory::getInstance()->releaseWindow(myScreen, this);

		Thread::mutexDestroy(&mutexC);

		this->isWaiting = false;
	    Thread::mutexDestroy(&cMutex);
	    Thread::condDestroy(&cond);

	    Thread::mutexDestroy(&rMutex);

	    unlock();
	    Thread::mutexDestroy(&mutex);

		clog << "SDLWindow::~SDLWindow(" << this << ") all done" << endl;
	}

	void SDLWindow::initialize(
			GingaWindowID windowID,
			GingaWindowID parentWindowID,
			GingaScreenID screenId,
			int x, int y, int w, int h,
			float z) {

		this->windowId = windowID;

		this->texture           = NULL;
		this->winISur           = 0;
		this->curSur            = NULL;

		this->textureUpdate     = false;
		this->textureOwner      = true;

		this->borderWidth       = 0;
		this->bgColor           = NULL;
		this->borderColor       = NULL;
		this->winColor          = NULL;
		this->colorKey          = NULL;

		this->myScreen          = screenId;

		this->rect.x            = x;
		this->rect.y            = y;
		this->rect.w            = w;
		this->rect.h            = h;
		this->z                 = z;
		this->ghost             = false;
		this->visible           = false;
		this->childSurface      = NULL;
		this->fit               = true;
		this->stretch           = true;
		this->caps              = 0;
		this->transparencyValue = 0x00;
		this->mirrorSrc         = NULL;

		Thread::mutexInit(&mutex);
		Thread::mutexInit(&mutexC);
		Thread::mutexInit(&texMutex);
		Thread::mutexInit(&surMutex, true);

		this->isWaiting = false;
	    Thread::mutexInit(&cMutex);
	    Thread::condInit(&cond, NULL);

	    Thread::mutexInit(&rMutex);
	}

	void SDLWindow::releaseWinISur() {
		if (winISur != 0) {
			ScreenManagerFactory::getInstance()->deleteSurface(winISur);
			winISur = 0;
		}
	}

	void SDLWindow::releaseBGColor() {
		if (bgColor != NULL) {
			delete bgColor;
			bgColor = NULL;
		}
	}

	void SDLWindow::releaseBorderColor() {
		if (borderColor != NULL) {
			delete borderColor;
			borderColor = NULL;
		}
	}

	void SDLWindow::releaseWinColor() {
		if (winColor != NULL) {
			delete winColor;
			winColor = NULL;
		}
	}

	void SDLWindow::releaseColorKey() {
		if (colorKey != NULL) {
			delete colorKey;
			colorKey = NULL;
		}
	}

	void SDLWindow::addMirror(IWindow* window) {
		assert(window != this);
		lockSurface();
		mirrors.insert(window);
		unlockSurface();
	}

	bool SDLWindow::removeMirror(IWindow* window) {
		set<IWindow*>::iterator i;

		lockSurface();
		i = mirrors.find(window);
		if (i != mirrors.end()) {
			mirrors.erase(i);
			unlockSurface();
			return true;
		}
		unlockSurface();
		return false;
	}

	void SDLWindow::setMirrorSrc(IWindow* mirrorSrc) {
		lockSurface();
		this->mirrorSrc = mirrorSrc;
		if (this->mirrorSrc != NULL) {
			this->mirrorSrc->addMirror(this);
		}
		unlockSurface();
	}

	IWindow* SDLWindow::getMirrorSrc() {
		return mirrorSrc;
	}

	void SDLWindow::setBgColor(int r, int g, int b, int alpha) {
		releaseBGColor();

		if (r < 0 || g < 0 || b < 0) {
			return;
		}

		bgColor = new Color(r, g, b, alpha);
	}

	IColor* SDLWindow::getBgColor() {
		return bgColor;
	}

	void SDLWindow::setColorKey(int r, int g, int b) {
		releaseColorKey();

		if (r < 0 || g < 0 || b < 0) {
			return;
		}

		colorKey = new Color(r, g, b);
	}

	IColor* SDLWindow::getColorKey() {
		return colorKey;
	}

	void SDLWindow::setWindowColor(int r, int g, int b, int alpha) {
		releaseWinColor();

		if (r < 0 || g < 0 || b < 0) {
			return;
		}

		winColor = new Color(r, g, b, alpha);
	}

	IColor* SDLWindow::getWindowColor() {
		return winColor;
	}

	void SDLWindow::setBorder(int r, int g, int b, int alpha, int bWidth) {
		releaseBorderColor();

		borderWidth = bWidth;

		if (r < 0 || g < 0 || b < 0) {
			return;
		}

		borderColor = new Color(r, g, b, alpha);
	}

	void SDLWindow::getBorder(
			int* r, int* g, int* b, int* alpha, int* bWidth) {

		if (borderColor != NULL) {
			*r      = borderColor->getR();
			*g      = borderColor->getG();
			*b      = borderColor->getB();
			*alpha  = borderColor->getAlpha();
			*bWidth = borderWidth;

		} else {
			*r      = 0;
			*g      = 0;
			*b      = 0;
			*alpha  = 0;
			*bWidth = 0;
		}
	}

	GingaScreenID SDLWindow::getScreen() {
		return myScreen;
	}

	void SDLWindow::revertContent() {
		lockSurface();
		releaseWinISur();
		unlockSurface();
	}

	void SDLWindow::setChildSurface(ISurface* iSur) {
		lockSurface();
		releaseWinISur();
		this->childSurface = iSur;
		unlockSurface();
	}

	int SDLWindow::getCap(string cap) {
		return 0;
	}

	void SDLWindow::setCaps(int caps) {
		this->caps = caps;
	}

	void SDLWindow::addCaps(int capability) {
		this->caps = (this->caps | capability);
	}

	int SDLWindow::getCaps() {
		return caps;
	}

	void SDLWindow::draw() {

	}

	void SDLWindow::setBounds(int posX, int posY, int w, int h) {
		this->rect.x = posX;
		this->rect.y = posY;
		this->rect.w = w;
		this->rect.h = h;
	}

	void SDLWindow::moveTo(int posX, int posY) {
		this->rect.x = posX;
		this->rect.y = posY;
	}

	void SDLWindow::resize(int width, int height) {
		this->rect.w = width;
		this->rect.h = height;
	}

	void SDLWindow::raiseToTop() {
//		SDLDeviceScreen::updateWindowState(
//				myScreen, this, SDLDeviceScreen::SUW_RAISETOTOP);
	}

	void SDLWindow::lowerToBottom() {
//		SDLDeviceScreen::updateWindowState(
//				myScreen, this, SDLDeviceScreen::SUW_LOWERTOBOTTOM);
	}

	void SDLWindow::setCurrentTransparency(int alpha) {
		if (alpha != 255) {
			this->visible = true;

		} else {
			this->visible = false;
		}

		transparencyValue = alpha;

		lockTexture();
		if (texture != NULL) {
			if (SDL_SetTextureAlphaMod(texture, 255 - alpha) < 0) {
				clog << "SDLWindow::setCurrentTransparency SDL error: '";
				clog << SDL_GetError() << "'" << endl;
			}
		}
		unlockTexture();
	}

	int SDLWindow::getTransparencyValue() {
		return this->transparencyValue;
	}

	GingaWindowID SDLWindow::getId() {
		GingaWindowID myId;

		myId = windowId;
		return myId;
	}

	void SDLWindow::show() {
		this->visible = true;
//		SDLDeviceScreen::updateWindowState(
//				myScreen, this, SDLDeviceScreen::SUW_SHOW);
	}

	void SDLWindow::hide() {
		visible = false;
//		SDLDeviceScreen::updateWindowState(
//				myScreen, this, SDLDeviceScreen::SUW_HIDE);
	}

	int SDLWindow::getX() {
		return this->rect.x;
	}

	int SDLWindow::getY() {
		return this->rect.y;
	}

	int SDLWindow::getW() {
		return this->rect.w;
	}

	int SDLWindow::getH() {
		return this->rect.h;
	}

	float SDLWindow::getZ() {
		return z;
	}

	void SDLWindow::setX(int x) {
		this->rect.x = x;
	}

	void SDLWindow::setY(int y) {
		this->rect.y = y;
	}

	void SDLWindow::setW(int w) {
		this->rect.w = w;
	}

	void SDLWindow::setH(int h) {
		this->rect.h = h;
	}

	void SDLWindow::setZ(float z) {
		float oldZ = this->z;

		this->z = z;

		SDLDeviceScreen::updateRenderMap(myScreen, this, oldZ, z);
	}

	bool SDLWindow::isGhostWindow() {
		return ghost;
	}

	void SDLWindow::setGhostWindow(bool ghost) {
		this->ghost = ghost;
	}

	bool SDLWindow::isVisible() {
		return this->visible;
	}

	void SDLWindow::validate() {
		lockSurface();
		unprotectedValidate();
		unlockSurface();
	}

	void SDLWindow::unprotectedValidate() {
		if (winISur != 0) {
			ScreenManagerFactory::getInstance()->flipSurface(winISur);
			curSur = (SDL_Surface*)
					(ScreenManagerFactory::getInstance()->getSurfaceContent(winISur));
			textureUpdate = true;

		} else if (childSurface != NULL) {
			childSurface->flip();
			textureUpdate = true;
		}
	}

	vector<DrawData*>* SDLWindow::createDrawDataList() {
		vector<DrawData*>* dd = NULL;

		lockChilds();
		if (childSurface != NULL &&
				ScreenManagerFactory::getInstance()->hasSurface(
						myScreen, childSurface->getId())) {

			dd = ((SDLSurface*)childSurface)->createDrawDataList();
		}
		unlockChilds();

		return dd;
	}

	void SDLWindow::setStretch(bool stretchTo) {
		this->stretch = stretchTo;
	}

	bool SDLWindow::getStretch() {
		return this->stretch;
	}

	void SDLWindow::setFit(bool fitTo) {
		this->fit = fitTo;
	}

	bool SDLWindow::getFit() {
		return this->fit;
	}

	void SDLWindow::clearContent() {
		lockSurface();
		if (curSur != NULL) {
			if (SDL_FillRect(
					curSur, NULL, SDL_MapRGBA(curSur->format, 0, 0, 0, 0)) < 0) {

				clog << "SDLWindow::clearContent SDL error: '";
				clog << SDL_GetError() << "'" << endl;
			}
			textureUpdate = true;
		}
		unlockSurface();
	}

	void SDLWindow::setRenderedSurface(SDL_Surface* uSur) {
		lockSurface();
		curSur = uSur;
		if (curSur != NULL) {
			textureUpdate = true;
		}
		unlockSurface();
	}

	void* SDLWindow::getContent() {
		return curSur;
	}

	void SDLWindow::setTexture(SDL_Texture* texture) {
		lockTexture();

		if (this->texture == texture) {
			unlockTexture();
			return;
		}

		if (textureOwner && this->texture != NULL) {
			SDLDeviceScreen::createReleaseContainer(NULL, this->texture, NULL);
		}

		if (texture == NULL) {
			textureOwner = true;

		} else {
			textureOwner = false;
		}

		this->texture = texture;
		unlockTexture();
	}

	SDL_Texture* SDLWindow::getTexture(SDL_Renderer* renderer) {
		SDL_Texture* uTex;

		lockTexture();
		if (renderer != NULL) {
			if (textureOwner && textureUpdate && texture != NULL) {
				SDLDeviceScreen::releaseTexture(texture);
				textureUpdate = false;
				texture = NULL;
			}

			if (texture == NULL) {
				lockSurface();
				if (curSur != NULL) {
					textureOwner = true;
					texture = SDLDeviceScreen::createTextureFromSurface(
							renderer, curSur);
				}
				unlockSurface();
			}
		}

		uTex = texture;
		unlockTexture();

		return uTex;
	}

	bool SDLWindow::isTextureOwner(SDL_Texture* texture) {
		bool itIs = false;
		if (texture != NULL && this->texture == texture && textureOwner) {
			itIs = true;
		}

		return itIs;
	}

	bool SDLWindow::isMine(ISurface* surface) {
		bool itIs = false;

		if (surface != NULL && surface->getSurfaceContent() != NULL) {
			if (surface->getId() == winISur || surface == childSurface) {
				itIs = true;
			}
		}

		return itIs;
	}

	void SDLWindow::renderImgFile(string serializedImageUrl) {
		IImageProvider* img;
		GingaProviderID providerId;
		GingaSurfaceID surId;

		providerId = ScreenManagerFactory::getInstance()->createImageProvider(
				myScreen, serializedImageUrl.c_str());

		IMediaProvider *mediaProvider =
				ScreenManagerFactory::getInstance()->getIMediaProviderFromId(providerId);
		if (mediaProvider &&
				mediaProvider->getType() == IMediaProvider::ImageProvider)
			img = (IImageProvider*) mediaProvider;

		surId = ScreenManagerFactory::getInstance()->createSurface(myScreen);
		img->playOver(surId);

		lockSurface();
		curSur = (SDL_Surface*)ScreenManagerFactory::getInstance()->
				getSurfaceContent(surId);
		unlockSurface();

		textureUpdate = true;

		ScreenManagerFactory::getInstance()->releaseImageProvider(myScreen,
		                                                        img->getId());
		ScreenManagerFactory::getInstance()->deleteSurface(surId);
	}

	void SDLWindow::renderFrom(ISurface* surface) {
		SDL_Surface* contentSurface;

		Thread::mutexLock(&rMutex);
		contentSurface = (SDL_Surface*)surface->getSurfaceContent();
		if (contentSurface == NULL) {
			clog << "SDLWindow::renderFrom(" << this;
			clog << ") Warning! NULL underlying ";
			clog << "surface!" << endl;
			Thread::mutexUnlock(&rMutex);
			return;
		}

		lockSurface();
		if (!isMine(surface)) {
			releaseWinISur();
			winISur = ScreenManagerFactory::getInstance()->createSurface(
					myScreen, contentSurface->w, contentSurface->h);

			ScreenManagerFactory::getInstance()->blitSurface(winISur, 0, 0,
			                                               surface->getId());
			ScreenManagerFactory::getInstance()->flipSurface(winISur);

			curSur = (SDL_Surface*)
					ScreenManagerFactory::getInstance()->getSurfaceContent(winISur);
			textureUpdate = true;

		} else {
			curSur = contentSurface;
			textureUpdate = true;
		}
		unlockSurface();

		Thread::mutexUnlock(&rMutex);
	}

	void SDLWindow::blit(IWindow* src) {
		/*SDL_Window* srcWin;
		SDL_Surface* srcSur;

		if (src != NULL) {
			srcWin = (SDL_Window*)(src->getSurfaceContent());
			srcWin->GetSurface(srcWin, &srcSur);

			if (winSur != NULL) {
				SDLCHECK(winSur->SetBlittingFlags(
						winSur,
						(SDLSurfaceBlittingFlags)DSBLIT_BLEND_ALPHACHANNEL));

				SDLCHECK(winSur->Blit(
						winSur, srcSur, NULL, src->getX(), src->getY()));
			}
		}*/
	}

	void SDLWindow::stretchBlit(IWindow* src) {
		//SDLRectangle rect, *r = NULL;
		//SDL_Window* srcWin;
		//SDL_Surface* srcSur;

		/*if (src != NULL) {
			srcWin = (SDL_Window*)(src->getSurfaceContent());
			srcWin->GetSurface(srcWin, &srcSur);

			if (winSur != NULL) {
				SDLCHECK(winSur->SetBlittingFlags(
						winSur,
						(SDLSurfaceBlittingFlags)DSBLIT_BLEND_ALPHACHANNEL));

				SDLCHECK(winSur->StretchBlit(winSur, srcSur, NULL, NULL));
			}
		}*/
	}

	string SDLWindow::getDumpFileUri(int quality, int dumpW, int dumpH) {
		string uri;
		SDL_Surface* dumpUSur;
		bool freeSurface = false;

		lockSurface();
		if (curSur != NULL) {
			dumpUSur = curSur;
		} else if (texture != NULL) {
			dumpUSur = SDLDeviceScreen::createUnderlyingSurfaceFromTexture(
					texture);
			freeSurface = true;
		} else {
			unlockSurface();
			clog << "SDLWindow::getDumpFileUri window '";
			clog << (unsigned long)getId() << "' can't dump: ";
			clog << "NULL surface and NULL texture" << endl;
			return "";
		}

		SDLDeviceScreen::lockSDL();
		uri = SystemCompat::getTemporaryDir() +
				"dump_" + itos((unsigned long)windowId) + ".jpg";
		int ret = SDLConvert::convertSurfaceToJPEG(uri.c_str(), dumpUSur,quality);
		if (ret == -1)
			uri ="";
		if (freeSurface)
			SDLDeviceScreen::createReleaseContainer(dumpUSur, NULL, NULL);
		SDLDeviceScreen::unlockSDL();

		unlockSurface();
		return uri;
	}

	void SDLWindow::lock() {
		Thread::mutexLock(&mutex);
	}

	void SDLWindow::unlock() {
		Thread::mutexUnlock(&mutex);
	}

	void SDLWindow::lockChilds() {
		Thread::mutexLock(&mutexC);
	}

	void SDLWindow::unlockChilds() {
		Thread::mutexUnlock(&mutexC);
	}

	bool SDLWindow::rendered() {
		if (isWaiting) {
			Thread::condSignal(&cond);
			return true;
		}
		return false;
	}

	void SDLWindow::waitRenderer() {
		isWaiting = true;
		Thread::mutexLock(&cMutex);
		Thread::condWait(&cond, &cMutex);
		isWaiting = false;
		Thread::mutexUnlock(&cMutex);
	}

	void SDLWindow::lockTexture() {
		Thread::mutexLock(&texMutex);
	}

	void SDLWindow::unlockTexture() {
		Thread::mutexUnlock(&texMutex);
	}

	void SDLWindow::lockSurface() {
		Thread::mutexLock(&surMutex);
	}

	void SDLWindow::unlockSurface() {
		Thread::mutexUnlock(&surMutex);
	}
}
}
}
}
}
}