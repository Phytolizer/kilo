#pragma region includes

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#pragma endregion

#pragma region defines

#define CTRL_KEY(k) ((k)&0x1f)

#pragma endregion

#pragma region data

struct EditorConfig
{
	int screenRows;
	int screenCols;
	struct termios origTermios;
};

struct EditorConfig g_editorConfig;

#pragma endregion

#pragma region terminal

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

char EditorReadKey()
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

#pragma endregion

#pragma region append buffer

struct AppendBuffer
{
	char* buffer;
	int length;
};

#define ABUF_INIT                                                                                                      \
	(struct AppendBuffer)                                                                                              \
	{                                                                                                                  \
		.buffer = NULL, .length = 0,                                                                                   \
	}

void AppendBufferAppend(struct AppendBuffer* ab, const char* s, int len)
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

void AppendBufferFree(struct AppendBuffer* ab)
{
	free(ab->buffer);
}

#pragma endregion

#pragma region output

void EditorDrawRows(struct AppendBuffer* ab)
{
	int y;
	for (y = 0; y < g_editorConfig.screenRows; ++y)
	{
		AppendBufferAppend(ab, "~", 1);

		if (y < g_editorConfig.screenRows - 1)
		{
			AppendBufferAppend(ab, "\r\n", 2);
		}
	}
}

void EditorRefreshScreen()
{
	struct AppendBuffer ab = ABUF_INIT;

	AppendBufferAppend(&ab, "\x1b[?25l", 6);
	AppendBufferAppend(&ab, "\x1b[2J", 4);
	AppendBufferAppend(&ab, "\x1b[H", 3);

	EditorDrawRows(&ab);

	AppendBufferAppend(&ab, "\x1b[H", 3);
	AppendBufferAppend(&ab, "\x1b[?25h", 6);

	write(STDOUT_FILENO, ab.buffer, ab.length);
	AppendBufferFree(&ab);
}

#pragma endregion

#pragma region input

void EditorProcessKeypress()
{
	char c = EditorReadKey();

	switch (c)
	{
	case CTRL_KEY('q'):
		write(STDOUT_FILENO, "\x1b[2J", 4);
		write(STDOUT_FILENO, "\x1b[H", 3);
		exit(EXIT_SUCCESS);
	}
}

#pragma endregion

#pragma region init

void InitEditor()
{
	if (GetWindowSize(&g_editorConfig.screenRows, &g_editorConfig.screenCols) == -1)
	{
		Die("GetWindowSize");
	}
}

int main()
{
	EnableRawMode();
	InitEditor();

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
	while (true)
	{
		EditorRefreshScreen();
		EditorProcessKeypress();
	}
#pragma clang diagnostic pop
}

#pragma endregion
