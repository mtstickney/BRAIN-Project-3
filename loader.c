#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "mem.h"
#include "vm.h"
#include "sched.h"
#include "sem.h"

/* note: buf should be able to hold word_size+1 bytes, since we use scanf to read */
void read_word(char *buf)
{
	size_t len;
	int c;

	len = 0;
	if (scanf("%4s", buf) != 1 || (len=strlen(buf)) < 4) {
		/* short or failed read */
		fprintf(stderr, "read_word: short or failed read, trying to recover\n");

		/* pull the remaining non-whitespace chars */
		do {
			c = fgetc(stdin);
			if (!isspace(c))
				buf[len++] = c;
		} while (len < 4 && !feof(stdin));
	}
}

/* check for requested process size (already seen BRAIN10) */
int load_header(FILE *fh)
{
	unsigned int procsize;

	if (fscanf(fh, "%u", &procsize) != 1) {
		fprintf(stderr, "load_procsize: invalid process size header\n");
		return -1;
	}
	return procsize;
}

/* return 0: loaded a process, saw BRAIN10 header *
 * return 1: loaded a process, saw DATA *
 * return -1: error */
int load_code(FILE *fh, struct proc *p)
{
	char buf[8];
	unsigned int addr;

	addr = 0;
	while (fscanf(fh, "%7s", buf) == 1) {
		while (fgetc(fh) != '\n');
		if (strncmp("BRAIN10", buf, 7) == 0)
			
			return 0;
		if (strncmp("DATA", buf, 4) == 0)
			return 1;
		if (store(p, buf, addr++) != 0) {
			fprintf(stderr, "load_code: store failed\n");
			return -1;
		}
	}
	fprintf(stderr, "load_code: failed to read characters, probably fell off end of file\n");
	return -1;
}

int load_file(FILE *fh)
{
	char buf[8];
	int procsize;
	struct proc *p;

	if (fscanf(fh, "%7s", buf) != 1) {
		fprintf(stderr, "load_file: failed to read header\n");
		return 1;
	}
	if (strncmp("BRAIN10", buf, 7) != 0) {
		fprintf(stderr, "load_file: bad header\n");
		return 1;
	}
	/* use up the rest of the line */
	while(fgetc(fh) != '\n');
	while (1) {
		procsize=load_header(fh);
		if (procsize < 0) {
			fprintf(stderr, "load_file: bad process size header\n");
			return 1;
		}
		p = procalloc(procsize);
		if (p == NULL) {
			fprintf(stderr, "load_file: error allocating process\n");
			return 1;
		}
		switch (load_code(fh, p)) {
		case 0: /* success, more procs to read */
			sched_reset(p->pid);
			sched_resume(p->pid);
			break;
		case 1: /* success, end of procs */
			sched_reset(p->pid);
			sched_resume(p->pid);
			return 0;
		case -1:
			fprintf(stderr, "load_code: load failed\n");
			return 1;
		}
	}
}

int main()
{
	set_mem('0');

	if (sched_init() != 0) {
		fprintf(stderr, "Scheduler init failed\n");
		return 1;
	}
	vm_init();
	sem_setup();
	init_mem();
	if (load_file(stdin) != 0) {
		fprintf(stderr, "loader: failed to load BRAIN programs from stdin\n");
		return 1;
	}

	print_mem();
	sched_run();
	/* print_mem(); */

	return 0;
}
