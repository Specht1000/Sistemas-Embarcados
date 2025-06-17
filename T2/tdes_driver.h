#ifndef TDES_DRIVER_H
#define TDES_DRIVER_H

#include <ucx.h>
#include <device.h>
#include "tdes_driver.h"

#define DES_BASE			0xe7000000
#define DES_CONTROL			(*(volatile uint32_t *)(DES_BASE + 0x000))
#define DES_KEY1_1			(*(volatile uint32_t *)(DES_BASE + 0x010))
#define DES_KEY1_2			(*(volatile uint32_t *)(DES_BASE + 0x020))
#define DES_KEY2_1			(*(volatile uint32_t *)(DES_BASE + 0x030))
#define DES_KEY2_2			(*(volatile uint32_t *)(DES_BASE + 0x040))
#define DES_KEY3_1			(*(volatile uint32_t *)(DES_BASE + 0x050))
#define DES_KEY3_2			(*(volatile uint32_t *)(DES_BASE + 0x060))
#define DES_IN0			    (*(volatile uint32_t *)(DES_BASE + 0x070))
#define DES_IN1			    (*(volatile uint32_t *)(DES_BASE + 0x080))
#define DES_OUT0			(*(volatile uint32_t *)(DES_BASE + 0x090))
#define DES_OUT1			(*(volatile uint32_t *)(DES_BASE + 0x0A0))

#define DES_ENCRYPT			(1 << 4)
#define DES_DECRYPT			(0 << 4)
#define DES_RESET			(1 << 3)
#define DES_LDKEY			(1 << 2)
#define DES_LDDATA			(1 << 1)
#define DES_READY			(1 << 0)
#define BLOCKLEN	        8

typedef enum {
    ECB,
    CBC,
    CTR
} tdes_op_mode_t;

typedef enum {
    ENCRYPT,
    DECRYPT
} tdes_crypt_mode_t;

typedef struct {
    tdes_op_mode_t op_mode;
    tdes_crypt_mode_t crypt_mode;
    uint32_t key1[2];
    uint32_t key2[2];
    uint32_t key3[2];
    uint32_t iv[2];
} tdes_cfg_t;


struct tdes_data_s {
	char *buf;
	size_t size;
	int in_use;
    tdes_cfg_t config;
};

extern struct device_api_s tdes_api;

#endif
