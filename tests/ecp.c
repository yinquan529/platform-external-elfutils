/* Copyright (C) 2002, 2005 Red Hat, Inc.
   This file is part of Red Hat elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2002.

   Red Hat elfutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by the
   Free Software Foundation; version 2 of the License.

   Red Hat elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with Red Hat elfutils; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301 USA.

   Red Hat elfutils is an included package of the Open Invention Network.
   An included package of the Open Invention Network is a package for which
   Open Invention Network licensees cross-license their patents.  No patent
   license is granted, either expressly or impliedly, by designation as an
   included package.  Should you wish to participate in the Open Invention
   Network licensing program, please visit www.openinventionnetwork.com
   <http://www.openinventionnetwork.com>.  */

#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <gelf.h>
#include <stdlib.h>
#include <unistd.h>

int
main (int argc, char *argv[])
{
  if (argc < 3)
    error (EXIT_FAILURE, 0, "usage: %s FROMNAME TONAME", argv[0]);

  elf_version (EV_CURRENT);

  int infd = open (argv[1], O_RDONLY);
  if (infd == -1)
    error (EXIT_FAILURE, errno, "cannot open input file '%s'", argv[1]);

  Elf *inelf = elf_begin (infd, ELF_C_READ, NULL);
  if (inelf == NULL)
    error (EXIT_FAILURE, 0, "problems opening '%s' as ELF file: %s",
	   argv[1], elf_errmsg (-1));

  int outfd = creat (argv[2], 0666);
  if (outfd == -1)
    error (EXIT_FAILURE, errno, "cannot open output file '%s'", argv[2]);

  Elf *outelf = elf_begin (outfd, ELF_C_WRITE, NULL);
  if (outelf == NULL)
    error (EXIT_FAILURE, 0, "problems opening '%s' as ELF file: %s",
	   argv[2], elf_errmsg (-1));

  gelf_newehdr (outelf, gelf_getclass (inelf));

  GElf_Ehdr ehdr_mem;
  GElf_Ehdr *ehdr;
  gelf_update_ehdr (outelf, (ehdr = gelf_getehdr (inelf, &ehdr_mem)));

  if (ehdr->e_phnum > 0)
    {
      int cnt;

      if (gelf_newphdr (outelf, ehdr->e_phnum) == 0)
	error (EXIT_FAILURE, 0, "cannot create program header: %s",
	       elf_errmsg (-1));

      for (cnt = 0; cnt < ehdr->e_phnum; ++cnt)
	{
	  GElf_Phdr phdr_mem;

	  gelf_update_phdr (outelf, cnt, gelf_getphdr (inelf, cnt, &phdr_mem));
	}
    }

  Elf_Scn *scn = NULL;
  while ((scn = elf_nextscn (inelf, scn)) != NULL)
    {
      Elf_Scn *newscn = elf_newscn (outelf);

      GElf_Shdr shdr_mem;
      gelf_update_shdr (newscn, gelf_getshdr (scn, &shdr_mem));

      *elf_newdata (newscn) = *elf_getdata (scn, NULL);
    }

  elf_flagelf (outelf, ELF_C_SET, ELF_F_LAYOUT);

  if (elf_update (outelf, ELF_C_WRITE) == -1)
    error (EXIT_FAILURE, 0, "elf_update failed: %s", elf_errmsg (-1));

  elf_end (outelf);
  close (outfd);

  elf_end (inelf);

  return 0;
}
