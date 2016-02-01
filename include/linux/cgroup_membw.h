/* cgroup_membw.h - memory bandwidth controller
 *
 * Copyright IBM Corporation, 2016
 * Author Raghavendra K T <raghavendra.kt@linux.vnet.ibm.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _LINUX_CGROUP_MEMBW_H
#define _LINUX_CGROUP_MEMBW_H
#include <linux/cgroup.h>

struct membw_cgroup {
	struct cgroup_subsys_state	css;
/*
 * A value of zero weight indicates unrestricted memory bandwidth
 * A value of 100 indicates maximum configurable bandwidth
 * ideally weight of should be equal to unrestricted bandwidth
 *
 */
	unsigned long	membw_weight;
	unsigned long	membw_opweight;
	unsigned long	membw_interval;
};
static void inline arch_membw_sched_in(struct membw_cgroup)
{
}
static void inline arch_membw_sched_out(void)
{
}
#endif /* _LINUX_CGROUP_MEMBW_H */
