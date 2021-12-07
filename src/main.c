#pragma region includes

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
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

int GetWindowSize(int* rows, int* cols)
{
	struct winsize ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
	{
		return -1;
	}

	*cols = ws.ws_col;
	*rows = ws.ws_row;
	return 0;
}

#pragma endregion

#pragma region output

void EditorDrawRows()
{
	int y;
	for (y = 0; y < 24; ++y)
	{
		write(STDOUT_FILENO, "~\r\n", 3);
	}
}

void EditorRefreshScreen()
{
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);

	EditorDrawRows();

	write(STDOUT_FILENO, "\x1b[H", 3);
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
