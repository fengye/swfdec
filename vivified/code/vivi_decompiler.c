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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include <swfdec/swfdec_as_interpret.h>
#include <swfdec/swfdec_bits.h>
#include <swfdec/swfdec_script_internal.h>

#include "vivi_decompiler.h"
#include "vivi_code_and.h"
#include "vivi_code_assignment.h"
#include "vivi_code_binary_default.h"
#include "vivi_code_block.h"
#include "vivi_code_boolean.h"
#include "vivi_code_break.h"
#include "vivi_code_builtin_value_statement_default.h"
#include "vivi_code_constant.h"
#include "vivi_code_continue.h"
#include "vivi_code_function.h"
#include "vivi_code_function_call.h"
#include "vivi_code_get.h"
#include "vivi_code_get_url.h"
#include "vivi_code_goto.h"
#include "vivi_code_if.h"
#include "vivi_code_init_object.h"
#include "vivi_code_loop.h"
#include "vivi_code_number.h"
#include "vivi_code_null.h"
#include "vivi_code_or.h"
#include "vivi_code_return.h"
#include "vivi_code_string.h"
#include "vivi_code_not.h"
#include "vivi_code_undefined.h"
#include "vivi_code_variable.h"
#include "vivi_decompiler_block.h"
#include "vivi_decompiler_duplicate.h"
#include "vivi_decompiler_state.h"
#include "vivi_decompiler_unknown.h"

#if 0
#define DEBUG g_printerr
#else
#define DEBUG(...)
#endif

#if 0
static G_GNUC_UNUSED void
DUMP_BLOCKS (GList *blocks)
{
  GList *walk;

  g_print ("dumping blocks:\n");
  for (walk = blocks; walk; walk = walk->next) {
    ViviDecompilerBlock *block = walk->data;
    g_printerr ("  %p -> %p\n", vivi_decompiler_block_get_start (block), 
	block->end ? vivi_decompiler_state_get_pc (block->end) : NULL);
  }
}
#else
#define DUMP_BLOCKS(dec) (void) 0
#endif

static ViviDecompilerBlock *
vivi_decompiler_push_block_for_state (GList **blocks, ViviDecompilerState *state)
{
  ViviDecompilerBlock *block;
  GList *walk;
  const guint8 *pc, *block_start;

  pc = vivi_decompiler_state_get_pc (state);
  DEBUG ("pc: %p\n", pc);
  DUMP_BLOCKS (*blocks);
  for (walk = *blocks; walk; walk = walk->next) {
    block = walk->data;
    block_start = vivi_decompiler_block_get_start (block);
    if (block_start < pc) {
      if (vivi_decompiler_block_contains (block, pc)) {
	vivi_decompiler_block_reset (block, FALSE);
	walk = walk->next;
	break;
      }
      continue;
    }
    if (block_start == pc) {
      const ViviDecompilerState *block_state = vivi_decompiler_block_get_start_state (block);

      if (!vivi_decompiler_state_is_compatible (block_state, state)) {
	g_printerr ("incompatible states, the decompiled code will be _really_ fucked up.\n");
	vivi_decompiler_state_free (state);
	return block;
      }
      if (!vivi_decompiler_block_is_compatible (block, state))
	vivi_decompiler_block_reset (block, TRUE);
      vivi_decompiler_state_free (state);
      return block;
    }
    break;
  }

  /* FIXME: see if the block is already there! */
  block = vivi_decompiler_block_new (state);
  *blocks = g_list_insert_before (*blocks, walk, block);
  return block;
}

/*** BYTECODE DECOMPILER ***/

typedef gboolean (* DecompileFunc) (ViviDecompilerBlock *block, ViviDecompilerState *state,
          guint code, const guint8 *data, guint len);

static gboolean
vivi_decompile_push (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviCodeValue *val;
  SwfdecBits bits;
  guint type;

  swfdec_bits_init_data (&bits, data, len);
  while (swfdec_bits_left (&bits)) {
    type = swfdec_bits_get_u8 (&bits);
    switch (type) {
      case 0: /* string */
	{
	  char *s = swfdec_bits_get_string (&bits, vivi_decompiler_state_get_version (state));
	  if (s == NULL) {
	    vivi_decompiler_block_add_error (block, state, "could not read string");
	    return FALSE;
	  }
	  val = vivi_code_string_new (s);
	  g_free (s);
	  break;
	}
      case 1: /* float */
	val = vivi_code_number_new (swfdec_bits_get_float (&bits));
	vivi_code_number_set_hint (VIVI_CODE_NUMBER (val),
	    VIVI_CODE_NUMBER_HINT_FLOAT);
	break;
      case 2: /* null */
	val = vivi_code_null_new ();
	break;
      case 3: /* undefined */
	val = vivi_code_undefined_new ();
	break;
      case 4: /* register */
	val = vivi_decompiler_state_get_register (
	      state, swfdec_bits_get_u8 (&bits));
	break;
      case 5: /* boolean */
	val = vivi_code_boolean_new (swfdec_bits_get_u8 (&bits) ? TRUE : FALSE);
	break;
      case 6: /* double */
	val = vivi_code_number_new (swfdec_bits_get_double (&bits));
	vivi_code_number_set_hint (VIVI_CODE_NUMBER (val),
	    VIVI_CODE_NUMBER_HINT_DOUBLE);
	break;
      case 7: /* 32bit int */
	val = vivi_code_number_new (swfdec_bits_get_s32 (&bits));
	vivi_code_number_set_hint (VIVI_CODE_NUMBER (val),
	    VIVI_CODE_NUMBER_HINT_INT);
	break;
      case 8: /* 8bit ConstantPool address */
      case 9: /* 16bit ConstantPool address */
	{
	  guint i = type == 8 ? swfdec_bits_get_u8 (&bits) : swfdec_bits_get_u16 (&bits);
	  SwfdecConstantPool *pool = vivi_decompiler_state_get_constant_pool (state);
	  if (pool == NULL) {
	    vivi_decompiler_block_add_error (block, state, "no constant pool to push from");
	    return TRUE;
	  }
	  if (i >= swfdec_constant_pool_size (pool)) {
	    vivi_decompiler_block_add_error (block, state, "constant pool index %u too high - only %u elements",
		i, swfdec_constant_pool_size (pool));
	    return TRUE;
	  }
	  val = vivi_code_string_new (swfdec_constant_pool_get (pool, i));
	  break;
	}
      default:
	vivi_decompiler_block_add_error (block, state, "Push: type %u unknown, skipping", type);
	val = NULL;
	break;
    }
    if (val != NULL)
      vivi_decompiler_state_push (state, val);
  }

  return TRUE;
}

static gboolean
vivi_decompile_pop (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviCodeValue *val;

  val = vivi_decompiler_state_pop (state);
  vivi_code_block_add_statement (VIVI_CODE_BLOCK (block),
      VIVI_CODE_STATEMENT (val));
  g_object_unref (val);
  return TRUE;
}

static gboolean
vivi_decompile_constant_pool (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  SwfdecConstantPool *pool;
  SwfdecBuffer *buffer;
  
  buffer = vivi_decompiler_state_get_script (state)->buffer;
  buffer = swfdec_buffer_new_subbuffer (buffer, data - buffer->data, len);
  pool = swfdec_constant_pool_new (NULL, buffer, 
      vivi_decompiler_state_get_version (state));
  swfdec_buffer_unref (buffer);

  vivi_decompiler_state_set_constant_pool (state, pool);
  return TRUE;
}

static gboolean
vivi_decompile_trace (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviCodeStatement *trace;
  ViviCodeValue *val;
  
  val = vivi_decompiler_state_pop (state);
  trace = vivi_code_trace_new (val);
  g_object_unref (val);
  vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), trace);
  g_object_unref (trace);
  return TRUE;
}

static gboolean
vivi_decompile_end (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviCodeStatement *ret;

  ret = vivi_code_return_new ();
  vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), ret);
  g_object_unref (ret);

  vivi_decompiler_block_finish (block, state);
  return FALSE;
}

static gboolean
vivi_decompile_return (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviCodeStatement *ret;
  ViviCodeValue *value;

  value = vivi_decompiler_state_pop (state);
  ret = vivi_code_return_new ();
  if (!VIVI_IS_CODE_UNDEFINED (value))
    vivi_code_return_set_value (VIVI_CODE_RETURN (ret), value);
  g_object_unref (value);
  vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), ret);
  g_object_unref (ret);

  vivi_decompiler_block_finish (block, state);
  return FALSE;
}

static gboolean
vivi_decompile_get_url2 (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviCodeValue *url, *target;

  target = vivi_decompiler_state_pop (state);
  url = vivi_decompiler_state_pop (state);

  if (len != 1) {
    vivi_decompiler_block_add_error (block, state, "invalid length in getURL2 command");   
  } else {
    ViviCodeStatement *stmt;
    guint method = data[0] & 3;
    guint internal = data[0] & 64;
    guint variables = data[0] & 128;

    stmt = vivi_code_get_url_new (target, url, method, internal, variables);
    vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), stmt);
    g_object_unref (stmt);
  }
  g_object_unref (target);
  g_object_unref (url);
  return TRUE;
}

static gboolean
vivi_decompile_not (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviCodeValue *val, *not;

  val = vivi_decompiler_state_pop (state);
  not = vivi_code_not_new (val);
  vivi_decompiler_state_push (state, not);
  g_object_unref (val);
  return TRUE;
}

static gboolean
vivi_decompile_get_variable (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviCodeValue *get, *name;

  name = vivi_decompiler_state_pop (state);
  get = vivi_code_get_new (NULL, name);
  g_object_unref (name);
  vivi_decompiler_state_push (state, get);
  return TRUE;
}

static gboolean
vivi_decompile_get_member (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviCodeValue *get, *from, *name;

  name = vivi_decompiler_state_pop (state);
  from = vivi_decompiler_state_pop (state);
  get = vivi_code_get_new (from, name);
  g_object_unref (from);
  g_object_unref (name);
  vivi_decompiler_state_push (state, get);
  return TRUE;
}

static gboolean
vivi_decompile_set_variable (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviCodeValue *name, *value;
  ViviCodeStatement *assign;

  value = vivi_decompiler_state_pop (state);
  name = vivi_decompiler_state_pop (state);
  assign = VIVI_CODE_STATEMENT (vivi_code_assignment_new (NULL, name, value));
  g_object_unref (name);
  g_object_unref (value);
  vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), assign);
  return TRUE;
}

static gboolean
vivi_decompile_set_member (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviCodeValue *from, *name, *value;
  ViviCodeStatement *assign;

  value = vivi_decompiler_state_pop (state);
  name = vivi_decompiler_state_pop (state);
  from = vivi_decompiler_state_pop (state);
  assign = VIVI_CODE_STATEMENT (vivi_code_assignment_new (from, name, value));
  g_object_unref (from);
  g_object_unref (name);
  g_object_unref (value);
  vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), assign);
  return TRUE;
}

static gboolean
vivi_decompile_define_local (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviCodeValue *name, *value;
  ViviCodeStatement *variable;

  if (code == SWFDEC_AS_ACTION_DEFINE_LOCAL)
    value = vivi_decompiler_state_pop (state);
  else
    value = vivi_code_undefined_new ();
  name = vivi_decompiler_state_pop (state);

  variable = vivi_code_variable_new (name, value);
  g_object_unref (name);
  g_object_unref (value);
  vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), variable);
  return TRUE;
}

static gboolean
vivi_decompile_binary (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviCodeValue *right, *left, *result;
#define DEFAULT_BINARY(CapsName, underscore_name, operatore_name, bytecode, precedence) \
    case bytecode: \
      result = vivi_code_ ## underscore_name ## _new (left, right); \
      break;

  right = vivi_decompiler_state_pop (state);
  left = vivi_decompiler_state_pop (state);
  switch (code) {
#include "vivi_code_defaults.h"
    default:
      g_assert_not_reached ();
      return FALSE;
  }
  g_object_unref (left);
  g_object_unref (right);
  vivi_decompiler_state_push (state, result);
  return TRUE;
}

static gboolean
vivi_decompile_duplicate (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviCodeValue *value, *dupl;

  value = vivi_decompiler_state_pop (state);
  vivi_decompiler_state_push (state, value);
  dupl = vivi_decompiler_duplicate_new (value);
  vivi_decompiler_state_push (state, dupl);
  return TRUE;
}

static ViviCodeValue *
vivi_decompile_function_call (ViviDecompilerBlock *block, ViviDecompilerState *state,
    ViviCodeValue *value, ViviCodeValue *name, ViviCodeValue *args)
{
  ViviCodeValue *call;
  double d;
  guint i, count;

  call = vivi_code_function_call_new (value, name);
  if (value)
    g_object_unref (value);
  if (name)
    g_object_unref (name);

  if (!VIVI_IS_CODE_NUMBER (args) || 
      ((count = d = vivi_code_number_get_value (VIVI_CODE_NUMBER (args))) != d)) {
    vivi_decompiler_block_add_error (block, state, "could not determine function argument count");
    g_object_unref (args);
    g_object_unref (call);
    return NULL;
  }
  g_object_unref (args);

  count = MIN (count, vivi_decompiler_state_get_stack_depth (state));
  for (i = 0; i < count; i++) {
    value = vivi_decompiler_state_pop (state);
    vivi_code_function_call_add_argument (VIVI_CODE_FUNCTION_CALL (call), value);
    g_object_unref (value);
  }
  vivi_decompiler_state_push (state, call);
  return call;
}

static gboolean
vivi_decompile_call (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviCodeValue *call, *name, *value, *args;

  name = vivi_decompiler_state_pop (state);
  if (code == SWFDEC_AS_ACTION_CALL_METHOD || code == SWFDEC_AS_ACTION_NEW_METHOD)
    value = vivi_decompiler_state_pop (state);
  else
    value = NULL;
  args = vivi_decompiler_state_pop (state);

  call = vivi_decompile_function_call (block, state, value, name, args);
  if (!call)
    return FALSE;
  if (code == SWFDEC_AS_ACTION_NEW_OBJECT || code == SWFDEC_AS_ACTION_NEW_METHOD)
    vivi_code_function_call_set_construct (VIVI_CODE_FUNCTION_CALL (call), TRUE);
  return TRUE;
}

static gboolean
vivi_decompile_define_function (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviCodeValue *value;
  char *function_name;
  const char *name = NULL;
  guint i, n_args, size, n_registers;
  SwfdecBits bits;
  SwfdecBuffer *buffer;
  SwfdecScript *script;
  guint flags = 0;
  SwfdecScriptArgument *args;
  gboolean v2 = (code == 0x8e);

  swfdec_bits_init_data (&bits, data, len);
  script = vivi_decompiler_state_get_script (state);
  function_name = swfdec_bits_get_string (&bits, script->version);
  if (function_name == NULL) {
    vivi_decompiler_block_add_error (block, state, "could not parse function name");
    return FALSE;
  }
  n_args = swfdec_bits_get_u16 (&bits);
  if (v2) {
    n_registers = swfdec_bits_get_u8 (&bits);
    if (n_registers == 0)
      n_registers = 4;
    flags = swfdec_bits_get_u16 (&bits);
  } else {
    n_registers = 5;
  }
  if (n_args) {
    args = g_new0 (SwfdecScriptArgument, n_args);
    for (i = 0; i < n_args && swfdec_bits_left (&bits); i++) {
      if (v2) {
	args[i].preload = swfdec_bits_get_u8 (&bits);
	if (args[i].preload && args[i].preload >= n_registers) {
	  vivi_decompiler_block_add_error (block, state, 
	      "argument %u cannot be preloaded into register %u out of %u", 
	      i, args[i].preload, n_registers);
	  /* FIXME: figure out correct error handling here */
	  args[i].preload = 0;
	}
      }
      args[i].name = swfdec_bits_get_string (&bits, script->version);
      if (args[i].name == NULL || args[i].name == '\0') {
	vivi_decompiler_block_add_error (block, state, "empty argument name not allowed");
	g_free (args);
	return FALSE;
      }
      /* FIXME: check duplicate arguments */
    }
  } else {
    args = NULL;
  }
  size = swfdec_bits_get_u16 (&bits);
  buffer = script->buffer;
  /* check the script can be created */
  if (vivi_decompiler_state_get_pc (state) + 3 + len + size > buffer->data + buffer->length) {
    vivi_decompiler_block_add_error (block, state, "size of function is too big");
    g_free (args);
    g_free (function_name);
    return FALSE;
  }
  /* create the script */
  buffer = swfdec_buffer_new_subbuffer (buffer, 
      vivi_decompiler_state_get_pc (state) + 3 + len - buffer->data, size);
  swfdec_bits_init (&bits, buffer);
  if (*function_name) {
    name = function_name;
  } else if (vivi_decompiler_state_get_stack_depth (state) > 0) {
    /* This is kind of a hack that uses a feature of the Adobe compiler:
     * foo = function () {} is compiled as these actions:
     * Push "foo", DefineFunction, SetVariable/SetMember
     * With this knowledge we can inspect the topmost stack member, since
     * it will contain the name this function will soon be assigned to.
     */
    value = vivi_decompiler_state_peek_nth (state, 0);
    if (VIVI_IS_CODE_STRING (value))
      name = vivi_code_string_get_value (VIVI_CODE_STRING (value));
  }
  if (name == NULL)
    name = "unnamed_function";
  script = swfdec_script_new_from_bits (&bits, name, script->version);
  swfdec_buffer_unref (buffer);
  if (script == NULL) {
    vivi_decompiler_block_add_error (block, state, "failed to create script");
    g_free (args);
    g_free (function_name);
    return FALSE;
  }
  if (vivi_decompiler_state_get_constant_pool (state)) {
    script->constant_pool = swfdec_buffer_ref (swfdec_constant_pool_get_buffer (
	  vivi_decompiler_state_get_constant_pool (state)));
  }
  script->flags = flags;
  script->n_registers = n_registers;
  script->n_arguments = n_args;
  script->arguments = args;

  value = vivi_code_function_new_from_script (script);
  /* attach the function */
  if (*function_name == '\0') {
    vivi_decompiler_state_push (state, value);
  } else {
    g_printerr ("FIXME: handle named functions");
  }

  vivi_decompiler_state_add_pc (state, 3 + len + size);
  g_free (function_name);
  return TRUE;
}

static gboolean
vivi_decompile_store_register (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviCodeValue *value;
  
  if (len != 1) {
    vivi_decompiler_block_add_error (block, state, "invalid length %u in StoreRegister", len);
    return FALSE;
  }

  value = vivi_decompiler_state_pop (state);
  if (!vivi_code_value_is_constant (value)) {
    ViviCodeStatement *assign;
    char *name;

    name = g_strdup_printf ("$reg%u", data[0]);
    assign = VIVI_CODE_STATEMENT (vivi_code_assignment_new_name (name, value));
    vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), assign);
    g_object_unref (assign);

    g_object_unref (value);
    value = vivi_code_get_new_name (NULL, name);
    g_free (name);
  }
  vivi_decompiler_state_set_register (state, data[0], value);
  vivi_decompiler_state_push (state, value);
  return TRUE;
}

static gboolean
vivi_decompile_init_object (ViviDecompilerBlock *block, ViviDecompilerState *state,
    guint code, const guint8 *data, guint len)
{
  ViviCodeValue *args, *init, *name, *value;
  double d;
  guint i, count;

  args = vivi_decompiler_state_pop (state);
  if (!VIVI_IS_CODE_NUMBER (args) || 
      ((count = d = vivi_code_number_get_value (VIVI_CODE_NUMBER (args))) != d)) {
    vivi_decompiler_block_add_error (block, state, "could not determine init object argument count");
    g_object_unref (args);
    return FALSE;
  }
  g_object_unref (args);
  count = MIN (count, (vivi_decompiler_state_get_stack_depth (state) + 1) / 2);

  init = vivi_code_init_object_new ();
  for (i = 0; i < count; i++) {
    value = vivi_decompiler_state_pop (state);
    name = vivi_decompiler_state_pop (state);
    vivi_code_init_object_add_variable (VIVI_CODE_INIT_OBJECT (init), name, value);
    g_object_unref (name);
    g_object_unref (value);
  }
  vivi_decompiler_state_push (state, init);
  return TRUE;
}

static DecompileFunc decompile_funcs[256] = {
  [SWFDEC_AS_ACTION_END] = vivi_decompile_end,
  [SWFDEC_AS_ACTION_NEXT_FRAME] = NULL,
  [SWFDEC_AS_ACTION_PREVIOUS_FRAME] = NULL,
  [SWFDEC_AS_ACTION_PLAY] = NULL,
  [SWFDEC_AS_ACTION_STOP] = NULL,
  [SWFDEC_AS_ACTION_TOGGLE_QUALITY] = NULL,
  [SWFDEC_AS_ACTION_STOP_SOUNDS] = NULL,
  [SWFDEC_AS_ACTION_ADD] = vivi_decompile_binary,
  [SWFDEC_AS_ACTION_SUBTRACT] = vivi_decompile_binary,
  [SWFDEC_AS_ACTION_MULTIPLY] = vivi_decompile_binary,
  [SWFDEC_AS_ACTION_DIVIDE] = vivi_decompile_binary,
  [SWFDEC_AS_ACTION_EQUALS] = vivi_decompile_binary,
  [SWFDEC_AS_ACTION_LESS] = vivi_decompile_binary,
  [SWFDEC_AS_ACTION_AND] = vivi_decompile_binary,
  [SWFDEC_AS_ACTION_OR] = vivi_decompile_binary,
  [SWFDEC_AS_ACTION_NOT] = vivi_decompile_not,
  [SWFDEC_AS_ACTION_STRING_EQUALS] = vivi_decompile_binary,
  [SWFDEC_AS_ACTION_STRING_LENGTH] = NULL,
  [SWFDEC_AS_ACTION_STRING_EXTRACT] = NULL,
  [SWFDEC_AS_ACTION_POP] = vivi_decompile_pop,
  [SWFDEC_AS_ACTION_TO_INTEGER] = NULL,
  [SWFDEC_AS_ACTION_GET_VARIABLE] = vivi_decompile_get_variable,
  [SWFDEC_AS_ACTION_SET_VARIABLE] = vivi_decompile_set_variable,
  [SWFDEC_AS_ACTION_SET_TARGET2] = NULL,
  [SWFDEC_AS_ACTION_STRING_ADD] = NULL,
  [SWFDEC_AS_ACTION_GET_PROPERTY] = NULL,
  [SWFDEC_AS_ACTION_SET_PROPERTY] = NULL,
  [SWFDEC_AS_ACTION_CLONE_SPRITE] = NULL,
  [SWFDEC_AS_ACTION_REMOVE_SPRITE] = NULL,
  [SWFDEC_AS_ACTION_TRACE] = vivi_decompile_trace,
  [SWFDEC_AS_ACTION_START_DRAG] = NULL,
  [SWFDEC_AS_ACTION_END_DRAG] = NULL,
  [SWFDEC_AS_ACTION_STRING_LESS] = vivi_decompile_binary,
  [SWFDEC_AS_ACTION_THROW] = NULL,
  [SWFDEC_AS_ACTION_CAST] = NULL,
  [SWFDEC_AS_ACTION_IMPLEMENTS] = NULL,
  [SWFDEC_AS_ACTION_RANDOM] = NULL,
  [SWFDEC_AS_ACTION_MB_STRING_LENGTH] = NULL,
  [SWFDEC_AS_ACTION_CHAR_TO_ASCII] = NULL,
  [SWFDEC_AS_ACTION_ASCII_TO_CHAR] = NULL,
  [SWFDEC_AS_ACTION_GET_TIME] = NULL,
  [SWFDEC_AS_ACTION_MB_STRING_EXTRACT] = NULL,
  [SWFDEC_AS_ACTION_MB_CHAR_TO_ASCII] = NULL,
  [SWFDEC_AS_ACTION_MB_ASCII_TO_CHAR] = NULL,
  [SWFDEC_AS_ACTION_DELETE] = NULL,
  [SWFDEC_AS_ACTION_DELETE2] = NULL,
  [SWFDEC_AS_ACTION_DEFINE_LOCAL] = vivi_decompile_define_local,
  [SWFDEC_AS_ACTION_CALL_FUNCTION] = vivi_decompile_call,
  [SWFDEC_AS_ACTION_RETURN] = vivi_decompile_return,
  [SWFDEC_AS_ACTION_MODULO] = vivi_decompile_binary,
  [SWFDEC_AS_ACTION_NEW_OBJECT] = vivi_decompile_call,
  [SWFDEC_AS_ACTION_DEFINE_LOCAL2] = vivi_decompile_define_local,
  [SWFDEC_AS_ACTION_INIT_ARRAY] = NULL,
  [SWFDEC_AS_ACTION_INIT_OBJECT] = vivi_decompile_init_object,
  [SWFDEC_AS_ACTION_TYPE_OF] = NULL,
  [SWFDEC_AS_ACTION_TARGET_PATH] = NULL,
  [SWFDEC_AS_ACTION_ENUMERATE] = NULL,
  [SWFDEC_AS_ACTION_ADD2] = vivi_decompile_binary,
  [SWFDEC_AS_ACTION_LESS2] = vivi_decompile_binary,
  [SWFDEC_AS_ACTION_EQUALS2] = vivi_decompile_binary,
  [SWFDEC_AS_ACTION_TO_NUMBER] = NULL,
  [SWFDEC_AS_ACTION_TO_STRING] = NULL,
  [SWFDEC_AS_ACTION_PUSH_DUPLICATE] = vivi_decompile_duplicate,
  [SWFDEC_AS_ACTION_SWAP] = NULL,
  [SWFDEC_AS_ACTION_GET_MEMBER] = vivi_decompile_get_member,
  [SWFDEC_AS_ACTION_SET_MEMBER] = vivi_decompile_set_member,
  [SWFDEC_AS_ACTION_INCREMENT] = NULL,
  [SWFDEC_AS_ACTION_DECREMENT] = NULL,
  [SWFDEC_AS_ACTION_CALL_METHOD] = vivi_decompile_call,
  [SWFDEC_AS_ACTION_NEW_METHOD] = vivi_decompile_call,
  [SWFDEC_AS_ACTION_INSTANCE_OF] = NULL,
  [SWFDEC_AS_ACTION_ENUMERATE2] = NULL,
  [SWFDEC_AS_ACTION_BREAKPOINT] = NULL,
  [SWFDEC_AS_ACTION_BIT_AND] = vivi_decompile_binary,
  [SWFDEC_AS_ACTION_BIT_OR] = vivi_decompile_binary,
  [SWFDEC_AS_ACTION_BIT_XOR] = vivi_decompile_binary,
  [SWFDEC_AS_ACTION_BIT_LSHIFT] = vivi_decompile_binary,
  [SWFDEC_AS_ACTION_BIT_RSHIFT] = vivi_decompile_binary,
  [SWFDEC_AS_ACTION_BIT_URSHIFT] = vivi_decompile_binary,
  [SWFDEC_AS_ACTION_STRICT_EQUALS] = vivi_decompile_binary,
  [SWFDEC_AS_ACTION_GREATER] = vivi_decompile_binary,
  [SWFDEC_AS_ACTION_STRING_GREATER] = vivi_decompile_binary,
  [SWFDEC_AS_ACTION_EXTENDS] = NULL,

  [SWFDEC_AS_ACTION_GOTO_FRAME] = NULL,
  [SWFDEC_AS_ACTION_GET_URL] = NULL,
  [SWFDEC_AS_ACTION_STORE_REGISTER] = vivi_decompile_store_register,
  [SWFDEC_AS_ACTION_CONSTANT_POOL] = vivi_decompile_constant_pool,
  [SWFDEC_AS_ACTION_STRICT_MODE] = NULL,
  [SWFDEC_AS_ACTION_WAIT_FOR_FRAME] = NULL,
  [SWFDEC_AS_ACTION_SET_TARGET] = NULL,
  [SWFDEC_AS_ACTION_GOTO_LABEL] = NULL,
  [SWFDEC_AS_ACTION_WAIT_FOR_FRAME2] = NULL,
  [SWFDEC_AS_ACTION_DEFINE_FUNCTION2] = NULL, /* handled directly */
  [SWFDEC_AS_ACTION_TRY] = NULL,
  [SWFDEC_AS_ACTION_WITH] = NULL,
  [SWFDEC_AS_ACTION_PUSH] = vivi_decompile_push,
  [SWFDEC_AS_ACTION_JUMP] = NULL, /* handled directly */
  [SWFDEC_AS_ACTION_GET_URL2] = vivi_decompile_get_url2,
  [SWFDEC_AS_ACTION_DEFINE_FUNCTION] = NULL, /* handled directly */
  [SWFDEC_AS_ACTION_IF] = NULL, /* handled directly */
  [SWFDEC_AS_ACTION_CALL] = NULL,
  [SWFDEC_AS_ACTION_GOTO_FRAME2] = NULL
};

static gboolean
vivi_decompiler_process (GList **blocks, ViviDecompilerBlock *block, 
    ViviDecompilerState *state, guint code, const guint8 *data, guint len)
{
  gboolean result;

  switch (code) {
    case SWFDEC_AS_ACTION_DEFINE_FUNCTION:
    case SWFDEC_AS_ACTION_DEFINE_FUNCTION2:
      return vivi_decompile_define_function (block, state, code, data, len);
    case SWFDEC_AS_ACTION_IF:
      {
	ViviDecompilerBlock *next, *branch;
	ViviCodeValue *val;
	ViviDecompilerState *new;
	gint16 offset;

	vivi_decompiler_state_add_pc (state, 5);
	if (len != 2) {
	  vivi_decompiler_block_add_error (block, state, "If action length invalid (is %u, should be 2)", len);
	  vivi_decompiler_block_finish (block, state);
	  return TRUE;
	}
	offset = data[0] | (data[1] << 8);
	val = vivi_decompiler_state_pop (state);
	vivi_decompiler_block_finish (block, state);
	new = vivi_decompiler_state_copy (state);
	next = vivi_decompiler_push_block_for_state (blocks, new);
	new = vivi_decompiler_state_copy (state);
	vivi_decompiler_state_add_pc (new, offset);
	branch = vivi_decompiler_push_block_for_state (blocks, new);
	if (vivi_decompiler_block_is_finished (block)) {
	  vivi_decompiler_block_set_next (block, next);
	  vivi_decompiler_block_set_branch (block, branch, val);
	}
	g_object_unref (val);
      }
      return FALSE;
    case SWFDEC_AS_ACTION_JUMP:
      {
	ViviDecompilerBlock *next;
	ViviDecompilerState *new;
	gint16 offset;

	vivi_decompiler_state_add_pc (state, 5);
	if (len != 2) {
	  vivi_decompiler_block_add_error (block, state, "Jump action length invalid (is %u, should be 2)", len);
	  vivi_decompiler_block_finish (block, state);
	  return FALSE;
	}
	offset = data[0] | (data[1] << 8);
	vivi_decompiler_block_finish (block, state);
	new = vivi_decompiler_state_copy (state);
	vivi_decompiler_state_add_pc (new, offset);
	next = vivi_decompiler_push_block_for_state (blocks, new);
	if (vivi_decompiler_block_is_finished (block)) {
	  vivi_decompiler_block_set_next (block, next);
	}
      }
      return FALSE;
    default:
      if (data)
	vivi_decompiler_state_add_pc (state, 3 + len);
      else
	vivi_decompiler_state_add_pc (state, 1);
      if (decompile_funcs[code]) {
	result = decompile_funcs[code] (block, state, code, data, len);
      } else {
	vivi_decompiler_block_add_warning (block, "unknown bytecode 0x%02X %u", code, code);
	result = TRUE;
      }
      return result;
  };

  return TRUE;
}

static GList *
vivi_decompiler_block_decompile (GList *blocks, ViviDecompilerBlock *block, SwfdecScript *script)
{
  ViviDecompilerState *state;
  ViviDecompilerBlock *next_block;
  GList *list;
  const guint8 *pc, *start, *end, *exit;
  const guint8 *data;
  guint code, len;

  start = script->buffer->data;
  end = start + script->buffer->length;
  state = vivi_decompiler_state_copy (vivi_decompiler_block_get_start_state (block));
  exit = script->exit;
  list = g_list_find (blocks, block);
  if (list->next) {
    next_block = list->next->data;
    exit = vivi_decompiler_block_get_start (next_block);
  } else {
    next_block = NULL;
  }

  while ((pc = vivi_decompiler_state_get_pc (state)) != exit) {
    if (pc < start || pc >= end) {
      vivi_decompiler_block_add_error (block, state, "program counter out of range");
      goto error;
    }
    code = pc[0];
    if (code & 0x80) {
      if (pc + 2 >= end) {
	vivi_decompiler_block_add_error (block, state, "bytecode %u length value out of range", code);
	goto error;
      }
      data = pc + 3;
      len = pc[1] | pc[2] << 8;
      if (data + len > end) {
	vivi_decompiler_block_add_error (block, state, "bytecode %u length %u out of range", code, len);
	goto error;
      }
    } else {
      data = NULL;
      len = 0;
    }
    if (!vivi_decompiler_process (&blocks, block, state, code, data, len))
      goto out;
  }
  vivi_decompiler_block_finish (block, state);
  if (next_block) {
    vivi_decompiler_block_set_next (block,
	vivi_decompiler_push_block_for_state (&blocks, state));
  } else {
    ViviCodeStatement *ret;

    ret = vivi_code_return_new ();
    vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), ret);
    g_object_unref (ret);

out:
error:
    vivi_decompiler_state_free (state);
  }
  return blocks;
}

/*** PROGRAM STRUCTURE ANALYSIS ***/

static ViviDecompilerBlock *
vivi_decompiler_find_start_block (GList *list, const guint8 *startpc)
{
  GList *walk;
  
  for (walk = list; walk; walk = walk->next) {
    ViviDecompilerBlock *block = walk->data;

    if (vivi_decompiler_block_get_start (block) == startpc)
      return block;
  }
  g_assert_not_reached ();
  return NULL;
}

static void
vivi_decompiler_remove_return (ViviCodeBlock *block)
{
  ViviCodeStatement *stmt;
  guint len;

  while ((len = vivi_code_block_get_n_statements (block))) {
    stmt = vivi_code_block_get_statement (block, len - 1);
    if (VIVI_IS_CODE_RETURN (stmt) &&
	vivi_code_return_get_value (VIVI_CODE_RETURN (stmt)) == NULL) {
      vivi_code_block_remove_statement (block, stmt);
    } else {
      break;
    }
  }
}

#define ASSERT_BLOCK_LIST(list) \
  { \
    GList *_walk; \
    for (_walk = list; _walk; _walk = _walk->next) { \
      ViviDecompilerBlock *_block = _walk->data; \
      g_assert (_block->next == NULL || g_list_find (list, _block->next)); \
      g_assert (_block->branch == NULL || g_list_find (list, _block->branch)); \
    } \
  }

static ViviCodeStatement *
vivi_decompiler_merge_blocks_last_resort (GList *list, const guint8 *startpc)
{
  ViviCodeBlock *block;
  ViviDecompilerBlock *current, *next;
  GList *ordered, *walk;

  //ASSERT_BLOCK_LIST (list);
  current = vivi_decompiler_find_start_block (list, startpc);

  ordered = NULL;
  while (current) {
    g_assert (g_list_find (list, current));
    list = g_list_remove (list, current);
    ordered = g_list_prepend (ordered, current);
    next = vivi_decompiler_block_get_branch (current);
    if (next)
      vivi_decompiler_block_force_label (next);
    next = vivi_decompiler_block_get_next (current);
    if (next == NULL || !g_list_find (list, next)) {
      if (next)
	vivi_decompiler_block_force_label (next);
      next = list ? list->data : NULL;
    }
    current = next;
  }
  g_assert (list == NULL);
  ordered = g_list_reverse (ordered);

  block = VIVI_CODE_BLOCK (vivi_code_block_new ());
  for (walk = ordered; walk; walk = walk->next) {
    current = walk->data;
    next = vivi_decompiler_block_get_next (current);
    if (walk->next && next == walk->next->data) {
      vivi_decompiler_block_add_to_block (current, block);
      /* FIXME: somehow remove gotos and block forcing */
    } else {
      vivi_decompiler_block_add_to_block (current, block);
    }
    vivi_decompiler_block_set_next (current, NULL);
    vivi_decompiler_block_set_branch (current, NULL, NULL);
  }
  g_list_foreach (ordered, (GFunc) g_object_unref, NULL);
  g_list_free (ordered);

  vivi_decompiler_remove_return (block);
  return VIVI_CODE_STATEMENT (block);
}

static GList *
vivi_decompiler_purge_block (GList *list, ViviDecompilerBlock *block)
{
  g_assert (vivi_decompiler_block_get_n_incoming (block) == 0);
  list = g_list_remove (list, block);
  g_object_unref (block);
  return list;
}

/*  ONE
 *   |              ==>   BLOCK
 *  TWO
 */
static gboolean
vivi_decompiler_merge_lines (GList **list)
{
  ViviDecompilerBlock *block, *next;
  ViviCodeValue *val;
  gboolean result;
  GList *walk;

  result = FALSE;
  for (walk = *list; walk; walk = walk->next) {
    block = walk->data;

    /* This is an if block or so */
    if (vivi_decompiler_block_get_branch (block) != NULL)
      continue;
    /* has no next block */
    next = vivi_decompiler_block_get_next (block);
    if (next == NULL || next == block)
      continue;
    /* The next block has multiple incoming blocks */
    if (vivi_decompiler_block_get_n_incoming (next) != 1)
      continue;

    vivi_decompiler_block_set_next (block, vivi_decompiler_block_get_next (next));
    val = vivi_decompiler_block_get_branch_condition (next);
    if (val) {
      vivi_decompiler_block_set_branch (block, 
	  vivi_decompiler_block_get_branch (next), val);
    }
    vivi_decompiler_block_set_next (next, NULL);
    vivi_decompiler_block_set_branch (next, NULL, NULL);
    vivi_decompiler_block_add_state_transition (block, next, VIVI_CODE_BLOCK (block));
    vivi_decompiler_block_add_to_block (next, VIVI_CODE_BLOCK (block));
    vivi_decompiler_block_finish (block, vivi_decompiler_block_get_start_state (next));
    *list = vivi_decompiler_purge_block (*list, next);
    result = TRUE;
  }

  return result;
}

/*      DUP
 *     (NOT)
 *     BLOCK
 *    /   |
 *[ANDOR] |
 *    \   |
 *     NEXT
 */
static gboolean
vivi_decompiler_merge_andor (GList **list)
{
  ViviDecompilerBlock *block, *andor, *next;
  gboolean result;
  ViviCodeValue *value, *value2;
  ViviDecompilerState *state;
  ViviCodeValue * (* func) (ViviCodeValue *, ViviCodeValue *);

  GList *walk;
  result = FALSE;
  for (walk = *list; walk; walk = walk->next) {
    block = walk->data;

    next = vivi_decompiler_block_get_branch (block);
    andor = vivi_decompiler_block_get_next (block);

    /* not an if block */
    if (andor == NULL)
      continue;

    /* not an && or || block */
    if (vivi_decompiler_block_get_n_incoming (andor) > 1 ||
	next != vivi_decompiler_block_get_next (andor) ||
	vivi_decompiler_block_get_branch (andor) != NULL)
      continue;
    /* extract the value */
    value = vivi_decompiler_block_get_branch_condition (block);
    if (VIVI_IS_CODE_NOT (value)) {
      value = vivi_code_not_get_value (VIVI_CODE_NOT (value));
      func = vivi_code_and_new;
    } else {
      func = vivi_code_or_new;
    }
    if (!VIVI_IS_DECOMPILER_DUPLICATE (value))
      continue;
    value = vivi_decompiler_duplicate_get_value (VIVI_DECOMPILER_DUPLICATE (value));
    /* check the extracted value is the only statement in the and/or block */
    switch (vivi_code_block_get_n_statements (VIVI_CODE_BLOCK (andor))) {
      case 0:
	break;
      case 1:
	{
	  ViviCodeStatement *stmt = vivi_code_block_get_statement (VIVI_CODE_BLOCK (andor), 0);
	  if (!VIVI_IS_CODE_VALUE (stmt))
	    continue;
	}
	break;
      default:
	continue;
    }
    value2 = vivi_decompiler_state_peek_nth (vivi_decompiler_block_get_end_state (block), 0);
    if (value != value2)
      continue;
    value2 = vivi_decompiler_state_peek_nth (vivi_decompiler_block_get_end_state (andor), 0);
    if (value2 == NULL)
      continue;

    /* setting code starts here */
    DEBUG ("merging %s code for %p\n", type, vivi_decompiler_block_get_start (andor));

    /* update our finish state */
    value = func (value, value2);
    state = (ViviDecompilerState *) vivi_decompiler_block_get_end_state (block);
    g_object_unref (vivi_decompiler_state_pop (state));
    vivi_decompiler_state_push (state, value);
    /* get rid of the and/or block */
    vivi_decompiler_block_set_branch (block, NULL, NULL);
    vivi_decompiler_block_set_next (block, next);
    *list = vivi_decompiler_purge_block (*list, andor);
    /* possibly update the start state of the next block */
    if (vivi_decompiler_block_get_n_incoming (next) == 1) {
      value2 = vivi_decompiler_state_peek_nth (vivi_decompiler_block_get_start_state (next), 0);
      if (VIVI_IS_DECOMPILER_UNKNOWN (value2) &&
	  vivi_decompiler_unknown_get_block (VIVI_DECOMPILER_UNKNOWN (value2)) == next) {
	vivi_decompiler_unknown_set_value (VIVI_DECOMPILER_UNKNOWN (value2), value);
      }
    }
    return TRUE;
  }
  return FALSE;
}

/*     COND
 *    /    \
 *  [IF] [ELSE]     ==>   BLOCK
 *    \    /
 *     NEXT
 */
static gboolean
vivi_decompiler_merge_if (GList **list)
{
  ViviDecompilerBlock *block, *if_block, *else_block, *next;
  ViviCodeBlock *b;
  ViviCodeIf *if_stmt;
  ViviCodeStatement *stmt;
  gboolean result;
  GList *walk;

  result = FALSE;
  for (walk = *list; walk; walk = walk->next) {
    block = walk->data;

    if_block = vivi_decompiler_block_get_branch (block);
    else_block = vivi_decompiler_block_get_next (block);
    /* not an if block */
    if (if_block == NULL)
      continue;
    /* one of the blocks doesn't exist */
    if (if_block == vivi_decompiler_block_get_next (else_block)) 
      if_block = NULL;
    else if (else_block == vivi_decompiler_block_get_next (if_block))
      else_block = NULL;
    /* if in if in if in if... */
    if ((else_block && vivi_decompiler_block_get_branch (else_block)) || 
	(if_block && vivi_decompiler_block_get_branch (if_block)))
      continue;
    /* if other blocks reference the blocks, bail, there's loops involved */
    if ((else_block && vivi_decompiler_block_get_n_incoming (else_block) > 1) ||
	(if_block && vivi_decompiler_block_get_n_incoming (if_block) > 1))
      continue;
    /* if both blocks exist, they must have the same exit block */
    if (if_block && else_block && 
	vivi_decompiler_block_get_next (if_block) != vivi_decompiler_block_get_next (else_block))
      continue;

    /* FINALLY we can merge the blocks */
    if_stmt = VIVI_CODE_IF (vivi_code_if_new (vivi_decompiler_block_get_branch_condition (block)));
    vivi_decompiler_block_set_branch (block, NULL, NULL);
    next = vivi_decompiler_block_get_next (if_block ? if_block : else_block);
    vivi_decompiler_block_set_next (block, next);
    /* assign the if block */
    b = VIVI_CODE_BLOCK (vivi_code_block_new ());
    if (if_block) {
      vivi_decompiler_block_set_next (if_block, NULL);
      vivi_decompiler_block_set_branch (if_block, NULL, NULL);
      vivi_decompiler_block_add_to_block (if_block, b);
      if (next)
	vivi_decompiler_block_add_state_transition (if_block, next, b);
      *list = vivi_decompiler_purge_block (*list, if_block);
    } else {
      if (next)
	vivi_decompiler_block_add_state_transition (block, next, b);
    }
    stmt = vivi_code_statement_optimize (VIVI_CODE_STATEMENT (b));
    g_object_unref (b);
    if (stmt) {
      vivi_code_if_set_if (if_stmt, stmt);
      g_object_unref (stmt);
    }
    /* assign the else block */
    b = VIVI_CODE_BLOCK (vivi_code_block_new ());
    if (else_block) {
      vivi_decompiler_block_set_next (else_block, NULL);
      vivi_decompiler_block_set_branch (else_block, NULL, NULL);
      vivi_decompiler_block_add_to_block (else_block, b);
      if (next)
	vivi_decompiler_block_add_state_transition (else_block, next, b);
      *list = vivi_decompiler_purge_block (*list, else_block);
    } else {
      if (next)
	vivi_decompiler_block_add_state_transition (block, next, b);
    }
    stmt = vivi_code_statement_optimize (VIVI_CODE_STATEMENT (b));
    g_object_unref (b);
    if (stmt) {
      vivi_code_if_set_else (if_stmt, stmt);
      g_object_unref (stmt);
    }
    /* finally finish the if statement */
    stmt = vivi_code_statement_optimize (VIVI_CODE_STATEMENT (if_stmt));
    g_object_unref (if_stmt);
    if (stmt) {
      vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), stmt);
      g_object_unref (stmt);
    }
    if (next)
      vivi_decompiler_block_finish (block, vivi_decompiler_block_get_start_state (next));
    result = TRUE;
  }

  return result;
}

static ViviCodeStatement *vivi_decompiler_merge_blocks (GList *blocks,
    const guint8 *startpc);

static gboolean
vivi_decompiler_merge_loops (GList **list)
{
  ViviDecompilerBlock *end, *start, *block, *next;
  ViviCodeBlock *body;
  gboolean result;
  GList *walk, *walk2;
  const guint8 *loop_start, *loop_end;
  GList *contained, *to_check;
  ViviCodeStatement *loop, *stmt;
  guint len;

  result = FALSE;
  for (walk = *list; walk; walk = walk->next) {
    end = walk->data;
    /* noone has a branch at the end of a loop */
    if (vivi_decompiler_block_get_branch (end))
      continue;
    start = vivi_decompiler_block_get_next (end);
    /* block that just returns - no loop at all */
    if (start == NULL)
      continue;
    /* not a jump backwards, so no loop end */
    if (vivi_decompiler_block_get_start (start) >
	vivi_decompiler_block_get_start (end))
      continue;
    /* We've found that "end" is a jump backwards. Now, this can be 3 things:
     * a) the end of a loop
     *    Woohoo, we've found our loop end.
     * b) a "continue" statement
     *    Wait, we need to find where the lop ends!
     * c) a goto backwards
     *    Whoops, we need to cleanly exit this loop!
     * In case a) and b), "start" will already be correct. So we'll assume that
     * it is and go from there.
     */
    loop_start = vivi_decompiler_block_get_start (start);
    /* this is just a rough guess for now */
    loop_end = vivi_decompiler_block_get_start (end);
    /* let's find the rest of the loop */
    to_check = g_list_prepend (NULL, start);
    contained = NULL;
    while (to_check) {
      block = to_check->data;
      to_check = g_list_remove (to_check, block);
      /* jump to before start?! */
      if (vivi_decompiler_block_get_start (block) < loop_start) {
	g_list_free (contained);
	g_list_free (to_check);
	goto failed;
      }
      /* found a new end of the loop */
      if (vivi_decompiler_block_get_start (block) > loop_end) {
	ViviDecompilerBlock *swap;
	loop_end = vivi_decompiler_block_get_start (block);
	swap = block;
	block = end;
	end = swap;
      }
      contained = g_list_prepend (contained, block);
      next = vivi_decompiler_block_get_next (block);
      if (next && next != end && 
	  !g_list_find (contained, next) && 
          !g_list_find (to_check, next))
	to_check = g_list_prepend (to_check, next);
      next = vivi_decompiler_block_get_branch (block);
      if (next && next != end && 
	  !g_list_find (contained, next) && 
          !g_list_find (to_check, next))
	to_check = g_list_prepend (to_check, next);
    }
    contained = g_list_reverse (contained);
    contained = g_list_remove (contained, start);
    /* now we have:
     * contained: contains all the blocks contained in the loop
     * start: starting block
     * end: end block - where "breaks" go to
     */
    loop = vivi_code_loop_new ();
    /* check if the first block is just a branch, in that case it's the
     * loop condition */
    if (vivi_code_block_get_n_statements (VIVI_CODE_BLOCK (start)) == 0 &&
	(vivi_decompiler_block_get_branch (start) == end ||
	 (vivi_decompiler_block_get_branch (start) &&
	  vivi_decompiler_block_get_next (start) == end))) {
      if (vivi_decompiler_block_get_branch (start) == end) {
	ViviCodeValue *value, *opt;
	value = vivi_code_not_new (vivi_decompiler_block_get_branch_condition (start));
	opt = vivi_code_value_optimize (value, SWFDEC_AS_TYPE_BOOLEAN);
	g_object_unref (value);
	vivi_code_loop_set_condition (VIVI_CODE_LOOP (loop), opt, FALSE);
	g_object_unref (opt);
      } else {
	vivi_code_loop_set_condition (VIVI_CODE_LOOP (loop), 
	    vivi_decompiler_block_get_branch_condition (start), FALSE);
	vivi_decompiler_block_set_next (start,
	    vivi_decompiler_block_get_branch (start));
      }
      vivi_decompiler_block_set_branch (start, NULL, NULL);
      loop_start = vivi_decompiler_block_get_start (
	  vivi_decompiler_block_get_next (start));
      vivi_code_block_add_statement (VIVI_CODE_BLOCK (start), VIVI_CODE_STATEMENT (loop));
      vivi_decompiler_block_set_next (start, end);
    } else if (start == end) {
      /* very simple for (;;) loop */
      g_assert (contained == NULL);
      contained = g_list_prepend (contained, start);
      block = vivi_decompiler_block_new (vivi_decompiler_state_copy (
	    vivi_decompiler_block_get_start_state (start)));
      vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), VIVI_CODE_STATEMENT (loop));
      if (vivi_decompiler_block_get_branch (start)) {
	ViviCodeValue *value, *opt;
	value = vivi_code_not_new (vivi_decompiler_block_get_branch_condition (start));
	opt = vivi_code_value_optimize (value, SWFDEC_AS_TYPE_BOOLEAN);
	g_object_unref (value);
	vivi_code_loop_set_condition (VIVI_CODE_LOOP (loop), opt, FALSE);
	g_object_unref (opt);
	vivi_decompiler_block_set_next (block, vivi_decompiler_block_get_branch (start));
	vivi_decompiler_block_set_branch (start, NULL, NULL);
      }
      vivi_decompiler_block_set_next (start, NULL);
      for (walk2 = *list; walk2; walk2 = walk2->next) {
	if (walk2->data == start) {
	  walk2->data = block;
	  break;
	}
      }
    } else {
      /* FIXME: for (;;) loop */
      contained = g_list_prepend (contained, start);
      block = vivi_decompiler_block_new (vivi_decompiler_state_copy (
	    vivi_decompiler_block_get_start_state (start)));
      vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), VIVI_CODE_STATEMENT (loop));
      for (walk2 = *list; walk2; walk2 = walk2->next) {
	if (walk2->data == start) {
	  walk2->data = block;
	  continue;
	}
	if (g_list_find (contained, walk2->data))
	  continue;
	if (vivi_decompiler_block_get_branch (walk2->data) == start)
	  vivi_decompiler_block_set_branch (walk2->data, block,
	      g_object_ref (vivi_decompiler_block_get_branch_condition (walk2->data)));
	if (vivi_decompiler_block_get_next (walk2->data) == start)
	  vivi_decompiler_block_set_next (walk2->data, block);
      }
      vivi_decompiler_block_set_next (block, end);
    }
    /* break all connections to the outside */
    for (walk2 = contained; walk2; walk2 = walk2->next) {
      block = walk2->data;
      *list = g_list_remove (*list, block);
      next = vivi_decompiler_block_get_branch (block);
      if (next != NULL && (!g_list_find (contained, next) || next == start)) {
	stmt = vivi_code_if_new (g_object_ref (vivi_decompiler_block_get_branch_condition (block)));
	if (next == start) {
	  vivi_code_if_set_if (VIVI_CODE_IF (stmt), vivi_code_continue_new ());
	} else if (next == end) {
	  vivi_code_if_set_if (VIVI_CODE_IF (stmt), vivi_code_break_new ());
	} else {
	  vivi_decompiler_block_force_label (next);
	  vivi_code_if_set_if (VIVI_CODE_IF (stmt), VIVI_CODE_STATEMENT (vivi_code_goto_new (
		VIVI_CODE_LABEL (vivi_decompiler_block_get_label (next)))));
	}
	vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), stmt);
	vivi_decompiler_block_set_branch (block, NULL, NULL);
      }
      next = vivi_decompiler_block_get_next (block);
      if (next != NULL && (!g_list_find (contained, next) || next == start)) {
	if (next == start) {
	  vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), vivi_code_continue_new ());
	} else if (next == end) {
	  vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), vivi_code_break_new ());
	} else {
	  vivi_decompiler_block_force_label (next);
	  vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), VIVI_CODE_STATEMENT (
		vivi_code_goto_new (VIVI_CODE_LABEL (vivi_decompiler_block_get_label (next)))));
	}
	vivi_decompiler_block_set_next (block, NULL);
      }
    }
    /* break all connections from the outside */
    for (walk2 = *list; walk2; walk2 = walk2->next) {
      block = walk2->data;
      next = vivi_decompiler_block_get_branch (block);
      if (next && !g_list_find (*list, next)) {
	stmt = vivi_code_if_new (g_object_ref (vivi_decompiler_block_get_branch_condition (block)));
	vivi_decompiler_block_force_label (next);
	vivi_code_if_set_if (VIVI_CODE_IF (stmt), VIVI_CODE_STATEMENT (vivi_code_goto_new (
	      VIVI_CODE_LABEL (vivi_decompiler_block_get_label (next)))));
	vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), stmt);
	vivi_decompiler_block_set_branch (block, NULL, NULL);
      }
      next = vivi_decompiler_block_get_next (block);
      if (next && !g_list_find (*list, next)) {
	vivi_decompiler_block_force_label (next);
	vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), VIVI_CODE_STATEMENT (
	      vivi_code_goto_new (VIVI_CODE_LABEL (vivi_decompiler_block_get_label (next)))));
	vivi_decompiler_block_set_next (block, NULL);
      }
    }
    body = VIVI_CODE_BLOCK (vivi_decompiler_merge_blocks (contained, loop_start));
    /* remove all continue statements at the end */
    while ((len = vivi_code_block_get_n_statements (body))) {
      stmt = vivi_code_block_get_statement (body, len - 1);
      if (VIVI_IS_CODE_CONTINUE (stmt)) {
	vivi_code_block_remove_statement (body, stmt);
      } else {
	break;
      }
    }
    vivi_code_loop_set_statement (VIVI_CODE_LOOP (loop), VIVI_CODE_STATEMENT (body));
    g_object_unref (body);
    g_object_unref (loop);

    return TRUE;
failed:
    continue;
  }
  return FALSE;
}

static ViviCodeStatement *
vivi_decompiler_merge_blocks (GList *blocks, const guint8 *startpc)
{
  gboolean restart;

  do {
    restart = FALSE;
    DEBUG ("merging: %u blocks left\n", g_list_length (blocks));

    restart |= vivi_decompiler_merge_lines (&blocks);
    if (vivi_decompiler_merge_andor (&blocks)) {
      restart = TRUE;
      continue;
    }
    restart |= vivi_decompiler_merge_if (&blocks);
    restart |= vivi_decompiler_merge_loops (&blocks);
  } while (restart);

  return vivi_decompiler_merge_blocks_last_resort (blocks, startpc);
}

static void
vivi_decompiler_dump_graphviz (GList *blocks)
{
  GString *string;
  GList *walk;
  const char *filename;

  filename = g_getenv ("VIVI_DECOMPILER_DUMP");
  if (filename == NULL)
    return;

  string = g_string_new ("digraph G\n{\n");
  g_string_append (string, "  node [ shape = box ]\n");
  for (walk = blocks; walk; walk = walk->next) {
    g_string_append_printf (string, "  node%p\n", 
	vivi_decompiler_block_get_start (walk->data));
  }
  g_string_append (string, "\n");
  for (walk = blocks; walk; walk = walk->next) {
    ViviDecompilerBlock *block, *next;

    block = walk->data;
    next = vivi_decompiler_block_get_next (block);
    if (next) {
      g_string_append_printf (string, "  node%p -> node%p\n", 
	  vivi_decompiler_block_get_start (block),
	  vivi_decompiler_block_get_start (next));
    }
    next = vivi_decompiler_block_get_branch (block);
    if (next) {
      g_string_append_printf (string, "  node%p -> node%p\n",
	  vivi_decompiler_block_get_start (block),
	  vivi_decompiler_block_get_start (next));
    }
  }
  g_string_append (string, "}\n");
  g_file_set_contents (filename, string->str, string->len, NULL);
  g_string_free (string, TRUE);
}

static void
vivi_decompiler_preload (ViviDecompilerState *state, SwfdecScript *script)
{
  static const struct {
    const char *name;
    guint	flag;
  } preloads[] = {
    { "this", SWFDEC_SCRIPT_PRELOAD_THIS },
    { "arguments", SWFDEC_SCRIPT_PRELOAD_ARGS },
    { "super", SWFDEC_SCRIPT_PRELOAD_SUPER },
    { "_root", SWFDEC_SCRIPT_PRELOAD_ROOT },
    { "_parent", SWFDEC_SCRIPT_PRELOAD_PARENT },
    { "_global", SWFDEC_SCRIPT_PRELOAD_GLOBAL }
  };
  guint reg, i;

  for (i = 0; i < script->n_arguments; i++) {
    if (script->arguments[i].preload) {
      ViviCodeValue *value = vivi_code_get_new_name (NULL, script->arguments[i].name);
      vivi_decompiler_state_set_register (state, script->arguments[i].preload, value);
      g_object_unref (value);
    }
  }
  reg = 1;
  for (i = 0; i < G_N_ELEMENTS (preloads); i++) {
    if (script->flags & preloads[i].flag) {
      ViviCodeValue *value = vivi_code_get_new_name (NULL, preloads[i].name);
      vivi_decompiler_state_set_register (state, reg++, value);
      g_object_unref (value);
    }
  }
}

ViviCodeStatement *
vivi_decompile_script (SwfdecScript *script)
{
  ViviDecompilerBlock *block;
  ViviDecompilerState *state;
  ViviCodeStatement *stmt;
  GList *walk, *blocks;

  g_return_val_if_fail (script != NULL, NULL);

  DEBUG ("--> starting decompilation\n");
  state = vivi_decompiler_state_new (script, script->main, script->n_registers);
  vivi_decompiler_preload (state, script);
  if (script->constant_pool) {
    vivi_decompiler_state_set_constant_pool (state,
	swfdec_constant_pool_new (NULL, script->constant_pool, script->version));
  }
  block = vivi_decompiler_block_new (state);
  blocks = g_list_prepend (NULL, block);
  while (TRUE) {
    for (walk = blocks; walk; walk = walk->next) {
      block = walk->data;
      if (!vivi_decompiler_block_is_finished (block))
	break;
    }
    if (walk == NULL)
      break;
    blocks = vivi_decompiler_block_decompile (blocks, block, script);
  }
  DEBUG ("--- decompilation done, starting optimization\n");

  vivi_decompiler_dump_graphviz (blocks);
  stmt = vivi_decompiler_merge_blocks (blocks, script->main);
  DEBUG ("<-- optimization done\n");
  return stmt;
}

