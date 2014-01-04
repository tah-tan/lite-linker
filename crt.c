#define	SYS_exit		1 /* http://asm.sourceforge.net/syscall.html */

int syscall2();

extern int main(int, char **, char **);

static void _exit(int status)
{
	syscall2(SYS_exit, status);
}

void exit(int status)
{
	_exit(status);
}

void _start(char *arguments, ...)
{
	int argc;
	char **argv;
	char **env;

	argv = &arguments;
	argc = * (int *)(argv - 1);
	env = argv + argc + 1;

	exit( main(argc, argv, env));
}