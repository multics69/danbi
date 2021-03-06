/*
 * syst_multi_np.c - simple system-wide counting no thread
 *
 * Copyright (c) 2009 Google, Inc
 * Contributed by Stephane Eranian <eranian@gmail.com>
 *
 * Based on:
 * Copyright (c) 2002-2006 Hewlett-Packard Development Company, L.P.
 * Contributed by Stephane Eranian <eranian@hpl.hp.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <sys/types.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <err.h>

#include <perfmon/pfmlib.h>
#include <perfmon/perfmon.h>


#include "detect_pmcs.h"

#define NUM_PMCS PFMLIB_MAX_PMCS
#define NUM_PMDS PFMLIB_MAX_PMDS

#define MAX_EVT_NAME_LEN	128

int
main(int argc, char **argv)
{
	char **p;
	pfarg_pmc_t pc[NUM_PMCS];
	pfarg_pmd_t pd[NUM_PMDS];
	pfarg_ctx_t ctx[1];
	pfarg_load_t load_args;
	pfmlib_input_param_t inp;
	pfmlib_output_param_t outp;
	pfmlib_options_t pfmlib_options;
	uint64_t *pdo;
	int *fds;
	int ret, i;
	unsigned int l;
	long n, ncpus;
	unsigned int num_counters;
	char name[MAX_EVT_NAME_LEN];

	/*
	 * pass options to library (optional)
	 */
	memset(&pfmlib_options, 0, sizeof(pfmlib_options));
	pfmlib_options.pfm_debug = 0; /* set to 1 for debug */
	pfm_set_options(&pfmlib_options);

	/*
	 * Initialize pfm library (required before we can use it)
	 */
	ret = pfm_initialize();
	if (ret != PFMLIB_SUCCESS)
		errx(1, "Cannot initialize library: %s", pfm_strerror(ret));

	pfm_get_num_counters(&num_counters);


	memset(pc, 0, sizeof(pc));
	memset(pd, 0, sizeof(pd));
	memset(ctx, 0, sizeof(ctx));
	memset(&inp,0, sizeof(inp));
	memset(&outp,0, sizeof(outp));
	memset(&load_args,0, sizeof(load_args));

	/*
	 * be nice to user!
	 */
	if (argc > 1) {
		p = argv+1;
		for (i=0; *p ; i++, p++) {
			if (pfm_find_full_event(*p, &inp.pfp_events[i]) != PFMLIB_SUCCESS)
				errx(1, "cannot find %s event", *p);
		}
	} else {
		if (pfm_get_cycle_event(&inp.pfp_events[0]) != PFMLIB_SUCCESS)
			errx(1, "cannot find cycle event");
		if (pfm_get_inst_retired_event(&inp.pfp_events[1]) != PFMLIB_SUCCESS)
			errx(1, "cannot find inst retired event");
		i = 2;
	}
	/*
	 * set the privilege mode:
	 * 	PFM_PLM3 : user   level
	 * 	PFM_PLM0 : kernel level
	 */
	inp.pfp_dfl_plm   = PFM_PLM3|PFM_PLM0;

	if (i > num_counters) {
		i = num_counters;
		warnx("too many events provided (max=%d events), using first %d event(s)", num_counters, i);
	}
	/*
	 * how many counters we use
	 */
	inp.pfp_event_count = i;

	/*
	 * indicate we are using the monitors for a system-wide session.
	 * This may impact the way the library sets up the PMC values.
	 */
	inp.pfp_flags = PFMLIB_PFP_SYSTEMWIDE;

	/*
	 * In system wide mode, the perfmon context cannot be inherited.
	 * Also in this mode, we cannot use the blocking form of user level notification.
	 */
	ctx[0].ctx_flags = PFM_FL_SYSTEM_WIDE;

	/*
	 * build the pfp_unavail_pmcs bitmask by looking
	 * at what perfmon has available. It is not always
	 * the case that all PMU registers are actually available
	 * to applications. For instance, on IA-32 platforms, some
	 * registers may be reserved for the NMI watchdog timer.
	 *
	 * With this bitmap, the library knows which registers NOT to
	 * use. Of source, it is possible that no valid assignement may
	 * be possible if certina PMU registers  are not available.
	 */
	detect_unavail_pmcs(-1, &inp.pfp_unavail_pmcs);
	/*
	 * let the library figure out the values for the PMCS
	 */
	if ((ret=pfm_dispatch_events(&inp, NULL, &outp, NULL)) != PFMLIB_SUCCESS)
		errx(1, "cannot configure events: %s", pfm_strerror(ret));

	/*
	 * Now prepare the argument to initialize the PMDs and PMCS.
	 * We use pfp_pmc_count to determine the number of PMC to intialize.
	 * We use pfp_pmd_count to determine the number of PMD to initialize.
	 * Some events/features may cause extra PMCs to be used, leading to:
	 * 	- pfp_pmc_count may be >= pfp_event_count
	 * 	- pfp_pmd_count may be >= pfp_event_count
	 */
	for (i=0; i < outp.pfp_pmc_count; i++) {
		pc[i].reg_num   = outp.pfp_pmcs[i].reg_num;
		pc[i].reg_value = outp.pfp_pmcs[i].reg_value;
	}

	for (i=0; i < outp.pfp_pmd_count; i++)
		pd[i].reg_num = outp.pfp_pmds[i].reg_num;

	ncpus = sysconf(_SC_NPROCESSORS_ONLN);
	if (ncpus < 1)
		ncpus = 1;

	fds = malloc(ncpus  * sizeof(int));
	if (!fds)
		err(1, "cannot allocate fds");

	pdo = calloc(ncpus * outp.pfp_pmd_count, sizeof(uint64_t));
	if (!pdo)
		err(1, "cannot allocate pdo");

	printf("ncpus=%ld pmd=%d\n", ncpus, outp.pfp_pmd_count);
	/*
 	 * create and load contexts
 	 */
	for(n=0; n < ncpus; n++) {
		fds[n] = pfm_create_context(ctx, NULL, NULL, 0);
		if (fds[n] == -1) {
			if (errno == ENOSYS) {
				errx(1, "your kernel does not have performance monitoring support!\n");
			}
			err(1, "cannot create context");
		}
		if (pfm_write_pmcs(fds[n], pc, outp.pfp_pmc_count))
			err(1, "pfm_write_pmcs error");

		if (pfm_write_pmds(fds[n], pd, outp.pfp_pmd_count))
			err(1, "pfm_write_pmds error");

		load_args.load_pid = n;
		if (pfm_load_context(fds[n], &load_args))
			err(1, "pfm_load_context error");

	}
	/*
 	 * start all
 	 */
	for(n=0; n < ncpus; n++) {
		if (pfm_start(fds[n], NULL) == -1)
			err(1, "pfm_start error");
	}

	for(l=0; l < 20; l++) {
		sleep(1);
		/*
		 * read the results
		 */
		puts("------------------------");
		for(n=0; n < ncpus; n++) {
			if (pfm_read_pmds(fds[n], pd, outp.pfp_pmd_count))
				err(1, "pfm_read_pmds error");

			for (i=0; i < outp.pfp_pmd_count; i++) {
				pfm_get_full_event_name(&inp.pfp_events[i], name, MAX_EVT_NAME_LEN);
				printf("CPU%-2ld PMD%-3u %-20"PRIu64" %s\n",
					n,
					pd[i].reg_num,
					pd[i].reg_value - pdo[n*outp.pfp_pmd_count+i],
					name);
				pdo[n*outp.pfp_pmd_count+i] = pd[i].reg_value;
			}
		}
	}
	for(n=0; n < ncpus; n++)
		close(fds[n]);

	free(fds);
	free(pdo);
	return 0;
}
