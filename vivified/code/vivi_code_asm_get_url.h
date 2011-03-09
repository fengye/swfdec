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

#ifndef _VIVI_CODE_ASM_GET_URL_H_
#define _VIVI_CODE_ASM_GET_URL_H_

#include <vivified/code/vivi_code_asm.h>
#include <vivified/code/vivi_code_asm_code.h>

G_BEGIN_DECLS

typedef struct _ViviCodeAsmGetUrl ViviCodeAsmGetUrl;
typedef struct _ViviCodeAsmGetUrlClass ViviCodeAsmGetUrlClass;

#define VIVI_TYPE_CODE_ASM_GET_URL                    (vivi_code_asm_get_url_get_type())
#define VIVI_IS_CODE_ASM_GET_URL(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_ASM_GET_URL))
#define VIVI_IS_CODE_ASM_GET_URL_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_CODE_ASM_GET_URL))
#define VIVI_CODE_ASM_GET_URL(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_ASM_GET_URL, ViviCodeAsmGetUrl))
#define VIVI_CODE_ASM_GET_URL_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_CODE_ASM_GET_URL, ViviCodeAsmGetUrlClass))
#define VIVI_CODE_ASM_GET_URL_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_CODE_ASM_GET_URL, ViviCodeAsmGetUrlClass))

struct _ViviCodeAsmGetUrl
{
  ViviCodeAsmCode	code;

  char *		url;
  char *		target;
};

struct _ViviCodeAsmGetUrlClass
{
  ViviCodeAsmCodeClass	code_class;
};

GType		vivi_code_asm_get_url_get_type	  	(void);

ViviCodeAsm *	vivi_code_asm_get_url_new	      	(const char *		url,
							 const char *		target);

const char *	vivi_code_asm_get_url_get_url		(ViviCodeAsmGetUrl *	url);
const char *	vivi_code_asm_get_url_get_target	(ViviCodeAsmGetUrl *	url);


G_END_DECLS
#endif
