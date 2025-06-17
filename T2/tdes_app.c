#include "tdes_driver.h"

struct tdes_data_s tdes_data;

const struct device_s tdes_dev = {
	.name = "TDES_HW",         
	.data = &tdes_data,        
	.api = &tdes_api           
};

// Chaves de 64 bits para uso com o 3DES (3 chaves)
const uint32_t key1[2] = {0x01234567, 0x89101112};
const uint32_t key2[2] = {0x12345678, 0x87654321};
const uint32_t key3[2] = {0xabcdef01, 0x1a2b3c4d};

// Vetor de inicialização (para CBC) e nonce (para CTR)
const uint32_t iv[2]   = {0x11223344, 0x55667788};
const uint32_t nonce[2] = {0xaaaabbbb, 0xccccdddd};

// Função que executa o teste de criptografia/descriptografia em um modo
void run_test_cycle(const char* mode_name, tdes_op_mode_t mode, const char* original_msg, size_t msg_len) {
    void *write_packet = NULL;                           // Buffer para enviar ao driver
    const size_t config_size = sizeof(tdes_cfg_t);       // Tamanho da estrutura de configuração
    tdes_cfg_t config;                                   // Estrutura de configuração

    // Ajuste do tamanho para múltiplos de 8 bytes, se necessário (ECB/CBC exigem isso)
    size_t processed_len = msg_len;
    if (mode == ECB || mode == CBC) {
        processed_len = (msg_len + BLOCKLEN - 1) / BLOCKLEN * BLOCKLEN;
    }

    // Buffers para criptografia, descriptografia e impressão
    char crypto_buffer[processed_len];
    char final_buffer[processed_len];
    char print_buffer[processed_len];

    printf("\n\n====== Testando Modo: %s ======\n", mode_name);

    // Abre o dispositivo TDES
    if (dev_open(&tdes_dev, 0) != 0) {
        printf("ERRO: Falha ao abrir dispositivo para %s!\n", mode_name);
        return;
    }

    // Preenche estrutura de configuração para criptografar
    config.op_mode = mode;
    config.crypt_mode = ENCRYPT;                         // Define operação como ENCRYPT
    memcpy(config.key1, key1, sizeof(key1));             // Copia chave 1
    memcpy(config.key2, key2, sizeof(key2));             // Copia chave 2
    memcpy(config.key3, key3, sizeof(key3));             // Copia chave 3

    // Define vetor de inicialização ou nonce de acordo com o modo
    if (mode == CTR) {
        memcpy(config.iv, nonce, sizeof(nonce));
    } else {
        memcpy(config.iv, iv, sizeof(iv));
    }

    // Aloca memória para o pacote de escrita: config + mensagem
    write_packet = malloc(config_size + msg_len);
    if (!write_packet) {
        printf("ERRO: Falha na alocacao de memoria para criptografar!\n");
        dev_close(&tdes_dev);
        return;
    }

    // Copia dados da configuração + mensagem original no buffer
    memcpy(write_packet, &config, config_size);
    memcpy((char*)write_packet + config_size, original_msg, msg_len);

    // Escreve dados no driver (criptografação)
    dev_write(&tdes_dev, write_packet, config_size + msg_len);
    free(write_packet);  // Libera memória do pacote

    // Lê resultado da criptografia do driver
    dev_read(&tdes_dev, crypto_buffer, processed_len);
    printf("\nDados Criptografados (hexdump):\n");
    memset(print_buffer, 0, sizeof(print_buffer));
    memcpy(print_buffer, crypto_buffer, processed_len);
    hexdump(print_buffer, processed_len);  // Imprime como hexadecimal

    // Atualiza configuração para descriptografar
    config.crypt_mode = DECRYPT;
    if (mode == CTR) {
        memcpy(config.iv, nonce, sizeof(nonce));         // Redefine nonce
    } else {
        memcpy(config.iv, iv, sizeof(iv));               // Redefine IV
    }

    // Aloca novo pacote para enviar dados criptografados
    write_packet = malloc(config_size + processed_len);
    if (!write_packet) {
        printf("ERRO: Falha na alocacao de memoria para descriptografar!\n");
        dev_close(&tdes_dev);
        return;
    }

    // Prepara pacote: configuração + dados criptografados
    memcpy(write_packet, &config, config_size);
    memcpy((char*)write_packet + config_size, crypto_buffer, processed_len);

    // Envia pacote para descriptografar
    dev_write(&tdes_dev, write_packet, config_size + processed_len);
    free(write_packet);  // Libera memória do pacote

    // Lê resultado da descriptografia
    dev_read(&tdes_dev, final_buffer, processed_len);
    printf("\n\nDados Descriptografados:\n");
    memset(print_buffer, 0, sizeof(print_buffer));
    memcpy(print_buffer, final_buffer, processed_len);
    hexdump(print_buffer, processed_len);

    printf("\n\nDados Descriptografados: \"%s\"\n", final_buffer);

    if (memcmp(original_msg, final_buffer, msg_len) == 0) {
        printf("Verificacao: SUCESSO!\n");
    } else {
        printf("Verificacao: FALHA!\n");
    }

    dev_close(&tdes_dev);  // Fecha o driver
}

// Função principal da task de teste
void main_test_task(void) {
    char message[] = "TRABALHO PRATICO 2 DE SISTEMAS EMBARCADOS"; 

    printf("\n====== Teste Completo do Driver 3DES ======\n");
    printf("Mensagem Original: %s\n", message);
    hexdump(message, sizeof(message));

    // Executa os testes nos três modos
    run_test_cycle("ECB", ECB, message, sizeof(message));
    _delay_us(3); 

    run_test_cycle("CBC", CBC, message, sizeof(message));
    _delay_us(3); 

    run_test_cycle("CTR", CTR, message, sizeof(message));

    printf("\n====== Todos os testes foram finalizados. ======\n");

    krnl_panic(0); 
    for (;;);  
}

int32_t app_main(void) {
    ucx_task_spawn(main_test_task, DEFAULT_STACK_SIZE); 
    dev_init(&tdes_dev);                                

    return 1;
}
