#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>
#include "linuxelf.h"
#include <sys/stat.h>
#include <sys/mman.h>

static int chflg(char *head)
{
	int i;
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *phdr;

	ehdr = (Elf32_Ehdr *)head;
	if (!IS_ELF(*ehdr)) {
		printf("This is not Elf file\n");
		return (-1);
	}

	if (ehdr->e_ident[EI_CLASS] != ELF_CLASS) {
		printf("unknown clas. (%d)\n", (int)ehdr->e_ident[EI_CLASS]);
		return (-1);
	}

	if (ehdr->e_ident[EI_DATA] != ELF_DATA) {
		printf("unknown endian. (%d)\n", (int)ehdr->e_ident[EI_DATA]);
		return (-1);
	}

	for (i = 0; i < ehdr->e_phnum; i++) {
		phdr = (Elf_Phdr *)((char *)ehdr + ehdr->e_phoff + ehdr->e_phentsize * i);
		phdr->p_flags = PF_X|PF_W|PF_R;
	}

	return (0);
}

int main(int argc, char *argv[])
{
	int fd;
	struct stat sb;
	char *head;

	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		printf("cannot open file.\n");
		exit(1);
	}
	fstat(fd, &sb);
	head = mmap(NULL, sb.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

	if (chflg(head) < 0) {
		printf("fail to change flags.\n");
	}

	close(fd);

	exit(0);
}

