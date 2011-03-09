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

#include "vivi_code_label.h"
#include "vivi_code_asm.h"
#include "vivi_code_compiler.h"
#include "vivi_code_emitter.h"
#include "vivi_code_printer.h"

static gboolean
vivi_code_label_emit (ViviCodeAsm *code, ViviCodeEmitter *emitter, GError **error)
{
  ViviCodeLabel *label = VIVI_CODE_LABEL (code);

  vivi_code_emitter_add_label (emitter, label);

  return TRUE;
}

static void
vivi_code_label_get_stack_change (ViviCodeAsm *code, int *add, int *remove)
{
  *add = 0;
  *remove = 0;
}

static void
vivi_code_label_asm_init (ViviCodeAsmInterface *iface)
{
  iface->emit = vivi_code_label_emit;
  iface->get_stack_change = vivi_code_label_get_stack_change;
}

G_DEFINE_TYPE_WITH_CODE (ViviCodeLabel, vivi_code_label, VIVI_TYPE_CODE_STATEMENT,
    G_IMPLEMENT_INTERFACE (VIVI_TYPE_CODE_ASM, vivi_code_label_asm_init))

static void
vivi_code_label_dispose (GObject *object)
{
  ViviCodeLabel *label = VIVI_CODE_LABEL (object);

  g_free (label->name);

  G_OBJECT_CLASS (vivi_code_label_parent_class)->dispose (object);
}

static void
vivi_code_label_print (ViviCodeToken *token, ViviCodePrinter *printer)
{
  ViviCodeLabel *label = VIVI_CODE_LABEL (token);

  vivi_code_printer_new_line (printer, TRUE);
  vivi_code_printer_print (printer, label->name);
  vivi_code_printer_print (printer, ":");
  vivi_code_printer_new_line (printer, FALSE);
}

static void
vivi_code_label_compile (ViviCodeToken *token, ViviCodeCompiler *compiler)
{
  ViviCodeLabel *label = VIVI_CODE_LABEL (token);

  vivi_code_compiler_take_code (compiler,
      VIVI_CODE_ASM (vivi_code_label_new (label->name)));
}

static gboolean
vivi_code_label_needs_braces (ViviCodeStatement *stmt)
{
  return TRUE;
}

static void
vivi_code_label_class_init (ViviCodeLabelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);
  ViviCodeStatementClass *statement_class = VIVI_CODE_STATEMENT_CLASS (klass);

  object_class->dispose = vivi_code_label_dispose;

  token_class->print = vivi_code_label_print;
  token_class->compile = vivi_code_label_compile;

  statement_class->needs_braces = vivi_code_label_needs_braces;
}

static void
vivi_code_label_init (ViviCodeLabel *token)
{
}

ViviCodeStatement *
vivi_code_label_new (const char *name)
{
  ViviCodeLabel *label;

  g_return_val_if_fail (name != NULL, NULL);

  label = g_object_new (VIVI_TYPE_CODE_LABEL, NULL);
  label->name = g_strdup (name);

  return VIVI_CODE_STATEMENT (label);
}

const char *
vivi_code_label_get_name (const ViviCodeLabel *label)
{
  g_return_val_if_fail (VIVI_IS_CODE_LABEL (label), NULL);

  return label->name;
}

