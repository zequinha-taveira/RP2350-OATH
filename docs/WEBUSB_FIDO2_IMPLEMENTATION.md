# WebUSB e FIDO2/U2F Implementation

Este documento descreve a implementação de WebUSB e FIDO2/U2F para o projeto RP2350-OATH.

## 🎯 Visão Geral

O firmware agora suporta três interfaces USB simultâneas:

1. **CCID** - Compatível com Yubico Authenticator (OATH)
2. **WebUSB** - Configuração avançada via navegador
3. **FIDO2/U2F** - Autenticação passwordless

## 🏗️ Arquitetura USB Composite

```
┌─────────────────────────────────────────────────────────┐
│              USB COMPOSITE DEVICE                       │
│  ┌──────────────┬──────────────┬──────────────┐         │
│  │   CCID       │   WebUSB     │   FIDO2      │         │
│  │  (0x0B)      │  (0xFF)      │  (0x03)      │         │
│  │  Smart Card  │  Vendor      │  HID         │         │
│  │  Interface   │  Specific    │  Interface   │         │
│  └──────┬───────┴──────┬───────┴──────┬───────┘         │
└─────────┼──────────────┼──────────────┼─────────────────┘
          │              │              │
    ┌─────▼──────┐ ┌────▼─────┐  ┌────▼─────┐
    │ Yubico Auth│ │ Web App  │  │ Browser  │
    │  App       │ │ Config   │  │ FIDO2    │
    └────────────┘ └──────────┘  └──────────┘
```

## 📋 Especificações Técnicas

### Device Descriptor
- **Vendor ID**: 0x1209 (USB Implementers Forum)
- **Product ID**: 0x4D41 ('MA' for RP2350-OATH)
- **USB Version**: 2.1 (suporte a WebUSB)
- **Configurations**: 1

### Interfaces

#### 1. CCID (Smart Card)
- **Class**: 0x0B (Smart Card)
- **Protocol**: T=0, T=1
- **Endpoints**: Bulk IN/OUT (64 bytes)
- **Uso**: Yubico Authenticator, gerenciamento de credenciais OATH

#### 2. WebUSB
- **Class**: 0xFF (Vendor Specific)
- **Endpoints**: Bulk IN/OUT (64 bytes)
- **Uso**: Configuração avançada via navegador
- **Protocolo**: Comandos customizados

#### 3. FIDO2/U2F
- **Class**: 0x03 (HID)
- **Usage Page**: 0xF1D0 (FIDO Alliance)
- **Endpoints**: Interrupt IN/OUT (64 bytes)
- **Uso**: Autenticação passwordless, U2F, CTAP2

## 🔧 Implementação WebUSB

### Comandos WebUSB

| Comando | ID | Descrição |
|---------|----|-----------|
| PING | 0x01 | Teste de conectividade |
| GET_INFO | 0x02 | Informações do dispositivo |
| GET_CONFIG | 0x03 | Obter configuração atual |
| SET_CONFIG | 0x04 | Definir configuração |
| RESET | 0x05 | Resetar dispositivo |

### Exemplo de Comunicação WebUSB

```javascript
// Conectar ao dispositivo WebUSB
const device = await navigator.usb.requestDevice({ filters: [{ vendorId: 0x1209 }] });
await device.open();
await device.selectConfiguration(1);
await device.claimInterface(1); // WebUSB Interface

// Enviar comando PING
const pingCommand = new Uint8Array([0x01]); // PING
await device.transferOut(3, pingCommand);

// Receber resposta
const response = await device.transferIn(3, 64);
const data = new Uint8Array(response.data);
console.log('Status:', data[1] === 0x00 ? 'OK' : 'Error');
```

## 🔐 Implementação FIDO2/U2F

### CTAP2 Commands

| Comando | ID | Descrição |
|---------|----|-----------|
| MAKE_CREDENTIAL | 0x01 | Criar nova credencial |
| GET_ASSERTION | 0x02 | Obter assinatura |
| GET_INFO | 0x04 | Informações do autenticador |
| CLIENT_PIN | 0x06 | Gerenciar PIN |
| RESET | 0x07 | Resetar dispositivo |

### Fluxo FIDO2

#### Registro (Make Credential)
1. Navegador envia comando MAKE_CREDENTIAL
2. Dispositivo solicita confirmação do usuário (botão)
3. Gera chave ECDSA P-256
4. Armazena credencial criptografada
5. Retorna credencial e assinatura

#### Autenticação (Get Assertion)
1. Navegador envia comando GET_ASSERTION
2. Dispositivo solicita confirmação do usuário
3. Recupera credencial
4. Gera assinatura
5. Retorna assinatura

## 📁 Arquivos Implementados

### Non-Secure World

```
non_secure_world/src/usb/
├── webusb_device.h      # Header WebUSB
├── webusb_device.c      # Implementação WebUSB
├── fido2_device.h       # Header FIDO2
├── fido2_device.c       # Implementação FIDO2
├── usb_composite.h      # Header composto
├── usb_composite.c      # Driver composto
└── ccid_device.c        # Atualizado (removido usbd_app_driver_get_cb)
```

### Configuração

```
non_secure_world/src/
├── usb_descriptors.c    # Atualizado (novos descritores)
├── tusb_config.h        # Atualizado (HID + Vendor)
└── main.c               # Atualizado (inicialização)
```

## 🚀 Uso

### WebUSB Configuration

1. **Acessar Web Interface**:
   ```bash
   # O dispositivo aparece como WebUSB
   # Acesse: https://localhost:3000 (ou URL configurada)
   ```

2. **Comandos Disponíveis**:
   - Listar credenciais
   - Adicionar credenciais manualmente
   - Configurar PIN
   - Sincronizar tempo
   - Backup/Restore

### FIDO2/U2F

1. **Registro em Site**:
   ```javascript
   // WebAuthn API
   const credential = await navigator.credentials.create({
     publicKey: {
       challenge: new Uint8Array(32),
       rp: { name: "Example Site", id: "example.com" },
       user: { id: new Uint8Array(16), name: "user@example.com" },
       pubKeyCredParams: [{ type: "public-key", alg: -7 }],
       timeout: 60000,
       authenticatorSelection: { userVerification: "preferred" }
     }
   });
   ```

2. **Autenticação**:
   ```javascript
   const assertion = await navigator.credentials.get({
     publicKey: {
       challenge: new Uint8Array(32),
       timeout: 60000,
       userVerification: "preferred"
     }
   });
   ```

## 🔒 Segurança

### WebUSB
- **Verificação de Origem**: Apenas URLs HTTPS permitidas
- **Rate Limiting**: Proteção contra brute force
- **Criptografia**: Comunicação criptografada via TLS

### FIDO2
- **User Presence**: Confirmação física necessária
- **User Verification**: Suporte a PIN
- **Attestation**: Chaves attestadas
- **Resident Keys**: Suporte a credenciais residentes

## 🧪 Testes

### WebUSB
```bash
# Testar com webusb-test.js
node webusb-test.js --vendor 0x1209 --product 0x4D41
```

### FIDO2
```bash
# Testar com fido2-test.js
node fido2-test.js --test all
```

## 📝 Notas de Implementação

1. **TinyUSB Multi-Driver**: O sistema usa `usbd_app_driver_get_cb` para múltiplos drivers
2. **Endpoint Allocation**: 
   - CCID: 0x02/0x82
   - WebUSB: 0x03/0x83  
   - FIDO2: 0x04/0x84
3. **Buffer Sizes**: Todos os endpoints usam 64 bytes (full speed)
4. **TrustZone**: Operações críticas permanecem no Secure World

## 🔧 Futuras Melhorias

- [ ] WebUSB: Suporte a WebCrypto API
- [ ] FIDO2: Suporte a bioenrollment
- [ ] WebUSB: WebSocket para notificações
- [ ] FIDO2: Suporte a CTAP2.1
- [ ] WebUSB: Interface gráfica completa

## 📚 Referências

- [WebUSB Specification](https://wicg.github.io/webusb/)
- [FIDO2 Specification](https://fidoalliance.org/specs/fido-v2.0-ps-20190130.html)
- [CTAP2 Protocol](https://fidoalliance.org/specs/fido-v2.0-ps-20190130.html)
- [TinyUSB Documentation](https://tinyusb.org/)

---

**Status**: ✅ Implementado
**Versão**: 2.0
**Data**: 2025-12-17