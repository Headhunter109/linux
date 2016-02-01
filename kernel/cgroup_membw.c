/*
 * Memory bandwidth controller
 *
 * Copyright (C) 2016 Raghavendra K T <raghavendra.kt@linux.vnet.ibm.com>
 * IBM Corporation
 *
 * This file is subject to the terms and conditions of version 2 of the GNU
 * General Public License.  See the file COPYING in the main directory of the
 * Linux distribution for more details.
 */

#include <linux/kernel.h>
#include <linux/threads.h>
#include <linux/atomic.h>
#include <linux/cgroup.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/cgroup_membw.h>

#define MEMBW_RANGE_MIN		0
#define MEMBW_RANGE_MAX		100
#define MEMBW_DEFAULT		0
#define MEMBW_INTERVAL_MAX	0x3fff
#define MEMBW_OPLIMIT_MAX	65000LU	


void membw_sched_in(struct task_struct *prev);
void membw_sched_out(struct task_struct *prev,
                          struct task_struct *next);


struct membw_cgroup *root_membw_cg;


static struct membw_cgroup *css_membw(struct cgroup_subsys_state *css)
{
	return container_of(css, struct membw_cgroup, css);
}

static struct membw_cgroup *parent_membw(struct membw_cgroup *membw)
{
	return css_membw(membw->css.parent);
}

static inline
struct membw_cgroup *membw_cgroup_from_css(struct cgroup_subsys_state *css)
{
	return css ? container_of(css, struct membw_cgroup, css) : NULL;
}

struct membw_cgroup *membw_cgroup_from_task(struct task_struct *p)
{
	if (unlikely(!p))
		return NULL;
	return membw_cgroup_from_css(task_css(p, membw_cgrp_id));
}
EXPORT_SYMBOL(membw_cgroup_from_task);

void membw_sched_in(struct task_struct *prev)
{
	struct membw_cgroup *membw;

	membw = membw_cgroup_from_task(prev);
	arch_membw_sched_in(membw);
}
EXPORT_SYMBOL(membw_sched_in);

void membw_sched_out(struct task_struct *prev,
                          struct task_struct *next)
{
	struct membw_cgroup *membw1, *membw2 = NULL;

	membw1 = membw_cgroup_from_task(prev);
	if (next) {
		membw2 = membw_cgroup_from_task(next);
	}
	/* TODO: Optimize schedule setting when prev and next cgroup are same */
	arch_membw_sched_out();
}
EXPORT_SYMBOL(membw_sched_out);

static struct cgroup_subsys_state *
membw_css_alloc(struct cgroup_subsys_state *parent)
{
	struct membw_cgroup *membw;

	membw = kzalloc(sizeof(struct membw_cgroup), GFP_KERNEL);
	if (!membw)
		return ERR_PTR(-ENOMEM);

	membw->membw_weight = MEMBW_DEFAULT;

	trace_printk("Raghu: membw1 = %p weight = %lu\n", membw, membw->membw_weight);
	if (!root_membw_cg)
		root_membw_cg = membw;

	return &membw->css;
}

static void membw_css_free(struct cgroup_subsys_state *css)
{
	kfree(css_membw(css));
}

static void membw_free(struct task_struct *task)
{
	struct membw_cgroup *membw = css_membw(task_css(task, membw_cgrp_id));

	membw_uncharge(membw);
}

static ssize_t membw_weight_write(struct kernfs_open_file *of, char *buf,
			      size_t nbytes, loff_t off)
{
	struct cgroup_subsys_state *css = of_css(of);
	struct membw_cgroup *membw = css_membw(css);
	int64_t weight;
	int err;

	buf = strstrip(buf);

	err = kstrtoll(buf, 0, &weight);
	if (err)
		return err;

	if (weight < 0 || weight > MEMBW_RANGE_MAX)
		return -EINVAL;

	membw->membw_weight = weight;
	return nbytes;
}

static int membw_weight_show(struct seq_file *sf, void *v)
{
	struct cgroup_subsys_state *css = seq_css(sf);
	struct membw_cgroup *membw = css_membw(css);
	int64_t weight = membw->membw_weight;

	seq_printf(sf, "%lld\n", weight);

	return 0;
}

static ssize_t membw_oplimit_write(struct kernfs_open_file *of, char *buf,
			      size_t nbytes, loff_t off)
{
	struct cgroup_subsys_state *css = of_css(of);
	struct membw_cgroup *membw = css_membw(css);
	int64_t oplimit;
	int err;

	buf = strstrip(buf);

	err = kstrtoll(buf, 0, &oplimit);
	if (err)
		return err;

	if (oplimit < 0 || oplimit > MEMBW_OPLIMIT_MAX)
		return -EINVAL;

	membw->membw_oplimit = weight;
	return nbytes;
}

static int membw_oplimit_show(struct seq_file *sf, void *v)
{
	struct cgroup_subsys_state *css = seq_css(sf);
	struct membw_cgroup *membw = css_membw(css);
	int64_t oplimit = membw->membw_oplimit;

	seq_printf(sf, "%lld\n", oplimit);

	return 0;
}

static ssize_t membw_interval_write(struct kernfs_open_file *of, char *buf,
			      size_t nbytes, loff_t off)
{
	struct cgroup_subsys_state *css = of_css(of);
	struct membw_cgroup *membw = css_membw(css);
	int64_t interval;
	int err;

	buf = strstrip(buf);

	err = kstrtoll(buf, 0, &interval);
	if (err)
		return err;

	interval = interval >> 6;

	if (interval < 0 || interval > MEMBW_INTERVAL_MAX)
		return -EINVAL;

	membw->membw_interval = interval;
	return nbytes;
}

static int membw_interval_show(struct seq_file *sf, void *v)
{
	struct cgroup_subsys_state *css = seq_css(sf);
	struct membw_cgroup *membw = css_membw(css);
	int64_t interval = membw->membw_interval;

	interval = interval << 6;
	seq_printf(sf, "%lld\n", interval);

	return 0;
}
static struct cftype membw_files[] = {
	{
		.name = "weight",
		.write = membw_weight_write,
		.seq_show = membw_weight_show,
		.flags = CFTYPE_NOT_ON_ROOT,
	},
	{
		.name = "oplimit",
		.write = membw_oplimit_write,
		.seq_show = membw_oplimit_show,
		.flags = CFTYPE_NOT_ON_ROOT,
	},
	{
		.name = "interval",
		.write = membw_interval_write,
		.seq_show = membw_interval_show,
		.flags = CFTYPE_NOT_ON_ROOT,
	},
	{ }	/* terminate */
};

struct cgroup_subsys membw_cgrp_subsys = {
	.css_alloc	= membw_css_alloc,
	.css_free	= membw_css_free,
	.free		= membw_free,
	.legacy_cftypes	= membw_files,
	.dfl_cftypes	= membw_files,
};
