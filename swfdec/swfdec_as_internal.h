/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_AS_INTERNAL_H_
#define _SWFDEC_AS_INTERNAL_H_

#include <swfdec/swfdec_as_object.h>
#include <swfdec/swfdec_as_types.h>

G_BEGIN_DECLS

/* This header contains all the non-exported symbols that can't go into 
 * exported headers 
 */
#define SWFDEC_AS_NATIVE(x, y, func) SWFDEC_AS_CONSTRUCTOR (x, y, func, NULL)
#define SWFDEC_AS_CONSTRUCTOR(x, y, func, type) void func (SwfdecAsContext *cx, \
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret);

#define SWFDEC_AS_OBJECT_PROTOTYPE_RECURSION_LIMIT 256

#define SWFDEC_AS_GC_MARK (1 << 0)		/* only valid during GC */
#define SWFDEC_AS_GC_ROOT (1 << 1)		/* for objects: rooted, for strings: static */

void		swfdec_as_function_set_constructor	(SwfdecAsFunction *	fun);
void		swfdec_as_function_init_context		(SwfdecAsContext *	context);

/* swfdec_as_context.c */
gboolean	swfdec_as_context_check_continue (SwfdecAsContext *	context);
void		swfdec_as_context_run		(SwfdecAsContext *	context);
void		swfdec_as_context_run_init_script (SwfdecAsContext *	context,
						 const guint8 *		data,
						 gsize			length,
						 guint			version);

/* swfdec_as_object.c */
typedef SwfdecAsVariableForeach SwfdecAsVariableForeachRemove;
typedef const char *(* SwfdecAsVariableForeachRename) (SwfdecAsObject *object, 
    const char *variable, SwfdecAsValue *value, guint flags, gpointer data);

SwfdecAsValue *	swfdec_as_object_peek_variable	(SwfdecAsObject *       object,
						 const char *		name);
guint		swfdec_as_object_foreach_remove	(SwfdecAsObject *       object,
						 SwfdecAsVariableForeach func,
						 gpointer		data);
void		swfdec_as_object_foreach_rename	(SwfdecAsObject *       object,
						 SwfdecAsVariableForeachRename func,
						 gpointer		data);

void		swfdec_as_object_init_context	(SwfdecAsContext *	context);
void		swfdec_as_object_decode		(SwfdecAsObject *	obj,
						 const char *		str);
SwfdecAsObject * swfdec_as_object_get_prototype (SwfdecAsObject *	object);
void		swfdec_as_object_add_native_variable (SwfdecAsObject *	object,
						 const char *		variable,
						 SwfdecAsNative		get,
						 SwfdecAsNative		set);
void		swfdec_as_object_set_constructor_by_name 
						(SwfdecAsObject *	object,
						 const char *		name,
						 ...) G_GNUC_NULL_TERMINATED;


G_END_DECLS
#endif
