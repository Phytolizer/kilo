// #region includes

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include "config.h"
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

// #endregion

// #region defines

#define CTRL_KEY(k) ((k)&0x1f)

enum EditorKey
{
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	DEL_KEY,
	HOME_KEY,
	END_KEY,
	PAGE_UP,
	PAGE_DOWN,
};

// #endregion

// #region data

typedef struct EditorRow
{
	int size;
	char* chars;
} EditorRow;

struct EditorConfig
{
	int cursorX;
	int cursorY;
	int rowOffset;
	int screenRows;
	int screenCols;
	int numRows;
	EditorRow* row;
	struct termios origTermios;
};

struct EditorConfig g_editorConfig;

// #endregion

// #region terminal

void Die(const char* s)
{
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);

	perror(s);
	exit(EXIT_FAILURE);
}

void DisableRawMode()
{
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_editorConfig.origTermios) == -1)
	{
		Die("tcsetattr");
	}
}

void EnableRawMode()
{
	if (tcgetattr(STDIN_FILENO, &g_editorConfig.origTermios) == -1)
	{
		Die("tcgetattr");
	}
	atexit(DisableRawMode);

	struct termios raw = g_editorConfig.origTermios;

	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
	{
		Die("tcsetattr");
	}
}

int EditorReadKey()
{
	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1)
	{
		if (nread == -1 && errno != EAGAIN)
		{
			Die("read");
		}
	}

	if (c == '\x1b')
	{
		char seq[3];

		if (read(STDIN_FILENO, &seq[0], 1) != 1)
		{
			return '\x1b';
		}
		if (read(STDIN_FILENO, &seq[1], 1) != 1)
		{
			return '\x1b';
		}

		if (seq[0] == '[')
		{
			switch (seq[1])
			{
			case '0' ... '9':
				if (read(STDIN_FILENO, &seq[2], 1) != 1)
				{
					return '\x1b';
				}
				if (seq[2] == '~')
				{
					switch (seq[1])
					{
					case '1':
						return HOME_KEY;
					case '3':
						return DEL_KEY;
					case '4':
						return END_KEY;
					case '5':
						return PAGE_UP;
					case '6':
						return PAGE_DOWN;
					case '7':
						return HOME_KEY;
					case '8':
						return END_KEY;
					}
				}
				break;
			case 'A':
				return ARROW_UP;
			case 'B':
				return ARROW_DOWN;
			case 'C':
				return ARROW_RIGHT;
			case 'D':
				return ARROW_LEFT;
			case 'H':
				return HOME_KEY;
			case 'F':
				return END_KEY;
			}
		}
		else if (seq[0] == 'O')
		{
			switch (seq[1])
			{
			case 'H':
				return HOME_KEY;
			case 'F':
				return END_KEY;
			}
		}

		return '\x1b';
	}
	return c;
}

int GetCursorPosition(int* rows, int* cols)
{
	char buf[32];
	unsigned int i = 0;

	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
	{
		return -1;
	}

	while (i < sizeof(buf) - 1)
	{
		if (read(STDIN_FILENO, &buf[i], 1) != 1)
		{
			break;
		}
		if (buf[i] == 'R')
		{
			break;
		}
		++i;
	}
	buf[i] = '\0';

	if (buf[0] != '\x1b' || buf[1] != '[')
	{
		return -1;
	}
	if (sscanf(&buf[2], "%d;%d", rows, cols) != 2)
	{
		return -1;
	}

	return 0;
}

int GetWindowSize(int* rows, int* cols)
{
	struct winsize ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
	{
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
		{
			return -1;
		}
		return GetCursorPosition(rows, cols);
	}

	*cols = ws.ws_col;
	*rows = ws.ws_row;
	return 0;
}

// #endregion

// #region row operations

void EditorAppendRow(char* s, size_t len)
{
	g_editorConfig.row = realloc(g_editorConfig.row, sizeof(EditorRow) * (g_editorConfig.numRows + 1));

	int at = g_editorConfig.numRows;
	g_editorConfig.row[at].size = len;
	g_editorConfig.row[at].chars = malloc(len + 1);
	memcpy(g_editorConfig.row[at].chars, s, len);
	g_editorConfig.row[at].chars[len] = '\0';
	++g_editorConfig.numRows;
}

// #endregion

// #region file IO

void EditorOpen(char* filename)
{
	FILE* fp = fopen(filename, "r");
	if (fp == NULL)
	{
		Die("fopen");
	}

	char* line = NULL;
	size_t linecap = 0;
	ssize_t linelen;
	while ((linelen = getline(&line, &linecap, fp)) != -1)
	{
		while (linelen > 0 && line[linelen - 1] == '\n' || line[linelen - 1] == '\r')
		{
			--linelen;
		}
		EditorAppendRow(line, linelen);
	}
	free(line);
	fclose(fp);
}

// #endregion

// #region append buffer

struct AppendBuffer
{
	char* buffer;
	int length;
};

#define APPEND_BUFFER_INIT                                                                                             \
	(struct AppendBuffer)                                                                                              \
	{                                                                                                                  \
		.buffer = NULL, .length = 0,                                                                                   \
	}

void AppendBuffer_Append(struct AppendBuffer* ab, const char* s, int len)
{
	char* newBuf = realloc(ab->buffer, ab->length + len);

	if (newBuf == NULL)
	{
		return;
	}

	memcpy(&newBuf[ab->length], s, len);
	ab->buffer = newBuf;
	ab->length += len;
}

void AppendBuffer_Free(struct AppendBuffer* ab)
{
	free(ab->buffer);
}

// #endregion

// #region output

void EditorScroll()
{
	if (g_editorConfig.cursorY < g_editorConfig.rowOffset)
	{
		g_editorConfig.rowOffset = g_editorConfig.cursorY;
	}
	if (g_editorConfig.cursorY >= g_editorConfig.rowOffset + g_editorConfig.screenRows)
	{
		g_editorConfig.rowOffset = g_editorConfig.cursorY - g_editorConfig.screenRows + 1;
	}
}

void EditorDrawRows(struct AppendBuffer* ab)
{
	int y;
	for (y = 0; y < g_editorConfig.screenRows; ++y)
	{
		int fileRow = y + g_editorConfig.rowOffset;
		if (fileRow >= g_editorConfig.numRows)
		{
			if (g_editorConfig.numRows == 0 && y == g_editorConfig.screenRows / 3)
			{
				char welcome[80];
				int welcomeLen = snprintf(welcome, sizeof(welcome), "Kilo editor -- version %s", KILO_VERSION);
				if (welcomeLen > g_editorConfig.screenCols)
				{
					welcomeLen = g_editorConfig.screenCols;
				}
				int padding = (g_editorConfig.screenCols - welcomeLen) / 2;
				if (padding > 0)
				{
					AppendBuffer_Append(ab, "~", 1);
					--padding;
				}
				for (; padding > 0; --padding)
				{
					AppendBuffer_Append(ab, " ", 1);
				}
				AppendBuffer_Append(ab, welcome, welcomeLen);
			}
			else
			{
				AppendBuffer_Append(ab, "~", 1);
			}
		}
		else
		{
			int len = g_editorConfig.row[fileRow].size;
			if (len > g_editorConfig.screenCols)
			{
				len = g_editorConfig.screenCols;
			}
			AppendBuffer_Append(ab, g_editorConfig.row[fileRow].chars, len);
		}

		AppendBuffer_Append(ab, "\x1b[K", 3);
		if (y < g_editorConfig.screenRows - 1)
		{
			AppendBuffer_Append(ab, "\r\n", 2);
		}
	}
}

void EditorRefreshScreen()
{
	EditorScroll();

	struct AppendBuffer ab = APPEND_BUFFER_INIT;

	AppendBuffer_Append(&ab, "\x1b[?25l", 6);
	AppendBuffer_Append(&ab, "\x1b[H", 3);

	EditorDrawRows(&ab);

	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", g_editorConfig.cursorY - g_editorConfig.rowOffset + 1,
	         g_editorConfig.cursorX + 1);
	AppendBuffer_Append(&ab, buf, strlen(buf));
	AppendBuffer_Append(&ab, "\x1b[?25h", 6);

	write(STDOUT_FILENO, ab.buffer, ab.length);
	AppendBuffer_Free(&ab);
}

// #endregion

// #region input

void EditorMoveCursor(int key)
{
	switch (key)
	{
	case ARROW_LEFT:
		if (g_editorConfig.cursorX > 0)
		{
			--g_editorConfig.cursorX;
		}
		break;
	case ARROW_RIGHT:
		if (g_editorConfig.cursorX < g_editorConfig.screenCols - 1)
		{
			++g_editorConfig.cursorX;
		}
		break;
	case ARROW_UP:
		if (g_editorConfig.cursorY > 0)
		{
			--g_editorConfig.cursorY;
		}
		break;
	case ARROW_DOWN:
		if (g_editorConfig.cursorY < g_editorConfig.numRows)
		{
			++g_editorConfig.cursorY;
		}
		break;
	}
}

void EditorProcessKeypress()
{
	int c = EditorReadKey();

	switch (c)
	{
	case CTRL_KEY('q'):
		write(STDOUT_FILENO, "\x1b[2J", 4);
		write(STDOUT_FILENO, "\x1b[H", 3);
		exit(EXIT_SUCCESS);
	case HOME_KEY:
		g_editorConfig.cursorX = 0;
		break;
	case END_KEY:
		g_editorConfig.cursorX = g_editorConfig.screenCols - 1;
		break;
	case PAGE_UP:
	case PAGE_DOWN: {
		for (int times = g_editorConfig.screenRows; times > 0; --times)
		{
			EditorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
		}
	}
	break;
	case ARROW_UP:
	case ARROW_RIGHT:
	case ARROW_DOWN:
	case ARROW_LEFT:
		EditorMoveCursor(c);
		break;
	}
}

// #endregion

// #region init

void InitEditor()
{
	g_editorConfig.cursorX = 0;
	g_editorConfig.cursorY = 0;
	g_editorConfig.numRows = 0;
	g_editorConfig.row = NULL;
	g_editorConfig.rowOffset = 0;

	if (GetWindowSize(&g_editorConfig.screenRows, &g_editorConfig.screenCols) == -1)
	{
		Die("GetWindowSize");
	}
}

int main(int argc, char** argv)
{
	EnableRawMode();
	InitEditor();
	if (argc >= 2)
	{
		EditorOpen(argv[1]);
	}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
	while (true)
	{
		EditorRefreshScreen();
		EditorProcessKeypress();
	}
#pragma clang diagnostic pop
}

// #endregion
