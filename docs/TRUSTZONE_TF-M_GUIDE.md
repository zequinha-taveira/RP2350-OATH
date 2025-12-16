# Guia de Implementação do TrustZone com TF-M

Este documento detalha a arquitetura de TrustZone para o RP2350-OATH, focando na integração com o **Trusted Firmware-M (TF-M)**, o framework de segurança padrão para microcontroladores Armv8-M (como o Cortex-M33 do RP2350).

---

## 1. Arquitetura TrustZone (Fase 2)

O objetivo da Fase 2 é refatorar o firmware em dois domínios isolados por hardware:

| Domínio | Localização do Código | Responsabilidades |
|---|---|---|
| **Secure World (SW)** | `secure_world/` | **Núcleo de Segurança**: OATH Protocol, Crypto Engine (AES, HMAC), Secure Storage (OTP, Flash Criptografada). |
| **Non-Secure World (NSW)** | `non_secure_world/` | **Interface Externa**: USB CCID Driver (TinyUSB), Comunicação Serial (stdio), Lógica de Inicialização. |

## 2. O Papel do Trusted Firmware-M (TF-M)

O TF-M é o *runtime* que gerencia o Secure World. Ele fornece:

- **Core**: Gerencia o boot, a inicialização e o isolamento de memória.
- **Secure Services**: Implementa serviços de segurança (ex: criptografia, armazenamento seguro) acessíveis pelo Non-Secure World.
- **Secure Gateway (Veneer)**: O mecanismo de comunicação segura entre o NSW e o SW.

### 2.1. Portabilidade para o RP2350

A integração do TF-M no RP2350 requer:

1.  **Configuração do Hardware**: Configurar o SAU (Secure Attribution Unit) e o PPC (Peripheral Protection Controller) para mapear corretamente as regiões de memória e periféricos para o SW e NSW.
2.  **Toolchain**: Compilar dois binários separados (um para o SW e um para o NSW).
3.  **Bootloader**: O TF-M atua como o bootloader, inicializando o SW e, em seguida, saltando para o ponto de entrada do NSW.

## 3. O Secure Gateway (SG)

O SG é a única forma de o Non-Secure World acessar as funções críticas do Secure World.

| Arquivo | Domínio | Função |
|---|---|---|
| `secure_gateway.h` | Compartilhado | Define os IDs das funções de serviço (ex: `SG_OATH_HANDLE_APDU`). |
| `secure_world/src/secure_gateway_s.c` | Secure World | **Handler**: Recebe a chamada do NSW, verifica a origem e despacha para a função OATH/Crypto correta. |
| `non_secure_world/src/secure_gateway.c` | Non-Secure World | **Stub**: Empacota os parâmetros e executa a chamada de *veneer* para o SW. |

### 3.1. Fluxo de Chamada APDU

1.  **NSW (USB)**: `ccid_device.c` recebe o APDU via USB.
2.  **NSW (Gateway)**: `ccid_device.c` chama `secure_gateway_oath_handle_apdu(apdu_in, ...)`
3.  **SW (Handler)**: `secure_world_handler` recebe a chamada com `func_id = SG_OATH_HANDLE_APDU`.
4.  **SW (OATH)**: `secure_world_handler` chama `oath_handle_apdu(apdu_in, ...)` (que está no SW).
5.  **SW (Resposta)**: O resultado (OTP) é copiado para o buffer de saída e retornado ao NSW.

## 4. Próximos Passos de Implementação

A implementação real do TF-M é complexa e envolve a configuração de *Makefiles* e *linker scripts* específicos. O esqueleto fornecido permite que você comece a refatorar o código existente:

1.  **Mover Código**: Mover o código de `oath_protocol.c`, `aes.c`, `hmac.c` e `security.c` para o diretório `secure_world/`.
2.  **Refatorar Chamadas**: Substituir todas as chamadas diretas a funções seguras no `non_secure_world/` por chamadas ao `secure_gateway.h`.
3.  **Integração TF-M**: Seguir a documentação oficial do TF-M para portar o projeto para o *framework* (o que exigirá a criação de *linker scripts* e *config files*).

---

**Referências:**

[1] Trusted Firmware-M. (n.d.). *Technical Documentation*.
[2] Arm. (n.d.). *Configuring TrustZone in your Cortex-M33 application*.
[3] trustedfirmware-m.readthedocs.io. (n.d.). *RP2350 Platform Port*.
