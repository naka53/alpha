# alpha
alpha is a simple linux kernel module (arm64) to DoS attack the TrustZone. He erases the smc_handle function of the kernel to block calls to the TrustZone.   

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
41 00 00 54     b.ne 0x00000028   
86 04 00 f9     str   x6, [x4,#8]   
c0 03 5f d6     ret
```
