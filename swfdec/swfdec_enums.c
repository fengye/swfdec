
/* Generated data (by glib-mkenums) */

#include "swfdec.h"

/* enumerations from "swfdec_as_context.h" */
GType
swfdec_as_context_state_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { SWFDEC_AS_CONTEXT_NEW, "SWFDEC_AS_CONTEXT_NEW", "new" },
      { SWFDEC_AS_CONTEXT_RUNNING, "SWFDEC_AS_CONTEXT_RUNNING", "running" },
      { SWFDEC_AS_CONTEXT_INTERRUPTED, "SWFDEC_AS_CONTEXT_INTERRUPTED", "interrupted" },
      { SWFDEC_AS_CONTEXT_ABORTED, "SWFDEC_AS_CONTEXT_ABORTED", "aborted" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static (g_intern_static_string ("SwfdecAsContextState"), values);
  }
  return etype;
}

/* enumerations from "swfdec_as_object.h" */
GType
swfdec_as_variable_flag_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GFlagsValue values[] = {
      { SWFDEC_AS_VARIABLE_HIDDEN, "SWFDEC_AS_VARIABLE_HIDDEN", "hidden" },
      { SWFDEC_AS_VARIABLE_PERMANENT, "SWFDEC_AS_VARIABLE_PERMANENT", "permanent" },
      { SWFDEC_AS_VARIABLE_CONSTANT, "SWFDEC_AS_VARIABLE_CONSTANT", "constant" },
      { SWFDEC_AS_VARIABLE_VERSION_6_UP, "SWFDEC_AS_VARIABLE_VERSION_6_UP", "version-6-up" },
      { SWFDEC_AS_VARIABLE_VERSION_NOT_6, "SWFDEC_AS_VARIABLE_VERSION_NOT_6", "version-not-6" },
      { SWFDEC_AS_VARIABLE_VERSION_7_UP, "SWFDEC_AS_VARIABLE_VERSION_7_UP", "version-7-up" },
      { SWFDEC_AS_VARIABLE_VERSION_8_UP, "SWFDEC_AS_VARIABLE_VERSION_8_UP", "version-8-up" },
      { SWFDEC_AS_VARIABLE_VERSION_9_UP, "SWFDEC_AS_VARIABLE_VERSION_9_UP", "version-9-up" },
      { 0, NULL, NULL }
    };
    etype = g_flags_register_static (g_intern_static_string ("SwfdecAsVariableFlag"), values);
  }
  return etype;
}
GType
swfdec_as_delete_return_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { SWFDEC_AS_DELETE_NOT_FOUND, "SWFDEC_AS_DELETE_NOT_FOUND", "not-found" },
      { SWFDEC_AS_DELETE_DELETED, "SWFDEC_AS_DELETE_DELETED", "deleted" },
      { SWFDEC_AS_DELETE_NOT_DELETED, "SWFDEC_AS_DELETE_NOT_DELETED", "not-deleted" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static (g_intern_static_string ("SwfdecAsDeleteReturn"), values);
  }
  return etype;
}

/* enumerations from "swfdec_as_types.h" */
GType
swfdec_as_value_type_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { SWFDEC_AS_TYPE_UNDEFINED, "SWFDEC_AS_TYPE_UNDEFINED", "undefined" },
      { SWFDEC_AS_TYPE_BOOLEAN, "SWFDEC_AS_TYPE_BOOLEAN", "boolean" },
      { SWFDEC_AS_TYPE_INT, "SWFDEC_AS_TYPE_INT", "int" },
      { SWFDEC_AS_TYPE_NUMBER, "SWFDEC_AS_TYPE_NUMBER", "number" },
      { SWFDEC_AS_TYPE_STRING, "SWFDEC_AS_TYPE_STRING", "string" },
      { SWFDEC_AS_TYPE_NULL, "SWFDEC_AS_TYPE_NULL", "null" },
      { SWFDEC_AS_TYPE_OBJECT, "SWFDEC_AS_TYPE_OBJECT", "object" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static (g_intern_static_string ("SwfdecAsValueType"), values);
  }
  return etype;
}

/* enumerations from "swfdec_keys.h" */
GType
swfdec_key_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { SWFDEC_KEY_LEFT_MOUSE, "SWFDEC_KEY_LEFT_MOUSE", "left-mouse" },
      { SWFDEC_KEY_RIGHT_MOUSE, "SWFDEC_KEY_RIGHT_MOUSE", "right-mouse" },
      { SWFDEC_KEY_MIDDLE_MOUSE, "SWFDEC_KEY_MIDDLE_MOUSE", "middle-mouse" },
      { SWFDEC_KEY_BACKSPACE, "SWFDEC_KEY_BACKSPACE", "backspace" },
      { SWFDEC_KEY_TAB, "SWFDEC_KEY_TAB", "tab" },
      { SWFDEC_KEY_CLEAR, "SWFDEC_KEY_CLEAR", "clear" },
      { SWFDEC_KEY_ENTER, "SWFDEC_KEY_ENTER", "enter" },
      { SWFDEC_KEY_SHIFT, "SWFDEC_KEY_SHIFT", "shift" },
      { SWFDEC_KEY_CONTROL, "SWFDEC_KEY_CONTROL", "control" },
      { SWFDEC_KEY_ALT, "SWFDEC_KEY_ALT", "alt" },
      { SWFDEC_KEY_CAPS_LOCK, "SWFDEC_KEY_CAPS_LOCK", "caps-lock" },
      { SWFDEC_KEY_ESCAPE, "SWFDEC_KEY_ESCAPE", "escape" },
      { SWFDEC_KEY_SPACE, "SWFDEC_KEY_SPACE", "space" },
      { SWFDEC_KEY_PAGE_UP, "SWFDEC_KEY_PAGE_UP", "page-up" },
      { SWFDEC_KEY_PAGE_DOWN, "SWFDEC_KEY_PAGE_DOWN", "page-down" },
      { SWFDEC_KEY_END, "SWFDEC_KEY_END", "end" },
      { SWFDEC_KEY_HOME, "SWFDEC_KEY_HOME", "home" },
      { SWFDEC_KEY_LEFT, "SWFDEC_KEY_LEFT", "left" },
      { SWFDEC_KEY_UP, "SWFDEC_KEY_UP", "up" },
      { SWFDEC_KEY_RIGHT, "SWFDEC_KEY_RIGHT", "right" },
      { SWFDEC_KEY_DOWN, "SWFDEC_KEY_DOWN", "down" },
      { SWFDEC_KEY_INSERT, "SWFDEC_KEY_INSERT", "insert" },
      { SWFDEC_KEY_DELETE, "SWFDEC_KEY_DELETE", "delete" },
      { SWFDEC_KEY_HELP, "SWFDEC_KEY_HELP", "help" },
      { SWFDEC_KEY_0, "SWFDEC_KEY_0", "0" },
      { SWFDEC_KEY_1, "SWFDEC_KEY_1", "1" },
      { SWFDEC_KEY_2, "SWFDEC_KEY_2", "2" },
      { SWFDEC_KEY_3, "SWFDEC_KEY_3", "3" },
      { SWFDEC_KEY_4, "SWFDEC_KEY_4", "4" },
      { SWFDEC_KEY_5, "SWFDEC_KEY_5", "5" },
      { SWFDEC_KEY_6, "SWFDEC_KEY_6", "6" },
      { SWFDEC_KEY_7, "SWFDEC_KEY_7", "7" },
      { SWFDEC_KEY_8, "SWFDEC_KEY_8", "8" },
      { SWFDEC_KEY_9, "SWFDEC_KEY_9", "9" },
      { SWFDEC_KEY_A, "SWFDEC_KEY_A", "a" },
      { SWFDEC_KEY_B, "SWFDEC_KEY_B", "b" },
      { SWFDEC_KEY_C, "SWFDEC_KEY_C", "c" },
      { SWFDEC_KEY_D, "SWFDEC_KEY_D", "d" },
      { SWFDEC_KEY_E, "SWFDEC_KEY_E", "e" },
      { SWFDEC_KEY_F, "SWFDEC_KEY_F", "f" },
      { SWFDEC_KEY_G, "SWFDEC_KEY_G", "g" },
      { SWFDEC_KEY_H, "SWFDEC_KEY_H", "h" },
      { SWFDEC_KEY_I, "SWFDEC_KEY_I", "i" },
      { SWFDEC_KEY_J, "SWFDEC_KEY_J", "j" },
      { SWFDEC_KEY_K, "SWFDEC_KEY_K", "k" },
      { SWFDEC_KEY_L, "SWFDEC_KEY_L", "l" },
      { SWFDEC_KEY_M, "SWFDEC_KEY_M", "m" },
      { SWFDEC_KEY_N, "SWFDEC_KEY_N", "n" },
      { SWFDEC_KEY_O, "SWFDEC_KEY_O", "o" },
      { SWFDEC_KEY_P, "SWFDEC_KEY_P", "p" },
      { SWFDEC_KEY_Q, "SWFDEC_KEY_Q", "q" },
      { SWFDEC_KEY_R, "SWFDEC_KEY_R", "r" },
      { SWFDEC_KEY_S, "SWFDEC_KEY_S", "s" },
      { SWFDEC_KEY_T, "SWFDEC_KEY_T", "t" },
      { SWFDEC_KEY_U, "SWFDEC_KEY_U", "u" },
      { SWFDEC_KEY_V, "SWFDEC_KEY_V", "v" },
      { SWFDEC_KEY_W, "SWFDEC_KEY_W", "w" },
      { SWFDEC_KEY_X, "SWFDEC_KEY_X", "x" },
      { SWFDEC_KEY_Y, "SWFDEC_KEY_Y", "y" },
      { SWFDEC_KEY_Z, "SWFDEC_KEY_Z", "z" },
      { SWFDEC_KEY_NUMPAD_0, "SWFDEC_KEY_NUMPAD_0", "numpad-0" },
      { SWFDEC_KEY_NUMPAD_1, "SWFDEC_KEY_NUMPAD_1", "numpad-1" },
      { SWFDEC_KEY_NUMPAD_2, "SWFDEC_KEY_NUMPAD_2", "numpad-2" },
      { SWFDEC_KEY_NUMPAD_3, "SWFDEC_KEY_NUMPAD_3", "numpad-3" },
      { SWFDEC_KEY_NUMPAD_4, "SWFDEC_KEY_NUMPAD_4", "numpad-4" },
      { SWFDEC_KEY_NUMPAD_5, "SWFDEC_KEY_NUMPAD_5", "numpad-5" },
      { SWFDEC_KEY_NUMPAD_6, "SWFDEC_KEY_NUMPAD_6", "numpad-6" },
      { SWFDEC_KEY_NUMPAD_7, "SWFDEC_KEY_NUMPAD_7", "numpad-7" },
      { SWFDEC_KEY_NUMPAD_8, "SWFDEC_KEY_NUMPAD_8", "numpad-8" },
      { SWFDEC_KEY_NUMPAD_9, "SWFDEC_KEY_NUMPAD_9", "numpad-9" },
      { SWFDEC_KEY_NUMPAD_MULTIPLY, "SWFDEC_KEY_NUMPAD_MULTIPLY", "numpad-multiply" },
      { SWFDEC_KEY_NUMPAD_ADD, "SWFDEC_KEY_NUMPAD_ADD", "numpad-add" },
      { SWFDEC_KEY_NUMPAD_SUBTRACT, "SWFDEC_KEY_NUMPAD_SUBTRACT", "numpad-subtract" },
      { SWFDEC_KEY_NUMPAD_DECIMAL, "SWFDEC_KEY_NUMPAD_DECIMAL", "numpad-decimal" },
      { SWFDEC_KEY_NUMPAD_DIVIDE, "SWFDEC_KEY_NUMPAD_DIVIDE", "numpad-divide" },
      { SWFDEC_KEY_F1, "SWFDEC_KEY_F1", "f1" },
      { SWFDEC_KEY_F2, "SWFDEC_KEY_F2", "f2" },
      { SWFDEC_KEY_F3, "SWFDEC_KEY_F3", "f3" },
      { SWFDEC_KEY_F4, "SWFDEC_KEY_F4", "f4" },
      { SWFDEC_KEY_F5, "SWFDEC_KEY_F5", "f5" },
      { SWFDEC_KEY_F6, "SWFDEC_KEY_F6", "f6" },
      { SWFDEC_KEY_F7, "SWFDEC_KEY_F7", "f7" },
      { SWFDEC_KEY_F8, "SWFDEC_KEY_F8", "f8" },
      { SWFDEC_KEY_F9, "SWFDEC_KEY_F9", "f9" },
      { SWFDEC_KEY_F10, "SWFDEC_KEY_F10", "f10" },
      { SWFDEC_KEY_F11, "SWFDEC_KEY_F11", "f11" },
      { SWFDEC_KEY_F12, "SWFDEC_KEY_F12", "f12" },
      { SWFDEC_KEY_F13, "SWFDEC_KEY_F13", "f13" },
      { SWFDEC_KEY_F14, "SWFDEC_KEY_F14", "f14" },
      { SWFDEC_KEY_F15, "SWFDEC_KEY_F15", "f15" },
      { SWFDEC_KEY_NUM_LOCK, "SWFDEC_KEY_NUM_LOCK", "num-lock" },
      { SWFDEC_KEY_SCROLL_LOCK, "SWFDEC_KEY_SCROLL_LOCK", "scroll-lock" },
      { SWFDEC_KEY_SEMICOLON, "SWFDEC_KEY_SEMICOLON", "semicolon" },
      { SWFDEC_KEY_EQUAL, "SWFDEC_KEY_EQUAL", "equal" },
      { SWFDEC_KEY_COMMA, "SWFDEC_KEY_COMMA", "comma" },
      { SWFDEC_KEY_MINUS, "SWFDEC_KEY_MINUS", "minus" },
      { SWFDEC_KEY_DOT, "SWFDEC_KEY_DOT", "dot" },
      { SWFDEC_KEY_SLASH, "SWFDEC_KEY_SLASH", "slash" },
      { SWFDEC_KEY_GRAVE, "SWFDEC_KEY_GRAVE", "grave" },
      { SWFDEC_KEY_LEFT_BRACKET, "SWFDEC_KEY_LEFT_BRACKET", "left-bracket" },
      { SWFDEC_KEY_BACKSLASH, "SWFDEC_KEY_BACKSLASH", "backslash" },
      { SWFDEC_KEY_RIGHT_BRACKET, "SWFDEC_KEY_RIGHT_BRACKET", "right-bracket" },
      { SWFDEC_KEY_APOSTROPHE, "SWFDEC_KEY_APOSTROPHE", "apostrophe" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static (g_intern_static_string ("SwfdecKey"), values);
  }
  return etype;
}

/* enumerations from "swfdec_loader.h" */
GType
swfdec_loader_data_type_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { SWFDEC_LOADER_DATA_UNKNOWN, "SWFDEC_LOADER_DATA_UNKNOWN", "unknown" },
      { SWFDEC_LOADER_DATA_SWF, "SWFDEC_LOADER_DATA_SWF", "swf" },
      { SWFDEC_LOADER_DATA_FLV, "SWFDEC_LOADER_DATA_FLV", "flv" },
      { SWFDEC_LOADER_DATA_XML, "SWFDEC_LOADER_DATA_XML", "xml" },
      { SWFDEC_LOADER_DATA_TEXT, "SWFDEC_LOADER_DATA_TEXT", "text" },
      { SWFDEC_LOADER_DATA_JPEG, "SWFDEC_LOADER_DATA_JPEG", "jpeg" },
      { SWFDEC_LOADER_DATA_PNG, "SWFDEC_LOADER_DATA_PNG", "png" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static (g_intern_static_string ("SwfdecLoaderDataType"), values);
  }
  return etype;
}

/* enumerations from "swfdec_player.h" */
GType
swfdec_mouse_cursor_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { SWFDEC_MOUSE_CURSOR_NORMAL, "SWFDEC_MOUSE_CURSOR_NORMAL", "normal" },
      { SWFDEC_MOUSE_CURSOR_NONE, "SWFDEC_MOUSE_CURSOR_NONE", "none" },
      { SWFDEC_MOUSE_CURSOR_TEXT, "SWFDEC_MOUSE_CURSOR_TEXT", "text" },
      { SWFDEC_MOUSE_CURSOR_CLICK, "SWFDEC_MOUSE_CURSOR_CLICK", "click" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static (g_intern_static_string ("SwfdecMouseCursor"), values);
  }
  return etype;
}
GType
swfdec_alignment_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { SWFDEC_ALIGNMENT_TOP_LEFT, "SWFDEC_ALIGNMENT_TOP_LEFT", "top-left" },
      { SWFDEC_ALIGNMENT_TOP, "SWFDEC_ALIGNMENT_TOP", "top" },
      { SWFDEC_ALIGNMENT_TOP_RIGHT, "SWFDEC_ALIGNMENT_TOP_RIGHT", "top-right" },
      { SWFDEC_ALIGNMENT_LEFT, "SWFDEC_ALIGNMENT_LEFT", "left" },
      { SWFDEC_ALIGNMENT_CENTER, "SWFDEC_ALIGNMENT_CENTER", "center" },
      { SWFDEC_ALIGNMENT_RIGHT, "SWFDEC_ALIGNMENT_RIGHT", "right" },
      { SWFDEC_ALIGNMENT_BOTTOM_LEFT, "SWFDEC_ALIGNMENT_BOTTOM_LEFT", "bottom-left" },
      { SWFDEC_ALIGNMENT_BOTTOM, "SWFDEC_ALIGNMENT_BOTTOM", "bottom" },
      { SWFDEC_ALIGNMENT_BOTTOM_RIGHT, "SWFDEC_ALIGNMENT_BOTTOM_RIGHT", "bottom-right" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static (g_intern_static_string ("SwfdecAlignment"), values);
  }
  return etype;
}
GType
swfdec_scale_mode_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { SWFDEC_SCALE_SHOW_ALL, "SWFDEC_SCALE_SHOW_ALL", "show-all" },
      { SWFDEC_SCALE_NO_BORDER, "SWFDEC_SCALE_NO_BORDER", "no-border" },
      { SWFDEC_SCALE_EXACT_FIT, "SWFDEC_SCALE_EXACT_FIT", "exact-fit" },
      { SWFDEC_SCALE_NONE, "SWFDEC_SCALE_NONE", "none" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static (g_intern_static_string ("SwfdecScaleMode"), values);
  }
  return etype;
}

/* Generated data ends here */

