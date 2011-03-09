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

#ifndef _VIVI_CODE_TOKEN_H_
#define _VIVI_CODE_TOKEN_H_

#include <swfdec/swfdec.h>

G_BEGIN_DECLS


typedef struct _ViviCodePrinter ViviCodePrinter;
typedef struct _ViviCodeAssembler ViviCodeAssembler;
typedef struct _ViviCodeCompiler ViviCodeCompiler;

typedef struct _ViviCodeToken ViviCodeToken;
typedef struct _ViviCodeTokenClass ViviCodeTokenClass;

#define VIVI_TYPE_CODE_TOKEN                    (vivi_code_token_get_type())
#define VIVI_IS_CODE_TOKEN(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_TOKEN))
#define VIVI_IS_CODE_TOKEN_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_CODE_TOKEN))
#define VIVI_CODE_TOKEN(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_TOKEN, ViviCodeToken))
#define VIVI_CODE_TOKEN_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_CODE_TOKEN, ViviCodeTokenClass))
#define VIVI_CODE_TOKEN_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_CODE_TOKEN, ViviCodeTokenClass))

struct _ViviCodeToken
{
  GObject		object;
};

struct _ViviCodeTokenClass
{
  GObjectClass		object_class;

  void			(* print)			(ViviCodeToken *	token,
							 ViviCodePrinter *	printer);
  void			(* compile)			(ViviCodeToken *	token,
							 ViviCodeCompiler *	compiler);
};

GType			vivi_code_token_get_type   	(void);

void			vivi_code_token_compile		(ViviCodeToken *	token,
							 ViviCodeCompiler *	compiler);


G_END_DECLS
#endif
