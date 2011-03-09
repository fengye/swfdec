/* Vivified
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <math.h>

#include "vivi_parser.h"

#include "vivi_parser_scanner.h"

#include "vivi_code_and.h"
#include "vivi_code_asm_code_default.h"
#include "vivi_code_asm_get_url2.h"
#include "vivi_code_asm_if.h"
#include "vivi_code_asm_jump.h"
#include "vivi_code_asm_pool.h"
#include "vivi_code_asm_push.h"
#include "vivi_code_asm_store.h"
#include "vivi_code_asm_try.h"
#include "vivi_code_assignment.h"
#include "vivi_code_binary_default.h"
#include "vivi_code_bit_not.h"
#include "vivi_code_block.h"
#include "vivi_code_boolean.h"
#include "vivi_code_break.h"
#include "vivi_code_builtin_statement_default.h"
#include "vivi_code_builtin_value_call_default.h"
#include "vivi_code_builtin_value_statement_default.h"
#include "vivi_code_concat.h"
#include "vivi_code_conditional.h"
#include "vivi_code_constant.h"
#include "vivi_code_continue.h"
#include "vivi_code_enumerate.h"
#include "vivi_code_function.h"
#include "vivi_code_function_call.h"
#include "vivi_code_function_declaration.h"
#include "vivi_code_get.h"
#include "vivi_code_get_timer.h"
#include "vivi_code_get_url.h"
#include "vivi_code_goto.h"
#include "vivi_code_if.h"
#include "vivi_code_inc_dec.h"
#include "vivi_code_init_array.h"
#include "vivi_code_init_object.h"
#include "vivi_code_loop.h"
#include "vivi_code_null.h"
#include "vivi_code_number.h"
#include "vivi_code_or.h"
#include "vivi_code_return.h"
#include "vivi_code_string.h"
#include "vivi_code_substring.h"
#include "vivi_code_throw.h"
#include "vivi_code_try.h"
#include "vivi_code_not.h"
#include "vivi_code_undefined.h"
#include "vivi_code_variable.h"
#include "vivi_code_void.h"
#include "vivi_compiler_empty_statement.h"

#include "vivi_code_text_printer.h"
#include "vivi_code_assembler.h"

enum {
  ERROR_TOKEN_LITERAL = TOKEN_LAST + 1,
  ERROR_TOKEN_IDENTIFIER,
  ERROR_TOKEN_PROPERTY_NAME,
  ERROR_TOKEN_PRIMARY_EXPRESSION,
  ERROR_TOKEN_FUNCTION_EXPRESSION,
  ERROR_TOKEN_ASSIGNMENT_EXPRESSION,
  ERROR_TOKEN_EXPRESSION,
  ERROR_TOKEN_ITERATION_STATEMENT,
  ERROR_TOKEN_EXPRESSION_STATEMENT,
  ERROR_TOKEN_STATEMENT,
  ERROR_TOKEN_FUNCTION_DECLARATION
};

static const struct {
  guint				token;
  const char *			name;
} error_names[] = {
  { ERROR_TOKEN_LITERAL, "LITERAL" },
  { ERROR_TOKEN_IDENTIFIER, "IDENTIFIER" },
  { ERROR_TOKEN_PROPERTY_NAME, "PROPERTY NAME" },
  { ERROR_TOKEN_PRIMARY_EXPRESSION, "PRIMARY EXPRESSION" },
  { ERROR_TOKEN_FUNCTION_EXPRESSION, "FUNCTION EXPRESSION" },
  { ERROR_TOKEN_ASSIGNMENT_EXPRESSION, "ASSIGNMENT EXPRESSION" },
  { ERROR_TOKEN_EXPRESSION, "EXPRESSION" },
  { ERROR_TOKEN_ITERATION_STATEMENT, "ITERATION STATEMENT" },
  { ERROR_TOKEN_EXPRESSION_STATEMENT, "EXPRESSION STATEMENT" },
  { ERROR_TOKEN_STATEMENT, "STATEMENT" },
  { ERROR_TOKEN_FUNCTION_DECLARATION, "FUNCTION DECLARATION" },
  { TOKEN_NONE, NULL }
};

typedef struct {
  GSList			*labels;
  GSList			*waiting_labels;
  guint				loop_count;
} ParseLevel;

typedef struct {
  ViviParserScanner *		scanner;

  GSList *			positions;

  GSList *			levels; // ParseLevel, earlier levels
  ParseLevel *			level;  // current level

  guint				error_count;
} ParseData;

typedef gboolean (*PeekFunction) (ParseData *data);
typedef ViviCodeValue * (*ParseValueFunction) (ParseData *data);
typedef ViviCodeStatement * (*ParseStatementFunction) (ParseData *data);

static ViviCodeStatement *
parse_statement_list (ParseData *data, PeekFunction peek,
    ParseStatementFunction parse, guint separator);

static void
parse_value_list (ParseData *data, PeekFunction peek, ParseValueFunction parse,
    ViviCodeValue ***list, guint separator);

// helpers

static const char *
vivi_parser_token_name (guint token)
{
  if (token < TOKEN_LAST) {
    return vivi_parser_scanner_token_name (token);
  } else {
    guint i;
    const char *name;

    name = NULL;
    for (i = 0; error_names[i].token != TOKEN_NONE; i++) {
      if (error_names[i].token == token) {
	name = error_names[i].name;
	break;
      }
    }

    g_assert (name != NULL);

    return name;
  }
}

static void
vivi_parser_error (ParseData *data, const char *format, ...)
{
  va_list args;
  char *message;
  const ViviParserValue *val;

  g_return_if_fail (data != NULL);
  g_return_if_fail (format != NULL);

  va_start (args, format);
  message = g_strdup_vprintf (format, args);
  va_end (args);

  val = vivi_parser_scanner_get_value (data->scanner, 1);
  g_printerr ("%i: error: %s\n", val->line_number, message);

  g_free (message);

  data->error_count++;
}

static void
vivi_parser_error_unexpected_or (ParseData *data, ...)
{
  va_list args;
  guint token;
  GString *message;

  g_return_if_fail (data != NULL);

  message = g_string_new ("Expected ");

  va_start (args, data);

  token = va_arg (args, guint);
  g_assert (token != TOKEN_NONE);
  g_string_append (message, vivi_parser_token_name (token));

  while ((token = va_arg (args, guint)) != TOKEN_NONE) {
    g_string_append (message, ", ");
    g_string_append (message, vivi_parser_token_name (token));
  }

  va_end (args);

  g_string_append (message, " before ");

  g_string_append (message, vivi_parser_token_name (
	vivi_parser_scanner_peek_next_token (data->scanner)));

  vivi_parser_error (data, "%s", g_string_free (message, FALSE));
}

static void
vivi_parser_error_unexpected (ParseData *data, guint expected)
{
  g_return_if_fail (data != NULL);
  g_return_if_fail (expected != TOKEN_NONE);

  vivi_parser_error_unexpected_or (data, expected, TOKEN_NONE);
}

static void
vivi_parser_error_unexpected_line_terminator (ParseData *data, guint expected)
{
  g_return_if_fail (data != NULL);
  g_return_if_fail (expected != TOKEN_NONE);

  vivi_parser_error (data, "Expected %s before end of line",
      vivi_parser_token_name (expected));
}

G_GNUC_WARN_UNUSED_RESULT static gboolean
peek_line_terminator (ParseData *data)
{
  return vivi_parser_scanner_get_value (data->scanner, 1)->line_terminator;
}

G_GNUC_WARN_UNUSED_RESULT static gboolean
peek_token (ParseData *data, ViviParserScannerToken token)
{
  return vivi_parser_scanner_peek_next_token (data->scanner) == token;
}

static void
parse_token (ParseData *data, ViviParserScannerToken token)
{
  if (vivi_parser_scanner_peek_next_token (data->scanner) != token) {
    vivi_parser_error_unexpected (data, token);
  } else {
    vivi_parser_scanner_get_next_token (data->scanner);
  }
}

G_GNUC_WARN_UNUSED_RESULT static gboolean
try_parse_token (ParseData *data, ViviParserScannerToken token)
{
  if (!peek_token (data, token))
    return FALSE;

  parse_token (data, token);
  return TRUE;
}

static void
parse_automatic_semicolon (ParseData *data)
{
  ViviParserScannerToken token;

  if (try_parse_token (data, TOKEN_SEMICOLON))
    return;
  if (peek_line_terminator (data))
    return;

  token = vivi_parser_scanner_peek_next_token (data->scanner);
  if (token == TOKEN_BRACE_LEFT || token == TOKEN_NONE)
    return;

  vivi_parser_error_unexpected (data, TOKEN_SEMICOLON);
}

G_GNUC_WARN_UNUSED_RESULT static gboolean
try_parse_restricted_semicolon (ParseData *data)
{
  if (try_parse_token (data, TOKEN_SEMICOLON))
    return TRUE;
  if (peek_line_terminator (data))
    return TRUE;

  return FALSE;
}

static void
free_value_list (ViviCodeValue **list)
{
  int i;

  for (i = 0; list[i] != NULL; i++) {
    g_object_unref (list[i]);
  }
  g_free (list);
}

static ViviCodeValue *
vivi_parser_function_call_new (ViviCodeValue *name)
{
  ViviCodeValue *value;

  g_return_val_if_fail (VIVI_IS_CODE_VALUE (name), NULL);

  value = NULL;

  if (VIVI_IS_CODE_GET (name)) {
    ViviCodeGet *get = VIVI_CODE_GET (name);

    if (get->from != NULL) {
      value = g_object_ref (get->from);
      name = g_object_ref (get->name);
    }
  }

  if (VIVI_IS_CODE_GET (name)) {
    ViviCodeGet *get = VIVI_CODE_GET (name);

    if (get->from == NULL)
      name = g_object_ref (get->name);
  }

  return vivi_code_function_call_new (value, name);
}

static ViviCodeValue *
vivi_parser_assignment_new (ViviCodeValue *left, ViviCodeValue *right)
{
  ViviCodeValue *from, *name;

  g_return_val_if_fail (VIVI_IS_CODE_VALUE (left), NULL);
  g_return_val_if_fail (VIVI_IS_CODE_VALUE (right), NULL);

  from = NULL;

  if (VIVI_IS_CODE_GET (left)) {
    ViviCodeGet *get = VIVI_CODE_GET (left);

    from = get->from;
    name = get->name;
  } else {
    from = NULL;
    name = left;
  }

  return vivi_code_assignment_new (from, name, right);
}

static gboolean
vivi_parser_value_is_left_hand_side (ViviCodeValue *value)
{
  // FIXME: Correct?
  return VIVI_IS_CODE_GET (value);
}

static void
vivi_parser_start_level (ParseData *data)
{
  g_return_if_fail (data != NULL);

  if (data->level != NULL)
    data->levels = g_slist_prepend (data->levels, data->level);
  data->level = g_new0 (ParseLevel, 1);
}

static ViviCodeLabel *
vivi_parser_get_waiting_label (ParseData *data, const char *name)
{
  GSList *iter;

  g_return_val_if_fail (data != NULL, NULL);
  g_return_val_if_fail (data->level != NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);

  for (iter = data->level->waiting_labels; iter != NULL; iter = iter->next) {
    ViviCodeLabel *label = VIVI_CODE_LABEL (iter->data);
    if (g_str_equal (vivi_code_label_get_name (label), name))
      return g_object_ref (label);
  }

  return NULL;
}

static void
vivi_parser_remove_waiting_label (ParseData *data, ViviCodeLabel *label)
{
  g_return_if_fail (data != NULL);
  g_return_if_fail (data->level != NULL);
  g_return_if_fail (VIVI_IS_CODE_LABEL (label));

  data->level->waiting_labels =
    g_slist_remove (data->level->waiting_labels, label);
  g_object_unref (label);
}

static ViviCodeLabel *
vivi_parser_get_label (ParseData *data, const char *name)
{
  GSList *iter;

  g_return_val_if_fail (data != NULL, NULL);
  g_return_val_if_fail (data->level != NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);

  for (iter = data->level->labels; iter != NULL; iter = iter->next) {
    ViviCodeLabel *label = VIVI_CODE_LABEL (iter->data);
    if (g_str_equal (vivi_code_label_get_name (label), name))
      return g_object_ref (label);
  }

  return vivi_parser_get_waiting_label (data, name);
}

static void
vivi_parser_end_level (ParseData *data)
{
  GSList *iter;

  g_return_if_fail (data != NULL);
  g_return_if_fail (data->level != NULL);

  for (iter = data->level->waiting_labels; iter != NULL; iter = iter->next) {
    ViviCodeLabel *label = VIVI_CODE_LABEL (iter->data);

    vivi_parser_error (data, "Label named '%s' doesn't exist in this block",
	vivi_code_label_get_name (label));

    g_object_unref (label);
  }
  g_slist_free (data->level->waiting_labels);

  for (iter = data->level->labels; iter != NULL; iter = iter->next) {
    g_object_unref (VIVI_CODE_LABEL (iter->data));
  }
  g_slist_free (data->level->labels);

  g_free (data->level);

  if (data->levels != NULL) {
    data->level = data->levels->data;
    data->levels = g_slist_delete_link (data->levels, data->levels);
  } else {
    data->level = NULL;
  }
}

static gboolean
vivi_parser_add_label (ParseData *data, ViviCodeLabel *label)
{
  g_return_val_if_fail (data != NULL, FALSE);
  g_return_val_if_fail (data->level != NULL, FALSE);
  g_return_val_if_fail (VIVI_IS_CODE_LABEL (label), FALSE);

  if (vivi_parser_get_label (data, vivi_code_label_get_name (label)) != NULL)
      return FALSE;

  data->level->labels =
    g_slist_prepend (data->level->labels, g_object_ref (label));

  return TRUE;
}

static void
vivi_parser_add_waiting_label (ParseData *data, ViviCodeLabel *label)
{
  g_return_if_fail (data != NULL);
  g_return_if_fail (data->level != NULL);
  g_return_if_fail (VIVI_IS_CODE_LABEL (label));
  g_return_if_fail (vivi_parser_get_waiting_label (data,
	vivi_code_label_get_name (label)) == NULL);

  data->level->waiting_labels =
    g_slist_prepend (data->level->waiting_labels, g_object_ref (label));
}

static gsize
vivi_parser_get_position (ParseData *data)
{
  return vivi_parser_scanner_get_value (data->scanner, 0)->position;
}

static void
vivi_parser_start_code_token (ParseData *data)
{
  gsize *position;

  g_return_if_fail (data != NULL);

  position = g_new (gsize, 1);
  *position = vivi_parser_get_position (data);

  data->positions = g_slist_prepend (data->positions, position);
}

static void
vivi_parser_end_code_token (ParseData *data, ViviCodeToken *token)
{
  gsize start;

  g_return_if_fail (data != NULL);
  g_return_if_fail (token == NULL || VIVI_IS_CODE_TOKEN (token));
  g_return_if_fail (data->positions != NULL);

  start = *(gsize *)data->positions->data;
  g_free (data->positions->data);
  data->positions = g_slist_delete_link (data->positions, data->positions);

  // TODO
}

static void
vivi_parser_duplicate_code_token (ParseData *data)
{
  gsize *position;

  g_return_if_fail (data != NULL);
  g_return_if_fail (data->positions != NULL);

  position = g_new (gsize, 1);
  *position = *(gsize *)data->positions->data;

  data->positions = g_slist_prepend (data->positions, position);
}

static void
vivi_parser_error_handler (const char *text, gpointer user_data)
{
  ParseData *data = user_data;

  vivi_parser_error (data, text);
}

// values

/* ActionScript specific */
static gboolean
peek_undefined (ParseData *data)
{
  return peek_token (data, TOKEN_UNDEFINED);
}

static ViviCodeValue *
parse_undefined (ParseData *data)
{
  ViviCodeValue *value;

  vivi_parser_start_code_token (data);

  parse_token (data, TOKEN_UNDEFINED);

  value = vivi_code_undefined_new ();

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));

  return value;
}

static gboolean
peek_null (ParseData *data)
{
  return peek_token (data, TOKEN_NULL);
}

static ViviCodeValue *
parse_null (ParseData *data)
{
  ViviCodeValue *value;

  vivi_parser_start_code_token (data);

  parse_token (data, TOKEN_NULL);

  value = vivi_code_null_new ();

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));

  return value;
}

G_GNUC_UNUSED static gboolean
peek_boolean_value (ParseData *data)
{
  const ViviParserValue *value =
    vivi_parser_scanner_get_value (data->scanner, 1);

  if (value->token == TOKEN_BOOLEAN) {
    return value->value.v_boolean;
  } else {
    return FALSE;
  }
}

static gboolean
parse_boolean_value (ParseData *data)
{
  const ViviParserValue *value;

  parse_token (data, TOKEN_BOOLEAN);

  value = vivi_parser_scanner_get_value (data->scanner, 0);
  if (value->token == TOKEN_BOOLEAN) {
    return value->value.v_boolean;
  } else {
    return FALSE;
  }
}

static gboolean
peek_boolean (ParseData *data)
{
  return peek_token (data, TOKEN_BOOLEAN);
}

static ViviCodeValue *
parse_boolean (ParseData *data)
{
  ViviCodeValue *value;

  vivi_parser_start_code_token (data);

  value = vivi_code_boolean_new (parse_boolean_value (data));

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));

  return value;
}

G_GNUC_UNUSED static double
peek_numeric_value (ParseData *data)
{
  const ViviParserValue *value =
    vivi_parser_scanner_get_value (data->scanner, 1);

  if (value->token == TOKEN_NUMBER) {
    return value->value.v_number;
  } else {
    return NAN;
  }
}

static double
parse_numeric_value (ParseData *data)
{
  const ViviParserValue *value;

  parse_token (data, TOKEN_NUMBER);

  value = vivi_parser_scanner_get_value (data->scanner, 0);
  if (value->token == TOKEN_NUMBER) {
    return value->value.v_number;
  } else {
    return NAN;
  }
}

static int
parse_integer_value (ParseData *data)
{
  double number = peek_numeric_value (data);

  if (swfdec_as_double_to_integer (number) != number)
    vivi_parser_error (data, "Expected integer, got double");

  vivi_parser_scanner_get_next_token (data->scanner);

  return swfdec_as_double_to_integer (number);
}

static gboolean
peek_numeric (ParseData *data)
{
  return peek_token (data, TOKEN_NUMBER);
}

static ViviCodeValue *
parse_numeric (ParseData *data)
{
  ViviCodeValue *value;

  vivi_parser_start_code_token (data);

  value = vivi_code_number_new (parse_numeric_value (data));

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));

  return value;
}

G_GNUC_UNUSED static const char *
peek_string_value (ParseData *data)
{
  const ViviParserValue *value =
    vivi_parser_scanner_get_value (data->scanner, 1);

  if (value->token == TOKEN_STRING) {
    return value->value.v_string->str;
  } else {
    return "undefined";
  }
}

static const char *
parse_string_value (ParseData *data)
{
  const ViviParserValue *value;

  parse_token (data, TOKEN_STRING);

  value = vivi_parser_scanner_get_value (data->scanner, 0);
  if (value->token == TOKEN_STRING) {
    return value->value.v_string->str;
  } else {
    return "undefined";
  }
}

static gboolean
peek_string (ParseData *data)
{
  return peek_token (data, TOKEN_STRING);
}

static ViviCodeValue *
parse_string (ParseData *data)
{
  ViviCodeValue *value;

  vivi_parser_start_code_token (data);

  value = vivi_code_string_new (parse_string_value (data));

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));

  return value;
}

static const struct {
  PeekFunction peek;
  ParseValueFunction parse;
} literal_functions[] = {
  { peek_undefined, parse_undefined },
  { peek_null, parse_null },
  { peek_boolean, parse_boolean },
  { peek_numeric, parse_numeric },
  { peek_string, parse_string },
  { NULL, NULL }
};

static gboolean
peek_literal (ParseData *data)
{
  guint i;

  for (i = 0; literal_functions[i].peek != NULL; i++) {
    if (literal_functions[i].peek (data))
      return TRUE;
  }

  return FALSE;
}

static ViviCodeValue *
parse_literal (ParseData *data)
{
  ViviCodeValue *value;
  guint i;

  for (i = 0; literal_functions[i].peek != NULL; i++) {
    if (literal_functions[i].peek (data)) {
      return literal_functions[i].parse (data);
    }
  }

  vivi_parser_error_unexpected (data, ERROR_TOKEN_LITERAL);
  vivi_parser_start_code_token (data);
  value = vivi_code_undefined_new ();
  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));

  return value;
}

static const char *
peek_identifier_value (ParseData *data)
{
  const ViviParserValue *value =
    vivi_parser_scanner_get_value (data->scanner, 1);

  if (value->token == TOKEN_IDENTIFIER) {
    return value->value.v_identifier;
  } else {
    return "undefined";
  }
}

static const char *
parse_identifier_value (ParseData *data)
{
  const ViviParserValue *value;

  parse_token (data, TOKEN_IDENTIFIER);

  value = vivi_parser_scanner_get_value (data->scanner, 0);
  if (value->token == TOKEN_IDENTIFIER) {
    return value->value.v_identifier;
  } else {
    return "undefined";
  }
}

static gboolean
peek_identifier (ParseData *data)
{
  return peek_token (data, TOKEN_IDENTIFIER);
}

static ViviCodeValue *
parse_identifier (ParseData *data)
{
  ViviCodeValue *value;

  vivi_parser_start_code_token (data);

  value = vivi_code_get_new_name (NULL, parse_identifier_value (data));

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));

  return value;
}

static ViviCodeValue *
parse_property_name (ParseData *data)
{
  ViviCodeValue *value;

  vivi_parser_start_code_token (data);

  if (peek_identifier (data)) {
    value = vivi_code_string_new (parse_identifier_value (data));
  } else if (peek_string (data)) {
    value = parse_string (data);
  } else if (peek_numeric (data)) {
    char s[G_ASCII_DTOSTR_BUF_SIZE];
    double d = parse_numeric_value (data);
    g_ascii_dtostr (s, G_ASCII_DTOSTR_BUF_SIZE, d);
    value = vivi_code_string_new (s);
  } else {
    vivi_parser_error_unexpected (data, ERROR_TOKEN_PROPERTY_NAME);
    value = vivi_code_string_new ("undefined");
  }

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));

  return value;
}

static gboolean
peek_assignment_expression (ParseData *data);

static ViviCodeValue *
parse_assignment_expression (ParseData *data);

static gboolean
peek_array_literal (ParseData *data)
{
  return peek_token (data, TOKEN_BRACKET_LEFT);
}

static ViviCodeValue *
parse_array_literal (ParseData *data)
{
  ViviCodeValue *value, *member;

  vivi_parser_start_code_token (data);

  value = vivi_code_init_array_new ();

  parse_token (data, TOKEN_BRACKET_LEFT);

  while (!try_parse_token (data, TOKEN_BRACKET_RIGHT)) {
    if (try_parse_token (data, TOKEN_COMMA)) {
      vivi_code_init_array_add_variable (VIVI_CODE_INIT_ARRAY (value),
	 vivi_code_undefined_new ());
    }
    else if (peek_assignment_expression (data))
    {
      member = parse_assignment_expression (data);

      vivi_code_init_array_add_variable (VIVI_CODE_INIT_ARRAY (value), member);
      g_object_unref (member);

      if (!try_parse_token (data, TOKEN_COMMA)) {
	if (!try_parse_token (data, TOKEN_BRACKET_RIGHT)) {
	  vivi_parser_error_unexpected_or (data, TOKEN_BRACKET_RIGHT,
	      TOKEN_COMMA, TOKEN_NONE);
	}
	break;
      }
    } else {
      vivi_parser_error_unexpected_or (data, TOKEN_BRACKET_RIGHT, TOKEN_COMMA,
	  ERROR_TOKEN_ASSIGNMENT_EXPRESSION, TOKEN_NONE);
      break;
    }
  }

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));

  return value;
}

static gboolean
peek_object_literal (ParseData *data)
{
  return peek_token (data, TOKEN_BRACE_LEFT);
}

static ViviCodeValue *
parse_object_literal (ParseData *data)
{
  ViviCodeValue *value;

  vivi_parser_start_code_token (data);

  value = vivi_code_init_object_new ();

  parse_token (data, TOKEN_BRACE_LEFT);

  if (!peek_token (data, TOKEN_BRACE_RIGHT)) {
    do {
      ViviCodeValue *property, *initializer;

      property = parse_property_name (data);
      parse_token (data, TOKEN_COLON);
      initializer = parse_assignment_expression (data);

      vivi_code_init_object_add_variable (VIVI_CODE_INIT_OBJECT (value),
	  property, initializer);
    } while (try_parse_token (data, TOKEN_COMMA));
  }

  parse_token (data, TOKEN_BRACE_RIGHT);

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));

  return value;
}

// misc

static gboolean
peek_variable_declaration (ParseData *data)
{
  return peek_identifier (data);
}

static ViviCodeStatement *
parse_variable_declaration (ParseData *data)
{
  ViviCodeValue *value;
  ViviCodeStatement *variable;
  char *identifier;

  vivi_parser_start_code_token (data);

  identifier = g_strdup (parse_identifier_value (data));

  if (try_parse_token (data, TOKEN_ASSIGN)) {
    value = parse_assignment_expression (data);
  } else {
    value = NULL;
  }

  variable = vivi_code_variable_new_name (identifier, value);
  g_free (identifier);
  if (value != NULL)
    g_object_unref (value);

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (variable));

  return variable;
}

// asm functions

static ViviCodeAsm *
parse_asm_store (ParseData *data)
{
  int id;

  // TODO: error/warn if not an integer
  id = swfdec_as_double_to_integer (parse_numeric_value (data));
  if (id < 0 || id >= 256) {
    vivi_parser_error (data, "Invalid register number: %i", id);
    id = 0;
  }

  parse_automatic_semicolon (data);

  return vivi_code_asm_store_new (id);
}

static ViviCodeAsm *
parse_asm_if (ParseData *data)
{
  ViviCodeAsm *code;
  ViviCodeLabel *label;
  const char *name;

  name = parse_identifier_value (data);
  label = vivi_parser_get_label (data, name);
  if (label == NULL) {
    label = VIVI_CODE_LABEL (vivi_code_label_new (name));
    vivi_parser_add_waiting_label (data, label);
  }

  parse_automatic_semicolon (data);

  code = vivi_code_asm_if_new (label);
  g_object_unref (label);

  return code;
}

static ViviCodeAsm *
parse_asm_jump (ParseData *data)
{
  ViviCodeAsm *code;
  ViviCodeLabel *label;
  const char *name;

  name = parse_identifier_value (data);
  label = vivi_parser_get_label (data, name);
  if (label == NULL) {
    label = VIVI_CODE_LABEL (vivi_code_label_new (name));
    vivi_parser_add_waiting_label (data, label);
  }

  parse_automatic_semicolon (data);

  code = vivi_code_asm_jump_new (label);
  g_object_unref (label);

  return code;
}

static ViviCodeAsm *
parse_asm_try (ParseData *data)
{
  gboolean has_catch, has_finally;
  guint reserved_flags, id;
  const char *name;
  char *variable;
  ViviCodeLabel *label_catch, *label_finally, *label_end;
  ViviCodeAsm *try_;

  has_catch = FALSE;
  has_finally = FALSE;
  reserved_flags = 0;

  while (peek_identifier (data) && !peek_line_terminator (data)) {
    name = parse_identifier_value (data);

    if (!g_ascii_strcasecmp (name, "has_catch")) {
      has_catch = TRUE;
    } else if (!g_ascii_strcasecmp (name, "has_finally")) {
      has_finally = TRUE;
    } else {
      guint i;

      for (i = 0; i < 5; i++) {
	char *str = g_strdup_printf ("reserved%i", i + 1);
	if (!g_ascii_strcasecmp (name, str)) {
	  g_free (str);
	  break;
	}
	g_free (str);
      }
      if (i < 5) {
	reserved_flags |= (1 << i);
      } else {
	vivi_parser_error (data, "Unknown flag for try: %s", name);
      }
    }
  }

  id = 0; // silence compiler warning
  if (peek_string (data)) {
    variable = g_strdup (parse_string_value (data));
  } else if (peek_numeric (data)) {
    // TODO: warning/error if not int
    variable = NULL;
    id = swfdec_as_double_to_integer (parse_numeric_value (data));
    if (id > G_MAXUINT8) {
      vivi_parser_error (data, "Too large register number for try: %i", id);
      id = 0;
    }
  } else {
    vivi_parser_error_unexpected_or (data, TOKEN_STRING, TOKEN_NUMBER,
	TOKEN_NONE);
    variable = g_strdup ("undefined");
  }

  name = parse_identifier_value (data);
  label_catch = vivi_parser_get_label (data, name);
  if (label_catch == NULL) {
    label_catch = VIVI_CODE_LABEL (vivi_code_label_new (name));
    vivi_parser_add_waiting_label (data, label_catch);
  }

  name = parse_identifier_value (data);
  label_finally = vivi_parser_get_label (data, name);
  if (label_finally == NULL) {
    label_finally = VIVI_CODE_LABEL (vivi_code_label_new (name));
    vivi_parser_add_waiting_label (data, label_finally);
  }

  name = parse_identifier_value (data);
  label_end = vivi_parser_get_label (data, name);
  if (label_end == NULL) {
    label_end = VIVI_CODE_LABEL (vivi_code_label_new (name));
    vivi_parser_add_waiting_label (data, label_end);
  }

  parse_automatic_semicolon (data);

  if (variable != NULL) {
    try_ = vivi_code_asm_try_new (label_catch, label_finally, label_end,
	variable);
  } else {
    try_ = vivi_code_asm_try_new_register (label_catch, label_finally,
	label_end, id);
  }
  vivi_code_asm_try_set_has_catch (VIVI_CODE_ASM_TRY (try_), has_catch);
  vivi_code_asm_try_set_has_finally (VIVI_CODE_ASM_TRY (try_), has_finally);
  vivi_code_asm_try_set_reserved_flags (VIVI_CODE_ASM_TRY (try_),
      reserved_flags);

  return try_;
}

static ViviCodeAsm *
parse_asm_pool (ParseData *data)
{
  SwfdecConstantPool *pool;
  ViviCodeAsm *code;
  SwfdecBots *bots;
  SwfdecBuffer *buffer;
  guint num;

  bots = swfdec_bots_open ();
  swfdec_bots_put_u16 (bots, 0);

  num = 0;
  if (!try_parse_restricted_semicolon (data)) {
    do {
      swfdec_bots_put_string (bots, parse_string_value (data));
      num++;
    } while (try_parse_token (data, TOKEN_COMMA));

    parse_automatic_semicolon (data);
  }

  *(guint16 *)bots->data = GUINT16_TO_LE (num);

  // FIXME: version
  buffer =  swfdec_bots_close (bots);
  pool = swfdec_constant_pool_new (NULL, buffer, 8);
  swfdec_buffer_unref (buffer);
  code = vivi_code_asm_pool_new (pool);
  swfdec_constant_pool_unref (pool);

  return code;
}

static ViviCodeAsm *
parse_asm_push (ParseData *data)
{
  ViviCodeAsmPush *push;

  push = VIVI_CODE_ASM_PUSH (vivi_code_asm_push_new ());

  if (try_parse_restricted_semicolon (data)) {
    // TODO: warning?
    return VIVI_CODE_ASM (push);
  }

  do {
    switch ((guint) vivi_parser_scanner_peek_next_token (data->scanner)) {
      case TOKEN_STRING:
	vivi_code_asm_push_add_string (push, parse_string_value (data));
	break;
      case TOKEN_NUMBER:
	{
	  double number = parse_numeric_value (data);
	  if (peek_token (data, TOKEN_IDENTIFIER) &&
	      g_ascii_strcasecmp (peek_identifier_value (data), "i") == 0) {
	    vivi_parser_scanner_get_next_token (data->scanner);
	    // TODO: add warning if losing accuracy
	    vivi_code_asm_push_add_integer (push,
		swfdec_as_double_to_integer (number));
	  } else if (peek_token (data, TOKEN_IDENTIFIER) &&
	      g_ascii_strcasecmp (peek_identifier_value (data), "f") == 0) {
	    vivi_parser_scanner_get_next_token (data->scanner);
	    // TODO: add warning if losing accuracy
	    vivi_code_asm_push_add_float (push, (float)number);
	  } else if (peek_token (data, TOKEN_IDENTIFIER) &&
	      g_ascii_strcasecmp (peek_identifier_value (data), "d") == 0) {
	    vivi_parser_scanner_get_next_token (data->scanner);
	    vivi_code_asm_push_add_double (push, number);
	  } else {
	    if (swfdec_as_double_to_integer (number) == number) {
	      vivi_code_asm_push_add_integer (push,
		  swfdec_as_double_to_integer (number));
	    } else {
	      vivi_code_asm_push_add_double (push, number);
	    }
	  }
	}
	break;
      case TOKEN_BOOLEAN:
	vivi_code_asm_push_add_boolean (push, parse_boolean_value (data));
	break;
      case TOKEN_NULL:
	vivi_parser_scanner_get_next_token (data->scanner);
	vivi_code_asm_push_add_null (push);
	break;
      case TOKEN_UNDEFINED:
	vivi_parser_scanner_get_next_token (data->scanner);
	vivi_code_asm_push_add_undefined (push);
	break;
      case TOKEN_IDENTIFIER:
	{
	  const char *identifier = parse_identifier_value (data);
	  if (g_ascii_strcasecmp (identifier, "pool") == 0) {
	    int number = parse_integer_value (data);
	    if (number < 0 || number > G_MAXUINT16) {
	      vivi_parser_error (data, "Invalid pool index: %i", number);
	      number = 0;
	    }
	    if (number < 256) {
	      vivi_code_asm_push_add_pool (push, number);
	    } else {
	      vivi_code_asm_push_add_pool_big (push, number);
	    }
	  } else if (g_ascii_strcasecmp (identifier, "reg") == 0) {
	    int number = parse_integer_value (data);
	    if (number < 0 || number >= 256) {
	      vivi_parser_error (data, "Invalid register number: %i", number);
	      number = 0;
	    }
	    vivi_code_asm_push_add_register (push, number);
	  } else {
	    vivi_parser_error (data, "Invalid identifier in push: %s",
		identifier);
	    vivi_code_asm_push_add_undefined (push);
	  }
	}
	break;
      default:
	vivi_parser_error (data, "Invalid token in push: %s",
	    vivi_parser_scanner_token_name (
	      vivi_parser_scanner_get_next_token (data->scanner)));
	vivi_code_asm_push_add_undefined (push);
	break;
    }
  } while (try_parse_token (data, TOKEN_COMMA));

  parse_automatic_semicolon (data);

  return VIVI_CODE_ASM (push);
}

static ViviCodeAsm *
parse_asm_get_url2 (ParseData *data)
{
  guint flags = 0;

  while (peek_identifier (data) && !peek_line_terminator (data)) {
    const char *name = parse_identifier_value (data);
    int value = vivi_code_asm_get_url2_flag_name_to_value (name);

    if (value == -1) {
      vivi_parser_error (data, "Invalid flag for get_url2: %s", name);
    } else {
      // TODO: warn if already set?
      flags |= 1 << value;
    }
  }

  parse_automatic_semicolon (data);

  return vivi_code_asm_get_url2_new_from_flags (flags);
}


typedef ViviCodeAsm *(*AsmConstructor) (void);
typedef ViviCodeAsm *(*ParseAsmFunction) (ParseData *data);

typedef struct {
  const char *			name;
  AsmConstructor		constructor;
  ParseAsmFunction		parse;
} AsmStatement;

static const AsmStatement asm_statements[] = {
#define DEFAULT_ASM(CapsName, underscore_name, byte_code) \
  { G_STRINGIFY (underscore_name), vivi_code_asm_ ## underscore_name ## _new, NULL },
#include "vivi_code_defaults.h"
#undef DEFAULT_ASM
  { "get_url2", NULL, parse_asm_get_url2 },
  { "if", NULL, parse_asm_if },
  { "jump", NULL, parse_asm_jump },
  { "pool", NULL, parse_asm_pool },
  { "push", NULL, parse_asm_push },
  { "store", NULL, parse_asm_store },
  { "try", NULL, parse_asm_try }
};
#if 0
DEFAULT_ASM (GotoFrame, goto_frame, SWFDEC_AS_ACTION_GOTO_FRAME)
DEFAULT_ASM (GetUrl, get_url, SWFDEC_AS_ACTION_GET_URL)
DEFAULT_ASM (StoreRegister, store_register, SWFDEC_AS_ACTION_STORE_REGISTER)
DEFAULT_ASM (ConstantPool, constant_pool, SWFDEC_AS_ACTION_CONSTANT_POOL)
DEFAULT_ASM (StrictMode, strict_mode, SWFDEC_AS_ACTION_STRICT_MODE)
DEFAULT_ASM (WaitForFrame, wait_for_frame, SWFDEC_AS_ACTION_WAIT_FOR_FRAME)
DEFAULT_ASM (SetTarget, set_target, SWFDEC_AS_ACTION_SET_TARGET)
DEFAULT_ASM (GotoLabel, goto_label, SWFDEC_AS_ACTION_GOTO_LABEL)
DEFAULT_ASM (WaitForFrame2, wait_for_frame2, SWFDEC_AS_ACTION_WAIT_FOR_FRAME2)
DEFAULT_ASM (DefineFunction2, define_function2, SWFDEC_AS_ACTION_DEFINE_FUNCTION2)
DEFAULT_ASM (Try, try, SWFDEC_AS_ACTION_TRY)
DEFAULT_ASM (With, with, SWFDEC_AS_ACTION_WITH)
DEFAULT_ASM (Push, push, SWFDEC_AS_ACTION_PUSH)
DEFAULT_ASM (Jump, jump, SWFDEC_AS_ACTION_JUMP)
DEFAULT_ASM (GetUrl2, get_url2, SWFDEC_AS_ACTION_GET_URL2)
DEFAULT_ASM (DefineFunction, define_function, SWFDEC_AS_ACTION_DEFINE_FUNCTION)
DEFAULT_ASM (If, if, SWFDEC_AS_ACTION_IF)
DEFAULT_ASM (Call, call, SWFDEC_AS_ACTION_CALL)
DEFAULT_ASM (GotoFrame2, goto_frame2, SWFDEC_AS_ACTION_GOTO_FRAME2)
#endif

static ViviCodeAsm *
parse_asm_code (ParseData *data)
{
  guint i;
  char *identifier;

  if (try_parse_token (data, TOKEN_THROW)) {
    identifier = g_strdup ("throw");
  } else if (try_parse_token (data, TOKEN_TRY)) {
    identifier = g_strdup ("try");
  } else if (try_parse_token (data, TOKEN_IF)) {
    identifier = g_strdup ("if");
  } else if (try_parse_token (data, TOKEN_IMPLEMENTS)) {
    identifier = g_strdup ("implements");
  } else if (try_parse_token (data, TOKEN_DELETE)) {
    identifier = g_strdup ("delete");
  } else if (try_parse_token (data, TOKEN_RETURN)) {
    identifier = g_strdup ("return");
  } else if (try_parse_token (data, TOKEN_EXTENDS)) {
    identifier = g_strdup ("extends");
  } else {
    identifier = g_strdup (parse_identifier_value (data));
  }

  if (try_parse_token (data, TOKEN_COLON)) {
    ViviCodeLabel *label;

    label = vivi_parser_get_waiting_label (data, identifier);
    if (label != NULL) {
      vivi_parser_remove_waiting_label (data, label);
    } else {
      label = VIVI_CODE_LABEL (vivi_code_label_new (identifier));
    }
    vivi_parser_add_label (data, label);

    g_free (identifier);
    return VIVI_CODE_ASM (label);
  } else {
    for (i = 0; i < G_N_ELEMENTS (asm_statements); i++) {
      if (g_ascii_strcasecmp (identifier, asm_statements[i].name) == 0)
	break;
    }
    if (i >= G_N_ELEMENTS (asm_statements)) {
      vivi_parser_error (data, "Unknown asm statement: %s", identifier);
      i = 0;
    }
    g_free (identifier);

    if (asm_statements[i].parse != NULL) {
      return asm_statements[i].parse (data);
    } else {
      parse_automatic_semicolon (data);
      g_assert (asm_statements[i].constructor != NULL);
      return asm_statements[i].constructor ();
    }
  }
}

static gboolean
peek_asm_statement (ParseData *data)
{
  const ViviParserValue *value;

  if (!peek_token (data, TOKEN_IDENTIFIER))
    return FALSE;

  if (g_ascii_strcasecmp (peek_identifier_value (data), "asm") != 0)
    return FALSE;

  value = vivi_parser_scanner_get_value (data->scanner, 2);

  return (value->token == TOKEN_BRACE_LEFT || value->token == TOKEN_PARENTHESIS_LEFT);
}

static ViviCodeStatement *
parse_asm_statement (ParseData *data)
{
  ViviCodeAssembler *assembler;
  ViviCodeAsm *code;

  assembler = VIVI_CODE_ASSEMBLER (vivi_code_assembler_new ());

  if (g_ascii_strcasecmp (parse_identifier_value (data), "asm") != 0)
    vivi_parser_error (data, "Expected 'asm'");

  if (try_parse_token (data, TOKEN_PARENTHESIS_LEFT)) {
    ViviCodeValue **arguments;
    guint i;

    if (!try_parse_token (data, TOKEN_PARENTHESIS_RIGHT)) {
      parse_value_list (data, peek_assignment_expression,
	  parse_assignment_expression, &arguments, TOKEN_COMMA);
      parse_token (data, TOKEN_PARENTHESIS_RIGHT);
    } else {
      arguments = NULL;
    }

    if (arguments != NULL) {
      for (i = 0; arguments[i] != NULL; i++) {
	vivi_code_assembler_push_argument (assembler, arguments[i]);
      }
      free_value_list (arguments);
    }
  }

  parse_token (data, TOKEN_BRACE_LEFT);

  if (!try_parse_token (data, TOKEN_BRACE_RIGHT)) {
    do {
      code = parse_asm_code (data);
      vivi_code_assembler_add_code (assembler, code);
      g_object_unref (code);
    } while (peek_token (data, TOKEN_IDENTIFIER) ||
	peek_token (data, TOKEN_THROW) ||
	peek_token (data, TOKEN_TRY) ||
	peek_token (data, TOKEN_IF) ||
	peek_token (data, TOKEN_IMPLEMENTS) ||
	peek_token (data, TOKEN_DELETE) ||
	peek_token (data, TOKEN_RETURN) ||
	peek_token (data, TOKEN_EXTENDS));

    if (!try_parse_token (data, TOKEN_BRACE_RIGHT)) {
      vivi_parser_error_unexpected_or (data, TOKEN_BRACE_RIGHT,
	  TOKEN_IDENTIFIER, TOKEN_NONE);
    }
  }

  return VIVI_CODE_STATEMENT (assembler);
}

// builtin functions

static ViviCodeStatement *
parse_get_url2 (ParseData *data, gboolean require_two, gboolean level,
    gboolean internal, gboolean variables)
{
  ViviCodeValue *url, *target;
  ViviCodeStatement *statement;
  guint method;

  parse_token (data, TOKEN_PARENTHESIS_LEFT);

  url = parse_assignment_expression (data);

  if (require_two || peek_token (data, TOKEN_COMMA)) {
    parse_token (data, TOKEN_COMMA);

    // target
    if (!level) {
      target = parse_assignment_expression (data);
    } else {
      // FIXME: integer only
      if (peek_numeric (data)) {
	char *level_name =
	  g_strdup_printf ("_level%i", (int)parse_numeric_value (data));
	target = vivi_code_string_new (level_name);
	g_free (level_name);
      } else {
	// TODO
	target = parse_assignment_expression (data);
      }
    }

    // method
    if (peek_token (data, TOKEN_COMMA)) {
      parse_token (data, TOKEN_COMMA);

      if (peek_identifier (data) || peek_string (data)) {
	const char *method_string;

	if (peek_identifier (data)) {
	  method_string = parse_identifier_value (data);
	} else {
	  method_string = parse_string_value (data);
	}
	if (g_ascii_strcasecmp (method_string, "GET") == 0) {
	  method = 1;
	} else if (g_ascii_strcasecmp (method_string, "POST") == 0) {
	  method = 2;
	} else {
	  method = 0;
	  // FIXME: only for identifiers?
	  vivi_parser_error (data, "Invalid URL method: %s\n", method_string);
	}
      } else {
	vivi_parser_error_unexpected_or (data, TOKEN_IDENTIFIER, TOKEN_STRING,
	    TOKEN_NONE);
	method = 0;
      }
    } else {
      method = 0;
    }
  } else {
    target = vivi_code_string_new ("");
    method = 0;
  }

  parse_token (data, TOKEN_PARENTHESIS_RIGHT);

  statement = vivi_code_get_url_new (target, url, method, internal, variables);
  g_object_unref (target);
  g_object_unref (url);

  return statement;
}

static ViviCodeStatement *
parse_get_url (ParseData *data)
{
  return parse_get_url2 (data, FALSE, FALSE, FALSE, FALSE);
}

static ViviCodeStatement *
parse_load_movie (ParseData *data)
{
  return parse_get_url2 (data, TRUE, FALSE, TRUE, FALSE);
}

static ViviCodeStatement *
parse_load_movie_num (ParseData *data)
{
  return parse_get_url2 (data, TRUE, TRUE, FALSE, FALSE);
}

static ViviCodeStatement *
parse_load_variables (ParseData *data)
{
  return parse_get_url2 (data, TRUE, FALSE, TRUE, TRUE);
}

static ViviCodeStatement *
parse_load_variables_num (ParseData *data)
{
  return parse_get_url2 (data, TRUE, TRUE, FALSE, TRUE);
}

static ViviCodeValue *
parse_concat (ParseData *data)
{
  ViviCodeValue *value, *first, *second;

  parse_token (data, TOKEN_PARENTHESIS_LEFT);
  first = parse_assignment_expression (data);
  parse_token (data, TOKEN_COMMA);
  second = parse_assignment_expression (data);
  parse_token (data, TOKEN_PARENTHESIS_RIGHT);

  value = vivi_code_concat_new (first, second);
  g_object_unref (first);
  g_object_unref (second);

  return value;
}

static ViviCodeValue *
parse_substring (ParseData *data)
{
  ViviCodeValue *value, *string, *index_, *count;

  parse_token (data, TOKEN_PARENTHESIS_LEFT);
  string = parse_assignment_expression (data);
  parse_token (data, TOKEN_COMMA);
  index_ = parse_assignment_expression (data);
  parse_token (data, TOKEN_COMMA);
  count = parse_assignment_expression (data);
  parse_token (data, TOKEN_PARENTHESIS_RIGHT);

  value = vivi_code_substring_new (string, index_, count);
  g_object_unref (string);
  g_object_unref (index_);
  g_object_unref (count);

  return value;
}

static ViviCodeValue *
parse_target_path (ParseData *data)
{
  ViviCodeValue *value, *identifier;

  parse_token (data, TOKEN_PARENTHESIS_LEFT);
  identifier = parse_identifier (data);
  parse_token (data, TOKEN_PARENTHESIS_RIGHT);

  value = vivi_code_target_path_new (identifier);
  g_object_unref (identifier);

  return value;
}

typedef ViviCodeStatement *(*NewStatementVoid) (void);
typedef ViviCodeStatement *(*NewStatementValue) (ViviCodeValue *value);

typedef struct {
  const char *			name;
  NewStatementVoid		constructor_void;
  NewStatementValue		constructor_value;
  ParseStatementFunction	parse_custom;
} BuiltinStatement;

static const BuiltinStatement builtin_statements[] = {
  //{ "callFrame",          NULL, vivi_code_call_frame_new, NULL },
  //{ "duplicateMovieClip", NULL, NULL, parse_duplicate_movie_clip },
  //{ "getURL1",            NULL, NULL, parse_get_url1 },
  { "getURL",             NULL, NULL, parse_get_url },
  //{ "gotoAndPlay",        NULL, vivi_code_goto_and_play_new, NULL },
  //{ "gotoAndStop",        NULL, vivi_code_goto_and_stop_new, NULL },
  { "loadMovie",          NULL, NULL, parse_load_movie },
  { "loadMovieNum",       NULL, NULL, parse_load_movie_num },
  { "loadVariables",      NULL, NULL, parse_load_variables },
  { "loadVariablesNum",   NULL, NULL, parse_load_variables_num },
  { "nextFrame",          vivi_code_next_frame_new, NULL, NULL },
  { "play",               vivi_code_play_new, NULL, NULL },
  { "prevFrame",          vivi_code_previous_frame_new, NULL, NULL },
  { "removeMovieClip",    NULL, vivi_code_remove_sprite_new, NULL },
  //{ "setProperty",        NULL, NULL, parse_set_property },
  { "setTarget",          NULL, vivi_code_set_target2_new, NULL },
  //{ "startDrag",          NULL, NULL, parse_start_drag },
  { "stop",               vivi_code_stop_new, NULL, NULL },
  { "stopDrag",           vivi_code_end_drag_new, NULL, NULL },
  { "stopSounds",         vivi_code_stop_sounds_new, NULL, NULL },
  { "toggleQuality",      vivi_code_toggle_quality_new, NULL, NULL },
  { "trace",              NULL, vivi_code_trace_new, NULL }
};

typedef ViviCodeValue *(*NewCallVoid) (void);
typedef ViviCodeValue *(*NewCallValue) (ViviCodeValue *value);

typedef struct {
  const char *		name;
  NewCallVoid		constructor_void;
  NewCallValue		constructor_value;
  ParseValueFunction	parse_custom;
} BuiltinCall;

static const BuiltinCall builtin_calls[] = {
  { "chr",         NULL, vivi_code_ascii_to_char_new, NULL },
  { "concat",      NULL, NULL, parse_concat },
  { "eval",        NULL, vivi_code_get_variable_new, NULL },
  //{ "getProperty", NULL, NULL, parse_get_property },
  { "getTimer",    vivi_code_get_timer_new, NULL, NULL },
  { "int",         NULL, vivi_code_to_integer_new, NULL },
  { "length",      NULL, vivi_code_string_length_new, NULL },
  { "Number",      NULL, vivi_code_to_number_new, NULL },
  { "ord",         NULL, vivi_code_char_to_ascii_new, NULL },
  { "random",      NULL, vivi_code_random_new, NULL },
  { "substring",   NULL, NULL, parse_substring },
  { "targetPath",  NULL, NULL, parse_target_path },
  { "typeOf",      NULL, vivi_code_type_of_new, NULL }
};

static gboolean
peek_builtin_call (ParseData *data)
{
  guint i;
  const char *identifier;
  const ViviParserValue *value;

  if (peek_token (data, TOKEN_TYPEOF)) {
    identifier = "typeOf";
  } else {
    if (!peek_token (data, TOKEN_IDENTIFIER))
      return FALSE;

    identifier =
      vivi_parser_scanner_get_value (data->scanner, 1)->value.v_identifier;
  }

  value = vivi_parser_scanner_get_value (data->scanner, 2);

  if (value->token != TOKEN_PARENTHESIS_LEFT)
    return FALSE;

  for (i = 0; i < G_N_ELEMENTS (builtin_calls); i++) {
    if (g_ascii_strcasecmp (identifier, builtin_calls[i].name) == 0)
      return TRUE;
  }

  return FALSE;
}

static ViviCodeValue *
parse_builtin_call (ParseData *data)
{
  guint i;
  const char *identifier;
  ViviCodeValue *value, *argument;

  if (try_parse_token (data, TOKEN_TYPEOF)) {
    identifier = "typeOf";
  } else {
    identifier = parse_identifier_value (data);
  }

  for (i = 0; i < G_N_ELEMENTS (builtin_calls); i++) {
    if (g_ascii_strcasecmp (identifier, builtin_calls[i].name) == 0)
      break;
  }
  if (i >= G_N_ELEMENTS (builtin_calls)) {
    vivi_parser_error (data, "Unknown builtin call: %s", identifier);
    i = 0;
  }

  if (builtin_calls[i].parse_custom != NULL)
    return builtin_calls[i].parse_custom (data);

  parse_token (data, TOKEN_PARENTHESIS_LEFT);

  if (builtin_calls[i].constructor_value != NULL)
    argument = parse_assignment_expression (data);

  parse_token (data, TOKEN_PARENTHESIS_RIGHT);

  if (builtin_calls[i].constructor_value != NULL) {
    value = builtin_calls[i].constructor_value (argument);
    g_object_unref (argument);
  } else {
    g_assert (builtin_calls[i].constructor_void != NULL);
    value = builtin_calls[i].constructor_void ();
  }

  return value;
}


static gboolean
peek_builtin_statement (ParseData *data)
{
  guint i;
  const char *identifier;
  const ViviParserValue *value;

  if (!peek_token (data, TOKEN_IDENTIFIER))
    return FALSE;

  identifier = peek_identifier_value (data);

  value = vivi_parser_scanner_get_value (data->scanner, 2);

  if (value->token != TOKEN_PARENTHESIS_LEFT)
    return FALSE;

  for (i = 0; i < G_N_ELEMENTS (builtin_statements); i++) {
    if (g_ascii_strcasecmp (identifier, builtin_statements[i].name) == 0)
      return TRUE;
  }

  return FALSE;
}

static ViviCodeStatement *
parse_builtin_statement (ParseData *data)
{
  guint i;
  const char *identifier;
  ViviCodeValue *argument;
  ViviCodeStatement *statement;

  identifier = parse_identifier_value (data);

  for (i = 0; i < G_N_ELEMENTS (builtin_statements); i++) {
    if (g_ascii_strcasecmp (identifier, builtin_statements[i].name) == 0)
      break;
  }
  if (i >= G_N_ELEMENTS (builtin_statements)) {
    vivi_parser_error (data, "Unknown builtin statement: %s", identifier);
    i = 0;
  }

  if (builtin_statements[i].parse_custom != NULL) {
    return builtin_statements[i].parse_custom (data);
  }

  parse_token (data, TOKEN_PARENTHESIS_LEFT);

  if (builtin_statements[i].constructor_value != NULL)
    argument = parse_assignment_expression (data);

  parse_token (data, TOKEN_PARENTHESIS_RIGHT);

  parse_automatic_semicolon (data);

  if (builtin_statements[i].constructor_value != NULL) {
    statement = builtin_statements[i].constructor_value (argument);
    g_object_unref (argument);
  } else {
    g_assert (builtin_statements[i].constructor_void != NULL);
    statement = builtin_statements[i].constructor_void ();
  }

  return statement;
}

// expression

static const struct {
  PeekFunction peek;
  ParseValueFunction parse_value;
} primary_expression_functions[] = {
  { peek_object_literal, parse_object_literal },
  { peek_array_literal, parse_array_literal },
  { peek_identifier, parse_identifier },
  { peek_literal, parse_literal },
  { NULL, NULL }
};

static gboolean
peek_primary_expression (ParseData *data)
{
  guint i;

  if (peek_token (data, TOKEN_THIS)||
      peek_token (data, TOKEN_PARENTHESIS_LEFT))
    return TRUE;

  for (i = 0; primary_expression_functions[i].peek != NULL; i++) {
    if (primary_expression_functions[i].peek (data))
      return TRUE;
  }

  return FALSE;
}

static ViviCodeValue *
parse_expression (ParseData *data);

static ViviCodeValue *
parse_primary_expression (ParseData *data)
{
  ViviCodeValue *value;
  guint i;

  vivi_parser_start_code_token (data);

  if (try_parse_token (data, TOKEN_THIS)) {
    value = vivi_code_get_new_name (NULL, "this");
    vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));
    return value;
  }

  if (try_parse_token (data, TOKEN_PARENTHESIS_LEFT)) {
    value = parse_expression (data);
    parse_token (data, TOKEN_PARENTHESIS_RIGHT);
    vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));
    return value;
  }

  for (i = 0; primary_expression_functions[i].peek != NULL; i++) {
    if (primary_expression_functions[i].peek (data)) {
      value = primary_expression_functions[i].parse_value (data);
      vivi_parser_end_code_token (data, NULL);
      return value;
    }
  }

  vivi_parser_error_unexpected (data, ERROR_TOKEN_PRIMARY_EXPRESSION);
  value = vivi_code_undefined_new ();
  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));

  return value;
}

static gboolean
peek_function_expression (ParseData *data);

static ViviCodeValue *
parse_function_expression (ParseData *data);

static gboolean
peek_left_hand_side_expression (ParseData *data)
{
  return (peek_token (data, TOKEN_NEW) || peek_primary_expression (data) ||
      peek_function_expression (data));

}

static ViviCodeValue *
parse_left_hand_side_expression (ParseData *data)
{
  ViviCodeValue *value, *name;
  ViviCodeValue **arguments;
  guint i;

  vivi_parser_start_code_token (data);

  if (try_parse_token (data, TOKEN_NEW)) {
    value = parse_left_hand_side_expression (data);

    if (!VIVI_IS_CODE_FUNCTION_CALL (value)) {
      ViviCodeValue *tmp = value;
      value = vivi_parser_function_call_new (tmp);
      g_object_unref (tmp);
    }

    vivi_code_function_call_set_construct (VIVI_CODE_FUNCTION_CALL (value),
	TRUE);

    vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));

    return value;
  }

  if (peek_builtin_call (data)) {
    value = parse_builtin_call (data);
  } else if (peek_primary_expression (data)) {
    value = parse_primary_expression (data);
  } else if (peek_function_expression (data)) {
    value = parse_function_expression (data);
  } else {
    vivi_parser_error_unexpected_or (data, ERROR_TOKEN_PRIMARY_EXPRESSION,
	ERROR_TOKEN_FUNCTION_EXPRESSION, TOKEN_NONE);
    value = vivi_code_undefined_new ();
    vivi_parser_duplicate_code_token (data);
    vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));
  }

  while (TRUE) {
    ViviCodeValue *tmp;

    if (try_parse_token (data, TOKEN_BRACKET_LEFT)) {
      ViviCodeValue *member = parse_expression (data);
      tmp = value;
      value = vivi_code_get_new (tmp, member);
      g_object_unref (tmp);
      g_object_unref (member);
      parse_token (data, TOKEN_BRACKET_RIGHT);
    } else if (try_parse_token (data, TOKEN_DOT)) {
      tmp = value;
      value = vivi_code_get_new_name (tmp, parse_identifier_value (data));
      g_object_unref (tmp);
    } else if (try_parse_token (data, TOKEN_PARENTHESIS_LEFT)) {
      if (!try_parse_token (data, TOKEN_PARENTHESIS_RIGHT)) {
	parse_value_list (data, peek_assignment_expression,
	    parse_assignment_expression, &arguments, TOKEN_COMMA);
	parse_token (data, TOKEN_PARENTHESIS_RIGHT);
      } else {
	arguments = NULL;
      }

      name = value;
      value = vivi_parser_function_call_new (name);
      g_object_unref (name);

      if (arguments != NULL) {
	for (i = 0; arguments[i] != NULL; i++) {
	  vivi_code_function_call_add_argument (VIVI_CODE_FUNCTION_CALL (value),
	      arguments[i]);
	}
	free_value_list (arguments);
      }

      vivi_parser_duplicate_code_token (data);
      vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));
    } else {
      vivi_parser_end_code_token (data, NULL);
      return value;
    }

    vivi_parser_duplicate_code_token (data);
    vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));
  }

  vivi_parser_end_code_token (data, NULL);

  return value;
}

static gboolean
peek_postfix_expression (ParseData *data)
{
  return peek_left_hand_side_expression (data);
}

static ViviCodeValue *
parse_postfix_expression (ParseData *data)
{
  ViviCodeValue *value;
  ViviCodeGet *get;
  gboolean add;

  vivi_parser_start_code_token (data);

  value = parse_left_hand_side_expression (data);

  if (peek_line_terminator (data)) {
    vivi_parser_end_code_token (data, NULL);
    return value;
  }

  if (try_parse_token (data, TOKEN_INCREASE)) {
    add = TRUE;
  } else if (try_parse_token (data, TOKEN_DESCREASE)) {
    add = FALSE;
  } else {
    vivi_parser_end_code_token (data, NULL);
    return value;
  }

  // FIXME: Correct?
  if (!vivi_parser_value_is_left_hand_side (value)) {
    vivi_parser_error (data,
	"Invalid left-hand side expression for INCREASE/DECREASE");
    g_object_unref (value);
    value = vivi_code_get_new_name (NULL, "undefined");
  }

  get = VIVI_CODE_GET (value);
  value = vivi_code_inc_dec_new (get->from, get->name, add, FALSE);
  g_object_unref (get);

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));

  return value;
}

static gboolean
peek_unary_expression (ParseData *data)
{
  switch ((guint) vivi_parser_scanner_peek_next_token (data->scanner)) {
    //case TOKEN_DELETE:
    //case TOKEN_TYPEOF:
    case TOKEN_INCREASE:
    case TOKEN_DESCREASE:
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_VOID:
    case TOKEN_BITWISE_NOT:
    case TOKEN_LOGICAL_NOT:
      return TRUE;
    default:
      return peek_postfix_expression (data);
  }

  g_assert_not_reached ();
}

static ViviCodeValue *
parse_unary_expression (ParseData *data)
{
  ViviCodeValue *(*constructor) (ViviCodeValue *value);
  ViviCodeValue *value, *tmp;
  ViviCodeGet *get;
  gboolean increment = FALSE;

  constructor = NULL;
  switch ((guint) vivi_parser_scanner_peek_next_token (data->scanner)) {
    //case TOKEN_DELETE:
    //case TOKEN_TYPEOF:
    case TOKEN_INCREASE:
      increment = TRUE;
    case TOKEN_DESCREASE:
      vivi_parser_start_code_token (data);

      vivi_parser_scanner_get_next_token (data->scanner);

      value = parse_unary_expression (data);

      // FIXME: Correct?
      if (!vivi_parser_value_is_left_hand_side (value)) {
	vivi_parser_error (data,
	    "Invalid left-hand side expression for INCREASE/DECREASE");
	g_object_unref (value);
	value = vivi_code_get_new_name (NULL, "undefined");
      }

      get = VIVI_CODE_GET (value);
      value = vivi_code_inc_dec_new (get->from, get->name, increment, TRUE);
      g_object_unref (get);

      vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));
      break;
    case TOKEN_PLUS:
      vivi_parser_scanner_get_next_token (data->scanner);

      value = parse_unary_expression (data);

      if (!VIVI_IS_CODE_NUMBER (value)) {
	tmp = value;
	value = vivi_code_to_number_new (tmp);
	g_object_unref (tmp);
      }
      break;
    case TOKEN_MINUS:
      vivi_parser_scanner_get_next_token (data->scanner);

      value = parse_unary_expression (data);

      if (VIVI_IS_CODE_NUMBER (value)) {
	tmp = value;
	value = vivi_code_number_new (
	    -vivi_code_number_get_value (VIVI_CODE_NUMBER (tmp)));
	g_object_unref (tmp);
      } else {
	ViviCodeValue *number;

	number = vivi_code_number_new (-1);
	tmp = value;
	value = vivi_code_multiply_new (number, tmp);
	g_object_unref (tmp);
	g_object_unref (number);
      }
      break;
    case TOKEN_VOID:
      if (constructor == NULL)
	constructor = vivi_code_void_new;
    case TOKEN_BITWISE_NOT:
      if (constructor == NULL)
	constructor = vivi_code_bit_not_new;
    case TOKEN_LOGICAL_NOT:
      if (constructor == NULL)
	constructor = vivi_code_not_new;

      vivi_parser_start_code_token (data);

      vivi_parser_scanner_get_next_token (data->scanner);

      value = parse_unary_expression (data);

      tmp = VIVI_CODE_VALUE (value);
      value = constructor (tmp);
      g_object_unref (tmp);

      vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));
      break;
    default:
      value = parse_postfix_expression (data);
      break;
  }

  return value;
}

typedef enum {
  PASS_ALWAYS,
  PASS_LOGICAL_OR,
  PASS_LOGICAL_AND
} ParseOperatorPass;

static ViviCodeValue *
parse_operator_expression (ParseData *data,
    const ViviParserScannerToken *tokens, ParseOperatorPass pass,
    ParseValueFunction next_parse_function)
{
  ViviCodeValue *value, *left, *right;
  guint i, j;
  static const struct {
    ViviParserScannerToken	token;
    ViviCodeValue *		(* func) (ViviCodeValue *, ViviCodeValue *);
  } table[] = {
//    { TOKEN_, vivi_code_add_new },
    { TOKEN_MINUS, vivi_code_subtract_new },
    { TOKEN_MULTIPLY, vivi_code_multiply_new },
    { TOKEN_DIVIDE, vivi_code_divide_new },
    { TOKEN_REMAINDER, vivi_code_modulo_new },
//    { TOKEN_, vivi_code_equals_new },
//    { TOKEN_, vivi_code_less_new },
//    { TOKEN_, vivi_code_logical_and_new },
//    { TOKEN_, vivi_code_logical_or_new },
//    { TOKEN_, vivi_code_string_equals_new },
//    { TOKEN_, vivi_code_string_less_new },
    { TOKEN_PLUS, vivi_code_add2_new },
    { TOKEN_LESS_THAN, vivi_code_less2_new },
    { TOKEN_LESS_THAN_OR_EQUAL, vivi_code_greater_new },
    { TOKEN_EQUAL, vivi_code_equals2_new },
    { TOKEN_NOT_EQUAL, vivi_code_equals2_new },
    { TOKEN_BITWISE_AND, vivi_code_bit_and_new },
    { TOKEN_BITWISE_OR, vivi_code_bit_or_new },
    { TOKEN_BITWISE_XOR, vivi_code_bit_xor_new },
    { TOKEN_SHIFT_LEFT, vivi_code_bit_lshift_new },
    { TOKEN_SHIFT_RIGHT, vivi_code_bit_rshift_new },
    { TOKEN_SHIFT_RIGHT_UNSIGNED, vivi_code_bit_urshift_new },
    { TOKEN_STRICT_EQUAL, vivi_code_strict_equals_new },
    { TOKEN_NOT_STRICT_EQUAL, vivi_code_strict_equals_new },
    { TOKEN_GREATER_THAN, vivi_code_greater_new },
    { TOKEN_GREATER_THAN_OR_EQUAL, vivi_code_less2_new },
    { TOKEN_INSTANCEOF, vivi_code_instance_of_new },
    { TOKEN_LOGICAL_AND, vivi_code_and_new },
    { TOKEN_LOGICAL_OR, vivi_code_or_new }
  };

  vivi_parser_start_code_token (data);

  value = next_parse_function (data);

again:
  for (i = 0; tokens[i] != TOKEN_NONE; i++) {
    if (try_parse_token (data, tokens[i])) {
      right = next_parse_function (data);

      left = VIVI_CODE_VALUE (value);
      for (j = 0; j < G_N_ELEMENTS (table); j++) {
	if (tokens[i] != table[j].token)
	  continue;

	value = table[j].func (left, VIVI_CODE_VALUE (right));
	g_object_unref (left);
	g_object_unref (right);

	vivi_parser_duplicate_code_token (data);
	vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));

	if (tokens[i] == TOKEN_NOT_EQUAL ||
	    tokens[i] == TOKEN_NOT_STRICT_EQUAL ||
	    tokens[i] == TOKEN_LESS_THAN_OR_EQUAL ||
	    tokens[i] == TOKEN_GREATER_THAN_OR_EQUAL) {
	  left = vivi_code_not_new (value);
	  g_object_unref (value);
	  value = left;

	  vivi_parser_duplicate_code_token (data);
	  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (value));
	}

	goto again;
      }
      g_assert_not_reached ();
    }
  }

  vivi_parser_end_code_token (data, NULL);

  return value;
}

static gboolean
peek_multiplicative_expression (ParseData *data)
{
  return peek_unary_expression (data);
}

static ViviCodeValue *
parse_multiplicative_expression (ParseData *data)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_MULTIPLY,
    TOKEN_DIVIDE, TOKEN_REMAINDER, TOKEN_NONE };

  return parse_operator_expression (data, tokens, PASS_ALWAYS,
      parse_unary_expression);
}

static gboolean
peek_additive_expression (ParseData *data)
{
  return peek_multiplicative_expression (data);
}

static ViviCodeValue *
parse_additive_expression (ParseData *data)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_PLUS, TOKEN_MINUS,
    TOKEN_NONE };

  return parse_operator_expression (data, tokens, PASS_ALWAYS,
      parse_multiplicative_expression);
}

static gboolean
peek_shift_expression (ParseData *data)
{
  return peek_additive_expression (data);
}

static ViviCodeValue *
parse_shift_expression (ParseData *data)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_SHIFT_LEFT,
    TOKEN_SHIFT_RIGHT, TOKEN_SHIFT_RIGHT_UNSIGNED, TOKEN_NONE };

  return parse_operator_expression (data, tokens, PASS_ALWAYS,
      parse_additive_expression);
}

static gboolean
peek_relational_expression (ParseData *data)
{
  return peek_shift_expression (data);
}

static ViviCodeValue *
parse_relational_expression (ParseData *data)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_LESS_THAN,
    TOKEN_GREATER_THAN, TOKEN_LESS_THAN_OR_EQUAL,
    TOKEN_GREATER_THAN_OR_EQUAL, TOKEN_INSTANCEOF, /*TOKEN_IN,*/ TOKEN_NONE };

  return parse_operator_expression (data, tokens, PASS_ALWAYS,
      parse_shift_expression);
}

static gboolean
peek_equality_expression (ParseData *data)
{
  return peek_relational_expression (data);
}

static ViviCodeValue *
parse_equality_expression (ParseData *data)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_EQUAL,
    TOKEN_NOT_EQUAL, TOKEN_STRICT_EQUAL, TOKEN_NOT_STRICT_EQUAL,
    TOKEN_NONE };

  return parse_operator_expression (data, tokens, PASS_ALWAYS,
      parse_relational_expression);
}

static gboolean
peek_bitwise_and_expression (ParseData *data)
{
  return peek_equality_expression (data);
}

static ViviCodeValue *
parse_bitwise_and_expression (ParseData *data)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_BITWISE_AND,
    TOKEN_NONE };

  return parse_operator_expression (data, tokens, PASS_ALWAYS,
      parse_equality_expression);
}

static gboolean
peek_bitwise_xor_expression (ParseData *data)
{
  return peek_bitwise_and_expression (data);
}

static ViviCodeValue *
parse_bitwise_xor_expression (ParseData *data)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_BITWISE_XOR,
    TOKEN_NONE };

  return parse_operator_expression (data, tokens, PASS_ALWAYS,
      parse_bitwise_and_expression);
}

static gboolean
peek_bitwise_or_expression (ParseData *data)
{
  return peek_bitwise_xor_expression (data);
}

static ViviCodeValue *
parse_bitwise_or_expression (ParseData *data)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_BITWISE_OR,
    TOKEN_NONE };

  return parse_operator_expression (data, tokens, PASS_ALWAYS,
      parse_bitwise_xor_expression);
}

static gboolean
peek_logical_and_expression (ParseData *data)
{
  return peek_bitwise_or_expression (data);
}

static ViviCodeValue *
parse_logical_and_expression (ParseData *data)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_LOGICAL_AND,
    TOKEN_NONE };

  return parse_operator_expression (data, tokens, PASS_LOGICAL_AND,
      parse_bitwise_or_expression);
}

static gboolean
peek_logical_or_expression (ParseData *data)
{
  return peek_logical_and_expression (data);
}

static ViviCodeValue *
parse_logical_or_expression (ParseData *data)
{
  static const ViviParserScannerToken tokens[] = { TOKEN_LOGICAL_OR,
    TOKEN_NONE };

  return parse_operator_expression (data, tokens, PASS_LOGICAL_OR,
      parse_logical_and_expression);
}

static gboolean
peek_conditional_expression (ParseData *data)
{
  return peek_logical_or_expression (data);
}

static ViviCodeValue *
parse_conditional_expression (ParseData *data)
{
  ViviCodeValue *condition, *if_value, *else_value, *conditional;

  condition = parse_logical_or_expression (data);

  if (!try_parse_token (data, TOKEN_QUESTION_MARK))
    return condition;

  if_value = parse_assignment_expression (data);
  parse_token (data, TOKEN_COLON);
  else_value = parse_assignment_expression (data);

  conditional = vivi_code_conditional_new (condition, if_value, else_value);
  g_object_unref (condition);
  g_object_unref (if_value);
  g_object_unref (else_value);

  return conditional;
}

static gboolean
peek_assignment_expression (ParseData *data)
{
  return peek_conditional_expression (data);
}

static ViviCodeValue *
parse_assignment_expression (ParseData *data)
{
  ViviCodeValue *value, *assignment, *right;
  ViviCodeValue * (* func) (ViviCodeValue *, ViviCodeValue *);

  vivi_parser_start_code_token (data);

  value = parse_conditional_expression (data);

  // FIXME: Correct?
  if (!vivi_parser_value_is_left_hand_side (value)) {
    vivi_parser_end_code_token (data, NULL);
    return value;
  }

  switch ((guint) vivi_parser_scanner_peek_next_token (data->scanner)) {
    case TOKEN_ASSIGN_MULTIPLY:
      func = vivi_code_multiply_new;
      break;
    case TOKEN_ASSIGN_DIVIDE:
      func = vivi_code_divide_new;
      break;
    case TOKEN_ASSIGN_REMAINDER:
      func = vivi_code_modulo_new;
      break;
    case TOKEN_ASSIGN_ADD:
      func = vivi_code_add2_new;
      break;
    case TOKEN_ASSIGN_MINUS:
      func = vivi_code_subtract_new;
      break;
    case TOKEN_ASSIGN_SHIFT_LEFT:
      func = vivi_code_bit_lshift_new;
      break;
    case TOKEN_ASSIGN_SHIFT_RIGHT:
      func = vivi_code_bit_rshift_new;
      break;
    case TOKEN_ASSIGN_SHIFT_RIGHT_ZERO:
      func = vivi_code_bit_urshift_new;
      break;
    case TOKEN_ASSIGN_BITWISE_AND:
      func = vivi_code_bit_and_new;
      break;
    case TOKEN_ASSIGN_BITWISE_OR:
      func = vivi_code_bit_or_new;
      break;
    case TOKEN_ASSIGN_BITWISE_XOR:
      func = vivi_code_bit_xor_new;
      break;
    case TOKEN_ASSIGN:
      func = NULL;
      break;
    default:
      return value;
  }

  vivi_parser_scanner_get_next_token (data->scanner);

  right = parse_assignment_expression (data);

  if (func != NULL) {
    assignment = vivi_parser_assignment_new (value, func (value, right));
  } else {
    assignment = vivi_parser_assignment_new (value, right);
  }
  g_object_unref (right);
  g_object_unref (value);

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (assignment));

  return assignment;
}

static gboolean
peek_expression (ParseData *data)
{
  return peek_assignment_expression (data);
}

static ViviCodeValue *
parse_expression (ParseData *data)
{
  ViviCodeValue *value;

  value = NULL;

  do {
    if (value != NULL)
      g_object_unref (value);
    value = parse_assignment_expression (data);
  } while (try_parse_token (data, TOKEN_COMMA));

  return value;
}

// statement

static ViviCodeStatement *
parse_continue_or_break_statement (ParseData *data,
    ViviParserScannerToken token)
{
  ViviCodeStatement *statement;

  vivi_parser_start_code_token (data);

  parse_token (data, token);

  if (!try_parse_restricted_semicolon (data)) {
    ViviCodeLabel *label;
    const char *name = parse_identifier_value (data);

    label = vivi_parser_get_label (data, name);
    if (label == NULL) {
      label = VIVI_CODE_LABEL (vivi_code_label_new (name));
      vivi_parser_add_waiting_label (data, label);
    }
    statement = vivi_code_goto_new (label);

    parse_automatic_semicolon (data);
  } else {
    g_assert (data->level != NULL);
    if (data->level->loop_count == 0) {
      vivi_parser_error (data,
	  "Break or continue without label outside of loop");
    }
    if (token == TOKEN_CONTINUE) {
      statement = vivi_code_continue_new ();
    } else {
      statement = vivi_code_break_new ();
    }
  }

  vivi_parser_end_code_token (data, VIVI_CODE_TOKEN (statement));

  return statement;
}

static gboolean
peek_continue_statement (ParseData *data)
{
  return peek_token (data, TOKEN_CONTINUE);
}

static ViviCodeStatement *
parse_continue_statement (ParseData *data)
{
  return parse_continue_or_break_statement (data, TOKEN_CONTINUE);
}

static gboolean
peek_break_statement (ParseData *data)
{
  return peek_token (data, TOKEN_BREAK);
}

static ViviCodeStatement *
parse_break_statement (ParseData *data)
{
  return parse_continue_or_break_statement (data, TOKEN_BREAK);
}

static gboolean
peek_throw_statement (ParseData *data)
{
  return peek_token (data, TOKEN_THROW);
}

static ViviCodeStatement *
parse_throw_statement (ParseData *data)
{
  ViviCodeValue *value;
  ViviCodeStatement *statement;

  parse_token (data, TOKEN_THROW);

  if (peek_line_terminator (data)) {
    vivi_parser_error_unexpected_line_terminator (data,
	ERROR_TOKEN_EXPRESSION);
  }

  value = parse_expression (data);

  statement = vivi_code_throw_new (value);
  g_object_unref (value);

  parse_automatic_semicolon (data);

  return statement;
}

static ViviCodeStatement *parse_block (ParseData *data);

static gboolean
peek_try_statement (ParseData *data)
{
  return peek_token (data, TOKEN_TRY);
}

static ViviCodeStatement *
parse_try_statement (ParseData *data)
{
  ViviCodeStatement *statement;
  ViviCodeStatement *try_statement, *catch_statement, *finally_statement;
  char *catch_identifier;

  parse_token (data, TOKEN_TRY);

  try_statement = parse_block (data);

  if (try_parse_token (data, TOKEN_CATCH)) {
    parse_token (data, TOKEN_PARENTHESIS_LEFT);
    catch_identifier = g_strdup (parse_identifier_value (data));
    parse_token (data, TOKEN_PARENTHESIS_RIGHT);
    catch_statement = parse_block (data);
  } else {
    catch_identifier = NULL;
    catch_statement = NULL;
  }

  if (try_parse_token (data, TOKEN_FINALLY)) {
    finally_statement = parse_block (data);
  } else {
    finally_statement = NULL;
  }

  if (catch_statement == NULL && finally_statement == NULL) {
    vivi_parser_error_unexpected_or (data, TOKEN_CATCH, TOKEN_FINALLY,
	TOKEN_NONE);
    finally_statement = vivi_code_block_new ();
  }

  statement = vivi_code_try_new (try_statement, catch_identifier,
      catch_statement, finally_statement);
  g_object_unref (try_statement);
  g_free (catch_identifier);
  if (catch_statement != NULL)
    g_object_unref (catch_statement);
  if (finally_statement != NULL)
    g_object_unref (finally_statement);

  return statement;
}

static gboolean
peek_return_statement (ParseData *data)
{
  return peek_token (data, TOKEN_RETURN);
}

static ViviCodeStatement *
parse_return_statement (ParseData *data)
{
  ViviCodeStatement *statement;

  statement = vivi_code_return_new ();

  parse_token (data, TOKEN_RETURN);

  if (!try_parse_restricted_semicolon (data)) {
    ViviCodeValue *value;

    value = parse_expression (data);

    vivi_code_return_set_value (VIVI_CODE_RETURN (statement), value);
    g_object_unref (value);

    parse_automatic_semicolon (data);
  }

  return statement;
}

static gboolean
peek_statement (ParseData *data);

static ViviCodeStatement *
parse_statement (ParseData *data);

static gboolean
peek_iteration_statement (ParseData *data)
{
  return (peek_token (data, TOKEN_DO) || peek_token (data, TOKEN_WHILE) ||
      peek_token (data, TOKEN_FOR));
}

static ViviCodeStatement *
parse_iteration_statement (ParseData *data)
{
  ViviCodeValue *condition, *enumerate_variable;
  ViviCodeStatement *statement;
  ViviCodeStatement *pre_statement, *loop_statement;
  gboolean post_condition, enumerate, enumerate_local;

  enumerate = FALSE;
  enumerate_local = FALSE;
  enumerate_variable = NULL;

  g_assert (data->level != NULL);
  data->level->loop_count++;

  if (try_parse_token (data, TOKEN_DO))
  {
    loop_statement = parse_statement (data);
    parse_token (data, TOKEN_WHILE);
    parse_token (data, TOKEN_PARENTHESIS_LEFT);
    condition = parse_expression (data);
    post_condition = TRUE;
    parse_token (data, TOKEN_PARENTHESIS_RIGHT);
    parse_automatic_semicolon (data);

    pre_statement = NULL;
  }
  else if (try_parse_token (data, TOKEN_WHILE))
  {
    parse_token (data, TOKEN_PARENTHESIS_LEFT);
    condition = parse_expression (data);
    post_condition = FALSE;
    parse_token (data, TOKEN_PARENTHESIS_RIGHT);
    loop_statement = parse_statement (data);

    pre_statement = NULL;
  }
  else if (try_parse_token (data, TOKEN_FOR))
  {
    ViviCodeStatement *post_statement;

    parse_token (data, TOKEN_PARENTHESIS_LEFT);

    if (try_parse_token (data, TOKEN_VAR)) {
      // FIXME: no in
      pre_statement = parse_statement_list (data, peek_variable_declaration,
	  parse_variable_declaration, TOKEN_COMMA);
      // FIXME: ugly... if there was only one VariableDeclaration,
      // get the name for enumerate_variable
      g_assert (VIVI_IS_CODE_BLOCK (pre_statement));
      if (vivi_code_block_get_n_statements (VIVI_CODE_BLOCK (pre_statement))
	  == 1) {
	ViviCodeVariable *variable = VIVI_CODE_VARIABLE (
	    vivi_code_block_get_statement (VIVI_CODE_BLOCK (pre_statement), 0));
	enumerate_variable = vivi_code_get_new (NULL, variable->name);
	enumerate_local = TRUE;
      } else {
	enumerate_variable = NULL;
      }
    } else {
      if (try_parse_token (data, TOKEN_SEMICOLON)) {
	enumerate_variable = NULL;
	pre_statement = NULL;
      } else {
	// FIXME: no in
	enumerate_variable = parse_expression (data);
	enumerate_local = FALSE;
	pre_statement =
	  g_object_ref (VIVI_CODE_STATEMENT (enumerate_variable));
      }
    }

    if (try_parse_token (data, TOKEN_SEMICOLON)) {
      if (enumerate_variable != NULL)
	g_object_unref (enumerate_variable);

      if (try_parse_token (data, TOKEN_SEMICOLON)) {
	condition = vivi_code_boolean_new (TRUE);
	post_condition = FALSE;
      } else {
	condition = parse_expression (data);
	post_condition = FALSE;
	parse_token (data, TOKEN_SEMICOLON);
      }

      if (!peek_token (data, TOKEN_PARENTHESIS_RIGHT)) {
	post_statement = VIVI_CODE_STATEMENT (parse_expression (data));
      } else {
	post_statement = NULL;
      }
    } else if (enumerate_variable != NULL && try_parse_token (data, TOKEN_IN)) {
      // FIXME: correct?
      if (!vivi_parser_value_is_left_hand_side (enumerate_variable)) {
	vivi_parser_error (data, "Invalid left-hand side expression for in");
	enumerate_variable = vivi_code_string_new ("undefined");
      }

      if (pre_statement != NULL) {
	g_object_unref (pre_statement);
	pre_statement = NULL;
      }

      condition = parse_expression (data);
      post_condition = FALSE;
      post_statement = NULL;
      enumerate = TRUE;
    } else {
      enumerate = FALSE;

      if (enumerate_variable != NULL) {
	vivi_parser_error_unexpected_or (data, TOKEN_SEMICOLON, TOKEN_IN,
	    TOKEN_NONE);
      } else {
	vivi_parser_error_unexpected (data, TOKEN_SEMICOLON);
      }

      condition = vivi_code_undefined_new ();
      post_condition = FALSE;
      post_statement = NULL;

      if (enumerate_variable != NULL)
	g_object_unref (enumerate_variable);
      if (pre_statement != NULL)
	g_object_unref (pre_statement);
    }

    parse_token (data, TOKEN_PARENTHESIS_RIGHT);
    loop_statement = parse_statement (data);

    if (post_statement != NULL) {
      ViviCodeStatement *block = vivi_code_block_new ();
      vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), loop_statement);
      g_object_unref (loop_statement);
      vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), post_statement);
      g_object_unref (post_statement);

      loop_statement = block;
    }
  }
  else
  {
    vivi_parser_error_unexpected (data, ERROR_TOKEN_ITERATION_STATEMENT);

    enumerate = FALSE;
    condition = vivi_code_undefined_new ();
    post_condition = FALSE;
    pre_statement = NULL;
    loop_statement = vivi_compiler_empty_statement_new ();
  }

  if (enumerate) {
    g_assert (VIVI_IS_CODE_GET (enumerate_variable));
    g_assert (pre_statement == NULL);
    g_assert (post_condition == FALSE);

    statement = vivi_code_enumerate_new (condition,
	VIVI_CODE_GET (enumerate_variable)->from,
	VIVI_CODE_GET (enumerate_variable)->name, enumerate_local);
    vivi_code_enumerate_set_statement (VIVI_CODE_ENUMERATE (statement),
	loop_statement);
    return statement;
  }

  statement = vivi_code_loop_new ();
  vivi_code_loop_set_condition (VIVI_CODE_LOOP (statement), condition,
      post_condition);
  g_object_unref (condition);
  vivi_code_loop_set_statement (VIVI_CODE_LOOP (statement), loop_statement);
  g_object_unref (loop_statement);

  if (pre_statement != NULL) {
    ViviCodeStatement *block = vivi_code_block_new ();
    vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), pre_statement);
    g_object_unref (pre_statement);
    vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), statement);
    g_object_unref (statement);
    statement = block;
  }

  data->level->loop_count--;

  return statement;
}

static gboolean
peek_if_statement (ParseData *data)
{
  return peek_token (data, TOKEN_IF);
}

static ViviCodeStatement *
parse_if_statement (ParseData *data)
{
  ViviCodeValue *condition;
  ViviCodeStatement *statement, *if_statement, *else_statement;

  parse_token (data, TOKEN_IF);
  parse_token (data, TOKEN_PARENTHESIS_LEFT);
  condition = parse_expression (data);
  parse_token (data, TOKEN_PARENTHESIS_RIGHT);
  if_statement = parse_statement (data);

  if (try_parse_token (data, TOKEN_ELSE)) {
    else_statement = parse_statement (data);
  } else {
    else_statement = NULL;
  }

  statement = vivi_code_if_new (condition);
  g_object_unref (condition);

  vivi_code_if_set_if (VIVI_CODE_IF (statement), if_statement);
  g_object_unref (if_statement);

  if (else_statement != NULL) {
    vivi_code_if_set_else (VIVI_CODE_IF (statement), else_statement);
    g_object_unref (else_statement);
  }

  return statement;
}

static gboolean
peek_expression_statement (ParseData *data)
{
  if (peek_token (data, TOKEN_BRACE_LEFT) || peek_token (data, TOKEN_FUNCTION))
    return FALSE;

  return peek_expression (data);
}

static ViviCodeStatement *
parse_expression_statement (ParseData *data)
{
  ViviCodeValue *value;

  if (peek_token (data, TOKEN_BRACE_LEFT) || peek_token (data, TOKEN_FUNCTION))
    vivi_parser_error_unexpected (data, ERROR_TOKEN_EXPRESSION_STATEMENT);

  value = parse_expression (data);

  parse_automatic_semicolon (data);

  return VIVI_CODE_STATEMENT (value);
}

static gboolean
peek_label_statement (ParseData *data)
{
  const ViviParserValue *value;

  if (!peek_token (data, TOKEN_IDENTIFIER))
    return FALSE;

  value = vivi_parser_scanner_get_value (data->scanner, 2);

  return value->token == TOKEN_COLON;
}

static ViviCodeStatement *
parse_label_statement (ParseData *data)
{
  ViviCodeLabel *label;
  const char *name;

  name = parse_identifier_value (data);

  label = vivi_parser_get_waiting_label (data, name);
  if (label != NULL) {
    vivi_parser_remove_waiting_label (data, label);
  } else {
    label = VIVI_CODE_LABEL (vivi_code_label_new (name));
  }

  parse_token (data, TOKEN_COLON);

  if (!vivi_parser_add_label (data, label))
    vivi_parser_error (data, "Same label name used twice");

  return VIVI_CODE_STATEMENT (label);
}

static gboolean
peek_empty_statement (ParseData *data)
{
  return peek_token (data, TOKEN_SEMICOLON);
}

static ViviCodeStatement *
parse_empty_statement (ParseData *data)
{
  parse_token (data, TOKEN_SEMICOLON);

  return vivi_compiler_empty_statement_new ();
}

static gboolean
peek_block (ParseData *data)
{
  return peek_token (data, TOKEN_BRACE_LEFT);
}

static ViviCodeStatement *
parse_block (ParseData *data)
{
  ViviCodeStatement *statement;

  parse_token (data, TOKEN_BRACE_LEFT);

  if (!try_parse_token (data, TOKEN_BRACE_RIGHT)) {
    statement = parse_statement_list (data, peek_statement, parse_statement,
	TOKEN_NONE);
    parse_token (data, TOKEN_BRACE_RIGHT);
  } else {
    statement = vivi_code_block_new ();;
  }

  return statement;
}

static gboolean
peek_variable_statement (ParseData *data)
{
  return peek_token (data, TOKEN_VAR);
}

static ViviCodeStatement *
parse_variable_statement (ParseData *data)
{
  ViviCodeStatement *statement;

  parse_token (data, TOKEN_VAR);
  statement = parse_statement_list (data, peek_variable_declaration,
      parse_variable_declaration, TOKEN_COMMA);
  parse_automatic_semicolon (data);

  return statement;
}

static const struct {
  PeekFunction peek;
  ParseStatementFunction parse;
} statement_functions[] = {
  { peek_asm_statement, parse_asm_statement },
  { peek_builtin_statement, parse_builtin_statement },
  { peek_block, parse_block },
  { peek_variable_statement, parse_variable_statement },
  { peek_empty_statement, parse_empty_statement },
  { peek_label_statement, parse_label_statement },
  { peek_expression_statement, parse_expression_statement },
  { peek_if_statement, parse_if_statement },
  { peek_iteration_statement, parse_iteration_statement },
  { peek_continue_statement, parse_continue_statement },
  { peek_break_statement, parse_break_statement },
  { peek_return_statement, parse_return_statement },
  //{ peek_with_statement, parse_with_statement },
  //{ peek_switch_statement, parse_switch_statement },
  { peek_throw_statement, parse_throw_statement },
  { peek_try_statement, parse_try_statement },
  { NULL, NULL }
};

static gboolean
peek_statement (ParseData *data)
{
  guint i;

  for (i = 0; statement_functions[i].peek != NULL; i++) {
    if (statement_functions[i].peek (data))
      return TRUE;
  }

  return FALSE;
}

static ViviCodeStatement *
parse_statement (ParseData *data)
{
  guint i;

  for (i = 0; statement_functions[i].peek != NULL; i++) {
    if (statement_functions[i].peek (data)) {
      return statement_functions[i].parse (data);
    }
  }

  vivi_parser_error_unexpected (data, ERROR_TOKEN_STATEMENT);
  return vivi_compiler_empty_statement_new ();
}

// function

static gboolean
peek_source_element (ParseData *data);

static ViviCodeStatement *
parse_source_element (ParseData *data);

static gboolean
peek_function_declaration (ParseData *data)
{
  return peek_token (data, TOKEN_FUNCTION);
}

static ViviCodeStatement *
parse_function_declaration (ParseData *data)
{
  ViviCodeFunctionDeclaration *function;
  ViviCodeStatement *body;

  parse_token (data, TOKEN_FUNCTION);

  function = VIVI_CODE_FUNCTION_DECLARATION (
    vivi_code_function_declaration_new (parse_identifier_value (data)));

  parse_token (data, TOKEN_PARENTHESIS_LEFT);

  if (peek_identifier (data)) {
    do {
      vivi_code_function_declaration_add_argument (function,
	  parse_identifier_value (data));
    } while (try_parse_token (data, TOKEN_COMMA));
    if (!try_parse_token (data, TOKEN_PARENTHESIS_RIGHT)) {
      vivi_parser_error_unexpected_or (data, TOKEN_PARENTHESIS_RIGHT,
	  TOKEN_COMMA, TOKEN_NONE);
    }
  } else {
    if (!try_parse_token (data, TOKEN_PARENTHESIS_RIGHT)) {
      vivi_parser_error_unexpected_or (data, TOKEN_PARENTHESIS_RIGHT,
	  TOKEN_IDENTIFIER, TOKEN_NONE);
    }
  }

  parse_token (data, TOKEN_BRACE_LEFT);

  vivi_parser_start_level (data);
  body = parse_statement_list (data, peek_source_element, parse_source_element,
      TOKEN_NONE);
  vivi_parser_end_level (data);

  parse_token (data, TOKEN_BRACE_RIGHT);

  if (body != NULL) {
    vivi_code_function_declaration_set_body (function, body);
    g_object_unref (body);
  }

  return VIVI_CODE_STATEMENT (function);
}

static gboolean
peek_function_expression (ParseData *data)
{
  return peek_token (data, TOKEN_FUNCTION);
}

static ViviCodeValue *
parse_function_expression (ParseData *data)
{
  ViviCodeFunction *function;
  ViviCodeStatement *body;

  parse_token (data, TOKEN_FUNCTION);
  parse_token (data, TOKEN_PARENTHESIS_LEFT);

  function = VIVI_CODE_FUNCTION (vivi_code_function_new ());

  if (peek_identifier (data)) {
    do {
      vivi_code_function_add_argument (function,
	  parse_identifier_value (data));
    } while (try_parse_token (data, TOKEN_COMMA));
    if (!try_parse_token (data, TOKEN_PARENTHESIS_RIGHT)) {
      vivi_parser_error_unexpected_or (data, TOKEN_PARENTHESIS_RIGHT,
	  TOKEN_COMMA, TOKEN_NONE);
    }
  } else {
    if (!try_parse_token (data, TOKEN_PARENTHESIS_RIGHT)) {
      vivi_parser_error_unexpected_or (data, TOKEN_PARENTHESIS_RIGHT,
	  TOKEN_IDENTIFIER, TOKEN_NONE);
    }
  }

  parse_token (data, TOKEN_BRACE_LEFT);

  vivi_parser_start_level (data);
  body = parse_statement_list (data, peek_source_element, parse_source_element,
      TOKEN_NONE);
  vivi_parser_end_level (data);

  parse_token (data, TOKEN_BRACE_RIGHT);

  if (body != NULL) {
    vivi_code_function_set_body (function, body);
    g_object_unref (body);
  }

  return VIVI_CODE_VALUE (function);
}

// top

static gboolean
peek_source_element (ParseData *data)
{
  return (peek_function_declaration (data) || peek_statement (data));
}

static ViviCodeStatement *
parse_source_element (ParseData *data)
{
  if (peek_function_declaration (data)) {
    return parse_function_declaration (data);
  } else if (peek_statement (data)) {
    return parse_statement (data);
  } else {
    vivi_parser_error_unexpected_or (data, ERROR_TOKEN_FUNCTION_DECLARATION,
	ERROR_TOKEN_STATEMENT, TOKEN_NONE);
    return vivi_compiler_empty_statement_new ();
  }
}

G_GNUC_UNUSED static gboolean
peek_program (ParseData *data)
{
  return peek_source_element (data);
}

static ViviCodeStatement *
parse_program (ParseData *data)
{
  ViviCodeStatement *statement;

  // empty file
  if (peek_token (data, TOKEN_NONE))
    return vivi_code_block_new ();

  g_assert (data->level == NULL);
  vivi_parser_start_level (data);

  statement = parse_statement_list (data, peek_source_element,
      parse_source_element, TOKEN_NONE);
  if (!try_parse_token (data, TOKEN_NONE))
    vivi_parser_error_unexpected (data, ERROR_TOKEN_STATEMENT);

  vivi_parser_end_level (data);
  g_assert (data->level == NULL);

  return statement;
}

// parsing

static ViviCodeStatement *
parse_statement_list (ParseData *data, PeekFunction peek,
    ParseStatementFunction parse, guint separator)
{
  ViviCodeStatement *statement;
  ViviCodeStatement *block;

  g_assert (data != NULL);
  g_assert (peek != NULL);
  g_assert (parse != NULL);

  block = vivi_code_block_new ();

  do {
    statement = parse (data);
    vivi_code_block_add_statement (VIVI_CODE_BLOCK (block), statement);
    g_object_unref (statement);
  } while ((separator == TOKEN_NONE || try_parse_token (data, separator)) &&
      peek (data));

  return block;
}

static void
parse_value_list (ParseData *data, PeekFunction peek, ParseValueFunction parse,
    ViviCodeValue ***list, guint separator)
{
  GPtrArray *array;
  ViviCodeValue *value;

  g_assert (data != NULL);
  g_assert (peek != NULL);
  g_assert (parse != NULL);
  g_assert (list != NULL);

  array = g_ptr_array_new ();

  do {
    value = parse (data);
    g_ptr_array_add (array, value);
  } while ((separator == TOKEN_NONE || try_parse_token (data, separator)) &&
      peek (data));

  g_ptr_array_add (array, NULL);

  *list = (ViviCodeValue **)g_ptr_array_free (array, FALSE);
}

static ViviCodeStatement *
vivi_do_parse (const guint8 *data, gsize length)
{
  ParseData parse;
  ViviCodeStatement *statement;

  parse.scanner = vivi_parser_scanner_new (data, length);
  vivi_parser_scanner_set_error_handler (parse.scanner,
      vivi_parser_error_handler, &parse);
  parse.levels = NULL;
  parse.level = NULL;
  parse.error_count = 0;

  statement = parse_program (&parse);
  if (parse.error_count != 0) {
    g_object_unref (statement);
    statement = NULL;
  }

  g_object_unref (parse.scanner);

  return statement;
}

// public

ViviCodeStatement *
vivi_parse_buffer (SwfdecBuffer *buffer)
{
  g_return_val_if_fail (buffer != NULL, NULL);

  return vivi_do_parse (buffer->data, buffer->length);
}

ViviCodeStatement *
vivi_parse_string (const char *string)
{
  g_return_val_if_fail (string != NULL, NULL);

  return vivi_do_parse ((const guint8 *) string, strlen (string));
}
