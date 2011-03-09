/* Vivified
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
 *               2008 Pekka Lampila <pekka.lampila@iki.fi>
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

#include "vivi_code_asm_code_default.h"
#include "vivi_code_compiler.h"
#include "vivi_code_init_array.h"
#include "vivi_code_number.h"
#include "vivi_code_printer.h"
#include "vivi_code_undefined.h"

G_DEFINE_TYPE (ViviCodeInitArray, vivi_code_init_array, VIVI_TYPE_CODE_VALUE)

typedef struct _VariableEntry VariableEntry;
struct _VariableEntry {
  ViviCodeValue *	name;
  ViviCodeValue *	value;
};

static void
vivi_code_init_array_dispose (GObject *object)
{
  ViviCodeInitArray *array = VIVI_CODE_INIT_ARRAY (object);
  guint i;

  for (i = 0; i < array->variables->len; i++) {
    g_object_unref (g_ptr_array_index (array->variables, i));
  }
  g_ptr_array_free (array->variables, TRUE);

  G_OBJECT_CLASS (vivi_code_init_array_parent_class)->dispose (object);
}

static ViviCodeValue * 
vivi_code_init_array_optimize (ViviCodeValue *value, SwfdecAsValueType hint)
{
  /* FIXME: write */

  return g_object_ref (value);
}

static void
vivi_code_init_array_print_value (ViviCodeValue *value,
    ViviCodePrinter *printer)
{
  ViviCodeInitArray *array = VIVI_CODE_INIT_ARRAY (value);
  guint i;

  vivi_code_printer_print (printer, "[");
  for (i = 0; i < array->variables->len; i++) {
    ViviCodeValue *member =
      VIVI_CODE_VALUE (g_ptr_array_index (array->variables, i));
    if (i > 0)
      vivi_code_printer_print (printer, ", ");
    /* FIXME: precedences? */
    // don't write undefined values
    if (!VIVI_IS_CODE_UNDEFINED (member))
      vivi_code_printer_print_value (printer, member, VIVI_PRECEDENCE_COMMA);
  }
  vivi_code_printer_print (printer, "]");
}

static void
vivi_code_init_array_compile_value (ViviCodeValue *value,
    ViviCodeCompiler *compiler)
{
  ViviCodeInitArray *array = VIVI_CODE_INIT_ARRAY (value);
  ViviCodeValue *count;
  guint i;

  for (i = array->variables->len; i > 0; i--) {
    vivi_code_compiler_compile_value (compiler,
	VIVI_CODE_VALUE (g_ptr_array_index (array->variables, i - 1)));
  }
  count = vivi_code_number_new (array->variables->len);
  vivi_code_compiler_compile_value (compiler, count);
  g_object_unref (count);

  vivi_code_compiler_take_code (compiler, vivi_code_asm_init_array_new ());
}

static gboolean
vivi_code_init_array_is_constant (ViviCodeValue *value)
{
  /* not constant, because we return a new array every time */
  return FALSE;
}

static void
vivi_code_init_array_class_init (ViviCodeInitArrayClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeValueClass *value_class = VIVI_CODE_VALUE_CLASS (klass);

  object_class->dispose = vivi_code_init_array_dispose;

  value_class->print_value = vivi_code_init_array_print_value;
  value_class->compile_value = vivi_code_init_array_compile_value;
  value_class->is_constant = vivi_code_init_array_is_constant;
  value_class->optimize = vivi_code_init_array_optimize;
}

static void
vivi_code_init_array_init (ViviCodeInitArray *array)
{
  ViviCodeValue *value = VIVI_CODE_VALUE (array);

  array->variables = g_ptr_array_new ();

  vivi_code_value_set_precedence (value, VIVI_PRECEDENCE_PARENTHESIS);
}

ViviCodeValue *
vivi_code_init_array_new (void)
{
  return g_object_new (VIVI_TYPE_CODE_INIT_ARRAY, NULL);
}

void
vivi_code_init_array_add_variable (ViviCodeInitArray *array,
    ViviCodeValue *value)
{
  g_return_if_fail (VIVI_IS_CODE_INIT_ARRAY (array));
  g_return_if_fail (VIVI_IS_CODE_VALUE (value));

  g_ptr_array_add (array->variables, g_object_ref (value));
}
