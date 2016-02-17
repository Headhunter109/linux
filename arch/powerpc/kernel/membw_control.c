#include <asm/reg.h>
#include <linux/kernel.h>
#include <linux/ctype.h>
#include <linux/uaccess.h>
#include <linux/cgroup_membw.h>

#define SPRN_PMMAR		0x356
#define MEMBW_CRDT_WINDOW_SHIFT 32
#define MEMBW_CRDT_MAX		0x3FFUL
#define MEMBW_CRDT_64NS		MEMBW_CRDT_MAX

typedef struct pmmscr_reg {
	union {
		unsigned long pmmar_val;
		struct {
			unsigned long reserve		:18;
			/* 0000 => disabled 0001 => 64ns and so on */
			unsigned long mem_crdt_window	:14;
			/* no of ld st operations for e.g. for 3GHZ machine on an avg 24060 op */
			unsigned long mem_op_limit	:32;
		} high_low;
	};
} pmmar_reg_t;

static inline unsigned long get_pmmar(void)
{
	 return mfspr(SPRN_PMMAR);
}

static inline void set_pmmar(unsigned long val)
{
	mtspr(SPRN_PMMAR, val);
}

static inline void membw_load_value(struct membw_cgroup *membw)
{
	pmmar_reg_t pmmar_reg = {{0}};
	unsigned long weight;
	unsigned long oplimit;
	unsigned long interval;

	weight = membw->weight;
	oplimit = membw->oplimit;
	interval = membw->interval;


	if (weight) {
		pmmar_reg.high_low.mem_crdt_window = interval;
                pmmar_reg.high_low.mem_op_limit = (oplimit * weight) / 100;

	} else {
		 pmmar_reg.pmmar_val = 0;
	}
	set_pmmar(pmmar_reg.pmmar_val);
}

#ifdef CONFIG_CGROUP_MEMBW
void arch_membw_sched_in(struct membw_cgroup *membw)
{

	pmmar_reg_t pmmar_reg;

	pmmar_reg.pmmar_val = get_pmmar();
	membw_load_value(membw);
	pmmar_reg.pmmar_val = get_pmmar();

	barrier();
}
#else
void arch_membw_sched_in(struct membw_cgroup *membw) {}
#endif
EXPORT_SYMBOL(arch_membw_sched_in);

#ifdef CONFIG_CGROUP_MEMBW
void arch_membw_sched_out(void)
{
	static int r;
	int display_after = 0;
	pmmar_reg_t pmmar_reg = {{0}};

	pmmar_reg.pmmar_val = get_pmmar();
	membw_load_value(0);
	pmmar_reg.pmmar_val = get_pmmar();
	smp_mb();
}
#else
void arch_membw_sched_out(void) {}
#endif
EXPORT_SYMBOL(arch_membw_sched_out);
