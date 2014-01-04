#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include "linuxelf.h"
#include "linklib.h"


static char * get_section_name(Elf32_Ehdr *ehdr, Elf32_Shdr *shdr)
{
	char *head;
	Elf32_Shdr *nhdr;

	head = (char *)ehdr;
	nhdr = (Elf32_Shdr *)(head + ehdr->e_shoff + ehdr->e_shentsize * ehdr->e_shstrndx);
	return ((char *)(head + nhdr->sh_offset + shdr->sh_name));
}


Elf32_Shdr * get_section(Elf32_Ehdr *ehdr, char *name)
{
	int i;
	char *head;
	Elf32_Shdr *shdr;

	head = (char *)ehdr;
	for (i = 0; i < ehdr->e_shnum; i++) {
		shdr = (Elf32_Shdr *)(head + ehdr->e_shoff + ehdr->e_shentsize * i);
		if (!strcmp(get_section_name(ehdr, shdr), name)) {
			return (shdr);
		}
	}

	return (NULL);
}


int search_symbol(struct obj objs[], char *name, struct obj *obj)
{
	int i, n;
	char *head;
	Elf32_Ehdr *ehdr;
	Elf32_Shdr *shdr;
	Elf32_Shdr *symtab;
	Elf32_Shdr *strtab;
	Elf32_Sym *symp;

	for (n = 0; objs[n].filename; n++) {		// ??? i think this loop end condition is a little bit harmful..
		ehdr = (Elf32_Ehdr *)objs[n].address;
		symtab = get_section(ehdr, ".symtab");
		strtab = get_section(ehdr, ".strtab");
		head = (char *)ehdr;

		if (symtab->sh_entsize != sizeof(Elf32_Sym)) {
			printf("Invalid entry size. (%d)\n", symtab->sh_entsize);
			exit(1);
		}

		for (i = 0; i < symtab->sh_size; i += symtab->sh_entsize) {
			symp = (Elf32_Sym *)(head + symtab->sh_offset + i);
			if (!symp->st_name) continue;
			if ((symp->st_shndx == SHN_UNDEF) || (symp->st_shndx == SHN_ABS)) continue;
			if (ELF_ST_BIND(symp->st_info) != STB_GLOBAL) continue;
			if ((ELF_ST_TYPE(symp->st_info) != STT_NOTYPE) &&
				(ELF_ST_TYPE(symp->st_info) != STT_OBJECT) &&
				(ELF_ST_TYPE(symp->st_info) != STT_FUNC)) {
				continue;
			}
			/*
			 * The following code must be comment-out,
			 * because the st_size of a symbol defined by assembly will be zero.
			 */
			// if (!symp->st_size) continue;

			if (!strcmp(name, head + strtab->sh_offset + symp->st_name)) {
				shdr = (Elf32_Shdr *)(head + ehdr->e_shoff + ehdr->e_shentsize * symp->st_shndx);
				obj->filename = objs[n].filename;
				obj->address = (char *)(head + shdr->sh_offset + symp->st_value);
				return 0;
			}
		}
	}

	printf("cannot find symbol %s.\n", name);
	exit(1);
}


int relocate_common_symbol(Elf32_Ehdr *ehdr)
{
	int i, n;
	char *head;
	Elf32_Shdr *symtab;
	Elf32_Shdr *bss;
	Elf32_Sym *symp;

	symtab = get_section(ehdr, ".symtab");
	bss = get_section(ehdr, ".bss");
	head = (char *)ehdr;

	if (symtab->sh_entsize != sizeof(Elf32_Sym)) {
		printf("Invalid entry size. (%d)\n", symtab->sh_entsize);
		exit(1);
	}

	n = bss->sh_size;
	for (i = 0; i < symtab->sh_size; i += symtab->sh_entsize) {
		symp = (Elf32_Sym *)(head + symtab->sh_offset + i);
		if (symp->st_shndx != SHN_COMMON) continue;
		/*
		 * Align by st_value because st_value represents alignment size
		 * at COMMON symbol.
		 */
		n = (n + symp->st_value - 1) & ~ (symp->st_value - 1);
		symp->st_shndx = bss - (Elf32_Shdr *)(head + ehdr->e_shoff);	// this difference is executed in Elf32_Shdr type. not in byte.
		symp->st_value = n;
		n += symp->st_size;
	}

	n = (n + 15) & ~ 15;
	bss->sh_size = n;
	return 0;
}


static int link_symbol(struct obj objs[], int objnum, Elf32_Ehdr *ehdr,
						Elf32_Shdr *reltab, Elf32_Shdr *defsec,
						Elf32_Shdr *symtab, Elf32_Shdr *strtab)
{
	int i, addr, addend;
	char *head;
	unsigned int *addp;
	Elf32_Shdr *shdr;
	Elf32_Sym *symp;
	Elf32_Rel *relp;
	struct obj obj;

	head = (char *)ehdr;
	for (i = 0; i < reltab->sh_size; i += reltab->sh_entsize) {
		relp = (Elf32_Rel *)(head + reltab->sh_offset + i);
		symp = (Elf32_Sym *)(head + symtab->sh_offset +
							(symtab->sh_entsize * ELF_R_SYM(relp->r_info)));
		/*
		 * Do not check the following rule
		 * because st_name is NULL if the simbol table points section.
		 */
		// if (!symp->st_name) continue;

		printf("%-10s ", objs[objnum].filename);

		if (symp->st_name) {
			printf("%-12s ", head + strtab->sh_offset + symp->st_name);
		}
		switch (symp->st_shndx) {
		case SHN_UNDEF:
			if(!symp->st_name) {
				printf("%-12s", "unknown");
			}
			search_symbol(objs, (char *)(head + strtab->sh_offset + symp->st_name), &obj);
			addr = (int)obj.address;
			printf("%-10s", obj.filename);
			break;
		case SHN_ABS:
		case SHN_COMMON:
			printf("invalid Ndx.\n");
			exit(1);
		default:
			shdr = (Elf32_Shdr *)(head + ehdr->e_shoff + ehdr->e_shentsize * symp->st_shndx);
			if (!symp->st_name) {
				printf("%-12s ", get_section_name(ehdr, shdr));
			}
			addr = (int)(head + shdr->sh_offset + symp->st_value);
			printf("internal  ");
			break;
		}

		addp = (unsigned int *)(head + defsec->sh_offset + relp->r_offset);
		switch (reltab->sh_type) {
		case SHT_REL:
			printf("REL   ");
			addend = *addp;
			break;
		case SHT_RELA:
			printf("RELA  ");
			addend = ((Elf32_Rela *)relp)->r_addend;
			break;
		default:
			printf("%4d ", reltab->sh_type);
			addend = 0;
			break;
		}

		printf("%08x %08x %08x ", addend, (int)addp, addr);

		/*
		 * The following switch-case condition is too few.
		 * If you use in other environment(e.g. ARM), you may add conditons below.
		 */
		switch (ELF_R_TYPE(relp->r_info)) {
		case R_386_PC32:
			printf("R_382_PC32\n");
			*addp = addend + (addr - (int)addp);
			break;
		case R_386_32:
			printf("R_382_32\n");
			*addp = addend + addr;
			break;
		default:
			printf("unknown(%d)\n", ELF_R_TYPE(relp->r_info));
		}
	}

	return 0;
}

int check_ehdr(Elf32_Ehdr *ehdr)
{
	if (!IS_ELF(*ehdr)) {
		printf("This is not ELF file.\n");
		exit(1);
	}

	if (ehdr->e_ident[EI_CLASS] != ELF_CLASS) {
		printf("unknown class. (%d)\n", (int)ehdr->e_ident[EI_CLASS]);
		exit(1);
	}

	if (ehdr->e_ident[EI_DATA] != ELF_DATA) {
		printf("unknown endian. (%d)\n", (int)ehdr->e_ident[EI_DATA]);
		exit(1);
	}

	if (ehdr->e_type != ET_REL) {
		printf("unknown type. (%d)\n", (int)ehdr->e_type);
		exit(1);
	}

	return 0;
}

int link_objs(struct obj objs[])		// entry function of symbol relocation
{									// objs is a list contains object file's address and name.
	int i, j;
	char *name;
	Elf32_Ehdr *ehdr;
	Elf32_Shdr *reltab;
	Elf32_Shdr *defsec;
	Elf32_Shdr *symtab;
	Elf32_Shdr *strtab;

	printf("\nfilename   symbol       found     type  addend   called   defined  rel type\n");
	printf("-----------------------------------------------------------------------------\n");

	for (i = 0; objs[i].filename; i++) {
		ehdr = (Elf32_Ehdr *)objs[i].address;

		symtab = get_section(ehdr, ".symtab");
		strtab = get_section(ehdr, ".strtab");

		if (symtab && strtab) {
			for (j = 0; j < ehdr->e_shnum; j++) {
				reltab = (Elf32_Shdr *)((char *)ehdr + ehdr->e_shoff + ehdr->e_shentsize * j);
				if (reltab->sh_type == SHT_REL) {		// ??? RELA??
					name = get_section_name(ehdr, reltab);
					defsec = get_section(ehdr, strchr(name + 1, '.'));
					link_symbol(objs, i, ehdr, reltab, defsec, symtab, strtab);
				}
			}
		}
	}

	return 0;
}


