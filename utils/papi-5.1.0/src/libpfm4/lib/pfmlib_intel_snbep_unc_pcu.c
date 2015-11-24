/*
 * pfmlib_intel_snbep_unc_pcu.c : Intel SandyBridge-EP Power Control Unit (PCU) uncore PMU
 *
 * Copyright (c) 2012 Google, Inc
 * Contributed by Stephane Eranian <eranian@gmail.com>
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
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* private headers */
#include "pfmlib_priv.h"
#include "pfmlib_intel_x86_priv.h"
#include "pfmlib_intel_snbep_unc_priv.h"
#include "events/intel_snbep_unc_pcu_events.h"

pfmlib_pmu_t intel_snbep_unc_pcu_support = {
	.desc			= "Intel Sandy Bridge-EP PCU uncore",
	.name			= "snbep_unc_pcu",
	.perf_name		= "uncore_pcu",
	.pmu			= PFM_PMU_INTEL_SNBEP_UNC_PCU,
	.pme_count		= LIBPFM_ARRAY_SIZE(intel_snbep_unc_p_pe),
	.type			= PFM_PMU_TYPE_UNCORE,
	.num_cntrs		= 4,
	.num_fixed_cntrs	= 0,
	.max_encoding		= 2,
	.pe			= intel_snbep_unc_p_pe,
	.atdesc			= snbep_unc_mods,
	.flags			= PFMLIB_PMU_FL_RAW_UMASK,
	.pmu_detect		= pfm_intel_snbep_unc_detect,
	.get_event_encoding[PFM_OS_NONE] = pfm_intel_snbep_unc_get_encoding,
	 PFMLIB_ENCODE_PERF(pfm_intel_snbep_unc_get_perf_encoding),
	.get_event_first	= pfm_intel_x86_get_event_first,
	.get_event_next		= pfm_intel_x86_get_event_next,
	.event_is_valid		= pfm_intel_x86_event_is_valid,
	.validate_table		= pfm_intel_x86_validate_table,
	.get_event_info		= pfm_intel_x86_get_event_info,
	.get_event_attr_info	= pfm_intel_x86_get_event_attr_info,
	PFMLIB_VALID_PERF_PATTRS(pfm_intel_snbep_unc_perf_validate_pattrs),
	.get_event_nattrs	= pfm_intel_x86_get_event_nattrs,
	.can_auto_encode	= pfm_intel_snbep_unc_can_auto_encode,
};
