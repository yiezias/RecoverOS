#ifndef __KERNEL_GLOBAL_H
#define __KERNEL_GLOBAL_H



#define bool uint8_t
#define false 0
#define true 1

#define NULL (void *)0

#define CODE_SCT 8
#define DATA_SCT 0x10
#define SCT_U_CODE (0x30 + 3)
#define SCT_U_DATA (0x28 + 3)

#define default_prio 31

#define UNUSED __attribute__((unused))

#define DIV_ROUND_UP(X, STEP) (((X) + (STEP)-1) / (STEP))


#endif
