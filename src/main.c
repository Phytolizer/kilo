#pragma region includes

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#pragma endregion

#pragma region defines

#define CTRL_KEY(k) ((k)&0x1f)

#pragma endregion

#pragma region data

struct termios g_origTermios;

#pragma endregion

#pragma region terminal

void Die(const char* s)
{
	perror(s);
	exit(EXIT_FAILURE);
}

void EnableRawMode()
{
	if (tcgetattr(STDIN_FILENO, &g_origTermios) == -1)
	{
		Die("tcgetattr");
	}

	struct termios raw = g_origTermios;

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

void DisableRawMode()
{
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_origTermios) == -1)
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

#pragma endregion

#pragma region input

void EditorProcessKeypress()
{
	char c = EditorReadKey();

	switch (c)
	{
	case CTRL_KEY('q'):
		exit(EXIT_SUCCESS);
	}
}

#pragma endregion

#pragma region init

int main()
{
	EnableRawMode();
	atexit(DisableRawMode);

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
	while (true)
	{
		EditorProcessKeypress();
	}
#pragma clang diagnostic pop
}

#pragma endregion
