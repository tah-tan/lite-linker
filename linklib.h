#include <elf.h>

struct obj {
	char * filename;
	char * address;
};

Elf32_Shdr * get_section(Elf32_Ehdr *ehdr, char *name);
int search_symbol(struct obj objs[], char *name, struct obj *obj);
int relocate_common_symbol(Elf32_Ehdr *ehdr);
int check_ehdr(Elf32_Ehdr *ehdr);
int link_objs(struct obj objs[]);

