#define ESC        0x1B
#define NONE(k)    { .str = { k }, .code = 0 }
#define KEY(k)     { .str = { '\0' }, .code = KEY_##k }
#define CONTROL(k) NONE((k)&0x1F)
#define META(k)    { .str = { ESC, (k) }, .code = 0 }
#define BACKSPACE(func, name, arg) \
	{ { KEY(BACKSPACE) }, (func), { .name = (arg) } }, \
	{ { CONTROL('H') },   (func), { .name = (arg) } }, \
	{ { NONE(127) },      (func), { .name = (arg) } }, \
	{ { CONTROL('B') },   (func), { .name = (arg) } }

static void switchmode(const Arg *arg);

enum {
	VIS_MODE_BASIC,
	VIS_MODE_MOVE,
	VIS_MODE_TEXTOBJ,
	VIS_MODE_OPERATOR,
	VIS_MODE_NORMAL,
	VIS_MODE_VISUAL,
	VIS_MODE_INSERT,
	VIS_MODE_REPLACE,
};

enum {
	OP_DELETE,
	OP_CHANGE,
	OP_YANK,
};

void op_delete(OperatorContext *c) {
	if (c->range.start == (size_t)-1)
		return;
	size_t len = c->range.end - c->range.start;
	vis_delete(vis, c->range.start, len);
	if (c->pos > c->range.start)
		window_cursor_to(vis->win->win, c->range.start);
	vis_draw(vis);
}

void op_change(OperatorContext *c) {
	op_delete(c);
	switchmode(&(const Arg){ .i = VIS_MODE_INSERT });
}

void op_yank(OperatorContext *c) {}

static Operator *ops[] = {
	[OP_DELETE] = op_delete,
	[OP_CHANGE] = op_change,
	[OP_YANK] = op_yank,
};

enum {
	MOVE_CHAR_PREV,
	MOVE_CHAR_NEXT,
	MOVE_LINE_UP,
	MOVE_LINE_DOWN,
	MOVE_LINE_BEGIN,
	MOVE_LINE_START,
	MOVE_LINE_FINISH,
	MOVE_LINE_END,
	MOVE_WORD_START_PREV,
	MOVE_WORD_START_NEXT,
	MOVE_WORD_END_PREV,
	MOVE_WORD_END_NEXT,
	MOVE_SENTENCE_PREV,
	MOVE_SENTENCE_NEXT,
	MOVE_PARAGRAPH_PREV,
	MOVE_PARAGRAPH_NEXT,
	MOVE_BRACKET_MATCH,
	MOVE_FILE_BEGIN,
	MOVE_FILE_END,
};

static Movement moves[] = {
	[MOVE_CHAR_PREV]       = { .win = window_char_prev                                 },
	[MOVE_CHAR_NEXT]       = { .win = window_char_next                                 },
	[MOVE_LINE_UP]         = { .win = window_line_up                                   },
	[MOVE_LINE_DOWN]       = { .win = window_line_down                                 },
	[MOVE_LINE_BEGIN]      = { .txt = text_line_begin,      .type = LINEWISE           },
	[MOVE_LINE_START]      = { .txt = text_line_start,      .type = LINEWISE           },
	[MOVE_LINE_FINISH]     = { .txt = text_line_finish,     .type = LINEWISE           },
	[MOVE_LINE_END]        = { .txt = text_line_end,        .type = LINEWISE           },
	[MOVE_WORD_START_PREV] = { .txt = text_word_start_prev, .type = CHARWISE           },
	[MOVE_WORD_START_NEXT] = { .txt = text_word_start_next, .type = CHARWISE           },
	[MOVE_WORD_END_PREV]   = { .txt = text_word_end_prev,   .type = CHARWISE|INCLUSIVE },
	[MOVE_WORD_END_NEXT]   = { .txt = text_word_end_next,   .type = CHARWISE|INCLUSIVE },
	[MOVE_SENTENCE_PREV]   = { .txt = text_sentence_prev,   .type = LINEWISE           },
	[MOVE_SENTENCE_NEXT]   = { .txt = text_sentence_next,   .type = LINEWISE           },
	[MOVE_PARAGRAPH_PREV]  = { .txt = text_paragraph_prev,  .type = LINEWISE           },
	[MOVE_PARAGRAPH_NEXT]  = { .txt = text_paragraph_next,  .type = LINEWISE           },
	[MOVE_BRACKET_MATCH]   = { .txt = text_bracket_match,   .type = LINEWISE|INCLUSIVE },
	[MOVE_FILE_BEGIN]      = { .txt = text_begin,           .type = LINEWISE           },
	[MOVE_FILE_END]        = { .txt = text_end,             .type = LINEWISE           },
};

enum {
	TEXT_OBJ_WORD,
	TEXT_OBJ_SENTENCE,
	TEXT_OBJ_PARAGRAPH,
	TEXT_OBJ_OUTER_SQUARE_BRACKET,
	TEXT_OBJ_INNER_SQUARE_BRACKET,
	TEXT_OBJ_OUTER_CURLY_BRACKET,
	TEXT_OBJ_INNER_CURLY_BRACKET,
	TEXT_OBJ_OUTER_ANGLE_BRACKET,
	TEXT_OBJ_INNER_ANGLE_BRACKET,
	TEXT_OBJ_OUTER_PARANTHESE,
	TEXT_OBJ_INNER_PARANTHESE,
	TEXT_OBJ_OUTER_QUOTE,
	TEXT_OBJ_INNER_QUOTE,
	TEXT_OBJ_OUTER_SINGLE_QUOTE,
	TEXT_OBJ_INNER_SINGLE_QUOTE,
	TEXT_OBJ_OUTER_BACKTICK,
	TEXT_OBJ_INNER_BACKTICK,
};

static TextObject textobjs[] = {
	[TEXT_OBJ_WORD]                 = { text_object_word                  },
	[TEXT_OBJ_SENTENCE]             = { text_object_sentence              },
	[TEXT_OBJ_PARAGRAPH]            = { text_object_paragraph             },
	[TEXT_OBJ_OUTER_SQUARE_BRACKET] = { text_object_square_bracket, OUTER },
	[TEXT_OBJ_INNER_SQUARE_BRACKET] = { text_object_square_bracket, INNER },
	[TEXT_OBJ_OUTER_CURLY_BRACKET]  = { text_object_curly_bracket,  OUTER },
	[TEXT_OBJ_INNER_CURLY_BRACKET]  = { text_object_curly_bracket,  INNER },
	[TEXT_OBJ_OUTER_ANGLE_BRACKET]  = { text_object_angle_bracket,  OUTER },
	[TEXT_OBJ_INNER_ANGLE_BRACKET]  = { text_object_angle_bracket,  INNER },
	[TEXT_OBJ_OUTER_PARANTHESE]     = { text_object_paranthese,     OUTER },
	[TEXT_OBJ_INNER_PARANTHESE]     = { text_object_paranthese,     INNER },
	[TEXT_OBJ_OUTER_QUOTE]          = { text_object_quote,          OUTER },
	[TEXT_OBJ_INNER_QUOTE]          = { text_object_quote,          INNER },
	[TEXT_OBJ_OUTER_SINGLE_QUOTE]   = { text_object_single_quote,   OUTER },
	[TEXT_OBJ_INNER_SINGLE_QUOTE]   = { text_object_single_quote,   INNER },
	[TEXT_OBJ_OUTER_BACKTICK]       = { text_object_backtick,       OUTER },
	[TEXT_OBJ_INNER_BACKTICK]       = { text_object_backtick,       INNER },
};

/* draw a statubar, do whatever you want with the given curses window */
static void statusbar(WINDOW *win, bool active, const char *filename, size_t line, size_t col) {
	int width, height;
	getmaxyx(win, height, width);
	(void)height;
	wattrset(win, active ? A_REVERSE|A_BOLD : A_REVERSE);
	mvwhline(win, 0, 0, ' ', width);
	mvwprintw(win, 0, 0, "%s", filename);
	char buf[width + 1];
	int len = snprintf(buf, width, "%d, %d", line, col);
	if (len > 0) {
		buf[len] = '\0';
		mvwaddstr(win, 0, width - len - 1, buf);
	}
}

void quit(const Arg *arg) {
	endwin();
	exit(0);
}

static void split(const Arg *arg) {
	vis_window_split(vis, arg->s);
}

static void mark_set(const Arg *arg) {
	vis_mark_set(vis, arg->i, window_cursor_get(vis->win->win));
}

static void mark_goto(const Arg *arg) {
	vis_mark_goto(vis, arg->i);
}

static Action action;
void action_do(Action *a); 

static void count(const Arg *arg) {
	action.count = action.count * 10 + arg->i;
}

static void operator(const Arg *arg) {
	action.op = ops[arg->i];
}

static bool operator_unknown(Key *key1, Key *key2) {
	action.op = NULL;
	return true;
}

static void movement(const Arg *arg) {
	action.movement = &moves[arg->i];
	action_do(&action);
}

static void textobj(const Arg *arg) {
	action.textobj = &textobjs[arg->i];
	action_do(&action);
}

void action_reset(Action *a) {
	a->count = 0;
	a->op = NULL;
	a->movement = NULL;
	a->textobj = NULL;
	a->reg = NULL;
}

void action_do(Action *a) {
	Text *txt = vis->win->text;
	Win *win = vis->win->win;
	OperatorContext c;
	size_t pos = window_cursor_get(win);
	c.pos = pos;
	if (a->count == 0)
		a->count = 1;
	if (a->movement) {
		size_t start = pos;
		for (int i = 0; i < a->count; i++) {
			if (a->movement->txt)
				pos = a->movement->txt(txt, pos);
			else
				pos = a->movement->win(win);
		}
		c.range.start = MIN(start, pos);
		c.range.end = MAX(start, pos);
		if (!a->op) {
			if (a->movement->type & CHARWISE)
				window_scroll_to(win, pos);
			else
				window_cursor_to(win, pos);
		} else if (a->movement->type & INCLUSIVE) {
			Iterator it = text_iterator_get(txt, c.range.end);
			text_iterator_char_next(&it, NULL);
			c.range.end = it.pos;
		}
	} else if (a->textobj) {
		c.range = a->textobj->range(txt, pos);
		if (c.range.start != (size_t)-1 && a->textobj->type == OUTER) {
			c.range.start--;
			c.range.end++;
		}
	}
	c.count = a->count;
	if (a->op)
		a->op(&c);
	action_reset(a);
}

/* use vim's  
   :help motion
   :h operator
   :h text-objects
 as reference 
*/

static KeyBinding basic_movement[] = {
	{ { KEY(LEFT)               }, movement, { .i = MOVE_CHAR_PREV         } },
	{ { KEY(RIGHT)              }, movement, { .i = MOVE_CHAR_NEXT         } },
	{ { KEY(UP)                 }, movement, { .i = MOVE_LINE_UP           } },
	{ { KEY(DOWN)               }, movement, { .i = MOVE_LINE_DOWN         } },
	{ { KEY(PPAGE)              }, cursor,   { .m = window_page_up         } },
	{ { KEY(NPAGE)              }, cursor,   { .m = window_page_down       } },
	{ { KEY(HOME)               }, movement, { .i = MOVE_LINE_START        } },
	{ { KEY(END)                }, movement, { .i = MOVE_LINE_FINISH       } },
	// temporary until we have a way to enter user commands
	{ { CONTROL('c')            }, quit,                                   },
	{ /* empty last element, array terminator */                           },
};

#if 0
static KeyBinding vis_commands[] = {
	// DEMO STUFF
	{ { NONE('5')               }, line,          { .i = 50              } },
	{ { NONE('s')               }, mark_set,      { .i = 0               } },
	{ { NONE('9')               }, mark_goto,     { .i = 0               } },

	{ /* empty last element, array terminator */                           },
};
#endif

static KeyBinding vis_movements[] = {
	BACKSPACE(                     movement,    i,  MOVE_CHAR_PREV           ),
	{ { NONE('h')               }, movement, { .i = MOVE_CHAR_PREV         } },
	{ { NONE(' ')               }, movement, { .i = MOVE_CHAR_NEXT         } },
	{ { NONE('l')               }, movement, { .i = MOVE_CHAR_NEXT         } },
	{ { NONE('k')               }, movement, { .i = MOVE_LINE_UP           } },
	{ { CONTROL('P')            }, movement, { .i = MOVE_LINE_UP           } },
	{ { NONE('j')               }, movement, { .i = MOVE_LINE_DOWN         } },
	{ { CONTROL('J')            }, movement, { .i = MOVE_LINE_DOWN         } },
	{ { CONTROL('N')            }, movement, { .i = MOVE_LINE_DOWN         } },
	{ { KEY(ENTER)              }, movement, { .i = MOVE_LINE_DOWN         } },
	{ { NONE('0')               }, movement, { .i = MOVE_LINE_BEGIN        } },
	{ { NONE('^')               }, movement, { .i = MOVE_LINE_START        } },
	{ { NONE('g'), NONE('_')    }, movement, { .i = MOVE_LINE_FINISH       } },
	{ { NONE('$')               }, movement, { .i = MOVE_LINE_END          } },
	{ { NONE('%')               }, movement, { .i = MOVE_BRACKET_MATCH     } },
	{ { NONE('b')               }, movement, { .i = MOVE_WORD_START_PREV   } },
	{ { KEY(SLEFT)              }, movement, { .i = MOVE_WORD_START_PREV   } },
	{ { NONE('w')               }, movement, { .i = MOVE_WORD_START_NEXT   } },
	{ { KEY(SRIGHT)             }, movement, { .i = MOVE_WORD_START_NEXT   } },
	{ { NONE('g'), NONE('e')    }, movement, { .i = MOVE_WORD_END_PREV     } },
	{ { NONE('e')               }, movement, { .i = MOVE_WORD_END_NEXT     } },
	{ { NONE('{')               }, movement, { .i = MOVE_PARAGRAPH_PREV    } },
	{ { NONE('}')               }, movement, { .i = MOVE_PARAGRAPH_NEXT    } },
	{ { NONE('(')               }, movement, { .i = MOVE_SENTENCE_PREV     } },
	{ { NONE(')')               }, movement, { .i = MOVE_SENTENCE_NEXT     } },
	{ { NONE('g'), NONE('g')    }, movement, { .i = MOVE_FILE_BEGIN        } },
	{ { NONE('G')               }, movement, { .i = MOVE_FILE_END          } },
	{ /* empty last element, array terminator */                           },
};

// TODO: factor out prefix [ia] into spearate mode which sets a flag
static KeyBinding vis_textobjs[] = {
	{ { NONE('a'), NONE('w')    }, textobj,  { .i = TEXT_OBJ_WORD                 } },
	{ { NONE('i'), NONE('w')    }, textobj,  { .i = TEXT_OBJ_WORD                 } },
	{ { NONE('a'), NONE('s')    }, textobj,  { .i = TEXT_OBJ_SENTENCE             } },
	{ { NONE('i'), NONE('s')    }, textobj,  { .i = TEXT_OBJ_SENTENCE             } },
	{ { NONE('a'), NONE('p')    }, textobj,  { .i = TEXT_OBJ_PARAGRAPH            } },
	{ { NONE('i'), NONE('p')    }, textobj,  { .i = TEXT_OBJ_PARAGRAPH            } },
	{ { NONE('a'), NONE('[')    }, textobj,  { .i = TEXT_OBJ_OUTER_SQUARE_BRACKET } },
	{ { NONE('a'), NONE(']')    }, textobj,  { .i = TEXT_OBJ_OUTER_SQUARE_BRACKET } },
	{ { NONE('i'), NONE('[')    }, textobj,  { .i = TEXT_OBJ_INNER_SQUARE_BRACKET } },
	{ { NONE('i'), NONE(']')    }, textobj,  { .i = TEXT_OBJ_INNER_SQUARE_BRACKET } },
	{ { NONE('a'), NONE('(')    }, textobj,  { .i = TEXT_OBJ_OUTER_PARANTHESE     } },
	{ { NONE('a'), NONE(')')    }, textobj,  { .i = TEXT_OBJ_OUTER_PARANTHESE     } },
	{ { NONE('a'), NONE('b')    }, textobj,  { .i = TEXT_OBJ_OUTER_PARANTHESE     } },
	{ { NONE('i'), NONE('(')    }, textobj,  { .i = TEXT_OBJ_INNER_PARANTHESE     } },
	{ { NONE('i'), NONE(')')    }, textobj,  { .i = TEXT_OBJ_INNER_PARANTHESE     } },
	{ { NONE('i'), NONE('b')    }, textobj,  { .i = TEXT_OBJ_INNER_PARANTHESE     } },
	{ { NONE('a'), NONE('<')    }, textobj,  { .i = TEXT_OBJ_OUTER_ANGLE_BRACKET  } },
	{ { NONE('a'), NONE('>')    }, textobj,  { .i = TEXT_OBJ_OUTER_ANGLE_BRACKET  } },
	{ { NONE('i'), NONE('<')    }, textobj,  { .i = TEXT_OBJ_INNER_ANGLE_BRACKET  } },
	{ { NONE('i'), NONE('>')    }, textobj,  { .i = TEXT_OBJ_INNER_ANGLE_BRACKET  } },
	{ { NONE('a'), NONE('{')    }, textobj,  { .i = TEXT_OBJ_OUTER_CURLY_BRACKET  } },
	{ { NONE('a'), NONE('}')    }, textobj,  { .i = TEXT_OBJ_OUTER_CURLY_BRACKET  } },
	{ { NONE('a'), NONE('B')    }, textobj,  { .i = TEXT_OBJ_OUTER_CURLY_BRACKET  } },
	{ { NONE('i'), NONE('{')    }, textobj,  { .i = TEXT_OBJ_INNER_CURLY_BRACKET  } },
	{ { NONE('i'), NONE('}')    }, textobj,  { .i = TEXT_OBJ_INNER_CURLY_BRACKET  } },
	{ { NONE('i'), NONE('B')    }, textobj,  { .i = TEXT_OBJ_INNER_CURLY_BRACKET  } },
	{ { NONE('a'), NONE('"')    }, textobj,  { .i = TEXT_OBJ_OUTER_QUOTE          } },
	{ { NONE('i'), NONE('"')    }, textobj,  { .i = TEXT_OBJ_INNER_QUOTE          } },
	{ { NONE('a'), NONE('\'')   }, textobj,  { .i = TEXT_OBJ_OUTER_SINGLE_QUOTE   } },
	{ { NONE('i'), NONE('\'')   }, textobj,  { .i = TEXT_OBJ_INNER_SINGLE_QUOTE   } },
	{ { NONE('a'), NONE('`')    }, textobj,  { .i = TEXT_OBJ_OUTER_BACKTICK       } },
	{ { NONE('i'), NONE('`')    }, textobj,  { .i = TEXT_OBJ_INNER_BACKTICK       } },
};

static KeyBinding vis_operators[] = {
	{ { NONE('1')               }, count,         { .i = 1               } },
	{ { NONE('2')               }, count,         { .i = 2               } },
	{ { NONE('3')               }, count,         { .i = 3               } },
	{ { NONE('4')               }, count,         { .i = 4               } },
	{ { NONE('5')               }, count,         { .i = 5               } },
	{ { NONE('6')               }, count,         { .i = 6               } },
	{ { NONE('7')               }, count,         { .i = 7               } },
	{ { NONE('8')               }, count,         { .i = 8               } },
	{ { NONE('9')               }, count,         { .i = 9               } },
	{ { NONE('d')               }, operator,      { .i = OP_DELETE       } },
	{ { NONE('c')               }, operator,      { .i = OP_CHANGE       } },
	{ { NONE('y')               }, operator,      { .i = OP_YANK         } },
	{ /* empty last element, array terminator */                           },
};

static KeyBinding vis_registers[] = { /* {a-zA-Z0-9.%#:-"} */
//	{ { NONE('"'), NONE('a')    }, reg,           { .i = 1               } },
	{ /* empty last element, array terminator */                           },
};

static KeyBinding vis_marks[] = { /* {a-zA-Z} */
//	{ { NONE('`'), NONE('a')    }, mark,          { .i = 1               } },
//	{ { NONE('\''), NONE('a')    }, mark,          { .i = 1               } },
	{ /* empty last element, array terminator */                           },
};

static KeyBinding vis_normal[] = {
	{ { CONTROL('w'), NONE('c') }, split,    { .s = NULL                   } },
	{ { CONTROL('w'), NONE('j') }, call,     { .f = vis_window_next     } },
	{ { CONTROL('w'), NONE('k') }, call,     { .f = vis_window_prev     } },
	{ { CONTROL('F')            }, cursor,   { .m = window_page_up         } },
	{ { CONTROL('B')            }, cursor,   { .m = window_page_down       } },
	{ { NONE('n')               }, find_forward,  { .s = "if"            } },
	{ { NONE('p')               }, find_backward, { .s = "if"            } },
	{ { NONE('x')               }, cursor,        { .f = vis_delete_key   } },
	{ { NONE('i')               }, switchmode,    { .i = VIS_MODE_INSERT } },
	{ { NONE('v')               }, switchmode,    { .i = VIS_MODE_VISUAL } },
	{ { NONE('R')               }, switchmode,    { .i = VIS_MODE_REPLACE} },
	{ { NONE('u')               }, call,          { .f = vis_undo     } },
	{ { CONTROL('R')            }, call,          { .f = vis_redo     } },
	{ { CONTROL('L')            }, call,          { .f = vis_draw     } },
	{ /* empty last element, array terminator */                           },
};

static KeyBinding vis_visual[] = {
	{ { NONE(ESC)               }, switchmode,    { .i = VIS_MODE_NORMAL } },
	{ /* empty last element, array terminator */                           },
};

static void vis_visual_enter(void) {
	window_selection_start(vis->win->win);
}

static void vis_visual_leave(void) {
	window_selection_clear(vis->win->win);
}

static KeyBinding vis_insert_mode[] = {
	{ { NONE(ESC)               }, switchmode,    { .i = VIS_MODE_NORMAL  } },
	{ { CONTROL('D')            }, cursor,        { .f = vis_delete_key   } },
	BACKSPACE(                     cursor,           f,  vis_backspace_key  ),
	{ /* empty last element, array terminator */                            },
};

static bool vis_insert_input(const char *str, size_t len) {
	vis_insert_key(vis, str, len);
	return true;
}

static KeyBinding vis_replace[] = {
	{ { NONE(ESC)               }, switchmode,   { .i = VIS_MODE_NORMAL  } },
	{ /* empty last element, array terminator */                           },
};

static bool vis_replace_input(const char *str, size_t len) {
	vis_replace_key(vis, str, len);
	return true;
}

static Mode vis_modes[] = {
	[VIS_MODE_BASIC] = {
		.parent = NULL,
		.bindings = basic_movement,
	},
	[VIS_MODE_MOVE] = { 
		.parent = &vis_modes[VIS_MODE_BASIC],
		.bindings = vis_movements,
	},
	[VIS_MODE_TEXTOBJ] = { 
		.parent = &vis_modes[VIS_MODE_MOVE],
		.bindings = vis_textobjs,
	},
	[VIS_MODE_OPERATOR] = { 
		.parent = &vis_modes[VIS_MODE_MOVE],
		.bindings = vis_operators,
	},
	[VIS_MODE_NORMAL] = {
		.parent = &vis_modes[VIS_MODE_OPERATOR],
		.bindings = vis_normal,
	},
	[VIS_MODE_VISUAL] = {
		.name = "VISUAL",
		.parent = &vis_modes[VIS_MODE_OPERATOR],
		.bindings = vis_visual,
		.enter = vis_visual_enter,
		.leave = vis_visual_leave,
	},
	[VIS_MODE_INSERT] = {
		.name = "INSERT",
		.parent = &vis_modes[VIS_MODE_BASIC],
		.bindings = vis_insert_mode,
		.input = vis_insert_input,
	},
	[VIS_MODE_REPLACE] = {
		.name = "REPLACE",
		.parent = &vis_modes[VIS_MODE_INSERT],
		.bindings = vis_replace,
		.input = vis_replace_input,
	},
};

static void switchmode(const Arg *arg) {
	if (mode->leave)
		mode->leave();
	mode = &vis_modes[arg->i];
	if (mode->enter)
		mode->enter();
	// TODO display mode name somewhere?
}

/* incomplete list of usefule but currently missing functionality from nanos help ^G:

^X      (F2)            Close the current file buffer / Exit from nano
^O      (F3)            Write the current file to disk
^J      (F4)            Justify the current paragraph

^R      (F5)            Insert another file into the current one
^W      (F6)            Search for a string or a regular expression

^K      (F9)            Cut the current line and store it in the cutbuffer
^U      (F10)           Uncut from the cutbuffer into the current line
^T      (F12)           Invoke the spell checker, if available


^_      (F13)   (M-G)   Go to line and column number
^\      (F14)   (M-R)   Replace a string or a regular expression
^^      (F15)   (M-A)   Mark text at the cursor position
M-W     (F16)           Repeat last search

M-^     (M-6)           Copy the current line and store it in the cutbuffer
M-V                     Insert the next keystroke verbatim

XXX: CONTROL(' ') = 0, ^Space                  Go forward one word
*/

/* key binding configuration */
static KeyBinding nano_keys[] = {
#if 0
	{ { CONTROL('D') },   cursor,   { .m = vis_delete          } },
	BACKSPACE(            cursor,      m,  vis_backspace         ),
	{ { KEY(LEFT) },      cursor,   { .m = vis_char_prev       } },
	{ { KEY(RIGHT) },     cursor,   { .m = vis_char_next       } },
	{ { CONTROL('F') },   cursor,   { .m = vis_char_next       } },
	{ { KEY(UP) },        cursor,   { .m = vis_line_up         } },
	{ { CONTROL('P') },   cursor,   { .m = vis_line_up         } },
	{ { KEY(DOWN) },      cursor,   { .m = vis_line_down       } },
	{ { CONTROL('N') },   cursor,   { .m = vis_line_down       } },
	{ { KEY(PPAGE) },     cursor,   { .m = vis_page_up         } },
	{ { CONTROL('Y') },   cursor,   { .m = vis_page_up         } },
	{ { KEY(F(7)) },      cursor,   { .m = vis_page_up         } },
	{ { KEY(NPAGE) },     cursor,   { .m = vis_page_down       } },
	{ { CONTROL('V') },   cursor,   { .m = vis_page_down       } },
	{ { KEY(F(8)) },      cursor,   { .m = vis_page_down       } },
//	{ { CONTROL(' ') },   cursor,   { .m = vis_word_start_next } },
	{ { META(' ') },      cursor,   { .m = vis_word_start_prev } },
	{ { CONTROL('A') },   cursor,   { .m = vis_line_start      } },
	{ { CONTROL('E') },   cursor,   { .m = vis_line_end        } },
	{ { META(']') },      cursor,   { .m = vis_bracket_match   } },
	{ { META(')') },      cursor,   { .m = vis_paragraph_next  } },
	{ { META('(') },      cursor,   { .m = vis_paragraph_prev  } },
	{ { META('\\') },     cursor,   { .m = vis_file_begin      } },
	{ { META('|') },      cursor,   { .m = vis_file_begin      } },
	{ { META('/') },      cursor,   { .m = vis_file_end        } },
	{ { META('?') },      cursor,   { .m = vis_file_end        } },
	{ { META('U') },      call,     { .f = vis_undo            } },
	{ { META('E') },      call,     { .f = vis_redo            } },
	{ { CONTROL('I') },   insert,   { .s = "\t"                   } },
	/* TODO: handle this in vis to insert \n\r when appriopriate */
	{ { CONTROL('M') },   insert,   { .s = "\n"                   } },
	{ { CONTROL('L') },   call,     { .f = vis_draw            } },
#endif
	{ /* empty last element, array terminator */              },
};

static Mode nano[] = {
	{ .parent = NULL,     .bindings = basic_movement,  },
	{ .parent = &nano[0], .bindings = nano_keys, .input = vis_insert_input, },
};

/* list of vis configurations, first entry is default. name is matched with
 * argv[0] i.e. program name upon execution
 */
static Config editors[] = {
	{ .name = "vis",  .mode = &vis_modes[VIS_MODE_NORMAL], .statusbar = statusbar },
	{ .name = "nano", .mode = &nano[1], .statusbar = statusbar },
};

/* Color definitions, by default the i-th color is used for the i-th syntax
 * rule below. A color value of -1 specifies the default terminal color.
 */
static Color colors[] = {
	{ .fg = COLOR_RED,     .bg = -1, .attr = A_BOLD },
	{ .fg = COLOR_GREEN,   .bg = -1, .attr = A_BOLD },
	{ .fg = COLOR_GREEN,   .bg = -1, .attr = A_NORMAL },
	{ .fg = COLOR_MAGENTA, .bg = -1, .attr = A_BOLD },
	{ .fg = COLOR_MAGENTA, .bg = -1, .attr = A_NORMAL },
	{ .fg = COLOR_BLUE,    .bg = -1, .attr = A_BOLD },
	{ .fg = COLOR_RED,     .bg = -1, .attr = A_NORMAL },
	{ .fg = COLOR_BLUE,    .bg = -1, .attr = A_NORMAL },
	{ .fg = COLOR_BLUE,    .bg = -1, .attr = A_NORMAL },
	{ /* empty last element, array terminator */ }
};

/* Syntax color definition, you can define up to SYNTAX_REGEX_RULES
 * number of regex rules per file type. Each rule is requires a regular
 * expression and corresponding compilation flags. Optionally a color in
 * the form
 *
 *   { .fg = COLOR_YELLOW, .bg = -1, .attr = A_BOLD }
 *
 * can be specified. If such a color definition is missing the i-th element
 * of the colors array above is used instead.
 *
 * The array of syntax definition must be terminated with an empty element.
 */
#define B "\\b"
/* Use this if \b is not in your libc's regex implementation */
// #define B "^| |\t|\\(|\\)|\\[|\\]|\\{|\\}|\\||$

// changes wrt sandy #precoressor: #   idfdef, #include <file.h> between brackets
static Syntax syntaxes[] = {{
	.name = "c",
	.file = "\\.(c(pp|xx)?|h(pp|xx)?|cc)$",
	.rules = {{
		"$^",
		REG_NEWLINE,
	},{
		B"(for|if|while|do|else|case|default|switch|try|throw|catch|operator|new|delete)"B,
		REG_NEWLINE,
	},{
		B"(float|double|bool|char|int|short|long|sizeof|enum|void|static|const|struct|union|"
		"typedef|extern|(un)?signed|inline|((s?size)|((u_?)?int(8|16|32|64|ptr)))_t|class|"
		"namespace|template|public|protected|private|typename|this|friend|virtual|using|"
		"mutable|volatile|register|explicit)"B,
		REG_NEWLINE,
	},{
		B"(goto|continue|break|return)"B,
		REG_NEWLINE,
	},{
		"(^#[\\t ]*(define|include(_next)?|(un|ifn?)def|endif|el(if|se)|if|warning|error|pragma))|"
		B"[A-Z_][0-9A-Z_]+"B"",
		REG_NEWLINE,
	},{
		"(\\(|\\)|\\{|\\}|\\[|\\])",
		REG_NEWLINE,
	},{
		"(\"(\\\\.|[^\"])*\")",
		//"([\"<](\\\\.|[^ \">])*[\">])",
		REG_NEWLINE,
	},{
		"(//.*)",
		REG_NEWLINE,
	},{
		"(/\\*([^*]|\\*[^/])*\\*/|/\\*([^*]|\\*[^/])*$|^([^/]|/[^*])*\\*/)",
	}},
},{
	/* empty last element, array terminator */
}};