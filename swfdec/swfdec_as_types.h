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

#ifndef _SWFDEC_AS_TYPES_H_
#define _SWFDEC_AS_TYPES_H_

#include <glib-object.h>

G_BEGIN_DECLS

/* fundamental types */
typedef enum {
  SWFDEC_AS_TYPE_UNDEFINED = 0,
  SWFDEC_AS_TYPE_BOOLEAN,
  SWFDEC_AS_TYPE_INT, /* unimplemented, but reserved if someone wants it */
  SWFDEC_AS_TYPE_NUMBER,
  SWFDEC_AS_TYPE_STRING,
  SWFDEC_AS_TYPE_NULL,
  SWFDEC_AS_TYPE_OBJECT
} SwfdecAsValueType;

typedef struct _SwfdecAsArray SwfdecAsArray;
typedef struct _SwfdecAsContext SwfdecAsContext;
typedef struct _SwfdecAsDebugger SwfdecAsDebugger;
typedef struct _SwfdecAsFrame SwfdecAsFrame;
typedef struct _SwfdecAsFunction SwfdecAsFunction;
typedef struct _SwfdecAsObject SwfdecAsObject;
typedef struct _SwfdecAsScope SwfdecAsScope;
typedef struct _SwfdecAsStack SwfdecAsStack;
typedef struct _SwfdecAsValue SwfdecAsValue;
typedef void (* SwfdecAsNative) (SwfdecAsContext *	context, 
				 SwfdecAsObject *	thisp,
				 guint			argc,
				 SwfdecAsValue *	argv,
				 SwfdecAsValue *	retval);
typedef struct _SwfdecGcObject SwfdecGcObject;
typedef struct _SwfdecScript SwfdecScript;


/* IMPORTANT: a SwfdecAsValue memset to 0 is a valid undefined value */
struct _SwfdecAsValue {
  SwfdecAsValueType	type;
  /*< private >*/
  union {
    gboolean		boolean;
    double		number;
    const char *	string;
    SwfdecAsObject *	object;
  } value;
};

#define SWFDEC_IS_AS_VALUE(val) ((val) != NULL && (val)->type <= SWFDEC_TYPE_AS_OBJECT)

#define SWFDEC_AS_VALUE_IS_UNDEFINED(val) ((val)->type == SWFDEC_AS_TYPE_UNDEFINED)
#define SWFDEC_AS_VALUE_SET_UNDEFINED(val) (val)->type = SWFDEC_AS_TYPE_UNDEFINED

#define SWFDEC_AS_VALUE_IS_BOOLEAN(val) ((val)->type == SWFDEC_AS_TYPE_BOOLEAN)
#define SWFDEC_AS_VALUE_GET_BOOLEAN(val) ((val)->value.boolean)
#define SWFDEC_AS_VALUE_SET_BOOLEAN(val,b) G_STMT_START { \
  SwfdecAsValue *__val = (val); \
  gboolean __tmp = (b); \
  g_assert (__tmp == TRUE || __tmp == FALSE); \
  (__val)->value.boolean = __tmp; \
  (__val)->type = SWFDEC_AS_TYPE_BOOLEAN; \
} G_STMT_END

#define SWFDEC_AS_VALUE_IS_NUMBER(val) ((val)->type == SWFDEC_AS_TYPE_NUMBER)
#define SWFDEC_AS_VALUE_GET_NUMBER(val) ((val)->value.number)
#define SWFDEC_AS_VALUE_SET_NUMBER(val,d) G_STMT_START { \
  SwfdecAsValue *__val = (val); \
  (__val)->value.number = (d); \
  (__val)->type = SWFDEC_AS_TYPE_NUMBER; \
} G_STMT_END

#define SWFDEC_AS_VALUE_SET_INT(val,d) SWFDEC_AS_VALUE_SET_NUMBER(val,(int) (d))

#define SWFDEC_AS_VALUE_IS_STRING(val) ((val)->type == SWFDEC_AS_TYPE_STRING)
#define SWFDEC_AS_VALUE_GET_STRING(val) ((val)->value.string)
#define SWFDEC_AS_VALUE_SET_STRING(val,s) G_STMT_START { \
  SwfdecAsValue *__val = (val); \
  (__val)->value.string = s; \
  (__val)->type = SWFDEC_AS_TYPE_STRING; \
} G_STMT_END

#define SWFDEC_AS_VALUE_IS_NULL(val) ((val)->type == SWFDEC_AS_TYPE_NULL)
#define SWFDEC_AS_VALUE_SET_NULL(val) (val)->type = SWFDEC_AS_TYPE_NULL

#define SWFDEC_AS_VALUE_IS_OBJECT(val) ((val)->type == SWFDEC_AS_TYPE_OBJECT)
#if 0
#define SWFDEC_AS_VALUE_GET_OBJECT(val) ((val)->value.object)
#else
#define SWFDEC_AS_VALUE_GET_OBJECT swfdec_as_value_get_object
SwfdecAsObject *swfdec_as_value_get_object (const SwfdecAsValue *val);
#endif
#define SWFDEC_AS_VALUE_SET_OBJECT(val,o) G_STMT_START { \
  SwfdecAsValue *__val = (val); \
  SwfdecAsObject *__o = (o); \
  g_assert (__o != NULL); \
  (__val)->type = SWFDEC_AS_TYPE_OBJECT; \
  (__val)->value.object = __o; \
} G_STMT_END

/* value conversion functions */
gboolean	swfdec_as_value_to_boolean	(SwfdecAsContext *	context,
						 const SwfdecAsValue *	value);
int		swfdec_as_value_to_integer	(SwfdecAsContext *	context,
						 const SwfdecAsValue *	value);
double		swfdec_as_value_to_number	(SwfdecAsContext *	context,
						 const SwfdecAsValue *	value);
SwfdecAsObject *swfdec_as_value_to_object	(SwfdecAsContext *	context,
						 const SwfdecAsValue *	value);
void		swfdec_as_value_to_primitive	(SwfdecAsValue *	value);
const char *	swfdec_as_value_to_string	(SwfdecAsContext *	context,
						 const SwfdecAsValue *	value);
char *		swfdec_as_value_to_debug	(const SwfdecAsValue *	value);

/* special conversion functions */
const char *	swfdec_as_integer_to_string	(SwfdecAsContext *      context,
						 int			i);
int		swfdec_as_double_to_integer	(double			d);
const char *	swfdec_as_double_to_string	(SwfdecAsContext *	context,
						 double			d);
const char *	swfdec_as_str_concat		(SwfdecAsContext *	cx,
						 const char *		s1,
						 const char *		s2);


G_END_DECLS
#endif
