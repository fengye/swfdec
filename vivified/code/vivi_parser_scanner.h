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

#ifndef _VIVI_PARSER_SCANNER_H_
#define _VIVI_PARSER_SCANNER_H_

#include <stdio.h>
#include <swfdec/swfdec.h>
#include <swfdec/swfdec_ringbuffer.h>

G_BEGIN_DECLS


typedef enum {
  // special
  TOKEN_NONE = 0,
  TOKEN_UNKNOWN,

  // comparision
  TOKEN_BRACE_LEFT,
  TOKEN_BRACE_RIGHT,
  TOKEN_BRACKET_LEFT,
  TOKEN_BRACKET_RIGHT,
  TOKEN_PARENTHESIS_LEFT,
  TOKEN_PARENTHESIS_RIGHT,

  // punctuation
  TOKEN_DOT,
  TOKEN_SEMICOLON,
  TOKEN_COMMA,

  // comparision
  TOKEN_LESS_THAN,
  TOKEN_GREATER_THAN,
  TOKEN_LESS_THAN_OR_EQUAL,
  TOKEN_GREATER_THAN_OR_EQUAL,

  // equality
  TOKEN_EQUAL,
  TOKEN_NOT_EQUAL,
  TOKEN_STRICT_EQUAL,
  TOKEN_NOT_STRICT_EQUAL,

  // operator
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_MULTIPLY,
  TOKEN_DIVIDE,
  TOKEN_REMAINDER,

  // shift
  TOKEN_SHIFT_LEFT,
  TOKEN_SHIFT_RIGHT,
  TOKEN_SHIFT_RIGHT_UNSIGNED,

  // bitwise
  TOKEN_BITWISE_AND,
  TOKEN_BITWISE_OR,
  TOKEN_BITWISE_XOR,
  
  // unary/postfix
  TOKEN_LOGICAL_NOT,
  TOKEN_BITWISE_NOT,
  TOKEN_INCREASE,
  TOKEN_DESCREASE,

  // conditional
  TOKEN_QUESTION_MARK,
  TOKEN_COLON,

  // logical
  TOKEN_LOGICAL_AND,
  TOKEN_LOGICAL_OR,

  // assign
  TOKEN_ASSIGN,
  TOKEN_ASSIGN_MULTIPLY,
  TOKEN_ASSIGN_DIVIDE,
  TOKEN_ASSIGN_REMAINDER,
  TOKEN_ASSIGN_ADD,
  TOKEN_ASSIGN_MINUS,
  TOKEN_ASSIGN_SHIFT_LEFT,
  TOKEN_ASSIGN_SHIFT_RIGHT,
  TOKEN_ASSIGN_SHIFT_RIGHT_ZERO,
  TOKEN_ASSIGN_BITWISE_AND,
  TOKEN_ASSIGN_BITWISE_XOR,
  TOKEN_ASSIGN_BITWISE_OR,

  // values
  TOKEN_NULL,
  TOKEN_BOOLEAN,
  TOKEN_NUMBER,
  TOKEN_STRING,
  TOKEN_IDENTIFIER,

  // keywords
  TOKEN_BREAK,
  TOKEN_CASE,
  TOKEN_CATCH,
  TOKEN_CONTINUE,
  TOKEN_DEFAULT,
  TOKEN_DELETE,
  TOKEN_DO,
  TOKEN_ELSE,
  TOKEN_FINALLY,
  TOKEN_FOR,
  TOKEN_FUNCTION,
  TOKEN_IF,
  TOKEN_IN,
  TOKEN_INSTANCEOF,
  TOKEN_NEW,
  TOKEN_RETURN,
  TOKEN_SWITCH,
  TOKEN_THIS,
  TOKEN_THROW,
  TOKEN_TRY,
  TOKEN_TYPEOF,
  TOKEN_VAR,
  TOKEN_VOID,
  TOKEN_WHILE,
  TOKEN_WITH,

  // reserved keywords
  TOKEN_RESERVED_KEYWORD,

  // ActionScript specific
  TOKEN_EXTENDS,
  TOKEN_IMPLEMENTS,
  TOKEN_UNDEFINED,

  TOKEN_LAST
} ViviParserScannerToken;

typedef struct {
  ViviParserScannerToken	token;
  union {
    gboolean	  		v_boolean;
    double	  		v_number;
    GString *			v_string;
    char *			v_identifier;
  }				value;
  gboolean			line_terminator;
  guint				line_number;
  guint				column;		/* FIXME: not supported by flex */
  gsize				position;	/* FIXME: neither supported by flex nor us */
} ViviParserValue;

typedef void (*ViviParserScannerFunction) (const char *text, gpointer user_data);

typedef struct _ViviParserScanner ViviParserScanner;
typedef struct _ViviParserScannerClass ViviParserScannerClass;

#define VIVI_TYPE_PARSER_SCANNER                    (vivi_parser_scanner_get_type())
#define VIVI_IS_PARSER_SCANNER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_PARSER_SCANNER))
#define VIVI_IS_PARSER_SCANNER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_PARSER_SCANNER))
#define VIVI_PARSER_SCANNER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_PARSER_SCANNER, ViviParserScanner))
#define VIVI_PARSER_SCANNER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_PARSER_SCANNER, ViviParserScannerClass))
#define VIVI_PARSER_SCANNER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_PARSER_SCANNER, ViviParserScannerClass))

struct _ViviParserScanner
{
  GObject			object;

  ViviParserScannerFunction	error_handler;
  gpointer			error_handler_data;

  gpointer			scanner;
  SwfdecRingBuffer *		values;
};

struct _ViviParserScannerClass
{
  GObjectClass		object_class;
};

GType				vivi_parser_scanner_get_type   	(void);

ViviParserScanner *		vivi_parser_scanner_new		(const guint8 *		data,
								 gsize			len);
void				vivi_parser_scanner_set_error_handler (ViviParserScanner *	scanner,
								 ViviParserScannerFunction	error_handler,
								 gpointer			user_data);

ViviParserScannerToken		vivi_parser_scanner_get_next_token	(ViviParserScanner *	scanner);
ViviParserScannerToken		vivi_parser_scanner_peek_next_token	(ViviParserScanner *	scanner);

const ViviParserValue *		vivi_parser_scanner_get_value 	(ViviParserScanner *	scanner,
								 guint			i);

void				vivi_parser_scanner_error	(ViviParserScanner *	scanner, 
								 guint			line,
								 guint			column,
								 const char *		format,
								 ...) G_GNUC_PRINTF (4, 5);
void				vivi_parser_scanner_errorv	(ViviParserScanner *	scanner, 
								 guint			line,
								 guint			column,
								 const char *		format,
								 va_list		args);

const char *			vivi_parser_scanner_token_name	(ViviParserScannerToken token);

G_END_DECLS
#endif
