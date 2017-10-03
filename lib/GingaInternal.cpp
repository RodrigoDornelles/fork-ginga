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

#include "aux-ginga.h"
#include "GingaInternal.h"

#include "formatter/Scheduler.h"
using namespace ::ginga::formatter;

#include "player/TextPlayer.h"
using namespace ::ginga::player;

// Option defaults.
static GingaOptions opts_defaults = {
  800,                          // width
  600,                          // height
  false,                        // debug
  false,                        // experimental
  "",                           // background ("" == none)
};

// Option data.
typedef struct GingaOptionData
{
  GType type;                   // option type
  int offset;                   // offset in GingaOption struct
  void *func;                   // update function
} OptionTab;

#define OPTS_ENTRY(name,type,func)                              \
  {G_STRINGIFY (name),                                          \
      {(type), offsetof (GingaOptions, name),                   \
         pointerof (G_PASTE (GingaInternal::setOption, func))}}

// Option table.
static map<string, GingaOptionData> opts_table =
{
 OPTS_ENTRY (background,   G_TYPE_STRING,  Background),
 OPTS_ENTRY (debug,        G_TYPE_BOOLEAN, Debug),
 OPTS_ENTRY (experimental, G_TYPE_BOOLEAN, Experimental),
 OPTS_ENTRY (height,       G_TYPE_INT,     Size),
 OPTS_ENTRY (width,        G_TYPE_INT,     Size),
};

// Indexes option table.
static bool
opts_table_index (const string &key, GingaOptionData **result)
{
  map<string, GingaOptionData>::iterator it;
  if ((it = opts_table.find (key)) == opts_table.end ())
    return false;
  tryset (result, &it->second);
  return true;
}

// Compares the z-index and z-order of two players.
static gint
win_cmp_z (Player *p1, Player *p2)
{
  int z1, zo1, z2, zo2;

  g_assert_nonnull (p1);
  g_assert_nonnull (p2);

  p1->getZ (&z1, &zo1);
  p2->getZ (&z2, &zo2);

  if (z1 < z2)
    return -1;
  if (z1 > z2)
    return 1;
  if (zo1 < zo2)
    return -1;
  if (zo1 > zo2)
    return 1;
  return 0;
}


// External API.

/**
 * @brief Get current state.
 * @return Current state.
 */
GingaState
GingaInternal::getState ()
{
  return _state;
}

/**
 * @brief Starts NCL from file.
 * @param file Path to NCL file.
 * @param errmsg Address of a variable to store an error message.
 * @return True if successfully, or false otherwise.
 */
bool
GingaInternal::start (const string &file, string *errmsg)
{
  if (_state != GINGA_STATE_STOPPED)
    return false;               // nothing to do

  _scheduler = new Scheduler (this);
  _ncl_file = file;
  _eos = false;
  _last_tick_total = 0;
  _last_tick_diff = 0;
  _last_tick_frameno = 0;

  if (unlikely (!_scheduler->run (file, errmsg)))
    {
      delete _scheduler;
      return false;
    }

  _state = GINGA_STATE_PLAYING;
  return true;
}

/**
 * @brief Stops NCL.
 */
bool
GingaInternal::stop ()
{
  if (_state == GINGA_STATE_STOPPED)
    return false;               // nothing to do

  delete _scheduler;
  _scheduler = nullptr;
  g_list_free (_listeners);
  _listeners = nullptr;
  g_list_free (_players);
  _players = nullptr;
  _state = GINGA_STATE_STOPPED;
  return true;
}

/**
 * @brief Resize current surface.
 * @param width New width (in pixels).
 * @param height New height (in pixels).
 */
void
GingaInternal::resize (int width, int height)
{
  g_assert (width > 0 && height > 0);
  _opts.width = width;
  _opts.height = height;
  for (GList *l = _players; l != nullptr; l = l->next)
    {
      Player *pl = (Player *) l->data;
      g_assert_nonnull (pl);
      pl->setProperty ("top", pl->getProperty ("top"));
      pl->setProperty ("left", pl->getProperty ("left"));
      pl->setProperty ("width", pl->getProperty ("width"));
      pl->setProperty ("height", pl->getProperty ("height"));
    }
}

/**
 * @brief Draw current surface onto cairo context.
 * @param cr Target cairo context.
 */
void
GingaInternal::redraw (cairo_t *cr)
{
  GList *l;

  if (_background.alpha > 0)
    {
      GingaColor c = _background;
      cairo_save (cr);
      cairo_set_source_rgba (cr, c.red, c.green, c.blue, c.alpha);
      cairo_rectangle (cr, 0, 0, _opts.width, _opts.height);
      cairo_fill (cr);
      cairo_restore (cr);
    }

  _players = g_list_sort (_players, (GCompareFunc) win_cmp_z);
  l = _players;
  while (l != NULL)             // can be modified while being traversed
    {
      GList *next = l->next;
      Player *pl = (Player *) l->data;
      if (pl == NULL)
        {
          _players = g_list_remove_link (_players, l);
        }
      else
        {
          cairo_save (cr);
          pl->redraw (cr);
          cairo_restore (cr);
        }
      l = next;
    }

  if (_opts.debug)
    {
      static GingaColor fg = {1., 1., 1., 1.};
      static GingaColor bg = {0, 0, 0, 0};
      static GingaRect rect = {0, 0, 0, 0};

      string info;
      cairo_surface_t *debug;
      GingaRect ink;

      info = xstrbuild ("%s: #%lu %" GINGA_TIME_FORMAT " %.1ffps",
                        _ncl_file.c_str (),
                        _last_tick_frameno,
                        GINGA_TIME_ARGS (_last_tick_total),
                        1 * GINGA_SECOND / (double) _last_tick_diff);

      rect.width = _opts.width;
      rect.height = _opts.height;
      debug = TextPlayer::renderSurface
        (info, "monospace", "", "bold", "9", fg, bg,
         rect, "center", "", true, &ink);
      ink = {0, 0, rect.width, ink.height - ink.y + 4};

      cairo_save (cr);
      cairo_set_source_rgba (cr, 1., 0., 0., .5);
      cairo_rectangle (cr, 0, 0, ink.width, ink.height);
      cairo_fill (cr);
      cairo_set_source_surface (cr, debug, 0, 0);
      cairo_paint (cr);
      cairo_restore (cr);
      cairo_surface_destroy (debug);
    }
}

#if WITH_OPENGL
/**
 * @brief Draw current surface onto current opengl context.
 */
void
GingaInternal::redraw_gl ()
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glViewport (0, 0, _opts.width, _opts.height);
  glOrtho (0, _opts.width, 0, _opts.height, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  GList *l;

  _players = g_list_sort (_players, (GCompareFunc) win_cmp_z);
  l = _players;
  while (l != NULL)             // can be modified while being traversed
    {
      GList *next = l->next;
      Player *pl = (Player *) l->data;
      if (pl == NULL)
        {
          _players = g_list_remove_link (_players, l);
        }
      else
        {
          pl->redraw_gl ();
        }
      l = next;
    }
}
#endif

// Stop formatter if EOS has been seen.
#define _GINGA_CHECK_EOS(ginga)                                 \
  G_STMT_START                                                  \
  {                                                             \
    if ((ginga)->getEOS ())                                     \
      {                                                         \
        g_assert ((ginga)->_state == GINGA_STATE_PLAYING);      \
        (ginga)->setEOS (false);                                \
        g_assert ((ginga)->stop ());                            \
        g_assert ((ginga)->_state == GINGA_STATE_STOPPED);      \
      }                                                         \
  }                                                             \
  G_STMT_END

// This gymnastics is necessary to ensure that the list can be safely
// modified while it is being traversed.
#define _GINGA_NOTIFY_LISTENERS(list, Type, method, ...)        \
  G_STMT_START                                                  \
  {                                                             \
    guint n = g_list_length ((list));                           \
    for (guint i = 0; i < n; i++)                               \
      {                                                         \
        Type *obj = (Type *) g_list_nth_data ((list), i);       \
        if (obj == NULL)                                        \
          break;                                                \
        obj->method (__VA_ARGS__);                              \
      }                                                         \
  }                                                             \
  G_STMT_END

/**
 * @brief Sends key event.
 * @param key Key name.
 * @param press True if press, False if release.
 * @return True if successful, or false otherwise.
 */
bool
GingaInternal::sendKeyEvent (const string &key, bool press)
{
  _GINGA_CHECK_EOS (this);
  if (_state != GINGA_STATE_PLAYING)
    return false;               // nothing to do

  _GINGA_NOTIFY_LISTENERS (_listeners, IGingaInternalEventListener,
                           handleKeyEvent, key, press);
  return true;
}

/**
 * @brief Sends tick event.
 * @param total Time passed since start (in microseconds).
 * @param diff Time passed since last tick (in microseconds).
 * @param frame Current frame number.
 * @return True if successful, or false otherwise.
 */
bool
GingaInternal::sendTickEvent (uint64_t total, uint64_t diff, uint64_t frame)
{
  _GINGA_CHECK_EOS (this);
  if (_state != GINGA_STATE_PLAYING)
    return false;               // nothing to do

  _last_tick_total = total;
  _last_tick_diff = diff;
  _last_tick_frameno = frame;
  _GINGA_NOTIFY_LISTENERS (_listeners, IGingaInternalEventListener,
                           handleTickEvent, total, diff, (int) frame);
  return true;
}

/**
 * @brief Gets current options.
 * @return The current options.
 */
const GingaOptions *
GingaInternal::getOptions ()
{
  return &_opts;
}

#define OPT_ERR_UNKNOWN(name)\
  ERROR ("unknown GingaOption '%s'", (name))
#define OPT_ERR_BAD_TYPE(name, typename)\
  ERROR ("GingaOption '%s' is of type '%s'", (name), (typename))

#define OPT_GETSET_DEFN(Name, Type, GType)                              \
  Type                                                                  \
  GingaInternal::getOption##Name (const string &name)                   \
  {                                                                     \
    GingaOptionData *opt;                                               \
    if (unlikely (!opts_table_index (name, &opt)))                      \
      OPT_ERR_UNKNOWN (name.c_str ());                                  \
    if (unlikely (opt->type != (GType)))                                \
      OPT_ERR_BAD_TYPE (name.c_str (), G_STRINGIFY (Type));             \
    return *((Type *)(((ptrdiff_t) &_opts) + opt->offset));             \
  }                                                                     \
  void                                                                  \
  GingaInternal::setOption##Name (const string &name, Type value)       \
  {                                                                     \
    GingaOptionData *opt;                                               \
    if (unlikely (!opts_table_index (name, &opt)))                      \
      OPT_ERR_UNKNOWN (name.c_str ());                                  \
    if (unlikely (opt->type != (GType)))                                \
      OPT_ERR_BAD_TYPE (name.c_str (), G_STRINGIFY (Type));             \
    *((Type *)(((ptrdiff_t) &_opts) + opt->offset)) = value;            \
    if (opt->func)                                                      \
      {                                                                 \
        ((void (*) (GingaInternal *, const string &, Type)) opt->func)  \
          (this, name, value);                                          \
      }                                                                 \
  }

OPT_GETSET_DEFN (Bool, bool, G_TYPE_BOOLEAN)
OPT_GETSET_DEFN (Int, int, G_TYPE_INT)
OPT_GETSET_DEFN (String, string, G_TYPE_STRING)


// Internal API.

/**
 * @brief Creates a new instance.
 */
GingaInternal::GingaInternal (unused (int argc), unused (char **argv),
                              GingaOptions *opts) : Ginga (argc, argv, opts)
{
  const char *s;

  _state = GINGA_STATE_STOPPED;
  _opts = (opts) ? *opts : opts_defaults;
  _scheduler = nullptr;
  _listeners = nullptr;
  _players = nullptr;

  _ncl_file = "";
  _eos = false;
  _last_tick_total = 0;
  _last_tick_diff = 0;
  _last_tick_frameno = 0;
  _saved_G_MESSAGES_DEBUG = (s = g_getenv ("G_MESSAGES_DEBUG"))
    ? string (s) : "";

  setOptionBackground (this, "background", _opts.background);
  setOptionDebug (this, "debug", _opts.debug);
  setOptionExperimental (this, "experimental", _opts.experimental);

#if defined WITH_CEF && WITH_CEF
  CefMainArgs args (argc, argv);
  CefSettings settings;
  int pstatus = CefExecuteProcess (args, nullptr, nullptr);
  if (pstatus >= 0)
    return pstatus;
  if (unlikely (!CefInitialize (args, settings, nullptr, nullptr)))
    exit (EXIT_FAILURE);
#endif
}

/**
 * @brief Destroys instance.
 */
GingaInternal::~GingaInternal ()
{
  if (_state != GINGA_STATE_STOPPED)
    this->stop ();
#if defined WITH_CEF && WITH_CEF
  CefShutdown ();
#endif
}

/**
 * @brief Gets associated scheduler.
 * @return The associated scheduler.
 */
Scheduler *
GingaInternal::getScheduler ()
{
  return (Scheduler *) this->getData ("scheduler");
}

/**
 * @brief Gets EOS flag value.
 * @return EOS flag value.
 */
bool
GingaInternal::getEOS ()
{
  return _eos;
}

/**
 * @brief Sets EOS flag.
 * @param eos Flag value.
 */
void
GingaInternal::setEOS (bool eos)
{
  _eos = eos;
}

/**
 * @brief Adds event listener.
 * @param obj Event listener.
 * @return True if successful, or false otherwise.
 */
bool
GingaInternal::registerEventListener (IGingaInternalEventListener *obj)
{
  g_assert_nonnull (obj);
  return this->add (&_listeners, obj);
}

/**
 * @brief Removes event listener.
 * @param obj Event listener.
 * @return True if successful, or false otherwise.
 */
bool
GingaInternal::unregisterEventListener (IGingaInternalEventListener *obj)
{
  g_assert_nonnull (obj);
  return this->remove (&_listeners, obj);
}

/**
 * @brief Adds handled player.
 * @param player Player.
 */
void
GingaInternal::registerPlayer (Player *player)
{
  g_assert_nonnull (player);
  g_assert (this->add (&_players, player));
}

/**
 * @brief Removes handled player.
 */
void
GingaInternal::unregisterPlayer (Player *player)
{
  g_assert_nonnull (player);
  g_assert (this->remove (&_players, player));
}

/**
 * @brief Gets data previously attached to state.
 * @param key Name of the key.
 * @return Data associated with key.
 */
void *
GingaInternal::getData (const string &key)
{
  map<string, void *>::iterator it;
  return ((it = _userdata.find (key)) == _userdata.end ())
    ? nullptr : it->second;
}

/**
 * @brief Attaches data to state.
 * @param key Name of the key.
 * @param data Data to associate with key.
 */
void
GingaInternal::setData (const string &key, void *data)
{
  _userdata[key] = data;
}

/**
 * @brief Updates debug option.
 */
void
GingaInternal::setOptionDebug (GingaInternal *self, const string &name,
                               bool value)
{
  g_assert (name == "debug");
  if (value)
    {
      const char *curr = g_getenv ("G_MESSAGES_DEBUG");
      if (curr != nullptr)
        self->_saved_G_MESSAGES_DEBUG = string (curr);
      g_assert (g_setenv ("G_MESSAGES_DEBUG", "all", true));
    }
  else
    {
      g_assert (g_setenv ("G_MESSAGES_DEBUG",
                          self->_saved_G_MESSAGES_DEBUG.c_str (), true));
    }
  TRACE ("setting GingaOption '%s' to %s", name.c_str (), strbool (value));
}

/**
 * @brief Updates experimental option.
 */

void
GingaInternal::setOptionExperimental (unused (GingaInternal *self),
                                      const string &name, bool value)
{
  g_assert (name == "experimental");
  TRACE ("setting GingaOption '%s' to %s", name.c_str (), strbool (value));
}

/**
 * @brief Updates size option.
 */
void
GingaInternal::setOptionSize (GingaInternal *self, const string &name,
                              int value)
{
  const GingaOptions *opts;
  g_assert (name == "width" || name == "height");
  opts = self->getOptions ();
  self->resize (opts->width, opts->height);
  TRACE ("setting GingaOption '%s' to %d", name.c_str (), value);
}

/**
 * @brief Updates background option.
 */
void
GingaInternal::setOptionBackground (GingaInternal *self, const string &name,
                                    string value)
{
  g_assert (name == "background");
  if (value == "")
    self->_background = {0.,0.,0.,0.};
  else
    self->_background = ginga_parse_color (value);
  TRACE ("setting GingaOption '%s' to '%s'", name.c_str (), value.c_str ());
}


// Private methods.

bool
GingaInternal::add (GList **list, gpointer data)
{
  bool found;

  g_assert_nonnull (list);
  if (unlikely (found = g_list_find (*list, data)))
    {
      WARNING ("object %p already in list %p", data, *list);
      goto done;
    }
  *list = g_list_append (*list, data);
done:
  return !found;
}

bool
GingaInternal::remove (GList **list, gpointer data)
{
  GList *l;

  g_assert_nonnull (list);
  l = *list;
  while (l != NULL)
    {
      GList *next = l->next;
      if (l->data == data)
        {
          *list = g_list_delete_link (*list, l);
          return true;
        }
      l = next;
    }
  WARNING ("object %p not in list %p", data, *list);
  return false;
}
