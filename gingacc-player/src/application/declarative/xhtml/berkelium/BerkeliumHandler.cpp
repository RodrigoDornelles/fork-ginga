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

#include "../../../../../config.h"

#include "player/BerkeliumHandler.h"
#include "player/PlayersComponentSupport.h"

namespace br {
namespace pucrio {
namespace telemidia {
namespace ginga {
namespace core {
namespace player {
	BerkeliumHandler::BerkeliumHandler(GingaScreenID myScreen) {
#if HAVE_COMPSUPPORT
		dm = ((LocalScreenManagerCreator*)(
				cm->getObject("LocalScreenManager")))();

#else
		dm = LocalScreenManager::getInstance();
#endif

		this->myScreen = myScreen;

		im = dm->getInputManager(myScreen);
		surface = dm->createSurface(myScreen);

		w = 0;
		h = 0;

		isValid = false;
	}

	BerkeliumHandler::~BerkeliumHandler() {
		clog << "BerkeliumHandler::~BerkeliumHandler " << endl;
		if (isValid) {
			isValid = false;
			bWindow->stop();
			bWindow->setDelegate(NULL);
			bWindow->del();
		}

		if (context != NULL) {
			context->destroy();
			context = NULL;
		}

		if (im != NULL) {
			im->removeInputEventListener(this);
		}
		//Caution: Surface is deleted by Player
	}

	void BerkeliumHandler::setKeyHandler(bool handler) {
		if (im != NULL && isValid) {
			if (handler) {
				bWindow->focus();

				im->addInputEventListener(this, NULL);
				im->addMotionEventListener(this);

			} else {
				bWindow->unfocus();
				im->removeInputEventListener(this);
				im->removeMotionEventListener(this);
			}
		}
	}

	void BerkeliumHandler::setContext(Context* context) {
		this->context = context;
	}

	void BerkeliumHandler::setWindow(std::auto_ptr<Window> window) {
		bWindow = window;
		isValid = true;
	}

	void BerkeliumHandler::getSize(int* w, int* h) {
		*w = this->w;
		*h = this->h;
	}

	void BerkeliumHandler::setSize(int w, int h) {
		this->w = w;
		this->h = h;

		if (isValid) {
			bWindow->resize(w, h);
		}
	}

	void BerkeliumHandler::setUrl(string url) {
		mURL = url;
	}

	string BerkeliumHandler::getUrl() {
		return mURL;
	}

	ISurface* BerkeliumHandler::getSurface() {
		return surface;
	}

	bool BerkeliumHandler::userEventReceived(IInputEvent* userEvent) {
		clog << "BerkeliumHandler::userEventReceived " << endl;

		//browserReceiveEvent(mBrowser, (void*)(userEvent->getContent()));

		if (isValid && userEvent->isButtonPressType()) {
			bWindow->mouseButton(1, true);
		}

		return true;
	}

	bool BerkeliumHandler::motionEventReceived(int x, int y, int z) {
		clog << "BerkeliumHandler::motionEventReceived " << endl;

		if (isValid) {
			bWindow->mouseMoved(x, y);
		}

		return true;
	}

	void BerkeliumHandler::onAddressBarChanged(Window *win, URLString newURL) {
        std::string x = "hi";
        x+= newURL;
        mURL = newURL.get<std::string>();
        clog << "BerkeliumHandler::onAddressChanged to " << newURL << endl;
	}

	void BerkeliumHandler::onStartLoading(Window *win, URLString newURL) {
		clog << "BerkeliumHandler::Start loading " << newURL << " from " << mURL << endl;

		wstring str_css(L"::-webkit-scrollbar { display: none; }");

		win->insertCSS(
				WideString::point_to(str_css.c_str()),
				WideString::empty());
	}

	void BerkeliumHandler::onLoadingStateChanged(Window *win, bool isLoading) {
		clog << "BerkeliumHandler::Loading state changed ";
		clog << mURL << " to " << (isLoading?"loading":"stopped") << endl;
	}

	void BerkeliumHandler::onLoad(Window *win) {
		wstring str_css(L"::-webkit-scrollbar { display: none; }");

		win->insertCSS(
				WideString::point_to(str_css.c_str()),
				WideString::empty());
	}

	void BerkeliumHandler::onLoadError(Window *win, WideString error) {
        clog << L"*** onLoadError " << mURL << ": ";
        clog << error << endl;
	}

	void BerkeliumHandler::onResponsive(Window *win) {
		clog << "BerkeliumHandler::onResponsive " << mURL << endl;
	}

	void BerkeliumHandler::onUnresponsive(Window *win) {
		clog << "BerkeliumHandler::onUnresponsive " << mURL << endl;
	}

	void BerkeliumHandler::onPaint(
			Window *wini,
			const unsigned char *bitmap_in,
			const Rect &bitmap_rect,
			size_t num_copy_rects,
			const Rect *copy_rects,
			int dx,
			int dy,
			const Rect &scroll_rect) {

		string str;
		static int call_count = 0;
		IWindow* win;

		clog << "BerkeliumHandler::onPaint " << mURL << endl;

		if (bitmap_rect.left() != 0 || bitmap_rect.top() != 0 ||
				bitmap_rect.right() != w || bitmap_rect.bottom() != h) {

			clog << "BerkeliumHandler::onPaint '" << mURL << "' not full" << endl;
			return;
		}

		FILE *outfile;
		{
			std::ostringstream os;
			os << "/tmp/bh_r_" << time(NULL) << "_" << (call_count++) << ".ppm";
			str = os.str();
			outfile = fopen(str.c_str(), "wb");
		}

		const int width = bitmap_rect.width();
		const int height = bitmap_rect.height();

		fprintf(outfile, "P6 %d %d 255\n", width, height);
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				unsigned char r,g,b,a;

				b = *(bitmap_in++);
				g = *(bitmap_in++);
				r = *(bitmap_in++);
				a = *(bitmap_in++);
				fputc(r, outfile);  // Red
				//fputc(255-a, outfile);  // Alpha
				fputc(g, outfile);  // Green
				fputc(b, outfile);  // Blue
				//(pixel >> 24) & 0xff;  // Alpha
			}
		}
		fclose(outfile);

		win = (IWindow*)(surface->getParent());
		if (win != NULL) {
			win->renderImgFile(str);
			clog << "BerkeliumHandler::onPaint rendered" << endl;
		}

		clog << "BerkeliumHandler::onPaint all done" << endl;
	}

	void BerkeliumHandler::onCrashed(Window *win) {
		clog << "BerkeliumHandler::onCrashed " << mURL << endl;
	}

	void BerkeliumHandler::onCreatedWindow(
			Window *win, Window *newWindow, const Rect &initialRect) {

		clog << "BerkeliumHandler::onCreatedWindow from source ";
		clog << mURL << endl;
        //newWindow->setDelegate(new BerkeliumHandler);
	}

	void BerkeliumHandler::onExternalHost(
			Window *win,
			WideString message,
			URLString origin,
			URLString target) {

		clog << "BerkeliumHandler::onChromeSend at URL ";
		clog << mURL << " from " << origin;
		clog << " to " << target << ": ";
		clog << message << endl;
	}

	void BerkeliumHandler::onPaintPluginTexture(
			Window *win,
			void* sourceGLTexture,
			const std::vector<Rect> srcRects,
			const Rect &destRect) {

		clog << "BerkeliumHandler::onPaintPluginTexture from source ";
		clog << mURL << endl;

	}

	void BerkeliumHandler::onWidgetCreated(
			Window *win, Widget *newWidget, int zIndex) {

		clog << "BerkeliumHandler::onWidgetCreated from source " << mURL;
		clog << endl;
	}

	void BerkeliumHandler::onWidgetDestroyed(Window *win, Widget *newWidget) {
		clog << "BerkeliumHandler::onWidgetDestroyed from source ";
		clog << mURL << endl;
	}

	void BerkeliumHandler::onWidgetResize(
			Window *win, Widget *wid, int newWidth, int newHeight) {

		clog << "BerkeliumHandler::onWidgetResize from source " << mURL << endl;
	}

	void BerkeliumHandler::onWidgetMove(
			Window *win, Widget *wid, int newX, int newY) {

		clog << "BerkeliumHandler::onWidgetMove from source " << mURL << endl;
	}

	void BerkeliumHandler::onWidgetPaint(
			Window *win,
			Widget *wid,
			const unsigned char *sourceBuffer,
			const Rect &rect,
			size_t num_copy_rects,
			const Rect *copy_rects,
			int dx,
			int dy,
			const Rect &scrollRect) {

		clog << "BerkeliumHandler::onWidgetPaint from source " << mURL << endl;
	}
}
}
}
}
}
}
