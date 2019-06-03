#include <linux/arm-smccc.h>
#include <linux/kernel.h>
#include <linux/kallsyms.h>
#include <linux/mm.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nathan Castets");

static pte_t *ptep;
static struct mm_struct *mm;
static uint8_t buff[16];

asmlinkage void hook(unsigned long a0, unsigned long a1, unsigned long a2,
		     unsigned long a3, unsigned long a4, unsigned long a5,
		     unsigned long a6, unsigned long a7,
		     struct arm_smccc_res *res, struct arm_smccc_quirk *quirk)
{
	/* pre-routine to save registers */
	__asm__("stp   x0, x1, [sp, #-16]!;"
		"stp   x2, x3, [sp, #-16]!;"
		"stp   x4, x5, [sp, #-16]!;"
		"stp   x6, x7, [sp, #-16]!;"
		"stp   x18, x19, [sp, #-16]!;"
		"stp   x20, x21, [sp, #-16]!;"
		"stp   x22, x23, [sp, #-16]!;"
		"stp   x24, x25, [sp, #-16]!;"
		"stp   x26, x27, [sp, #-16]!;"
		"stp   x28, x29, [sp, #-16]!;"
		"str   x30, [sp, #-8]!;");
	
	/* hook function body */
	printk(KERN_INFO "SMC call hooked!");
	
	/* post-routine to get back registers */
	__asm__("ldr   x30, [sp], #8;"
		"ldp   x28, x29, [sp], #16;"
		"ldp   x26, x27, [sp], #16;"
		"ldp   x24, x25, [sp], #16;"
		"ldp   x22, x23, [sp], #16;"
		"ldp   x20, x21, [sp], #16;"
		"ldp   x18, x19, [sp], #16;"
		"ldp   x6, x7, [sp], #16;"
		"ldp   x4, x5, [sp], #16;"
		"ldp   x2, x3, [sp], #16;"
		"ldp   x0, x1, [sp], #16;"
		"ldp   x29, x30, [sp], #16;");
	
	/* __arm_smccc_smc function */
	__asm__("smc   #0;"
		"ldr   x4, [sp];"
		"stp   x0, x1, [x4];"
		"stp   x2, x3, [x4, #16];"
		"ldr   x4, [sp, #8];"
		"cbz   x4, .+20;"
		"ldr   x9, [x4];"
		"cmp   x9, #0x1;"
		"b.ne  .+8;"
		"str   x6, [x4, #8];"
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
	uint32_t i;
	/*
	hook(1, 2, 3, 4, 5, 6, 7, 8,
	     (struct arm_smccc_res *)0x0011223344556677,
	     (struct arm_smccc_quirk *)0x8844556600332299);
	*/
	set_pte_write();
		 
	for (i = 0; i < 16; i++)
		buff[i] = *(uint8_t *)(__arm_smccc_smc + i);
	
	*(uint32_t *)(__arm_smccc_smc + 0) = 0x58000048;
	*(uint32_t *)(__arm_smccc_smc + 4) = 0xd61f0100;
	*(uint64_t *)(__arm_smccc_smc + 8) = (unsigned long)hook;
	
	set_pte_rdonly();	
}

static void enable_smc_call(void)
{
	uint32_t i;
	
	set_pte_write();
	
	for (i = 0; i < 16; i++)
		*(uint8_t *)(__arm_smccc_smc + i) = buff[i];
	
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
