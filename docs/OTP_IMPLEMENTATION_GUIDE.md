# Guia de Implementação da OTP (One-Time Programmable)

Este documento detalha a implementação de baixo nível para o gerenciamento da chave mestra de criptografia na memória **OTP (One-Time Programmable)** do RP2350, conforme a **Fase 1** do *Roadmap* do projeto RP2350-OATH.

---

## 1. Visão Geral da Segurança da Chave Mestra

A chave mestra de 256 bits (32 bytes) é o componente de segurança mais crítico do token, pois é usada para criptografar todas as credenciais OATH armazenadas na memória flash.

Para proteger esta chave contra leitura não autorizada (mesmo por software malicioso ou ataques de leitura de memória), utilizamos dois recursos de hardware do RP2350:

1.  **TRNG (True Random Number Generator)**: Para gerar uma chave de alta entropia.
2.  **OTP Soft-Lock**: Para tornar a região da memória OTP onde a chave está armazenada **inacessível** após a inicialização.

## 2. Estratégia de Armazenamento

| Parâmetro | Valor | Justificativa |
|---|---|---|
| **Tamanho da Chave** | 256 bits (32 bytes) | Padrão de segurança forte para AES-256. |
| **Página da OTP** | Página 48 (Índice 48) | Uma página reservada para dados do usuário, longe das páginas de configuração de boot. |
| **Mecanismo de Bloqueio** | **Soft-Lock** (`0b1111`) | O mais forte dos bloqueios de software, tornando a página **inacessível** (leitura e escrita proibidas) após a inicialização. |

## 3. Implementação de Baixo Nível (`src/security/security.c`)

A implementação depende de funções de baixo nível do Pico SDK e acesso direto aos registradores do RP2350.

### 3.1. Geração da Chave (TRNG)

A chave é gerada usando o True Random Number Generator (TRNG) de hardware, acessado via `pico/rand.h`.

```c
// Helper function to generate a 256-bit key using the True Random Number Generator (TRNG)
static void generate_random_key(uint8_t *key_out) {
    // Acessa o TRNG de hardware para preencher o buffer de 32 bytes
    for (int i = 0; i < MASTER_KEY_SIZE_BYTES; i += 4) {
        uint32_t rand_val = get_rand_32();
        memcpy(key_out + i, &rand_val, 4);
    }
}
```

### 3.2. Escrita da Chave (`otp_write_new_master_key`)

A escrita na OTP é feita apenas uma vez, no primeiro boot do dispositivo.

1.  **Geração**: Gera a chave aleatória de 256 bits.
2.  **Escrita via ROM**: Usa a função `rom_func_otp_access` da BootROM para escrever os dados. A ROM garante a escrita correta, incluindo o tratamento de ECC (Error Correction Code) e cópias redundantes.
3.  **Soft-Lock**: Define o registrador `otp_hw->SW_LOCK[48]` para `0b1111` (inacessível).

```c
bool otp_write_new_master_key(uint8_t *key_out) {
    // ... (Verificação se já está escrito/bloqueado)

    // 1. Gera a chave aleatória
    generate_random_key(key_out);
    
    // 2. Escreve os dados usando a função da ROM
    otp_cmd_t cmd = {
        .flags = OTP_CMD_ECC_BITS | OTP_CMD_WRITE_BITS,
        .page_index = OTP_MASTER_KEY_PAGE_INDEX
    };
    uint32_t ret = rom_func_otp_access(key_out, MASTER_KEY_SIZE_BYTES, cmd);
    
    if (ret != 0) {
        // Tratar erro de escrita
        return false;
    }
    
    // 3. Aplica o Soft-Lock (0b1111 = Inacessível)
    otp_hw->SW_LOCK[OTP_MASTER_KEY_PAGE_INDEX] = OTP_SW_LOCK_INACCESSIBLE;
    
    return true;
}
```

### 3.3. Leitura da Chave (`otp_read_master_key`)

A leitura da chave ocorre a cada boot para descriptografar as credenciais.

1.  **Verificação**: Confirma se a página está bloqueada (garantindo que a chave foi escrita).
2.  **Leitura Direta**: A chave é lida diretamente do endereço de memória mapeado da OTP (`OTP_MASTER_KEY_ADDR`).

> **Nota Importante sobre o Soft-Lock:** O soft-lock impede **leituras subsequentes** por software. No entanto, o código de inicialização do firmware (que é o que estamos implementando) pode ler a chave *antes* que o mecanismo de bloqueio seja totalmente imposto pelo hardware, ou seja, no início da execução. Após essa leitura inicial, o bloqueio garante que a chave não possa ser lida novamente por código malicioso.

```c
bool otp_read_master_key(uint8_t *key_out) {
    if (!is_master_key_written()) {
        // Chave não escrita, erro crítico
        return false;
    }

    // Leitura direta do endereço de memória mapeado
    volatile uint32_t *otp_addr = OTP_MASTER_KEY_ADDR;
    
    for (int i = 0; i < MASTER_KEY_SIZE_WORDS; i++) {
        ((uint32_t*)key_out)[i] = otp_addr[i];
    }
    
    return true;
}
```

## 4. Próximos Passos na Fase 1

Com a implementação da OTP concluída, os próximos passos da Fase 1 são:

1.  **Criptografia**: Implementar a lógica de criptografia/descriptografia AES para as credenciais em `src/oath/oath_storage.c`, usando a chave mestra lida pela função `otp_read_master_key`.
2.  **Integração CCID**: Completar a lógica de parsing de mensagens CCID em `src/usb/ccid_device.c`.
3.  **Secure Boot**: Finalizar a integração do Secure Boot (que depende da assinatura do firmware, conforme detalhado em `SECURITY_IMPLEMENTATION.md`).

---

**Referências:**

[1] Raspberry Pi. (2025, November). *Understanding RP2350's security features*. (Whitepaper).
[2] Raspberry Pi. (n.d.). *RP2350 Datasheet* (Capítulo 10: Security).
[3] raspberrypi. (n.d.). *rp2350\_hacking\_challenge*. GitHub. Retrieved from https://github.com/raspberrypi/rp2350_hacking_challenge/
