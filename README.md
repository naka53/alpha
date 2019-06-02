# alpha
alpha is a simple kernel rootkit (Linux arm64) to attack the TrustZone. He erases the smc_handle function of the kernel to hook calls to the SMC instruction. It has been tested with OP-TEE but should work with any other TrustZone implementation.

### __arm_smccc_smc (arch/arm64/kernel/smccc-call.S)   
   
```   
   smc   #0
   ldr   x4, [sp]
   stp   x0, x1, [x4, #ARM_SMCCC_RES_X0_OFFS]
   stp   x2, x3, [x4, #ARM_SMCCC_RES_X2_OFFS]
   ldr   x4, [sp, #8]
   cbz   x4, 1f /* no quirk structure */
   ldr   x9, [x4, #ARM_SMCCC_QUIRK_ID_OFFS]
   cmp   x9, #ARM_SMCCC_QUIRK_QCOM_A6
   b.ne  1f
   str   x6, [x4, ARM_SMCCC_QUIRK_STATE_OFFS]
   ret
```   
   
### DoS attack   
   
It's the easiest way to avoid SMC call. Replace the first instruction of the function:   
```   
03 00 00 d4     smc   #0x0   
```   

With the following instruction:   
```   
d5 03 20 1f     nop      
```   

### MITM attack    

The purpose of this attack is to insert our routine function in __arm_smccc_smc to filter the SMC call. First step is to jump to our hook function:    
```

```

Next, the hook function have to manage the register to be able to continue to the SMC call:    
```

```

### References   

http://infocenter.arm.com/help/topic/com.arm.doc.den0028b/ARM_DEN0028B_SMC_Calling_Convention.pdf   
https://github.com/torvalds/linux/blob/master/include/linux/arm-smccc.h   
https://github.com/torvalds/linux/blob/master/arch/arm64/kernel/smccc-call.S   
