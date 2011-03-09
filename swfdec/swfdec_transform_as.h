/* Swfdec
 * Copyright (C) 2008 Pekka Lampila <pekka.lampila@iki.fi>
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

#ifndef _SWFDEC_TRANSFORM_H_
#define _SWFDEC_TRANSFORM_H_

#include <swfdec/swfdec_movie.h>
#include <swfdec/swfdec_as_object.h>
#include <swfdec/swfdec_as_types.h>

G_BEGIN_DECLS

typedef struct _SwfdecTransformAs SwfdecTransformAs;
typedef struct _SwfdecTransformAsClass SwfdecTransformAsClass;

#define SWFDEC_TYPE_TRANSFORM_AS                    (swfdec_transform_as_get_type())
#define SWFDEC_IS_TRANSFORM_AS(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_TRANSFORM_AS))
#define SWFDEC_IS_TRANSFORM_AS_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_TRANSFORM_AS))
#define SWFDEC_TRANSFORM_AS(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_TRANSFORM_AS, SwfdecTransformAs))
#define SWFDEC_TRANSFORM_AS_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_TRANSFORM_AS, SwfdecTransformAsClass))
#define SWFDEC_TRANSFORM_AS_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_TRANSFORM_AS, SwfdecTransformAsClass))

struct _SwfdecTransformAs {
  SwfdecAsObject	object;

  SwfdecMovie *		target;
};

struct _SwfdecTransformAsClass {
  SwfdecAsObjectClass	object_class;
};

GType			swfdec_transform_as_get_type	(void);

SwfdecTransformAs *	swfdec_transform_as_new		(SwfdecAsContext *		context,
							 SwfdecMovie *			target);

G_END_DECLS
#endif
