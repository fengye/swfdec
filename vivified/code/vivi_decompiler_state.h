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

#ifndef _VIVI_DECOMPILER_STATE_H_
#define _VIVI_DECOMPILER_STATE_H_

#include <swfdec/swfdec.h>
#include <swfdec/swfdec_script_internal.h>
#include <vivified/code/vivi_code_value.h>

G_BEGIN_DECLS


typedef struct _ViviDecompilerState ViviDecompilerState;


void				vivi_decompiler_state_free	(ViviDecompilerState *		state);
ViviDecompilerState *		vivi_decompiler_state_new	(SwfdecScript *			script,
								 const guint8 *			pc,
								 guint				n_registers);
ViviDecompilerState *		vivi_decompiler_state_copy	(const ViviDecompilerState *	src);

void				vivi_decompiler_state_push	(ViviDecompilerState *		state,
								 ViviCodeValue *		val);
ViviCodeValue *			vivi_decompiler_state_pop	(ViviDecompilerState *		state);
ViviCodeValue *			vivi_decompiler_state_peek_nth	(const ViviDecompilerState *	state,
								 guint				i);
guint				vivi_decompiler_state_get_stack_depth
								(const ViviDecompilerState *  	state);
ViviCodeValue *			vivi_decompiler_state_get_register (const ViviDecompilerState *	state,
								 guint				reg);
void				vivi_decompiler_state_set_register (ViviDecompilerState *	state,
								 guint				reg,
								 ViviCodeValue *		value);

const guint8 *			vivi_decompiler_state_get_pc	(const ViviDecompilerState *	state);
void				vivi_decompiler_state_add_pc	(ViviDecompilerState *		state,
								 int				diff);
SwfdecScript *	        	vivi_decompiler_state_get_script(const ViviDecompilerState *	state);
SwfdecConstantPool *    	vivi_decompiler_state_get_constant_pool 
								(const ViviDecompilerState *	state);
void				vivi_decompiler_state_set_constant_pool 
								(ViviDecompilerState *		state, 
								 SwfdecConstantPool *		pool);
guint				vivi_decompiler_state_get_version
								(const ViviDecompilerState *  	state);

gboolean			vivi_decompiler_state_is_compatible	
								(const ViviDecompilerState *	a,
								 const ViviDecompilerState *	b);



G_END_DECLS
#endif
