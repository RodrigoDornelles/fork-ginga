/* Copyright (C) 2006-2018 PUC-Rio/Laboratorio TeleMidia

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
along with Ginga.  If not, see <https://www.gnu.org/licenses/>.  */

#include "aux-ginga.h"
#include "aux-gl.h"
#include "Player.h"
#include "player/PlayerImage.h"
#include "player/PlayerText.h"
#include "player/PlayerVideo.h"
#include "player/PlayerSigGen.h"

#if defined WITH_NCLUA && WITH_NCLUA
#include "player/PlayerLua.h"
#endif

#if defined WITH_LIBRSVG && WITH_LIBRSVG
#include "player/PlayerSvg.h"
#endif

#if defined WITH_CEF && WITH_CEF
#include "player/PlayerHTML.h"
#endif

GINGA_NAMESPACE_BEGIN

typedef struct PlayerPropertyInfo
{
  Player::PlayerProperty code; // property code
  bool init;                   // whether it should be initialized
  string defval;               // default value
} PlayerPropertyInfo;

static map<string, PlayerPropertyInfo> player_property_map =
{
 {"background",   {Player::PROP_BACKGROUND,    true,  ""}},
 {"balance",      {Player::PROP_BALANCE,       false,  "0.0"}},
 {"bass",         {Player::PROP_BASS,          false,  "0"}},
 {"bottom",       {Player::PROP_BOTTOM,        false, "0%"}},
 {"bounds",       {Player::PROP_BOUNDS,        false, "0%,0%,100%,100%"}},
 {"debug",        {Player::PROP_DEBUG,         true,  "false"}},
 {"duration",     {Player::PROP_DURATION,      true,  "indefinite"}},
 {"focusIndex",   {Player::PROP_FOCUS_INDEX,   true,  ""}},
 {"fontBgColor",  {Player::PROP_FONT_BG_COLOR, true,  ""}},
 {"fontColor",    {Player::PROP_FONT_COLOR,    true,  "black"}},
 {"fontFamily",   {Player::PROP_FONT_FAMILY,   true,  "sans"}},
 {"fontSize",     {Player::PROP_FONT_SIZE,     true,  "12"}},
 {"fontStyle",    {Player::PROP_FONT_STYLE,    true,  ""}},
 {"fontVariant",  {Player::PROP_FONT_VARIANT,  true,  ""}},
 {"fontWeight",   {Player::PROP_FONT_WEIGHT,   true,  ""}},
 {"freeze",       {Player::PROP_FREEZE,        true,  "false"}},
 {"height",       {Player::PROP_HEIGHT,        true,  "100%"}},
 {"horzAlign",    {Player::PROP_HORZ_ALIGN,    true,  "left"}},
 {"left",         {Player::PROP_LEFT,          true,  "0"}},
 {"location",     {Player::PROP_LOCATION,      false, "0,0"}},
 {"mute",         {Player::PROP_MUTE,          false, "false"}},
 {"rate",         {Player::PROP_RATE,          true,  "1"}},
 {"right",        {Player::PROP_RIGHT,         false, "0%"}},
 {"size",         {Player::PROP_SIZE,          false, "100%,100%"}},
 {"time",         {Player::PROP_TIME,          false, "0"}},
 {"top",          {Player::PROP_TOP,           true,  "0"}},
 {"transparency", {Player::PROP_TRANSPARENCY,  true,  "0%"}},
 {"treble",       {Player::PROP_TREBLE,        false, "0"}}, 
 {"vertAlign",    {Player::PROP_VERT_ALIGN,    true,  "top"}},
 {"visible",      {Player::PROP_VISIBLE,       true,  "true"}},
 {"volume",       {Player::PROP_VOLUME,        true,  "100%"}},
 {"width",        {Player::PROP_WIDTH,         true,  "100%"}}, 
 {"zIndex",       {Player::PROP_Z_INDEX,       true,  "0"}},
 {"freq",         {Player::PROP_FREQ,          true,  "440"}},
};

static map<string, string> player_property_aliases = {
  { "backgroundColor", "background" },
  { "balanceLevel", "balance" },
  { "bassLevel", "bass" },
  { "explicitDur", "duration" },
  { "soundLevel", "volume" },
  { "trebleLevel", "treble" },
};

// Public.

Player::Player (Formatter *formatter, const string &id, const string &uri)
{
  g_assert_nonnull (formatter);
  _formatter = formatter;
  _opengl = _formatter->getOptionBool ("opengl");
  _id = id;
  _uri = uri;
  _state = SLEEPING;
  _time = 0;
  _eos = false;
  _dirty = true;
  _animator = new PlayerAnimator (_formatter);
  _surface = nullptr;
  _gltexture = 0;
  this->resetProperties ();
}

Player::~Player ()
{
  delete _animator;
  if (_surface != nullptr)
    cairo_surface_destroy (_surface);
  if (_gltexture)
    GL::delete_texture (&_gltexture);
  _properties.clear ();
}

Player::PlayerState
Player::getState ()
{
  return _state;
}

bool
Player::isFocused ()
{
  return _prop.focusIndex != "" && _prop.focusIndex == _currentFocus;
}

Time
Player::getTime ()
{
  return _time;
}

void
Player::incTime (Time inc)
{
  _time += inc;
}

Time
Player::getDuration ()
{
  return _prop.duration;
}

void
Player::setDuration (Time duration)
{
  _prop.duration = duration;
}

bool
Player::getEOS ()
{
  return _eos;
}

void
Player::setEOS (bool eos)
{
  _eos = eos;
}

void
Player::getZ (int *z, int *zorder)
{
  tryset (z, _prop.z);
  tryset (zorder, _prop.zorder);
}

void
Player::setZ (int z, int zorder)
{
  _prop.z = z;
  _prop.zorder = zorder;
}

void
Player::start ()
{
  g_assert (_state != OCCURRING);
  _state = OCCURRING;
  _time = 0;
  _eos = false;
  _animator->notifyPlayerStartOrStop("start");
  this->reload ();
}

void
Player::stop ()
{
  g_assert (_state != SLEEPING);
  _state = SLEEPING;
  _animator->notifyPlayerStartOrStop("stop");
  this->resetProperties ();
}

void
Player::pause ()
{
  g_assert (_state != PAUSED && _state != SLEEPING);
  _state = PAUSED;
}

void
Player::resume ()
{
  g_assert (_state == PAUSED);
  _state = OCCURRING;
}

string
Player::getProperty (string const &name)
{
  return (_properties.count (name) != 0) ? _properties[name] : "";
}

void
Player::setProperty (const string &name, const string &value)
{

  Player::PlayerProperty code;
  bool use_defval;
  string defval;
  string _value;

  if (name == "transIn" || name == "transOut")
    _animator->setTransitionProperties (name, value);

  use_defval = false;
  _value = value;

  code = Player::getPlayerProperty (name, &defval);
  if (code == Player::PROP_UNKNOWN)
    goto done;

  if (_value == "")
    {
      use_defval = true;
      _value = defval;
    }

  if (unlikely (!this->doSetProperty (code, name, _value)))
    {
      ERROR ("property '%s': bad value '%s'", name.c_str (),
             _value.c_str ());
    }

  if (use_defval) // restore value
    _value = "";

done:
  _properties[name] = _value;
  return;
}

void
Player::resetProperties ()
{
  for (auto it : player_property_map)
    if (it.second.init)
      this->setProperty (it.first, "");
  _properties.clear ();
}

void
Player::resetProperties (set<string> *props)
{
  for (auto name : *props)
    this->setProperty (name, "");
}

void
Player::schedulePropertyAnimation (const string &name, const string &from,
                                   const string &to, Time dur)
{
  // TRACE ("%s.%s from '%s' to '%s' in %" GINGA_TIME_FORMAT,
  //        _id.c_str (), name.c_str (), from.c_str (), to.c_str (),
  //        GINGA_TIME_ARGS (dur));
  _animator->schedule (name, from, to, dur);
}

void
Player::reload ()
{
  _dirty = false;
}

void
Player::redraw (cairo_t *cr)
{
  g_assert (_state != SLEEPING);
  _animator->update (&_prop.rect, &_prop.bgColor, &_prop.alpha);

  if (!_prop.visible || !(_prop.rect.width > 0 && _prop.rect.height > 0))
    return; // nothing to do

  if (_dirty)
    {
      this->reload ();
    }

  if (_prop.bgColor.alpha > 0)
    {
      if (_opengl)
        {
          GL::draw_quad (
              _prop.rect.x, _prop.rect.y, _prop.rect.width,
              _prop.rect.height,
              // Color
              (GLfloat) _prop.bgColor.red, (GLfloat) _prop.bgColor.green,
              (GLfloat) _prop.bgColor.blue, (GLfloat) (_prop.alpha / 255.));
        }
      else
        {
          cairo_save (cr);
          cairo_set_source_rgba (cr, _prop.bgColor.red, _prop.bgColor.green,
                                 _prop.bgColor.blue, _prop.alpha / 255.);
          cairo_rectangle (cr, _prop.rect.x, _prop.rect.y, _prop.rect.width,
                           _prop.rect.height);
          cairo_fill (cr);
          cairo_restore (cr);
        }
    }

  if (_opengl)
    {
      if (_gltexture)
        {
          GL::draw_quad (_prop.rect.x, _prop.rect.y, _prop.rect.width,
                         _prop.rect.height, _gltexture,
                         (GLfloat) (_prop.alpha / 255.));
        }
    }
  else
    {
      if (_surface != nullptr)
        {
          double sx, sy;
          sx = (double) _prop.rect.width
               / cairo_image_surface_get_width (_surface);
          sy = (double) _prop.rect.height
               / cairo_image_surface_get_height (_surface);
          cairo_save (cr);
          cairo_translate (cr, _prop.rect.x, _prop.rect.y);
          cairo_scale (cr, sx, sy);
          cairo_set_source_surface (cr, _surface, 0., 0.);
          cairo_paint_with_alpha (cr, _prop.alpha / 255.);
          cairo_restore (cr);
        }
    }

  if (this->isFocused ())
    {
      if (_opengl)
        {
          // TODO
        }
      else
        {
          cairo_save (cr);
          cairo_set_source_rgba (cr, 1., 1., 0., 1.);
          cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
          cairo_rectangle (cr, _prop.rect.x, _prop.rect.y, _prop.rect.width,
                           _prop.rect.height);
          cairo_stroke (cr);
          cairo_restore (cr);
        }
    }

  if (_prop.debug || _formatter->getOptionBool ("debug"))
    this->redrawDebuggingInfo (cr);
}

void
Player::sendKeyEvent (unused (const string &key), unused (bool press))
{
}

// Public: Static.

// Current focus index value.
string Player::_currentFocus = "";

string
Player::getCurrentFocus ()
{
  return _currentFocus;
}

void
Player::setCurrentFocus (const string &index)
{
  _currentFocus = index;
}

Player::PlayerProperty
Player::getPlayerProperty (const string &name, string *defval)
{
  map<string, PlayerPropertyInfo>::iterator it;
  PlayerPropertyInfo *info;
  string _name = name;

  if ((it = player_property_map.find (_name)) == player_property_map.end ())
    {
      map<string, string>::iterator italias;
      if ((italias = player_property_aliases.find (_name))
          == player_property_aliases.end ())
        {
          tryset (defval, "");
          return PROP_UNKNOWN;
        }
      _name = italias->second;
      it = player_property_map.find (_name);
      g_assert (it != player_property_map.end ());
    }
  info = &it->second;
  tryset (defval, info->defval);
  return info->code;
}

Player *
Player::createPlayer (Formatter *formatter, const string &id,
                      const string &uri, const string &mime)
{
  Player *player = nullptr;
  g_assert_nonnull (formatter);

  if (xstrhasprefix (mime, "audio") || xstrhasprefix (mime, "video"))
    {
      player = new PlayerVideo (formatter, id, uri);
    }
  else if (mime == "application/x-ginga-siggen")
    {
      player = new PlayerSigGen (formatter, id, uri);
    }
  else if (xstrhasprefix (mime, "image"))
    {
      player = new PlayerImage (formatter, id, uri);
    }
  else if (mime == "text/plain")
    {
      player = new PlayerText (formatter, id, uri);
    }
#if defined WITH_CEF && WITH_CEF
  else if (xstrhasprefix (mime, "text/html"))
    {
      player = new PlayerHTML (formatter, id, uri);
    }
#endif // WITH_CEF
#if defined WITH_NCLUA && WITH_NCLUA
  else if (mime == "application/x-ginga-NCLua")
    {
      player = new PlayerLua (formatter, id, uri);
    }
#endif // WITH_NCLUA
#if WITH_LIBRSVG && WITH_LIBRSVG
  else if (xstrhasprefix (mime, "image/svg"))
    {
      player = new PlayerSvg (formatter, id, uri);
    }
#endif // WITH_LIBRSVG
  else
    {
      player = new Player (formatter, id, uri);
      if (unlikely (mime != "application/x-ginga-timer" && uri != ""))
        {
          WARNING ("unknown mime '%s': creating an empty player",
                   mime.c_str ());
        }
    }

  g_assert_nonnull (player);
  return player;
}

// Protected.

bool
Player::doSetProperty (PlayerProperty code, unused (const string &name),
                       const string &value)
{
  switch (code)
    {
    case PROP_DEBUG:
      _prop.debug = ginga::parse_bool (value);
      break;
    case PROP_FOCUS_INDEX:
      _prop.focusIndex = value;
      break;
    case PROP_BOUNDS:
      {
        list<string> lst;
        if (unlikely (!ginga::try_parse_list (value, ',', 4, 4, &lst)))
          return false;
        auto it = lst.begin ();
        this->setProperty ("left", *it++);
        this->setProperty ("top", *it++);
        this->setProperty ("width", *it++);
        this->setProperty ("height", *it++);
        g_assert (it == lst.end ());
        break;
      }
    case PROP_LOCATION:
      {
        list<string> lst;
        if (unlikely (!ginga::try_parse_list (value, ',', 2, 2, &lst)))
          return false;
        auto it = lst.begin ();
        this->setProperty ("left", *it++);
        this->setProperty ("top", *it++);
        g_assert (it == lst.end ());
        break;
      }
    case PROP_SIZE:
      {
        list<string> lst;
        if (unlikely (!ginga::try_parse_list (value, ',', 2, 2, &lst)))
          return false;
        auto it = lst.begin ();
        this->setProperty ("width", *it++);
        this->setProperty ("height", *it++);
        g_assert (it == lst.end ());
        break;
      }
    case PROP_LEFT:
      {
        int width = _formatter->getOptionInt ("width");
        _prop.rect.x = ginga::parse_percent (value, width, 0, G_MAXINT);
        _dirty = true;
        break;
      }
    case PROP_RIGHT:
      {
        int width = _formatter->getOptionInt ("width");
        _prop.rect.x
            = width - _prop.rect.width
              - ginga::parse_percent (value, _prop.rect.width, 0, G_MAXINT);
        _dirty = true;
        break;
      }
    case PROP_TOP:
      {
        int height = _formatter->getOptionInt ("height");
        _prop.rect.y = ginga::parse_percent (value, height, 0, G_MAXINT);
        _dirty = true;
        break;
      }
    case PROP_BOTTOM:
      {
        int height = _formatter->getOptionInt ("height");
        _prop.rect.y = height - _prop.rect.height
                       - ginga::parse_percent (value, _prop.rect.height, 0,
                                               G_MAXINT);
        _dirty = true;
        break;
      }
    case PROP_WIDTH:
      {
        int width = _formatter->getOptionInt ("width");
        _prop.rect.width = ginga::parse_percent (value, width, 0, G_MAXINT);
        _dirty = true;
        break;
      }
    case PROP_HEIGHT:
      {
        int height = _formatter->getOptionInt ("height");
        _prop.rect.height
            = ginga::parse_percent (value, height, 0, G_MAXINT);
        _dirty = true;
        break;
      }
    case PROP_Z_INDEX:
      this->setZ (xstrtoint (value, 10), _prop.zorder);
      break;
    case PROP_TRANSPARENCY:
      _prop.alpha
          = (guint8) CLAMP (255 - ginga::parse_pixel (value), 0, 255);
      break;
    case PROP_BACKGROUND:
      if (value == "")
        _prop.bgColor = { 0, 0, 0, 0 };
      else
        _prop.bgColor = ginga::parse_color (value);
      break;
    case PROP_VISIBLE:
      _prop.visible = ginga::parse_bool (value);
      break;
    case PROP_DURATION:
      {
        if (value == "indefinite")
          _prop.duration = GINGA_TIME_NONE;
        else
          _prop.duration = ginga::parse_time (value);
        break;
      }
    default:
      break;
    }
  return true;
}

// Private.

void
Player::redrawDebuggingInfo (cairo_t *cr)
{
  cairo_surface_t *debug;
  string id;
  string str;
  double sx, sy;

  id = _id;
  if (id.find ("/") != std::string::npos)
    {
      id = xpathdirname (id);
      if (id.find ("/") != std::string::npos)
        id = xpathbasename (id);
    }

  // Draw info.
  str = xstrbuild ("%s:%.1fs\n%dx%d:(%d,%d):%d", id.c_str (),
                   ((double) GINGA_TIME_AS_MSECONDS (_time)) / 1000.,
                   _prop.rect.width, _prop.rect.height, _prop.rect.x,
                   _prop.rect.y, _prop.z);

  debug = PlayerText::renderSurface (
      str, "monospace", "", "", "7", { 1., 0, 0, 1. }, { 0, 0, 0, .75 },
      _prop.rect, "center", "middle", true, nullptr);
  g_assert_nonnull (debug);

  sx = (double) _prop.rect.width / cairo_image_surface_get_width (debug);
  sy = (double) _prop.rect.height / cairo_image_surface_get_height (debug);

  cairo_save (cr);
  cairo_translate (cr, _prop.rect.x, _prop.rect.y);
  cairo_scale (cr, sx, sy);
  cairo_set_source_surface (cr, debug, 0., 0.);
  cairo_paint (cr);
  cairo_restore (cr);

  cairo_surface_destroy (debug);
}

GINGA_NAMESPACE_END
