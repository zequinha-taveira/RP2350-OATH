# Guia de Implementação - RP2350-OATH 2.0

## Visão Geral

Este documento descreve a implementação completa do firmware RP2350-OATH, um token de autenticação de dois fatores (2FA) compatível com Yubico Authenticator, baseado no microcontrolador RP2350.

## Arquitetura de Segurança

### TrustZone Implementation

O firmware utiliza a tecnologia Arm TrustZone do Cortex-M33 para isolar operações críticas:

```
Secure World (SW):
├── OATH Protocol Handler
├── Crypto Engine (HMAC-SHA256, AES-GCM)
├── Secure Storage (OTP + Flash Criptografada)
├── Time Synchronization
└── HID Keyboard Mode

Non-Secure World (NSW):
├── USB CCID Interface (TinyUSB)
├── APDU Parser
└── Secure Gateway
```

### Memória

- **Secure Flash**: 0x10000000 - 0x1003FFFF (256KB)
- **Secure RAM**: 0x20000000 - 0x2001FFFF (128KB)
- **Non-Secure Flash**: 0x10040000 - 0x1FFFFFFF (~1.75MB)
- **Non-Secure RAM**: 0x20020000 - 0x2007FFFF (~392KB)
- **NSC Region**: 0x1003F000 - 0x1003FFFF (4KB para Secure Gateway)

## Componentes Principais

### 1. OATH Protocol (RFC 6238)

#### Suporte a Algoritmos
- **TOTP**: Time-based One-Time Password (30s period, 6 digits)
- **HOTP**: HMAC-based One-Time Password (counter-based)
- **Algoritmos**: SHA1, SHA256, SHA512

#### Comandos APDU Suportados
| Comando | INS | Descrição |
|---------|-----|-----------|
| SELECT | 0xA4 | Selecionar aplicativo OATH |
| PUT | 0x01 | Adicionar credencial |
| DELETE | 0x02 | Remover credencial |
| LIST | 0xA1 | Listar credenciais |
| CALCULATE | 0xA1 | Calcular código |
| SET CODE | 0x03 | Definir PIN |
| VALIDATE | 0xA3 | Verificar PIN |
| RESET | 0x04 | Apagar todas |
| TIME SYNC | 0x05 | Sincronizar tempo (custom) |

### 2. Armazenamento Seguro

#### Estrutura de Dados
```c
typedef struct {
    uint32_t magic;           // 0xDEADBEEF
    uint32_t version;
    uint8_t access_code_hash[32]; // SHA-256 do PIN
    uint8_t access_code_set;
    encrypted_credential_t encrypted_creds[MAX_CREDENTIALS];
    bool slot_used[MAX_CREDENTIALS];
    uint8_t master_key_salt[16];
} oath_persist_t;
```

#### Criptografia AES-GCM
- **Chave**: 256 bits derivada da OTP
- **IV**: 12 bytes aleatórios por credencial
- **Tag**: 16 bytes para autenticação
- **Credencial**: Criptografada em bloco único

#### OTP Memory
- Armazena chave mestra de criptografia
- Soft-lock após escrita
- Inacessível por software não autorizado

### 3. Sincronização de Tempo

#### Mecanismo
- Recebe timestamp Unix via APDU (8 bytes big-endian)
- Armazena base timestamp + offset de boot
- Validação: `timestamp = base + (time_us_64() - boot_time) / 1000000`

#### Comando de Sincronização
```c
// APDU: CLA=00, INS=05, Lc=08, Data=Timestamp(8 bytes)
// Response: 0x01 (success) + SW_OK
```

### 4. HID Keyboard Mode

#### Funcionalidades
- **Modo Standalone**: Gera códigos sem software adicional
- **Botão Físico**: 
  - Curto: Gera código
  - Duplo: Próxima credencial
  - Longo (3s): Troca modo

#### Simulação de Digitação
- Emite códigos TOTP via HID
- Suporte a múltiplas credenciais
- Feedback visual via LED RGB

### 5. Drivers de Hardware

#### WS2812 RGB LED (GP22)
- **Verde**: Modo CCID pronto
- **Amarelo**: Sem sincronização de tempo
- **Azul**: Modo HID
- **Vermelho**: Erro/Timeout
- **Branco**: Processando

#### Botão Físico (GP21)
- Pull-up, ativo em baixo
- Debounce: 50ms
- Long press: 3 segundos

## Fluxo de Operação

### 1. Inicialização
```
BootROM → Secure World → OATH Init → Time Sync Init → Jump to NS
```

### 2. Modo CCID (Yubico Authenticator)
```
Host → USB CCID → APDU Parser → Secure Gateway → OATH Handler → Response
```

### 3. Modo HID (Standalone)
```
Botão → HID Handler → TOTP Calc → Keyboard Output → Code Typed
```

### 4. Sincronização de Tempo
```
Yubico App → Time Sync APDU → Secure World → Update Timestamp
```

## Implementação de Segurança

### 1. Secure Boot
- Chave ECDSA P-256 na OTP
- BootROM verifica assinatura
- Firmware não executado se inválido

### 2. Criptografia
- **Mestre**: Chave da OTP (256 bits)
- **Credenciais**: AES-GCM por credencial
- **PIN**: SHA-256 + salt

### 3. Isolamento
- Crypto no Secure World
- USB no Non-Secure World
- Comunicação via Secure Gateway

## Configuração do Build

### Pré-requisitos
- Raspberry Pi Pico SDK
- CMake ≥ 3.13
- ARM GCC
- TinyUSB

### Build Secure World
```bash
cd secure_world
mkdir build && cd build
cmake ..
make
```

### Build Non-Secure World
```bash
cd non_secure_world
mkdir build && cd build
cmake ..
make
```

### Flash
```bash
# Assinar firmware (ferramenta de assinatura)
python3 tools/sign_firmware.py secure_app.bin

# Gravar no RP2350
picotool load secure_app_signed.uf2
```

## Testes e Validação

### Compatibilidade com Yubico Authenticator
1. Conectar dispositivo via USB
2. Abrir Yubico Authenticator
3. Adicionar credencial (PUT)
4. Sincronizar tempo (TIME SYNC)
5. Gerar código (CALCULATE)
6. Validar código no site

### Modo HID
1. Conectar dispositivo
2. Pressionar botão curto
3. Verificar código gerado
4. Usar em campo de login

### Segurança
1. Verificar Secure Boot
2. Validar criptografia de storage
3. Testar proteção por PIN
4. Verificar isolamento TrustZone

## Roadmap de Desenvolvimento

### ✅ Fase 1: MVP com Segurança Essencial
- [x] Interface USB CCID com TinyUSB
- [x] Integração libcotp para TOTP
- [x] Armazenamento criptografado na flash
- [x] Chave mestra na OTP
- [x] Configuração Secure Boot

### ✅ Fase 2: TrustZone
- [x] Separação Secure/Non-Secure Worlds
- [x] Build system configurado
- [x] Linker scripts para isolamento
- [x] Secure Gateway implementado

### 🔄 Fase 3: Recursos Avançados
- [ ] Suporte completo a HOTP
- [ ] Política de toque (touch policy)
- [ ] WebUSB para configuração
- [ ] Backup/Restore via WebUSB
- [ ] Suporte a múltiplos slots

## Referências

- [YKOATH Protocol Specification](https://developers.yubico.com/OATH/YKOATH_Protocol.html)
- [RFC 6238 - TOTP](https://tools.ietf.org/html/rfc6238)
- [Arm TrustZone Documentation](https://developer.arm.com/architectures/security-architectures/trustzone)
- [RP2350 Datasheet](https://www.raspberrypi.com/documentation/microcontrollers/rp2350.html)
- [TinyUSB Documentation](https://docs.tinyusb.org/)

## Notas de Implementação

### Limitações Conhecidas
1. **Tempo**: Sem RTC hardware, depende de sincronização via USB
2. **Criptografia**: Implementação simulada (usar hardware crypto em produção)
3. **TrustZone**: Requer TF-M para implementação completa
4. **HID**: Simulação de digitação (requer integração com TinyUSB HID)

### Otimizações Futuras
1. Usar acelerador SHA-256 de hardware
2. Implementar TRNG real para IVs
3. Adicionar suporte a NFC (via I2C)
4. Implementar display OLED (via SPI)
5. Adicionar suporte a U2F/FIDO2