#pragma once

//     STB_TEXTEDIT_CHARTYPE             the character type
#define STB_TEXTEDIT_CHARTYPE char

//     STB_TEXTEDIT_POSITIONTYPE         small type that is a valid cursor position
#define STB_TEXTEDIT_POSITIONTYPE int

//     STB_TEXTEDIT_UNDOSTATECOUNT       the number of undo states to allow
#define STB_TEXTEDIT_UNDOSTATECOUNT 99

//     STB_TEXTEDIT_UNDOCHARCOUNT        the number of characters to store in the undo buffer
#define STB_TEXTEDIT_UNDOCHARCOUNT 999

//    STB_TEXTEDIT_STRING                the type of object representing a string being edited,
//                                       typically this is a wrapper object with other data you need
#define STB_TEXTEDIT_STRING StbString

//    STB_TEXTEDIT_STRINGLEN(obj)        the length of the string (ideally O(1))
#define STB_TEXTEDIT_STRINGLEN(obj) ((int)(obj)->data.size())

//    STB_TEXTEDIT_LAYOUTROW(&r,obj,n)   returns the results of laying out a line of characters
//                                       starting from character #n (see discussion below)
#define STB_TEXTEDIT_LAYOUTROW(r, obj, n) RN_stb_layout_row(r, obj, n)

//    STB_TEXTEDIT_GETWIDTH(obj,n,i)     returns the pixel delta from the xpos of the i'th character
//                                       to the xpos of the i+1'th char for a line of characters
//                                       starting at character #n (i.e. accounts for kerning
//                                       with previous char)
#define STB_TEXTEDIT_GETWIDTH(obj, n, i) RN_stb_get_char_width(obj, n, i)

//    STB_TEXTEDIT_KEYTOTEXT(k)          maps a keyboard input to an insertable character
//                                       (return type is int, -1 means not valid to insert)
#define STB_TEXTEDIT_KEYTOTEXT(k) RN_stb_key_to_char(k)

//    STB_TEXTEDIT_GETCHAR(obj,i)        returns the i'th character of obj, 0-based
#define STB_TEXTEDIT_GETCHAR(obj, i) RN_stb_get_char(obj, i)

//    STB_TEXTEDIT_NEWLINE               the character returned by _GETCHAR() we recognize
//                                       as manually wordwrapping for end-of-line positioning
#define STB_TEXTEDIT_NEWLINE '\n'

//
//    STB_TEXTEDIT_DELETECHARS(obj,i,n)      delete n characters starting at i
#define STB_TEXTEDIT_DELETECHARS(obj, i, n) RN_stb_delete_chars(obj, i, n)

//    STB_TEXTEDIT_INSERTCHARS(obj,i,c*,n)   insert n characters at i (pointed to by STB_TEXTEDIT_CHARTYPE*)
#define STB_TEXTEDIT_INSERTCHARS(obj, i, c, n) RN_stb_insert_chars(obj, i, c, n)

// Custom bitflags for modifier keys
#define STB_TEXTEDIT_K_CTRL      512
#define STB_TEXTEDIT_K_SHIFT     1024

#define STB_TEXTEDIT_K_LEFT      KEY_LEFT
#define STB_TEXTEDIT_K_RIGHT     KEY_RIGHT
#define STB_TEXTEDIT_K_UP        KEY_UP
#define STB_TEXTEDIT_K_DOWN      KEY_DOWN
#define STB_TEXTEDIT_K_DELETE    KEY_DELETE
#define STB_TEXTEDIT_K_BACKSPACE KEY_BACKSPACE
#define STB_TEXTEDIT_K_LINESTART KEY_HOME
#define STB_TEXTEDIT_K_LINEEND   KEY_END
#define STB_TEXTEDIT_K_TEXTSTART (STB_TEXTEDIT_K_CTRL | KEY_HOME)
#define STB_TEXTEDIT_K_TEXTEND   (STB_TEXTEDIT_K_CTRL | KEY_END)
#define STB_TEXTEDIT_K_PGUP      KEY_PAGE_UP
#define STB_TEXTEDIT_K_PGDOWN    KEY_PAGE_DOWN
#define STB_TEXTEDIT_K_UNDO      (STB_TEXTEDIT_K_CTRL | KEY_Z)
#define STB_TEXTEDIT_K_REDO      (STB_TEXTEDIT_K_CTRL | STB_TEXTEDIT_K_SHIFT | KEY_Z)

////////////////////////////////////////
// Optional
#define STB_TEXTEDIT_K_INSERT    KEY_INSERT

//    STB_TEXTEDIT_IS_SPACE(ch)          true if character is whitespace (e.g. 'isspace'),
//                                       required for default WORDLEFT/WORDRIGHT handlers
#define STB_TEXTEDIT_IS_SPACE(ch) RN_stb_is_space(ch)

//    STB_TEXTEDIT_MOVEWORDLEFT(obj,i)   custom handler for WORDLEFT, returns index to move cursor to
// default

//    STB_TEXTEDIT_MOVEWORDRIGHT(obj,i)  custom handler for WORDRIGHT, returns index to move cursor to
// default

//    STB_TEXTEDIT_K_WORDLEFT            keyboard input to move cursor left one word // e.g. ctrl-LEFT
#define STB_TEXTEDIT_K_WORDLEFT  (STB_TEXTEDIT_K_CTRL | KEY_LEFT)

//    STB_TEXTEDIT_K_WORDRIGHT           keyboard input to move cursor right one word // e.g. ctrl-RIGHT
#define STB_TEXTEDIT_K_WORDRIGHT (STB_TEXTEDIT_K_CTRL | KEY_RIGHT)

//    STB_TEXTEDIT_K_LINESTART2          secondary keyboard input to move cursor to start of line
//    STB_TEXTEDIT_K_LINEEND2            secondary keyboard input to move cursor to end of line
//    STB_TEXTEDIT_K_TEXTSTART2          secondary keyboard input to move cursor to start of text
//    STB_TEXTEDIT_K_TEXTEND2            secondary keyboard input to move cursor to end of text
////////////////////////////////////////
#include "stb_textedit.h"
