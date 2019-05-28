#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/arm-smccc.h>
#include <linux/kallsyms.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nathan Castets");

#define ARM_NOP_INS 0xd503201f
#define ARM_SMC_INS 0xd4000003

static unsigned long addr = (unsigned long)__arm_smccc_smc;
static pte_t *ptep;

static void get_page_table_entry(void)
{
	struct mm_struct *mm =
		(struct mm_struct *)kallsyms_lookup_name("init_mm");
	struct mm_struct init_mm = *mm;
	pgd_t *pgdp;
	pud_t *pudp;
	pmd_t *pmdp;
	
	pgdp = pgd_offset_k(addr);
	if (pgd_none(READ_ONCE(*pgdp))) {
		printk(KERN_INFO "failed pgdp");
		return;
	}

	pudp = pud_offset(pgdp, addr);
	if (pud_none(READ_ONCE(*pudp))) {
		printk(KERN_INFO "failed pudp");
		return;
	}

	pmdp = pmd_offset(pudp, addr);
	if (pmd_none(READ_ONCE(*pmdp))) {
		printk(KERN_INFO "failed pmdp");
		return;
	}

	ptep = pte_offset_kernel(pmdp, addr);
	if (!pte_valid(READ_ONCE(*ptep))) {
		printk(KERN_INFO "failed pte");
		return;
	}
}

static void disable_smc_call(void)
{
	struct mm_struct *mm =
		(struct mm_struct *)kallsyms_lookup_name("init_mm");
	pte_t pte;

	if (!ptep)
		get_page_table_entry();
	
	pte = READ_ONCE(*ptep);

	/* check memory page permissions */
	if (!pte_write(pte)) {
		pte = clear_pte_bit(pte, __pgprot(PTE_RDONLY));
		pte = set_pte_bit(pte, __pgprot(PTE_WRITE));

		set_pte_at(mm, addr, ptep, pte);
			
	}
	*(uint32_t *)addr = ARM_NOP_INS;
	printk(KERN_INFO "0x%lx : 0x%08x", addr, *(uint32_t *)addr);
	
	pte = clear_pte_bit(pte, __pgprot(PTE_WRITE));
	pte = set_pte_bit(pte, __pgprot(PTE_RDONLY));
	
	set_pte_at(mm, addr, ptep, pte);
}

static void enable_smc_call(void)
{
	struct mm_struct *mm =
		(struct mm_struct *)kallsyms_lookup_name("init_mm");
	pte_t pte;
	
	if (!ptep)
		get_page_table_entry();
	
	pte = READ_ONCE(*ptep);

	/* check memory page permissions */
	if (!pte_write(pte)) {
		pte = clear_pte_bit(pte, __pgprot(PTE_RDONLY));
		pte = set_pte_bit(pte, __pgprot(PTE_WRITE));

		set_pte_at(mm, addr, ptep, pte);
			
	}

	*(uint32_t *)addr = ARM_SMC_INS;
	printk(KERN_INFO "0x%lx : 0x%08x", addr, *(uint32_t *)addr);
	
	pte = clear_pte_bit(pte, __pgprot(PTE_WRITE));
	pte = set_pte_bit(pte, __pgprot(PTE_RDONLY));
	
	set_pte_at(mm, addr, ptep, pte);
}

int init_module(void)
{
	printk(KERN_INFO "alpha module started");
	disable_smc_call();
	return 0;
}

void cleanup_module(void)
{
	enable_smc_call();
	printk(KERN_INFO "alpha stopped");
}
