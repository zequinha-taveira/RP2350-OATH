# Guia de Implementação de Segurança do RP2350-OATH

Este documento detalha a arquitetura de segurança do firmware RP2350-OATH, aproveitando os recursos de hardware do microcontrolador RP2350.

---

## 1. Arquitetura de Segurança em Camadas

A segurança do token é construída em três pilares principais, que serão implementados nas Fases 1 e 2 do roadmap:

| Pilar | Recurso do RP2350 | Implementação (Fase) |
|---|---|---|
| **Integridade** | Secure Boot (BootROM) | Assinatura de Firmware (Fase 1) |
| **Confidencialidade** | OTP (One-Time Programmable) | Chave Mestra de Criptografia (Fase 1) |
| **Isolamento** | Arm TrustZone | Separação Secure/Non-Secure (Fase 2) |

## 2. Inicialização Segura (Secure Boot)

O Secure Boot é o mecanismo que garante a integridade e autenticidade do firmware. No RP2350, ele é imposto pela ROM on-chip e funciona verificando a assinatura do firmware antes de permitir sua execução.

### 2.1. Processo de Assinatura

O processo de Secure Boot requer um par de chaves ECDSA P-256. O hash da chave pública é gravado na OTP do RP2350, e a chave privada é usada para assinar o firmware.

1.  **Geração de Chaves**: Um par de chaves ECDSA P-256 é gerado.
2.  **Injeção da Chave Pública**: O hash da chave pública é gravado em uma área específica da OTP (por exemplo, `CRIT1.SECURE_BOOT_PUBKEY_HASH`). **Este é um passo irreversível e deve ser feito com cuidado.**
3.  **Assinatura do Firmware**: O binário do firmware (`.bin`) é assinado usando a chave privada. A assinatura e a chave pública são anexadas ao final do binário.

O script `tools/sign_firmware.py` simula este processo de assinatura. Ele calcula o SHA-256 do firmware e anexa uma assinatura ECDSA P-256 simulada, resultando em um arquivo `firmware_signed.bin`.

### 2.2. Verificação pela BootROM

Ao ligar, a BootROM do RP2350:
1.  Lê o hash da chave pública da OTP.
2.  Lê a chave pública e a assinatura anexadas ao firmware.
3.  Verifica se a assinatura é válida para o firmware usando a chave pública.
4.  Se a verificação for bem-sucedida, o firmware é executado. Caso contrário, a execução é abortada.

## 3. Armazenamento Seguro de Chaves (OTP)

A chave mestra de criptografia (AES-256) é armazenada na OTP.

- **Implementação**: `src/security/security.c`
- **Mecanismo**: A chave é gravada uma única vez e o **Soft-Lock** é aplicado para impedir a leitura por software não autorizado.
- **Geração**: A chave é gerada usando o **TRNG (True Random Number Generator)** de hardware do RP2350.

## 4. Aceleração de Hardware (HMAC)

O RP2350 possui um acelerador SHA-256 dedicado, que é utilizado para o cálculo do HMAC (Hash-based Message Authentication Code) no algoritmo TOTP/HOTP.

- **Implementação**: `src/crypto/hmac.c`
- **Benefício**: Garante alta performance e resistência a ataques de tempo.

## 5. Criptografia de Credenciais (AES-256)

As credenciais OATH (segredos) são armazenadas na memória Flash criptografadas.

- **Implementação**: `src/crypto/aes.c` e `src/oath/oath_storage.c`
- **Mecanismo**: AES-256 CBC, utilizando a chave mestra da OTP e um IV (Initialization Vector) aleatório gerado pelo TRNG.

## 6. Isolamento de Domínio (TrustZone)

A **Fase 2** do roadmap se concentrará na separação do código em domínios Secure e Non-Secure usando o Arm TrustZone, isolando o núcleo criptográfico (OATH, AES, HMAC) da interface USB (CCID).

---

## 7. Estrutura de Arquivos de Segurança

| Arquivo | Função |
|---|---|
| `src/security/security.c` | Funções de baixo nível para OTP e TRNG. |
| `src/crypto/aes.c` | Criptografia AES-256 (mbedTLS). |
| `src/crypto/hmac.c` | HMAC-SHA256 acelerado por hardware. |
| `tools/sign_firmware.py` | Script de simulação de assinatura de firmware. |
| `docs/OTP_IMPLEMENTATION_GUIDE.md` | Detalhes da implementação da OTP. |
| `docs/AES_IMPLEMENTATION_GUIDE.md` | Detalhes da implementação da AES. |
| `docs/HMAC_HARDWARE_GUIDE.md` | Detalhes da implementação do HMAC de hardware. |
