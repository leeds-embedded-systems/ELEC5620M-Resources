#include "FPGA_PIO/FPGA_PIO.h"
#include "DE1SoC_Addresses/DE1SoC_Addresses.h"
#include "DE1SoC_LT24/DE1SoC_LT24.h"
FPGAPIOCtx_t* gpio1;
LT24Ctx_t* lt24;
FPGA_PIO_initialise(LSC_BASE_GPIO_JP1, LSC_CONFIG_GPIO, &gpio1);
LT24_initialise(&gpio1->gpio, LSC_BASE_LT24HWDATA, &lt24);
