#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "kernel.h"

static bool repeat_idx_round_down = false;
static bool repeat_next_round_up = false;
static bool offset_is_address = false;
static unsigned int order = 2;
static unsigned int offset = 0;
static unsigned int count = 50;

static void usage(const char *cmd)
{
	printf("Usage:\n");
	printf("%s\n", cmd);
	printf("--help                           this help menu\n");
	printf("--order                          page order, default is 2 for 16k if on x86\n");
	printf("--offset                         initial count offset, default is 0\n");
	printf("--count                          how many times to loop, default is 50\n");
	printf("--offset-is-address              the given offset should be treated as an address instead of an index\n");
	printf("--repeat-idx-round-down          repeats round_down() on idx\n");
	printf("--repeat-next-round-up           repeats round_up() on next\n");
	exit(1);
}

void check_arg(const char *cmd, char *argv[], int *argc)
{
	if (strcmp(argv[*argc], "--repeat-idx-round-down") == 0) {
		repeat_idx_round_down = true;
		return;
	}
	if (strcmp(argv[*argc], "--repeat-next-round-up") == 0) {
		repeat_next_round_up = true;
		return;
	}
	if (strcmp(argv[*argc], "--offset-is-address") == 0) {
		offset_is_address = true;
		return;
	}
	if (strcmp(argv[*argc], "--order") == 0) {
		*argc = (*argc) + 1;
		if (*argc <= 1)
			usage(cmd);
		order = atoi(argv[*argc]);
		return;
	}
	if (strcmp(argv[*argc], "--offset") == 0) {
		*argc = (*argc) + 1;
		if (*argc <= 1)
			usage(cmd);
		offset = atoi(argv[*argc]);
		return;
	}
	if (strcmp(argv[*argc], "--count") == 0) {
		*argc = (*argc) + 1;
		if (*argc <= 1)
			usage(cmd);
		count = atoi(argv[*argc]);
		return;
	}
	usage(cmd);
}

int main(int argc, char *argv[])
{
	unsigned int i;
	unsigned int nrpages;
	const char *cmd_argv = argv[0];
	bool last_idx_set = false;

	for (i=1; i < argc; i++)
		check_arg(cmd_argv, argv, &i);

	nrpages = 1UL << order;
	printf("Order: %u  nrpages: %u, Offset: %u  Count: %u\n", order, nrpages, offset, count);

	for (i=0; i < count; i++) {
		unsigned int pos = offset + i;
		unsigned int idx;
		unsigned int idx_next;
		unsigned int last_idx_e;
		unsigned int last_idx_i;

		/*
		 * last_idx is the index of the page beyond the end of the read
		 * It mimics the setting in filemap_get_pages().
		 *
		 * That is, it is not inclusive. If you want the folio index
		 * which also includes the last read just subtract nrpages.
		 */

		if (offset_is_address) {
			idx      = round_down(pos >> PAGE_SHIFT, nrpages);
			idx_next = round_down(pos >> PAGE_SHIFT, nrpages) + nrpages;
			if (!last_idx_set) {
				last_idx_e = DIV_ROUND_UP(pos + count, PAGE_SIZE);
				last_idx_e = round_up(last_idx_e, nrpages);
				last_idx_i = last_idx_e - 1;
				last_idx_set = true;
			}
		} else {
			idx = round_down(pos, nrpages);
			idx_next = round_up(pos, nrpages);
			if (!last_idx_set) {
				last_idx_e = round_up(pos + count, nrpages);
				last_idx_i = last_idx_e - 1;
				last_idx_set = true;
			}
		}

		if (repeat_idx_round_down) {
			if (offset_is_address)
				idx = round_down(idx, PAGE_SIZE * nrpages);
			else
				idx = round_down(idx, nrpages);
		}
		if (repeat_next_round_up) {
			if (offset_is_address)
				idx_next = round_up(idx_next, PAGE_SIZE * nrpages);
			else
				idx_next = round_up(idx_next, nrpages);
		}

		/* this verifies alignment always works */
		WARN_ON(idx & (nrpages - 1));
		WARN_ON(idx_next & (nrpages - 1));
		WARN_ON(last_idx_e & (nrpages - 1));

		printf("i: %5u  pos: %5u  idx: %5u  idx_next: %5u  last_idx_e: %5u last_idx_i: %5u\n",
			i,      pos,      idx,      idx_next,      last_idx_e,     last_idx_i);
	}

	return 0;
}
