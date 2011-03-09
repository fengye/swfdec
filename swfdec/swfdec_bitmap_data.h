/* Swfdec
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#ifndef _SWFDEC_BITMAP_DATA_H_
#define _SWFDEC_BITMAP_DATA_H_

#include <cairo.h>
#include <swfdec/swfdec_as_object.h>

G_BEGIN_DECLS

typedef struct _SwfdecBitmapData SwfdecBitmapData;
typedef struct _SwfdecBitmapDataClass SwfdecBitmapDataClass;

#define SWFDEC_TYPE_BITMAP_DATA                    (swfdec_bitmap_data_get_type())
#define SWFDEC_IS_BITMAP_DATA(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_BITMAP_DATA))
#define SWFDEC_IS_BITMAP_DATA_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_BITMAP_DATA))
#define SWFDEC_BITMAP_DATA(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_BITMAP_DATA, SwfdecBitmapData))
#define SWFDEC_BITMAP_DATA_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_BITMAP_DATA, SwfdecBitmapDataClass))

struct _SwfdecBitmapData {
  SwfdecAsObject	object;

  cairo_surface_t *	surface;	/* An image surface or NULL */
};

struct _SwfdecBitmapDataClass {
  SwfdecAsObjectClass	object_class;
};

GType			swfdec_bitmap_data_get_type		(void);

SwfdecBitmapData *	swfdec_bitmap_data_new			(SwfdecAsContext *	context,
								 gboolean		transparent,
								 guint			width,
								 guint			height);


G_END_DECLS
#endif
