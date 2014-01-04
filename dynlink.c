#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include "linklib.h"

int main(int argc, char *argv[])
{
	int i, n, c, ret;
	FILE *fp;
	char *funcname, *p;
	struct obj objs[10];
	struct obj obj;
	int (*f)();
	Elf32_Shdr *shdr;
	static char buffer[64 * 1024];	// buffer for loading object files

	memset(buffer, 0, sizeof(buffer));
	p = buffer;
	funcname = argv[1];

	// align 4KB
	p = (char *)(((unsigned int)p + 4095) & ~4095);

	printf("base address is 0x%08x\n", (int)buffer);
	for (n = 0, i = 2; i < argc; n++, i++) {
		objs[n].filename = argv[i];
		fp = fopen(objs[n].filename, "rb");
		if (!fp) {
			printf("cannot open file %s.\n", objs[n].filename);
			exit(1);
		}
		objs[n].address = p;
		printf("load to 0x%08x (%s)\n", (int)p, objs[n].filename);
		while((c = fgetc(fp)) != EOF) {
			*(p++) = c;
		}
		fclose(fp);

		p = (char *)(((unsigned int)p + 15) & ~15);

		check_ehdr((Elf32_Ehdr *)objs[n].address);
		relocate_common_symbol((Elf32_Ehdr *)objs[n].address);

		// get BSS area and initialize
		shdr = get_section((Elf32_Ehdr *)objs[n].address, ".bss");
		if (shdr) {
			// make BSS at the last area
			shdr->sh_offset = p - objs[n].address;
			memset(p, 0,shdr->sh_size);
			p += shdr->sh_size;
		}

		// align 16 byte just in case
		p = (char *)(((unsigned int) p + 15) &~ 15);
	}
	objs[n].filename = NULL;
	objs[n].address	= NULL;

	link_objs(objs);

	// execution test code.
	search_symbol(objs, funcname, &obj);
	f = (int (*)())obj.address;
	printf("\n%s is found at 0x%08x (%s).\n\n", funcname, (int)f, obj.filename);
	ret = f();
	printf("\n%s return (%d)\n", funcname, ret);

	exit(0);
}
