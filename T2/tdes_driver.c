#include "tdes_driver.h"

//========================================
// Funções de criptografia e descriptografia de bloco
//========================================

// Executa criptografia de um bloco de 64 bits (2x32 bits)
static void des_encrypt_block(uint32_t msg[2]) {
    DES_IN0 = msg[0];
    DES_IN1 = msg[1];
    DES_CONTROL |= DES_LDDATA;         // Sinaliza carregamento de dados
    DES_CONTROL &= (~DES_LDDATA);
    _delay_us(1);                      // Espera para processamento
    msg[0] = DES_OUT0;                 // Captura resultado
    msg[1] = DES_OUT1;
}

// Executa descriptografia de um bloco de 64 bits (mesma lógica)
static void des_decrypt_block(uint32_t msg[2]) {
    DES_IN0 = msg[0];
    DES_IN1 = msg[1];
    DES_CONTROL |= DES_LDDATA;
    DES_CONTROL &= (~DES_LDDATA);
    _delay_us(1);
    msg[0] = DES_OUT0;
    msg[1] = DES_OUT1;
}

// Inicia o módulo para criptografia com chaves
static void encrypt_start(const uint32_t key1[2], const uint32_t key2[2], const uint32_t key3[2]) {
    DES_CONTROL = 0;
    DES_CONTROL |= DES_ENCRYPT;        // Define modo criptografar
    DES_CONTROL |= DES_RESET;
    DES_CONTROL &= (~DES_RESET);       // Reset de sincronização
    DES_KEY1_1 = key1[0]; DES_KEY1_2 = key1[1];
    DES_KEY2_1 = key2[0]; DES_KEY2_2 = key2[1];
    DES_KEY3_1 = key3[0]; DES_KEY3_2 = key3[1];
    DES_CONTROL |= DES_LDKEY;          // Carrega chaves
    DES_CONTROL &= (~DES_LDKEY);
}

// Inicia o módulo para descriptografia com chaves
static void decrypt_start(const uint32_t key1[2], const uint32_t key2[2], const uint32_t key3[2]) {
    DES_CONTROL = 0;
    DES_CONTROL |= DES_DECRYPT;
    DES_CONTROL |= DES_RESET;
    DES_CONTROL &= (~DES_RESET);
    DES_KEY1_1 = key1[0]; DES_KEY1_2 = key1[1];
    DES_KEY2_1 = key2[0]; DES_KEY2_2 = key2[1];
    DES_KEY3_1 = key3[0]; DES_KEY3_2 = key3[1];
    DES_CONTROL |= DES_LDKEY;
    DES_CONTROL &= (~DES_LDKEY);
}

//========================================
// Modo ECB (Electronic Codebook)
//========================================

// Criptografa em ECB, bloco a bloco
static void des_ecb_encrypt(uint8_t *out, uint8_t *in, uint32_t len, const uint32_t k1[2], const uint32_t k2[2], const uint32_t k3[2]) {
    encrypt_start(k1, k2, k3);
    for (uint32_t i = 0; i < len; i += BLOCKLEN) {
        uint32_t temp_block[2] = {0, 0};
        size_t bytes_to_copy = (len - i < BLOCKLEN) ? (len - i) : BLOCKLEN;
        memcpy(temp_block, in + i, bytes_to_copy);
        des_encrypt_block(temp_block);
        memcpy(out + i, temp_block, BLOCKLEN);
    }
}

// Descriptografa em ECB
static void des_ecb_decrypt(uint8_t *out, uint8_t *in, uint32_t len, const uint32_t k1[2], const uint32_t k2[2], const uint32_t k3[2]) {
    decrypt_start(k1, k2, k3);
    for (uint32_t i = 0; i < len; i += BLOCKLEN) {
        uint32_t temp_block[2] = {0, 0};
        memcpy(temp_block, in + i, BLOCKLEN);
        des_decrypt_block(temp_block);
        memcpy(out + i, temp_block, BLOCKLEN);
    }
}

//========================================
// Modo CBC (Cipher Block Chaining)
//========================================

// Criptografa usando CBC
static void des_cbc_encrypt(uint8_t *out, uint8_t *in, uint32_t len, const uint32_t k1[2],const uint32_t k2[2],const uint32_t k3[2], const uint32_t iv[2]) {
    uint32_t block[2], tiv[2];
    encrypt_start(k1, k2, k3);
    tiv[0] = iv[0]; 
	tiv[1] = iv[1];
    for (uint32_t i = 0; i < len; i += BLOCKLEN) {
        memset(block, 0, sizeof(block));
        size_t bytes_to_copy = (len - i < BLOCKLEN) ? (len - i) : BLOCKLEN;
        memcpy((char *)block, in + i, bytes_to_copy);
        block[0] ^= tiv[0]; 
		block[1] ^= tiv[1];
        des_encrypt_block(block);
        tiv[0] = block[0]; 
		tiv[1] = block[1];
        memcpy(out + i, (char *)block, BLOCKLEN);
    }
}

// Descriptografa usando CBC
static void des_cbc_decrypt(uint8_t *out, uint8_t *in, uint32_t len, const uint32_t k1[2],const uint32_t k2[2],const uint32_t k3[2], const uint32_t iv[2]) {
    uint32_t block[2], block2[2], tiv[2];
    decrypt_start(k1, k2, k3);
    tiv[0] = iv[0]; 
	tiv[1] = iv[1];
    for (uint32_t i = 0; i < len; i += BLOCKLEN) {
        memcpy((char *)block, in + i, BLOCKLEN);
        block2[0] = block[0]; 
		block2[1] = block[1];
        des_decrypt_block(block);
        block[0] ^= tiv[0]; 
		block[1] ^= tiv[1];
        tiv[0] = block2[0]; 
		tiv[1] = block2[1];
        memcpy(out + i, (char *)block, BLOCKLEN);
    }
}

//========================================
// Modo CTR (Counter)
//========================================

// Criptografa ou descriptografa com CTR (só criptografa bloco contador)
static void des_ctr_crypt(uint8_t *out, uint8_t *in, uint32_t len, const uint32_t k1[2], const uint32_t k2[2], const uint32_t k3[2], const uint32_t nonce[2]) {
    uint32_t counter[2], keystream[2];
    encrypt_start(k1, k2, k3);
    counter[0] = nonce[0]; counter[1] = nonce[1];
    for (uint32_t i = 0; i < len; i += BLOCKLEN) {
        keystream[0] = counter[0]; 
		keystream[1] = counter[1];
        des_encrypt_block(keystream);
        uint32_t data_block[2] = {0,0};
        size_t bytes_to_copy = (len - i < BLOCKLEN) ? (len - i) : BLOCKLEN;
        memcpy(data_block, in + i, bytes_to_copy);
        data_block[0] ^= keystream[0]; 
		data_block[1] ^= keystream[1];
        memcpy(out + i, data_block, bytes_to_copy);
        counter[1]++;
        if (counter[1] == 0) counter[0]++;
    }
}

//========================================
// API do Driver UCX/OS
//========================================

// Inicializa estrutura interna do driver
static int tdes_init(const struct device_s *dev) {
    struct tdes_data_s *pdata = (struct tdes_data_s *)dev->data;
    CRITICAL_ENTER();
    pdata->buf = NULL; pdata->size = 0; pdata->in_use = -1;
    CRITICAL_LEAVE();
    return 0;
}

// Abre o driver (reserva uso exclusivo)
static int tdes_open(const struct device_s *dev, int mode) {
    struct tdes_data_s *pdata = (struct tdes_data_s *)dev->data;
    int retval = 0;
    NOSCHED_ENTER();
    if (pdata->in_use == -1)
        pdata->in_use = ucx_task_id();
    else
        retval = -1;
    NOSCHED_LEAVE();
    return retval;
}

// Fecha o driver e libera buffer
static int tdes_close(const struct device_s *dev) {
    struct tdes_data_s *pdata = (struct tdes_data_s *)dev->data;
    NOSCHED_ENTER();
    if (pdata->in_use == ucx_task_id()) {
        if (pdata->buf) {
            free(pdata->buf);
            pdata->buf = NULL;
            pdata->size = 0;
        }
        pdata->in_use = -1;
    }
    NOSCHED_LEAVE();
    return 0;
}

// Lê dados do buffer criptografado/descriptografado
static size_t tdes_read(const struct device_s *dev, void *buf, size_t count) {
    struct tdes_data_s *pdata = (struct tdes_data_s *)dev->data;
    NOSCHED_ENTER();
    if (pdata->in_use != ucx_task_id() || !pdata->buf) {
        NOSCHED_LEAVE();
        return -1;
    }
    size_t bytes_to_copy = (count < pdata->size) ? count : pdata->size;
    if (bytes_to_copy > 0) {
        memcpy(buf, pdata->buf, bytes_to_copy);
        free(pdata->buf);
        pdata->buf = NULL;
        pdata->size = 0;
    }
    NOSCHED_LEAVE();
    return bytes_to_copy;
}

// Escreve dados para serem processados (cripto ou decripto)
static size_t tdes_write(const struct device_s *dev, void *buf, size_t count) {
    struct tdes_data_s *pdata = (struct tdes_data_s *)dev->data;
    const size_t config_size = sizeof(tdes_cfg_t);
    
    NOSCHED_ENTER();
    if (pdata->in_use != ucx_task_id() || pdata->buf != NULL || count < config_size) {
        NOSCHED_LEAVE();
        return -1;
    }

    tdes_cfg_t current_config;
    memcpy(&current_config, buf, config_size);

    uint8_t *in_data = (uint8_t*)buf + config_size;
    const size_t data_count = count - config_size;
    size_t output_size = data_count;

    if (current_config.op_mode == ECB || current_config.op_mode == CBC)
        output_size = (data_count + BLOCKLEN - 1) / BLOCKLEN * BLOCKLEN;

    pdata->buf = malloc(output_size);
    if (!pdata->buf) {
        printf("TDES_DRIVER: Erro de alocacao de memoria!\n");
        NOSCHED_LEAVE();
        return -1;
    }
    pdata->size = output_size;
    uint8_t *out_data = (uint8_t *)pdata->buf;

    switch (current_config.op_mode) {
		case ECB:
			if (current_config.crypt_mode == ENCRYPT)
				des_ecb_encrypt(out_data, in_data, data_count,
								current_config.key1, current_config.key2, current_config.key3);
			else
				des_ecb_decrypt(out_data, in_data, data_count,
								current_config.key1, current_config.key2, current_config.key3);
			break;

		case CBC:
			if (current_config.crypt_mode == ENCRYPT)
				des_cbc_encrypt(out_data, in_data, data_count,
								current_config.key1, current_config.key2, current_config.key3, current_config.iv);
			else
				des_cbc_decrypt(out_data, in_data, data_count,
								current_config.key1, current_config.key2, current_config.key3, current_config.iv);
			break;

		case CTR:
			des_ctr_crypt(out_data, in_data, data_count,
						current_config.key1, current_config.key2, current_config.key3, current_config.iv);
			break;

		default:
			printf("TDES_DRIVER: Modo de operacao invalido!\n");
			break;
	}

    NOSCHED_LEAVE();
    return data_count;
}

// Interface da API do driver UCX/OS
struct device_api_s tdes_api = {
    .dev_init = tdes_init,
    .dev_open = tdes_open,
    .dev_close = tdes_close,
    .dev_read = tdes_read,
    .dev_write = tdes_write,
};