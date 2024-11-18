LT24Ctx_t* lt24;
#define LSC_BASE_GPIO_JP1   ((unsigned char*)0xFF200060) // GPIO0 (JP1) for LT24
#define LSC_BASE_LT24HWDATA ((unsigned char*)0xFF200080) // LT24 HW Optimised Data
LT24_initialise(LSC_BASE_GPIO_JP1,LSC_BASE_LT24HWDATA,&lt24);
