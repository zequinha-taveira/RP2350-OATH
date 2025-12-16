# Guia de Integração do HMAC de Hardware (SHA-256)

Este documento detalha a implementação do **HMAC-SHA256** utilizando o acelerador de hardware dedicado do RP2350, conforme a **Fase 1** do *Roadmap* do projeto RP2350-OATH.

---

## 1. Importância do Acelerador de Hardware

O cálculo do HMAC (Hash-based Message Authentication Code) é o núcleo do algoritmo TOTP/HOTP. A utilização do hardware do RP2350 para esta tarefa oferece vantagens críticas:

| Vantagem | Descrição |
|---|---|
| **Performance** | O cálculo é significativamente mais rápido do que uma implementação puramente em software, garantindo que o token responda instantaneamente. |
| **Segurança** | O hardware é projetado para ser resistente a ataques de *side-channel* (como ataques de tempo), onde um invasor tenta inferir o segredo medindo o tempo de execução da função. |
| **Eficiência** | Reduz a carga de trabalho na CPU (Cortex-M33), liberando-a para outras tarefas. |

## 2. Integração com `libcotp`

A biblioteca `libcotp` foi configurada para usar um *backend* customizado para o HMAC. A função `whmac_sha256` em `src/crypto/hmac.c` atua como a ponte entre a `libcotp` e a API de hardware do Pico SDK.

### 2.1. Implementação `whmac_sha256`

A função `whmac_sha256` implementa o algoritmo HMAC (RFC 2104) em quatro passos, utilizando as funções `sha256_hw_init`, `sha256_hw_update` e `sha256_hw_final` do Pico SDK:

1.  **Normalização da Chave**: Se a chave secreta for maior que 64 bytes (tamanho do bloco SHA-256), ela é primeiro *hasheada* para 32 bytes.
2.  **Preparação das Chaves Padronizadas**: Criação das chaves `k_ipad` (Key XOR Inner Pad) e `k_opad` (Key XOR Outer Pad).
3.  **Hash Interno**: Calcula `H(K XOR ipad || data)`.
4.  **Hash Externo**: Calcula `H(K XOR opad || Hash Interno)`.

## 3. Próximos Passos na Fase 1

Com a funcionalidade principal do OATH (OTP, AES, CCID, HMAC) concluída, o último passo da Fase 1 é o **Secure Boot**.

1.  **Secure Boot**: Finalizar a integração do Secure Boot (que depende da assinatura do firmware).

---

**Referências:**

[1] Raspberry Pi. (n.d.). *Hardware APIs - SHA-256 Accelerator API*. Pico SDK Documentation.
[2] IETF. (1997). *RFC 2104: HMAC: Keyed-Hashing for Message Authentication*.
[3] raspberrypi. (n.d.). *pico-examples*. GitHub.
