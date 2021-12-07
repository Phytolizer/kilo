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

#pragma endregion

#pragma region init

int main()
{
	EnableRawMode();
	atexit(DisableRawMode);

	while (true)
	{
		char c = '\0';
		if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN)
		{
			Die("read");
		}
		if (iscntrl(c))
		{
			printf("%d\r\n", c);
		}
		else
		{
			printf("%d ('%c')\r\n", c, c);
		}
		if (c == CTRL_KEY('q'))
		{
			break;
		}
	}

	return 0;
}

#pragma endregion
