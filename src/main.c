#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

struct termios g_origTermios;

void EnableRawMode()
{
	tcgetattr(STDIN_FILENO, &g_origTermios);

	struct termios raw = g_origTermios;

	raw.c_iflag &= ~(ICRNL | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void DisableRawMode()
{
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_origTermios);
}

int main()
{
	EnableRawMode();
	atexit(DisableRawMode);

	char c;
	while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q')
	{
		if (iscntrl(c))
		{
			printf("%d\r\n", c);
		}
		else
		{
			printf("%d ('%c')\r\n", c, c);
		}
	}

	return 0;
}
