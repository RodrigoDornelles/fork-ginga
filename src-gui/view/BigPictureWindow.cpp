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

#include "ginga_gtk.h"
#include "BigPictureWindow.h"

GList *cards_list = NULL;
gboolean inBigPictureMode = FALSE;
GtkWidget *bigPictureWindow = NULL;
guint timeOutTag = 0;
cairo_pattern_t *background_pattern;

gint currentCard = 0;
gint carrouselDir = 0;
gdouble mid = 0;
gdouble speed = 1500.0; /* pixels/s */
gdouble frameRate = 1.000 / 60.0;

gint numCards = 20;
gdouble cardWidth = 300;
gdouble cardHeight = 169;


gint
comp_card_list (gconstpointer a, gconstpointer b)
{
  BigPictureCard *card_a = (BigPictureCard *)a;
  BigPictureCard *card_b = (BigPictureCard *)b;

  if (card_a->drawOrder < card_b->drawOrder)
    return 1;
  else if (card_a->drawOrder > card_b->drawOrder)
    return -1;
  else
    return 0;
}

gboolean
update_bigpicture_callback (GtkWidget *widget)
{
  GList *l;
  for (l = cards_list; l != NULL; l = l->next)
    {
      BigPictureCard *card = (BigPictureCard *)l->data;

      if (!card->animate)
        continue;

      card->position += (speed * frameRate) * carrouselDir;

      if ((carrouselDir > 0) && (card->position >= card->destPosition))
        {
          card->position = card->destPosition;
          card->animate = FALSE;
        }
      else if ((carrouselDir < 0) && (card->position <= card->destPosition))
        {
          card->position = card->destPosition;
          card->animate = FALSE;
        }
    }

  gtk_widget_queue_draw (widget);
  return G_SOURCE_CONTINUE;
}

void
draw_bigpicture_callback (GtkWidget *widget, cairo_t *cr, gpointer data)
{

  int w, h;
  w = gtk_widget_get_allocated_width (widget);
  h = gtk_widget_get_allocated_height (widget);

  // clear with pattern
  cairo_set_source (cr, background_pattern);
  cairo_paint (cr);

  GList *l;
  for (l = cards_list; l != NULL; l = l->next)
    {
      BigPictureCard *card = (BigPictureCard *)l->data;

      gdouble fa = mid - card->position;
      if (fa < 0)
        fa *= -1;

      gdouble scale = 1.4 - (fa / mid);
      // gdouble scale = 1.0;

      // gdouble posX = card->position - ((cardWidth * scale) / 2);
      gdouble posX = card->position - (cardWidth / 2);
      // gdouble posY = (h - cardHeight - 100) - ((cardHeight * scale) / 2);
      gdouble posY = (h - cardHeight - 100);

      // printf ("x: %f, w: %f, p %f, s: %f \n",  x, 400.0*scale, posX,
      // scale);

      cairo_set_source_rgba (cr, card->r, card->g, card->b, scale - 0.3);
      // cairo_scale(cr, scale, scale);
      cairo_set_source_surface (cr, card->surface, posX, posY);

      //  cairo_rectangle (cr, posX, posY, cardWidth * scale,  cardHeight *
      //  scale);
      cairo_paint (cr);

      cairo_text_extents_t extents;
      cairo_select_font_face (cr, "Arial", CAIRO_FONT_SLANT_NORMAL,
                              CAIRO_FONT_WEIGHT_BOLD);

      if (card->index == 0)
        {

          gdouble info_height = h / 2;
          gdouble info_weight = h * 1.2;

          cairo_set_source_rgba (cr, 0.3, 0.3, 0.3, 1.0);

          cairo_rectangle (cr, mid - (info_weight / 2), 100, info_weight,
                           info_height);
          cairo_fill (cr);

          cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, scale - 0.5);

          PangoLayout *layout = pango_cairo_create_layout (cr);
          g_assert_nonnull (layout);
          cairo_set_font_size (cr, 30);
          pango_layout_set_text (layout, card->appDesc, -1);
          pango_layout_set_alignment (layout, PANGO_ALIGN_LEFT);
          pango_layout_set_width (layout,
                                  ((info_weight / 2) - 20) * PANGO_SCALE);
          pango_layout_set_wrap (layout, PANGO_WRAP_WORD);
          pango_cairo_update_layout (cr, layout);
          cairo_move_to (cr, mid - (info_weight / 2) + 30,
                         (h / 2) - (info_height / 2) + 130);
          pango_cairo_show_layout (cr, layout);

          cairo_set_font_size (cr, 50);
          cairo_text_extents (cr, card->appName, &extents);
          cairo_move_to (cr, mid - (info_weight / 2) + 50, 200);
          cairo_show_text (cr, card->appName);

          g_object_unref (layout);
        }
    }

  // cairo_paint (cr);
}

void
carrousel_rotate (gint dir)
{
  carrouselDir = dir;

  if (dir > 0 && currentCard == 0)
    return;
  else if (dir > 0)
    currentCard--;

  if (dir < 0 && currentCard == numCards - 1)
    return;
  else if (dir < 0)
    currentCard++;

  GList *l;
  for (l = cards_list; l != NULL; l = l->next)
    {
      BigPictureCard *card = (BigPictureCard *)l->data;

      if (dir > 0)
        card->index++;
      else if (dir < 0)
        card->index--;

      if (card->index < 0)
        card->drawOrder = card->index * -1;
      else
        card->drawOrder = card->index;

      card->destPosition
          = mid + (card->index * cardWidth) + (card->index * 20);
      card->animate = TRUE;
    }

  cards_list = g_list_sort (cards_list, (GCompareFunc)comp_card_list);
}

/*void
create_bigpicture_window ()
{

  if (bigPictureWindow != NULL)
    return;

  cairo_surface_t *image_p = cairo_image_surface_create_from_png (
      g_strconcat (executableFolder, "icons/common/pattern_1.png", NULL));
  g_assert_nonnull (image_p);
  background_pattern = cairo_pattern_create_for_surface (image_p);
  cairo_pattern_set_extend (background_pattern, CAIRO_EXTEND_REPEAT);

  inBigPictureMode = TRUE;

  GdkRectangle rect;
  GdkDisplay *display = gdk_display_get_default ();
  g_assert_nonnull (display);
  GdkMonitor *monitor = gdk_display_get_monitor (GDK_DISPLAY (display), 0);
  g_assert_nonnull (monitor);
  gdk_monitor_get_geometry (GDK_MONITOR (monitor), &rect);

  mid = (rect.width / 2.0);

  BigPictureCard *bigPictureCard
      = (BigPictureCard *)malloc (sizeof (BigPictureCard) * numCards);

  // init cards
  for (guint i = 0; i < numCards; i++)
    {
      bigPictureCard[i].index = i;
      bigPictureCard[i].drawOrder = i;
      bigPictureCard[i].position = mid + (bigPictureCard[i].index * 200.0);
      bigPictureCard[i].animate = FALSE;

      bigPictureCard[i].surface = cairo_image_surface_create_from_png (
          g_strconcat (executableFolder, "icons/cover.png", NULL));

      if (i % 5 == 0)
        {
          bigPictureCard[i].r = 1.0;
          bigPictureCard[i].g = 0.0;
          bigPictureCard[i].b = 0.0;
          bigPictureCard[i].appName = g_strdup ("Primeiro Joao");
          bigPictureCard[i].appDesc = g_strdup (
              "Lorem ipsum dolor sit amet, consectetur adipiscing elit, "
              "sed do eiusmod tempor incididunt ut labore et dolore magna "
              "aliqua. Ut enim ad minim veniam, quis nostrud exercitation "
              "ullamco laboris nisi ut aliquip ex ea commodo consequat. "
              "Duis aute irure dolor in reprehenderit in voluptate velit "
              "esse cillum dolore eu fugiat nulla pariatur.");
        }
      else if (i % 5 == 1)
        {
          bigPictureCard[i].r = 0.0;
          bigPictureCard[i].g = 1.0;
          bigPictureCard[i].b = 0.0;
          bigPictureCard[i].appName = g_strdup ("Pacman");
          bigPictureCard[i].appDesc = g_strdup (
              "Sed ut perspiciatis unde omnis iste natus error sit "
              "voluptatem accusantium doloremque laudantium, totam rem "
              "aperiam, eaque ipsa quae ab illo inventore veritatis et "
              "quasi architecto beatae vitae dicta sunt explicabo. Nemo "
              "enim ipsam voluptatem quia voluptas sit aspernatur aut odit "
              "aut fugit, sed quia consequuntur magni dolores eos qui "
              "ratione voluptatem sequi nesciunt.");
        }
      else if (i % 5 == 2)
        {
          bigPictureCard[i].r = 0.0;
          bigPictureCard[i].g = 0.0;
          bigPictureCard[i].b = 1.0;
          bigPictureCard[i].appName = g_strdup ("Ponto Turistico");
          bigPictureCard[i].appDesc = g_strdup (
              "Ut enim ad minima veniam, quis nostrum exercitationem ullam "
              "corporis suscipit laboriosam, nisi ut aliquid ex ea commodi "
              "consequatur? Quis autem vel eum iure reprehenderit qui in "
              "ea voluptate velit esse quam nihil molestiae consequatur, "
              "vel illum qui dolorem eum fugiat quo voluptas nulla "
              "pariatur?");
        }
      else if (i % 5 == 3)
        {
          bigPictureCard[i].r = 1.0;
          bigPictureCard[i].g = 0.0;
          bigPictureCard[i].b = 1.0;
          bigPictureCard[i].appName = g_strdup ("App 123456");
          bigPictureCard[i].appDesc = g_strdup (
              "At vero eos et accusamus et iusto odio dignissimos ducimus "
              "qui blanditiis praesentium voluptatum deleniti atque "
              "corrupti quos dolores et quas molestias excepturi sint "
              "occaecati cupiditate non provident, similique sunt in culpa "
              "qui officia deserunt mollitia animi, id est laborum et "
              "dolorum fuga.");
        }
      else if (i % 5 == 4)
        {
          bigPictureCard[i].r = 0.0;
          bigPictureCard[i].g = 1.0;
          bigPictureCard[i].b = 1.0;
          bigPictureCard[i].appName = g_strdup ("Asteroides");
          bigPictureCard[i].appDesc = g_strdup (
              "Nam libero tempore, cum soluta nobis est eligendi optio "
              "cumque nihil impedit quo minus id quod maxime placeat "
              "facere possimus, omnis voluptas assumenda est, omnis dolor "
              "repellendus.");
        }
      cards_list = g_list_append (cards_list, (gpointer)&bigPictureCard[i]);
    }

  bigPictureWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_assert_nonnull (bigPictureWindow);
  gtk_window_set_title (GTK_WINDOW (bigPictureWindow), "Ginga");
  gtk_window_set_default_size (GTK_WINDOW (bigPictureWindow), rect.width,
                               rect.height);
  gtk_window_set_position (GTK_WINDOW (bigPictureWindow),
                           GTK_WIN_POS_CENTER);
  g_signal_connect (bigPictureWindow, "key-press-event",
                    G_CALLBACK (keyboard_callback), (void *)"press");
  g_signal_connect (bigPictureWindow, "key-release-event",
                    G_CALLBACK (keyboard_callback), (void *)"release");
  gtk_container_set_border_width (GTK_CONTAINER (bigPictureWindow), 0);

  // Create Drawing area
  GtkWidget *canvas = gtk_drawing_area_new ();
  g_assert_nonnull (canvas);
  gtk_widget_set_app_paintable (canvas, TRUE);
  g_signal_connect (canvas, "draw", G_CALLBACK (draw_bigpicture_callback),
                    NULL);
  gtk_widget_set_size_request (canvas, rect.width, rect.height);
  gtk_container_add (GTK_CONTAINER (bigPictureWindow), canvas);
  timeOutTag = g_timeout_add (1000 / 60, (GSourceFunc)update_bigpicture_callback,
                 canvas);

  gtk_window_fullscreen (GTK_WINDOW (bigPictureWindow));

  g_signal_connect (bigPictureWindow, "destroy",
                    G_CALLBACK (destroy_bigpicture_window), NULL);

  gtk_widget_show_all (bigPictureWindow);
}*/

void
destroy_card_list(gpointer data)
{
   BigPictureCard *card = (BigPictureCard *)data;
   g_free(card->appName);
   g_free(card->appDesc);
   cairo_surface_destroy (card->surface);
}
void
destroy_bigpicture_window ()
{
  inBigPictureMode = FALSE;
  g_source_remove(timeOutTag);
  gtk_widget_destroy (bigPictureWindow);
  bigPictureWindow = NULL;
  g_list_free_full (cards_list, destroy_card_list);
  cards_list = NULL;
}
