/* Vivified
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

#ifndef _VIVI_CODE_ASSIGNMENT_H_
#define _VIVI_CODE_ASSIGNMENT_H_

#include <vivified/code/vivi_code_value.h>
#include <vivified/code/vivi_code_value.h>

G_BEGIN_DECLS


typedef struct _ViviCodeAssignment ViviCodeAssignment;
typedef struct _ViviCodeAssignmentClass ViviCodeAssignmentClass;

#define VIVI_TYPE_CODE_ASSIGNMENT                    (vivi_code_assignment_get_type())
#define VIVI_IS_CODE_ASSIGNMENT(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_ASSIGNMENT))
#define VIVI_IS_CODE_ASSIGNMENT_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_CODE_ASSIGNMENT))
#define VIVI_CODE_ASSIGNMENT(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_ASSIGNMENT, ViviCodeAssignment))
#define VIVI_CODE_ASSIGNMENT_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_CODE_ASSIGNMENT, ViviCodeAssignmentClass))
#define VIVI_CODE_ASSIGNMENT_ASSIGNMENT_CLASS(obj)          (G_TYPE_INSTANCE_ASSIGNMENT_CLASS ((obj), VIVI_TYPE_CODE_ASSIGNMENT, ViviCodeAssignmentClass))

struct _ViviCodeAssignment
{
  ViviCodeValue		token;

  ViviCodeValue *	from;
  ViviCodeValue *	name;
  ViviCodeValue	*	value;
};

struct _ViviCodeAssignmentClass
{
  ViviCodeValueClass	value_class;
};

GType			vivi_code_assignment_get_type		(void);

ViviCodeValue *		vivi_code_assignment_new		(ViviCodeValue *	from,
								 ViviCodeValue *	name,
								 ViviCodeValue *	value);
ViviCodeValue *		vivi_code_assignment_new_name		(const char *		name,
								 ViviCodeValue *	value);


G_END_DECLS
#endif
