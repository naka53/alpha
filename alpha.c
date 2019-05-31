#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/arm-smccc.h>
#include <linux/mm.h>
#include <linux/kallsyms.h>
#include <asm/suspend.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nathan Castets");

#define ARM_NOP_INS 0xd503201f
#define ARM_SMC_INS 0xd4000003

static unsigned long smc_handler_addr = (unsigned long)__arm_smccc_smc;
static pte_t *ptep;
static struct mm_struct *mm;

static void set_pte_write(void)
{
	pte_t pte;

	pte = READ_ONCE(*ptep);
	
	pte = clear_pte_bit(pte, __pgprot(PTE_RDONLY));
	pte = set_pte_bit(pte, __pgprot(PTE_WRITE));
	
	set_pte_at(mm, smc_handler_addr, ptep, pte);
}

static void set_pte_rdonly(void)
{
	pte_t pte;

	pte = READ_ONCE(*ptep);
	
	pte = clear_pte_bit(pte, __pgprot(PTE_WRITE));
	pte = set_pte_bit(pte, __pgprot(PTE_RDONLY));
	
	set_pte_at(mm, smc_handler_addr, ptep, pte);
}

static void disable_smc_call(void)
{
	set_pte_write();
	
	*(uint32_t *)smc_handler_addr = ARM_NOP_INS;
	
	set_pte_rdonly();
}

static void enable_smc_call(void)
{
	set_pte_write();

	*(uint32_t *)smc_handler_addr = ARM_SMC_INS;
	
	set_pte_rdonly();
}

int init_module(void)
{
	struct mm_struct init_mm;
	pgd_t *pgdp;
	pud_t *pudp;
	pmd_t *pmdp;
	uint32_t i;
	uint8_t *target = (uint8_t *)arch_hibernation_header_restore;

	printk(KERN_INFO "alpha module started");

	printk(KERN_INFO "start addr %lx", target);
	for (i = 0; i < 1024; i += 8)
		printk(KERN_INFO "%02x %02x %02x %02x %02x %02x %02x %02x",
		       target[i],
		       target[i + 1],
		       target[i + 2],
		       target[i + 3],
		       target[i + 4],
		       target[i + 5],
		       target[i + 6],
		       target[i + 7]);
	
	mm = (struct mm_struct *)kallsyms_lookup_name("init_mm");
	init_mm = *mm;
	printk(KERN_INFO "ini_mm %lx", init_mm);
	pgdp = pgd_offset_k(smc_handler_addr);
	if (pgd_none(READ_ONCE(*pgdp))) {
		printk(KERN_INFO "failed pgdp");
		return 0;
	}
	
	pudp = pud_offset(pgdp, smc_handler_addr);
	if (pud_none(READ_ONCE(*pudp))) {
		printk(KERN_INFO "failed pudp");
		return 0;
	}
	
	pmdp = pmd_offset(pudp, smc_handler_addr);
	if (pmd_none(READ_ONCE(*pmdp))) {
		printk(KERN_INFO "failed pmdp");
		return 0;
	}
	
	ptep = pte_offset_kernel(pmdp, smc_handler_addr);
	if (!pte_valid(READ_ONCE(*ptep))) {
		printk(KERN_INFO "failed pte");
		return 0;
	}
	
	disable_smc_call();
	return 0;
}

void cleanup_module(void)
{
	//enable_smc_call();
	printk(KERN_INFO "alpha module stopped");
}
