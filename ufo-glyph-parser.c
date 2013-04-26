                                                                            /*
Kernagic a libre auto spacer and kerner for Unified Font Objects.
Copyright (C) 2013 Øyvind Kolås

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.       */

#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "kernagic.h"

static int   cc = 0;
static float cx[2];
static float cy[2];
static float scx = 0;
static float scy = 0;
static int   sc = 0;
static int   first = 0;

extern gboolean kernagic_strip_bearing; /* XXX: global and passed out of bounds.. */

static void
parse_start_element (GMarkupParseContext *context,
                     const gchar         *element_name,
                     const gchar        **attribute_names,
                     const gchar        **attribute_values,
                     gpointer             user_data,
                     GError             **error)
{
  Glyph *glyph = user_data;
  if (!strcmp (element_name, "glyph"))
    {
      const char **a_n, **a_v;
      for (a_n = attribute_names,
           a_v = attribute_values; *a_n; a_n++, a_v++)
        {
          if (!strcmp (*a_n, "name"))
            glyph->name = strdup (*a_v);
        }
      glyph->min_x =  8192;
      glyph->min_y =  8192;
      glyph->max_x = -8192;
      glyph->max_y = -8192;
    }
  else if (!strcmp (element_name, "advance"))
    {
      const char **a_n, **a_v;
      for (a_n = attribute_names,
           a_v = attribute_values; *a_n; a_n++, a_v++)
        {
          if (!strcmp (*a_n, "width"))
            glyph->advance = atoi (*a_v);
        }
    }
  else if (!strcmp (element_name, "unicode"))
    {
      const char **a_n, **a_v;
      for (a_n = attribute_names,
           a_v = attribute_values; *a_n; a_n++, a_v++)
        {
          if (!strcmp (*a_n, "hex"))
          {
            unsigned int value;
            if(sscanf(*a_v, "%X;", &value) != 1)
               printf("Parse error\n");
            glyph->unicode = value;
          }
        }
    }
  else if (!strcmp (element_name, "point"))
    {
      float x = 0;
      float y = 0;
      const char **a_n, **a_v;
      for (a_n = attribute_names,
           a_v = attribute_values; *a_n; a_n++, a_v++)
        {
          if (!strcmp (*a_n, "x"))
            x = atof (*a_v);
          if (!strcmp (*a_n, "y"))
            y = atof (*a_v);
        }
      if (x < glyph->min_x) glyph->min_x = x;
      if (y < glyph->min_y) glyph->min_y = y;
      if (x > glyph->max_x) glyph->max_x = x;
      if (y > glyph->max_y) glyph->max_y = y;
    }
}

static void
parse_end_element (GMarkupParseContext *context,
                   const gchar         *element_name,
                   gpointer             user_data,
                   GError             **error)
{
  //Glyph *glyph = user_data;
}


static void
glif_start_element (GMarkupParseContext *context,
                    const gchar         *element_name,
                    const gchar        **attribute_names,
                    const gchar        **attribute_values,
                    gpointer             user_data,
                    GError             **error)
{
  Glyph *glyph = user_data;
  cairo_t *cr = glyph->cr;

  if (!strcmp (element_name, "point"))
    {
      int   offcurve = 1;
      int   curveto = 0;
      float x = 0, y = 0;
      const char **a_n, **a_v;
      for (a_n = attribute_names,
           a_v = attribute_values; *a_n; a_n++, a_v++)
        {
          if (!strcmp (*a_n, "x"))
            x = atof (*a_v) + glyph->strip_offset;
          else if (!strcmp (*a_n, "y"))
            y = atof (*a_v);
          else if (!strcmp (*a_n, "type"))
            {
              if (!strcmp (*a_v, "line") ||
                  !strcmp (*a_v, "curve"))
                offcurve = 0;
              if (!strcmp (*a_v, "curve"))
                curveto = 1;
            }
        }
      if (offcurve)
        {
          cx[cc] = x;
          cy[cc] = y;
          cc++;
          assert (cc <= 2);
        }
      else
        {
          if (curveto && cc == 2)
              cairo_curve_to (cr, cx[0], cy[0],
                                  cx[1], cy[1],
                                  x, y);
          else if (curveto && cc == 1)
              cairo_curve_to (cr, cx[0], cy[0],
                                  cx[0], cy[0],
                                  x, y);
          else if (curveto && cc == 0)
            {
              if (first)
                {
                  scx = x;
                  scy = y;
                  sc = 1;
                  cairo_move_to (cr, x, y);
                }
              else
                {
                  cairo_curve_to (cr, cx[0], cy[0],
                                      cx[0], cy[0],
                                      x, y);
                }
            }
          else
              cairo_line_to (cr, x, y);
          cc = 0;
        }
      first = 0;
    }
  else if (!strcmp (element_name, "contour"))
    {
      cairo_new_sub_path (cr);
      first = 1;
      sc = 0;
      cc = 0;
    }
}

static void
glif_end_element (GMarkupParseContext *context,
                  const gchar         *element_name,
                  gpointer             user_data,
                  GError             **error)
{
  Glyph *glyph = user_data;
  cairo_t *cr = glyph->cr;

  if (!strcmp (element_name, "contour"))
  {
    if (sc)
      {
        assert (cc == 2);
        cairo_curve_to (cr, cx[0], cy[0],
                            cx[1], cy[1],
                            scx, scy);
      }
  }
  else if (!strcmp (element_name, "outline"))
    {
      cairo_set_source_rgb (cr, 0.0, 0.0,0.0);
      cairo_fill_preserve (cr);
    }
}

static GMarkupParser glif_parse =
{ parse_start_element, parse_end_element, NULL, NULL, NULL };

static GMarkupParser glif_render =
{ glif_start_element, glif_end_element, NULL, NULL, NULL };

extern float  scale_factor;

void render_glyph (Glyph *glyph)
{
  cairo_t *cr = glyph->cr;
  GMarkupParseContext *ctx;
  cairo_save (cr);
  ctx = g_markup_parse_context_new (&glif_render, 0, glyph, NULL);
  g_markup_parse_context_parse (ctx, glyph->xml, strlen (glyph->xml), NULL);
  g_markup_parse_context_free (ctx);

  cairo_restore (cr);
}

void
load_ufo_glyph (Glyph *glyph)
{
  GMarkupParseContext *ctx = g_markup_parse_context_new (&glif_parse, 0, glyph, NULL);
  g_markup_parse_context_parse (ctx, glyph->xml, strlen (glyph->xml), NULL);
  g_markup_parse_context_free (ctx);

  glyph->width = glyph->max_x - glyph->min_x;
  glyph->height = glyph->max_y - glyph->min_y;

  if (kernagic_strip_bearing)
    {
      glyph->strip_offset = -glyph->min_x;

      glyph->min_x   += glyph->strip_offset;
      glyph->max_x   += glyph->strip_offset;
      glyph->advance += glyph->strip_offset;
    }
}

void
render_ufo_glyph (Glyph *glyph)
{
  cairo_surface_t *surface;
  cairo_t *cr;
  assert (glyph->xml);

  load_ufo_glyph (glyph);
  

  {
    glyph->r_width = kernagic_x_height () * scale_factor * 2.0;
    glyph->r_height = kernagic_x_height () * scale_factor * 2.8;
    glyph->r_width /= 16;
    glyph->r_width *= 16;

    glyph->raster = calloc (glyph->r_width * glyph->r_height, 1);
  }
  int width = glyph->r_width;
  int height = glyph->r_height;
  uint8_t *raster = glyph->raster;
  assert (raster);

  surface = cairo_image_surface_create_for_data (raster, CAIRO_FORMAT_A8,
      width, height, width);
  cr = cairo_create (surface);
  glyph->cr = cr;

  cairo_set_source_rgba (cr, 1, 1, 1, 0);
  cairo_paint(cr);

  cairo_translate (cr, 0, kernagic_x_height () * 2.0 * scale_factor);
  cairo_scale (cr, scale_factor, -scale_factor);
  render_glyph (glyph);

  cairo_destroy (cr);
  glyph->cr = NULL;
  cairo_surface_destroy (surface);
}
