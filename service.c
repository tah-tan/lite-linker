#define	SYS_write	4	/* http://asm.sourceforge.net/syscall.html */

int syscall4();

static int _write(int fd, char *s, int n)
{
	return (syscall4(SYS_write, fd, s, n));
}

static int slen(char *s)
{
	int n = 0;
	while(s[n]) n++;
	return (n);
}

int print(int fd, char *s)
{
	return (_write(fd, s, slen(s)));
}

int printn(int fd, unsigned int n)
{
	char s[20];
	char *p = &s[sizeof(s) - 1];

	*(p) = '\0';
	do {
		*(--p) = '0' + (n % 10);
		n /= 10;
	} while(n);
	return (print(fd, p));
}

