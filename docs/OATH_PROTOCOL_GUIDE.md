# Guia de Implementação do Protocolo OATH (Application Protocol Data Unit)

Este documento detalha a implementação do protocolo OATH em `src/oath/oath_protocol.c`, que é a camada de aplicação que processa os comandos enviados pelo **Yubico Authenticator** via CCID.

---

## 1. Visão Geral do APDU e OATH

O protocolo OATH da Yubico é implementado sobre o padrão **ISO 7816-4** de Smart Cards, utilizando **APDUs (Application Protocol Data Units)**.

| Componente | Padrão | Descrição |
|---|---|---|
| **Transporte** | USB CCID | Camada de comunicação (implementada em `ccid_device.c`). |
| **Protocolo** | ISO 7816-4 | Define a estrutura do APDU (CLA, INS, P1, P2, Lc, Data, Le). |
| **Aplicação** | Yubico OATH | Define os comandos específicos (SELECT, CALCULATE, LIST). |

## 2. Estrutura do APDU

A função `oath_handle_apdu` é o ponto de entrada para todos os comandos APDU. Ela verifica o `CLA` (Classe) e o `INS` (Instrução) para despachar o comando para o handler correto.

| Campo | Posição | Valor Esperado (OATH) |
|---|---|---|
| **CLA** | 0 | `0x00` (Padrão ISO 7816) |
| **INS** | 1 | `0xA4` (SELECT) ou `0xA1` (CALCULATE/LIST) |
| **P1, P2** | 2, 3 | Parâmetros da instrução. |
| **Lc** | 4 | Comprimento do campo de dados. |
| **Data** | 5+ | Dados do comando (ex: AID, Tag OATH). |
| **Le** | (Final) | Comprimento esperado da resposta. |

## 3. Implementação dos Comandos Chave

### 3.1. `INS_SELECT` (`0xA4`)

- **Função**: Selecionar a aplicação OATH. O Yubico Authenticator envia o **AID (Application Identifier)** da aplicação OATH.
- **Implementação (`handle_select_apdu`)**:
    1.  Verifica se o `P1` é `0x04` (Seleção por AID) e `P2` é `0x00`.
    2.  Compara o campo `Data` com o AID OATH esperado (`0xA0000005272001`).
    3.  Se a comparação for bem-sucedida, a flag `oath_app_selected` é definida como `true` e o status `SW_OK` (`0x9000`) é retornado.

### 3.2. `INS_CALCULATE` (`0xA1`)

- **Função**: Este comando é usado para **CALCULATE** (gerar um código OTP) e **LIST** (listar credenciais). O Yubico OATH reutiliza o `INS=0xA1` e usa um **Tag** no campo de dados para diferenciar a operação.
- **Implementação (`handle_oath_command`)**:
    1.  Verifica se a aplicação OATH foi selecionada (`oath_app_selected`).
    2.  Lê o primeiro byte do campo `Data`, que é o **Tag OATH**.
    3.  **Tag `0x74` (OATH_TAG_CHALLENGE)**: Indica a solicitação de cálculo de um OTP.
        - **TODO**: Implementar a lógica de cálculo (carregar credencial, descriptografar, calcular TOTP/HOTP com `libcotp` e HMAC de hardware).
    4.  **Tag `0x79` (OATH_TAG_CREDENTIAL_LIST)**: Indica a solicitação da lista de credenciais.
        - **TODO**: Implementar a lógica de leitura do índice de credenciais da flash.

## 4. Próximos Passos na Fase 1

Com a estrutura de APDU e o despacho de comandos implementados, o próximo foco deve ser o **cálculo real do OTP**:

1.  **Finalizar `handle_oath_command`**: Preencher o `TODO` para o Tag `0x74` (CALCULATE), integrando a `libcotp` com o **HMAC de hardware** (implementação em `src/crypto/hmac.c`).
2.  **Secure Boot**: Finalizar a integração do Secure Boot.

---

**Referências:**

[1] Yubico. (n.d.). *OATH commands and APDUs*. Retrieved from https://docs.yubico.com/yesdk/users-manual/application-oath/oath-commands.html
[2] ISO/IEC 7816-4:2020. *Identification cards — Integrated circuit cards — Part 4: Organization, security and commands for interchange*.
