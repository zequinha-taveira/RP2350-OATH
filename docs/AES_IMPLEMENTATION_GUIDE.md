# Guia de Implementação da Criptografia AES-256

Este documento detalha a implementação da criptografia AES-256 para proteger as credenciais OATH armazenadas na memória flash, conforme a **Fase 1** do *Roadmap* do projeto RP2350-OATH.

---

## 1. Escolha da Criptografia

| Parâmetro | Valor | Justificativa |
|---|---|---|
| **Algoritmo** | **AES-256** | Padrão de segurança forte, amplamente aceito e resistente a ataques. |
| **Modo de Operação** | **CBC (Cipher Block Chaining)** | Modo comum para criptografia de dados em repouso. Exige um IV (Initialization Vector) único para cada bloco. |
| **Biblioteca** | **mbedTLS** | Integrada ao Pico SDK, leve, otimizada para sistemas embarcados e suporta AES-256 CBC. |
| **Chave** | **Master Key (256 bits)** | Chave única por dispositivo, lida da memória OTP (One-Time Programmable). |

## 2. Estrutura de Armazenamento de Credenciais (Flash)

A estrutura `oath_credential_t` foi atualizada para incluir o **IV (Initialization Vector)**, que é crucial para a segurança do modo CBC.

```c
typedef struct {
    uint8_t id[32];                 // Credential ID (e.g., issuer:user)
    uint8_t iv[AES_IV_SIZE_BYTES];  // NOVO: Initialization Vector (16 bytes)
    uint8_t encrypted_secret[32];   // Segredo criptografado (deve ser múltiplo de 16 bytes)
    uint32_t type;                  // TOTP ou HOTP
    uint32_t counter;               // Para HOTP
    uint32_t crc32;                 // Integridade (CRC32)
} oath_credential_t;
```

### 2.1. O Papel do IV (Initialization Vector)

- O IV é um valor aleatório de 16 bytes que é **gerado a cada vez que uma credencial é salva** (`oath_storage_save`).
- Ele é armazenado **junto** com a credencial, mas **não precisa ser secreto**.
- **Importância**: Garante que, mesmo que duas credenciais diferentes usem o mesmo segredo (o que não deve acontecer, mas é uma defesa), elas resultarão em textos cifrados completamente diferentes. Isso previne ataques de dicionário e reutilização de IV.

## 3. Implementação da Lógica de Criptografia (`src/oath/oath_storage.c`)

A lógica de armazenamento agora segue um fluxo seguro:

### 3.1. Inicialização (`oath_storage_init`)

A inicialização agora tenta ler a chave mestra da OTP. Se a leitura falhar (primeiro boot), ela gera uma nova chave aleatória usando o TRNG e a grava na OTP com soft-lock.

```c
void oath_storage_init(void) {
    // Tenta ler a chave mestra da OTP
    if (!otp_read_master_key(master_key)) {
        // Se falhar, gera e escreve uma nova chave (primeiro boot)
        if (!otp_write_new_master_key(master_key)) {
            // ... ERRO CRÍTICO ...
            return;
        }
    }
    // ... (continua a inicialização)
}
```

### 3.2. Salvamento (`oath_storage_save`)

1.  **Geração do IV**: Um novo IV de 16 bytes é gerado usando o TRNG (`get_rand_32()`).
2.  **Criptografia**: A função `aes_encrypt` (implementada em `src/crypto/aes.c` usando mbedTLS) é chamada:
    - **Chave**: `master_key` (da OTP)
    - **IV**: O IV recém-gerado
    - **Dados**: O segredo OATH (pré-preenchido e *padded* para 16 bytes)
3.  **Flash**: A estrutura completa (incluindo o IV e o segredo criptografado) é escrita na memória flash.

### 3.3. Carregamento (`oath_storage_load`)

1.  **Leitura da Flash**: A estrutura completa (incluindo o IV e o segredo criptografado) é lida da memória flash.
2.  **Descriptografia**: A função `aes_decrypt` é chamada:
    - **Chave**: `master_key` (da OTP)
    - **IV**: O IV lido da flash
    - **Dados**: O segredo criptografado lido da flash

## 4. Próximos Passos na Fase 1

Com a OTP e a Criptografia AES resolvidas, os próximos passos são:

1.  **Integração CCID**: Completar a lógica de parsing de mensagens CCID em `src/usb/ccid_device.c`.
2.  **Secure Boot**: Finalizar a integração do Secure Boot (que depende da assinatura do firmware).

---

**Referências:**

[1] Mbed TLS. (n.d.). *Encrypt data with AES-CBC mode*. Retrieved from https://mbed-tls.readthedocs.io/en/latest/kb/how-to/encrypt-with-aes-cbc/
[2] Raspberry Pi. (n.d.). *Pico SDK Documentation* (mbedTLS integration).
[3] Raspberry Pi. (n.d.). *Pico SDK Documentation* (TRNG access).
