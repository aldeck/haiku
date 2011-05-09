/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include "package.h"

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


extern const char* __progname;
const char* kCommandName = __progname;


static const char* kUsage =
	"Usage: %s <command> <command args>\n"
	"Creates, inspects, or extracts a Haiku package.\n"
	"\n"
	"Commands:\n"
	"  create [ <options> ] <package>\n"
	"    Creates package file <package> from contents of current directory.\n"
	"\n"
	"    -C <dir>   - Change to directory <dir> before starting.\n"
	"    -q         - be quiet (don't show any output except for errors).\n"
	"    -v         - be verbose (show more info about created package).\n"
	"\n"
	"  dump [ <options> ] <package>\n"
	"    Dumps the TOC section of package file <package>. For debugging only.\n"
	"\n"
	"  extract [ <options> ] <package>\n"
	"    Extracts the contents of package file <package>.\n"
	"\n"
	"    -C <dir>   - Change to directory <dir> before extracting the "
		"contents\n"
	"                  of the archive.\n"
	"\n"
	"  list [ <options> ] <package>\n"
	"    Lists the contents of package file <package>.\n"
	"\n"
	"    -a         - Also list the file attributes.\n"
	"\n"
	"Common Options:\n"
	"  -h, --help   - Print this usage info.\n"
;


void
print_usage_and_exit(bool error)
{
    fprintf(error ? stderr : stdout, kUsage, kCommandName);
    exit(error ? 1 : 0);
}


int
main(int argc, const char* const* argv)
{
	if (argc < 2)
		print_usage_and_exit(true);

	const char* command = argv[1];
	if (strcmp(command, "create") == 0)
		return command_create(argc - 1, argv + 1);

	if (strcmp(command, "dump") == 0)
		return command_dump(argc - 1, argv + 1);

	if (strcmp(command, "extract") == 0)
		return command_extract(argc - 1, argv + 1);

	if (strcmp(command, "list") == 0)
		return command_list(argc - 1, argv + 1);

	if (strcmp(command, "help") == 0)
		print_usage_and_exit(false);
	else
		print_usage_and_exit(true);

	// never gets here
	return 0;
}
