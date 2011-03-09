/* Swfdec
 * Copyright (c) 2008 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_RENDERER_INTERNAL_H_
#define _SWFDEC_RENDERER_INTERNAL_H_

#include <swfdec/swfdec_renderer.h>
#include <swfdec/swfdec_cached.h>
#include <swfdec/swfdec_color.h>

G_BEGIN_DECLS


typedef gboolean SwfdecRendererSearchFunc (SwfdecCached *cached, gpointer data);

SwfdecRenderer *	swfdec_renderer_new_default	(SwfdecPlayer *		player);

void			swfdec_renderer_attach		(SwfdecRenderer *	renderer,
							 cairo_t *		cr);
SwfdecRenderer *	swfdec_renderer_get		(cairo_t *		cr);
void			swfdec_renderer_reset_matrix	(cairo_t *		cr);

void			swfdec_renderer_add_cache	(SwfdecRenderer *	renderer,
							 gboolean		replace,
							 gpointer		key,
							 SwfdecCached *		value);
SwfdecCached *		swfdec_renderer_get_cache	(SwfdecRenderer *	renderer,
							 gpointer		key,
							 SwfdecRendererSearchFunc func,
							 gpointer		data);
gsize			swfdec_renderer_get_max_cache_size
							(SwfdecRenderer *	renderer);

cairo_surface_t *	swfdec_renderer_create_similar	(SwfdecRenderer *	renderer,
							 cairo_surface_t *	surface);
cairo_surface_t *	swfdec_renderer_create_for_data	(SwfdecRenderer *	renderer,
							 guint8 *		data,
							 cairo_format_t		format,
							 guint			width,
							 guint			height,
							 guint			rowstride);
cairo_surface_t *	swfdec_renderer_transform	(SwfdecRenderer *	renderer,
							 cairo_surface_t *	surface,
							 const SwfdecColorTransform *trans);


G_END_DECLS
#endif
