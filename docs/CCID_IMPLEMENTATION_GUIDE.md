# Guia de Implementação do Protocolo CCID (Chip Card Interface Device)

Este documento detalha a implementação do protocolo CCID em `src/usb/ccid_device.c`, que é essencial para que o RP2350-OATH seja reconhecido como um leitor de Smart Card pelo sistema operacional e, consequentemente, pelo **Yubico Authenticator**.

---

## 1. Visão Geral do CCID

O CCID é um protocolo USB de classe de dispositivo que permite a comunicação entre um host (PC) e um Smart Card (neste caso, o firmware OATH emulado) através de um leitor de cartão (o próprio RP2350).

O fluxo de comunicação é baseado em mensagens de comando (`PC_to_RDR_*`) e mensagens de resposta (`RDR_to_PC_*`).

## 2. Estrutura de Mensagens CCID

Todas as mensagens CCID possuem um cabeçalho de 10 bytes (`ccid_msg_header_t`) que contém informações cruciais para o controle de fluxo:

| Campo | Tamanho (Bytes) | Descrição |
|---|---|---|
| `bMessageType` | 1 | Identifica o tipo de comando (ex: `0x6F` para `PC_TO_RDR_XFRBLOCK`). |
| `dwLength` | 4 | Comprimento dos dados que seguem o cabeçalho. |
| `bSlot` | 1 | Número do slot (sempre 0 neste projeto de slot único). |
| `bSeq` | 1 | Número de sequência para garantir a ordem das mensagens. |
| `bSpecific_0..3` | 3 | Parâmetros específicos do comando. |

## 3. Fluxo de Mensagens Implementado (`ccid_device.c`)

A função `tud_ccid_rx_cb` do TinyUSB recebe o buffer de dados e o passa para a função principal `ccid_message_handler`, que faz o *parsing* e o *dispatch* dos comandos.

### 3.1. `PC_TO_RDR_ICCPOWERON` (0x62)

- **Função**: Solicita que o Smart Card seja ligado (emulado).
- **Resposta**: `RDR_TO_PC_SLOTSTATUS` (0x81).
- **Status**: O campo `bStatus` é definido como `SLOT_STATUS_ICC_PRESENT` (0x00), indicando que o cartão (o firmware OATH) está pronto.

### 3.2. `PC_TO_RDR_ICCPOWEROFF` (0x63)

- **Função**: Solicita que o Smart Card seja desligado (emulado).
- **Resposta**: `RDR_TO_PC_SLOTSTATUS` (0x81).
- **Status**: O campo `bStatus` é definido como `SLOT_STATUS_ICC_ABSENT` (0x01).

### 3.3. `PC_TO_RDR_GETSLOTSTATUS` (0x65)

- **Função**: Solicita o status atual do slot.
- **Resposta**: `RDR_TO_PC_SLOTSTATUS` (0x81) com `SLOT_STATUS_ICC_PRESENT`.

### 3.4. `PC_TO_RDR_XFRBLOCK` (0x6F) - O Comando Principal

- **Função**: Este é o comando mais importante, pois carrega o **APDU (Application Protocol Data Unit)**, que contém os comandos OATH reais (SELECT, CALCULATE, etc.).
- **Extração**: O APDU é extraído do buffer de dados, começando logo após o cabeçalho CCID.
- **Dispatch**: O APDU é enviado para a função `oath_handle_apdu` (em `src/oath/oath_protocol.c`) para processamento.
- **Resposta**: A resposta do APDU é encapsulada na mensagem `RDR_TO_PC_DATABLOCK` (0x80) e enviada de volta ao host.

## 4. Próximos Passos na Fase 1

Com a estrutura CCID implementada, o próximo passo é preencher a lógica de `oath_handle_apdu` para que o dispositivo possa, de fato, processar os comandos OATH.

1.  **OATH Protocol**: Implementar a lógica de APDU em `src/oath/oath_protocol.c` (SELECT, CALCULATE, etc.).
2.  **Secure Boot**: Finalizar a integração do Secure Boot.

---

**Referências:**

[1] USB Implementers Forum. (2005). *Smart Card CCID Specification for Integrated Circuit(s) Cards Interface Devices*.
[2] TinyUSB Project. (n.d.). *CCID Device Class Implementation*.
