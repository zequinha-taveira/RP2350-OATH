# Recursos e Orçamento do Projeto RP2350-OATH

## Visão Geral

Este documento detalha os recursos necessários e o orçamento estimado para o desenvolvimento, teste, validação e entrega do projeto RP2350-OATH.

---

## 💼 Estrutura de Custos

### 1. Recursos Humanos (60% do orçamento)
### 2. Hardware e Equipamentos (30% do orçamento)
### 3. Software e Serviços (10% do orçamento)

---

## 👥 Recursos Humanos

### 4.1 Equipe Principal (Dedicada)

#### 4.1.1 Engenheiro Senior Firmware
```yaml
Cargo: Engenheiro Senior Firmware
Senioridade: 8+ anos de experiência
Alocacao: 100% (40h/semana)
Duracao: 12 semanas
Competencias:
  - RP2350/Pico SDK
  - C avançado
  - Segurança embarcada
  - USB/CCID/FIDO2
  - TrustZone
Custo_Hora: R$ 120,00
Custo_Total: R$ 57.600,00
```

#### 4.1.2 Engenheiro de Testes/QA
```yaml
Cargo: Engenheiro de Testes/QA
Senioridade: 5+ anos de experiência
Alocacao: 75% (30h/semana)
Duracao: 12 semanas
Competencias:
  - Testes de hardware
  - Protocolos de segurança
  - Automação de testes
  - CI/CD
Custo_Hora: R$ 90,00
Custo_Total: R$ 32.400,00
```

#### 4.1.3 Especialista em Segurança
```yaml
Cargo: Especialista em Segurança
Senioridade: 7+ anos de experiência
Alocacao: 50% (20h/semana)
Duracao: 12 semanas
Competencias:
  - Criptografia embarcada
  - Análise de vulnerabilidades
  - Testes de penetração
  - Segurança de hardware
Custo_Hora: R$ 110,00
Custo_Total: R$ 26.400,00
```

#### 4.1.4 Engenheiro de Integração CI/CD
```yaml
Cargo: Engenheiro de Integração
Senioridade: 5+ anos de experiência
Alocacao: 50% (20h/semana)
Duracao: 12 semanas
Competencias:
  - GitHub Actions
  - Automação de builds
  - Deploy e release
  - Monitoramento
Custo_Hora: R$ 95,00
Custo_Total: R$ 22.800,00
```

### 4.2 Equipe de Apoio (Consultoria)

#### 4.2.1 Especialista FIDO2/WebAuthn
```yaml
Cargo: Consultor FIDO2
Senioridade: 5+ anos de experiência
Alocacao: 25% (10h/semana)
Duracao: 8 semanas
Competencias:
  - WebAuthn specification
  - FIDO2 protocols
  - CTAP2.1
  - Browser compatibility
Custo_Hora: R$ 130,00
Custo_Total: R$ 20.800,00
```

#### 4.2.2 Especialista Yubico OATH
```yaml
Cargo: Consultor Yubico OATH
Senioridade: 5+ anos de experiência
Alocacao: 20% (8h/semana)
Duracao: 6 semanas
Competencias:
  - YKOATH protocol
  - Yubico Authenticator
  - Smart Card/CCID
  - OTP standards
Custo_Hora: R$ 125,00
Custo_Total: R$ 12.000,00
```

#### 4.2.3 Designer UX/UI
```yaml
Cargo: Designer UX/UI
Senioridade: 4+ anos de experiência
Alocacao: 25% (10h/semana)
Duracao: 4 semanas
Competencias:
  - Interfaces web
  - Experiência do usuário
  - Documentação visual
  - Prototipagem
Custo_Hora: R$ 85,00
Custo_Total: R$ 6.800,00
```

### 4.3 Gestão de Projeto

#### 4.3.1 Gerente de Projeto
```yaml
Cargo: Gerente de Projeto
Senioridade: 6+ anos de experiência
Alocacao: 30% (12h/semana)
Duracao: 12 semanas
Competencias:
  - Metodologias ágeis
  - Gestão de equipe
  - Controle de entregas
  - Comunicação stakeholders
Custo_Hora: R$ 100,00
Custo_Total: R$ 14.400,00
```

---

## 🛠️ Hardware e Equipamentos

### 5.1 Hardware de Desenvolvimento

#### 5.1.1 Microcontroladores
```yaml
RP2350-USB:
  Quantidade: 20 unidades
  Uso: Desenvolvimento + Testes
  Custo_Unitario: R$ 80,00
  Custo_Total: R$ 1.600,00

Raspberry Pi Pico 2:
  Quantidade: 10 unidades
  Uso: Compatibilidade/Backup
  Custo_Unitario: R$ 40,00
  Custo_Total: R$ 400,00

ESP32-S3:
  Quantidade: 5 unidades
  Uso: Comparação/Referência
  Custo_Unitario: R$ 60,00
  Custo_Total: R$ 300,00
```

#### 5.1.2 Estação de Trabalho
```yaml
CPU Principal:
  Processador: Intel i9-13900K
  RAM: 64GB DDR5
  Armazenamento: 2TB NVMe
  GPU: RTX 4070
  Custo_Total: R$ 18.000,00

CPU Secundária:
  Processador: AMD Ryzen 7 7700
  RAM: 32GB DDR5
  Armazenamento: 1TB NVMe
  GPU: Integrada
  Custo_Total: R$ 6.000,00
```

#### 5.1.3 Periféricos
```yaml
Monitores:
  Quantidade: 3 unidades
  Tamanho: 27"
  Resolução: 2560x1440
  Custo_Unitario: R$ 1.200,00
  Custo_Total: R$ 3.600,00

Teclados/Mouse:
  Quantidade: 3 kits
  Tipo: Profissional
  Custo_Unitario: R$ 400,00
  Custo_Total: R$ 1.200,00

Hubs USB:
  Quantidade: 5 unidades
  Tipo: USB 3.0, com alimentação
  Custo_Unitario: R$ 200,00
  Custo_Total: R$ 1.000,00
```

### 5.2 Equipamentos de Teste

#### 5.2.1 Analisadores Lógicos
```yaml
USB Protocol Analyzer:
  Modelo: Total Phase Beagle USB 12
  Função: Captura USB em tempo real
  Custo_Unitario: R$ 3.000,00
  Quantidade: 1
  Custo_Total: R$ 3.000,00

Logic Analyzer:
  Modelo: Saleae Logic Pro 16
  Canais: 16
  Frequência: 100MHz
  Custo_Unitario: R$ 2.500,00
  Quantidade: 1
  Custo_Total: R$ 2.500,00
```

#### 5.2.2 Instrumentos de Medição
```yaml
Osciloscópio:
  Modelo: Rigol DS1054Z
  Canais: 4
  Frequência: 50MHz
  Custo_Unitario: R$ 4.000,00
  Quantidade: 1
  Custo_Total: R$ 4.000,00

Multímetro:
  Modelo: Fluke 87V
  Tipo: True RMS
  Precisão: Alta
  Custo_Unitario: R$ 2.000,00
  Quantidade: 2
  Custo_Total: R$ 4.000,00

Fonte de Alimentação:
  Modelo: Rigol DP832
  Canais: 3
  Potência: 192W
  Custo_Unitario: R$ 3.500,00
  Quantidade: 1
  Custo_Total: R$ 3.500,00
```

#### 5.2.3 Medição de Consumo
```yaml
Medidor de Potência:
  Modelo: USB Power Monitor
  Precisão: ±1%
  Resolução: 1mA
  Custo_Unitario: R$ 300,00
  Quantidade: 3
  Custo_Total: R$ 900,00

Câmera Térmica:
  Modelo: FLIR C5
  Resolução: 80x60
  Precisão: ±3°C
  Custo_Unitario: R$ 3.000,00
  Quantidade: 1
  Custo_Total: R$ 3.000,00
```

### 5.3 Equipamentos de Laboratório

#### 5.3.1 Condições Ambientais
```yaml
Câmara Térmica:
  Tipo: Peltier
  Faixa: -20°C a +60°C
  Controle: Preciso
  Custo_Total: R$ 8.000,00

Gerador de Umidade:
  Tipo: Ultrassônico
  Controle: 10% a 90% RH
  Custo_Total: R$ 2.000,00

Fonte de Interferência:
  Tipo: EMI Generator
  Frequência: 1MHz a 1GHz
  Custo_Total: R$ 5.000,00
```

#### 5.3.2 Segurança Física
```yaml
Gaiola de Faraday:
  Tamanho: 1m x 1m x 1m
  Atenuação: > 60dB
  Custo_Total: R$ 3.000,00

Balança de Precisão:
  Capacidade: 200g
  Precisão: 0.001g
  Custo_Total: R$ 1.500,00

Lupa Digital:
  Aumento: 10x a 200x
  Câmera: 5MP
  Custo_Total: R$ 1.000,00
```

### 5.4 Dispositivos de Teste

#### 5.4.1 Smartphones/Tablets
```yaml
Android:
  Modelos: Samsung S23, Pixel 7
  Versões: Android 12, 13
  Quantidade: 3
  Custo_Total: R$ 6.000,00

iOS:
  Modelos: iPhone 14, iPad Air
  Versões: iOS 16, iPadOS 16
  Quantidade: 2
  Custo_Total: R$ 8.000,00
```

#### 5.4.2 Computadores de Teste
```yaml
Windows:
  Modelos: Dell XPS, Surface Pro
  Versões: Win 10, Win 11
  Quantidade: 3
  Custo_Total: R$ 9.000,00

macOS:
  Modelos: MacBook Air, iMac
  Versões: macOS 12, 13
  Quantidade: 2
  Custo_Total: R$ 12.000,00

Linux:
  Modelos: Notebook genérico
  Distribuições: Ubuntu, Fedora, Debian
  Quantidade: 2
  Custo_Total: R$ 3.000,00
```

---

## 💻 Software e Serviços

### 6.1 Licenças de Software

#### 6.1.1 IDEs e Ferramentas de Desenvolvimento
```yaml
Visual Studio Code:
  Tipo: Gratuito
  Extensões: Premium
  Custo_Total: R$ 0,00

CLion:
  Licença: Profissional
  Duração: 12 meses
  Custo_Total: R$ 2.400,00

Wireshark:
  Tipo: Gratuito
  Plugins: Profissionais
  Custo_Total: R$ 0,00

GitLab Ultimate:
  Tipo: Enterprise
  Duração: 12 meses
  Custo_Total: R$ 6.000,00
```

#### 6.1.2 Ferramentas de Teste
```yaml
Burp Suite Professional:
  Licença: Anual
  Custo_Total: R$ 8.000,00

OWASP ZAP:
  Tipo: Gratuito
  Plugins: Comerciais
  Custo_Total: R$ 1.000,00

JMeter:
  Tipo: Gratuito
  Plugins: Comerciais
  Custo_Total: R$ 2.000,00

Selenium Grid:
  Cloud: Sauce Labs
  Duração: 6 meses
  Custo_Total: R$ 3.000,00
```

#### 6.1.3 Segurança e Análise
```yaml
SonarQube:
  Licença: Developer
  Duração: 12 meses
  Custo_Total: R$ 4.000,00

Checkmarx:
  Licença: SAST
  Duração: 6 meses
  Custo_Total: R$ 10.000,00

OWASP Dependency Check:
  Tipo: Gratuito
  Suporte: Comercial
  Custo_Total: R$ 2.000,00
```

### 6.2 Serviços em Nuvem

#### 6.2.1 Infraestrutura
```yaml
AWS:
  EC2: t3.large (CI/CD)
  S3: Armazenamento de builds
  CloudWatch: Monitoramento
  Duração: 12 meses
  Custo_Total: R$ 6.000,00

GitHub Actions:
  Uso: Ilimitado
  Armazenamento: 500GB
  Duração: 12 meses
  Custo_Total: R$ 3.000,00
```

#### 6.2.2 Monitoramento e Logs
```yaml
Datadog:
  Tipo: Logs + APM
  Duração: 12 meses
  Custo_Total: R$ 4.000,00

Grafana Cloud:
  Tipo: Pro
  Duração: 12 meses
  Custo_Total: R$ 1.200,00

Sentry:
  Tipo: Team
  Duração: 12 meses
  Custo_Total: R$ 2.400,00
```

### 6.3 Serviços Terceiros

#### 6.3.1 Testes de Segurança
```yaml
Pentest Externo:
  Escopo: Completo
  Duração: 1 semana
  Empresa: Especializada
  Custo_Total: R$ 15.000,00

Análise de Código:
  Ferramenta: Serviço terceiro
  Profundidade: Completa
  Custo_Total: R$ 8.000,00
```

#### 6.3.2 Certificação
```yaml
FIDO Alliance:
  Tipo: Membro associado
  Duração: 12 meses
  Custo_Total: R$ 5.000,00

Testes de Conformidade:
  Laboratório: Certificado
  Protocolos: FIDO2, WebAuthn
  Custo_Total: R$ 12.000,00
```

---

## 💰 Orçamento Detalhado

### 7.1 Resumo por Categoria

```yaml
Recursos_Humanos:
  Total: R$ 154.200,00
  Porcentagem: 60%

Hardware_Equipamentos:
  Total: R$ 83.100,00
  Porcentagem: 32%

Software_Servicos:
  Total: R$ 20.000,00
  Porcentagem: 8%

Total_Geral:
  Valor: R$ 257.300,00
  Impostos: 20% (R$ 51.460,00)
  Total_Final: R$ 308.760,00
```

### 7.2 Distribuição por Fase

```yaml
Fase_1_Desenvolvimento:
  Semanas: 1-6
  Custo: R$ 120.000,00 (40%)

Fase_2_Testes:
  Semanas: 7-10
  Custo: R$ 80.000,00 (26%)

Fase_3_Validacao:
  Semanas: 11-12
  Custo: R$ 50.000,00 (16%)

Fase_4_Entrega:
  Semanas: 13-14
  Custo: R$ 30.000,00 (10%)

Suporte_Inicial:
  Meses: 3
  Custo: R$ 28.760,00 (8%)
```

### 7.3 Custos Operacionais Mensais

```yaml
Mes_1:
  Pessoal: R$ 40.000,00
  Hardware: R$ 50.000,00 (setup)
  Software: R$ 5.000,00
  Total: R$ 95.000,00

Mes_2:
  Pessoal: R$ 40.000,00
  Hardware: R$ 10.000,00
  Software: R$ 3.000,00
  Total: R$ 53.000,00

Mes_3:
  Pessoal: R$ 40.000,00
  Hardware: R$ 10.000,00
  Software: R$ 3.000,00
  Total: R$ 53.000,00

Mes_4:
  Pessoal: R$ 34.200,00 (redução)
  Hardware: R$ 13.100,00
  Software: R$ 9.000,00
  Total: R$ 56.300,00
```

---

## 📊 Análise de Investimento

### 8.1 ROI (Retorno sobre Investimento)

```yaml
Custo_Total: R$ 308.760,00
Receita_Estimada: R$ 750.000,00 (vendas 2 anos)
ROI: 143%
Payback: 14 meses

Breakdown_Receita:
  Vendas_Diretas: R$ 400.000,00 (53%)
  Licencas_Empresa: R$ 200.000,00 (27%)
  Suporte_Servicos: R$ 100.000,00 (13%)
  Outros: R$ 50.000,00 (7%)
```

### 8.2 Comparativo de Mercado

```yaml
Custo_Producao_Unitario: R$ 45,00
Preco_Venda_Sugerido: R$ 199,00
Margem_Bruta: 77%

Concorrentes:
  YubiKey_5: $80 (custo), $120 (venda)
  Google_Titan: $40 (custo), $50 (venda)
  Authy_Pro: $0 (custo), $24/ano (venda)
```

### 8.3 Análise de Sensibilidade

```yaml
Cenario_Otimo:
  Custo: -10% (R$ 277.884,00)
  Receita: +20% (R$ 900.000,00)
  ROI: 223%

Cenario_Base:
  Custo: R$ 308.760,00
  Receita: R$ 750.000,00
  ROI: 143%

Cenario_Pessimista:
  Custo: +15% (R$ 355.074,00)
  Receita: -20% (R$ 600.000,00)
  ROI: 69%
```

---

## 💡 Otimização de Custos

### 9.1 Estratégias de Redução

#### 9.1.1 Recursos Humanos
```yaml
Home_Office:
  Reducao_Aluguel: 30%
  Reducao_Infra: 20%
  Aumento_Produtividade: 10%
  Economia_Estimada: R$ 15.000,00

Freelancers:
  Contratacao_Por_Tarefa
  Reducao_Beneficios: 40%
  Flexibilidade: Alta
  Economia_Estimada: R$ 20.000,00
```

#### 9.1.2 Hardware
```yaml
Compartilhamento:
  Laboratorio_Compartilhado
  Equipamentos_em_Comum
  Reducao_Custo: 25%
  Economia_Estimada: R$ 20.000,00

Emuladores:
  Testes_em_Software
  Reducao_Hardware: 40%
  Aumento_Tempo: 20%
  Economia_Estimada: R$ 10.000,00
```

#### 9.1.3 Software
```yaml
Open_Source:
  Substituicao_Ferramentas
  Reducao_Licencas: 60%
  Suporte_Comunidade
  Economia_Estimada: R$ 15.000,00

Negociacao:
  Licencas_Em_Grupo
  Descontos_Volume: 30%
  Pagamento_Antecipado: 15%
  Economia_Estimada: R$ 8.000,00
```

### 9.2 Alternativas de Financiamento

#### 9.2.1 Patrocínios
```yaml
Empresas_Tecnologia:
  Apoio_Tecnico
  Divulgacao_Marca
  Reducao_Custos: R$ 50.000,00

Universidades:
  Parceria_Academica
  Estagiarios
  Reducao_Custos: R$ 30.000,00
```

#### 9.2.2 Crowdfunding
```yaml
Plataforma: Kickstarter/Indiegogo
  Meta: R$ 100.000,00
  Recompensas: Early birds
  Taxa_Plataforma: 5%
  Imprevistos: 10%
  Valor_Necessario: R$ 115.000,00
```

---

## 📅 Fluxo de Caixa

### 10.1 Projeção Mensal

```yaml
Mes_0_(Setup):
  Entrada: R$ 0,00
  Saida: R$ 95.000,00
  Saldo: -R$ 95.000,00

Mes_1:
  Entrada: R$ 0,00
  Saida: R$ 53.000,00
  Saldo: -R$ 148.000,00

Mes_2:
  Entrada: R$ 0,00
  Saida: R$ 53.000,00
  Saldo: -R$ 201.000,00

Mes_3:
  Entrada: R$ 0,00
  Saida: R$ 56.300,00
  Saldo: -R$ 257.300,00

Mes_4_(Entrega):
  Entrada: R$ 100.000,00 (lote inicial)
  Saida: R$ 28.760,00 (suporte)
  Saldo: -R$ 186.060,00

Mes_5_em_diante:
  Entrada: R$ 50.000,00/mes (vendas)
  Saida: R$ 10.000,00/mes (operacional)
  Saldo_Mensal: R$ 40.000,00
  Payback: Mes 9
```

### 10.2 Pontos de Controle

```yaml
Checkpoint_1:
  Semana: 6
  Marco: MVP funcional
  Investimento: R$ 180.000,00
  Entregas: Firmware básico, testes iniciais

Checkpoint_2:
  Semana: 10
  Marco: Versão testável
  Investimento: R$ 250.000,00
  Entregas: Testes completos, documentação

Checkpoint_3:
  Semana: 12
  Marco: Versão final
  Investimento: R$ 280.000,00
  Entregas: Produto pronto, certificações

Checkpoint_4:
  Mes 6
  Marco: Mercado
  Receita: R$ 300.000,00
  Resultado: Break-even parcial
```

---

## ⚠️ Riscos Financeiros

### 11.1 Riscos de Custo

```yaml
Aumento_Salarios:
  Probabilidade: Média
  Impacto: Alto
  Mitigacao: Contratos fixos, outsorcing

Atraso_Entregas:
  Probabilidade: Baixa
  Impacto: Alto
  Mitigacao: Buffer de tempo, milestones

Falhas_Tecnicas:
  Probabilidade: Média
  Impacto: Médio
  Mitigacao: Testes extensivos, fallback

Inflacao:
  Probabilidade: Alta
  Impacto: Médio
  Mitigacao: Contratos indexados, compra antecipada
```

### 11.2 Riscos de Mercado

```yaml
Concorrencia:
  Probabilidade: Alta
  Impacto: Médio
  Mitigacao: Diferenciação, patentes

Aceitacao:
  Probabilidade: Média
  Impacto: Alto
  Mitigacao: Marketing, demonstrações

Regulatorio:
  Probabilidade: Baixa
  Impacto: Alto
  Mitigacao: Certificações, compliance
```

---

## 📈 Indicadores de Gestão

### 12.1 KPIs Financeiros

```yaml
Custo_por_Fase:
  Desenvolvimento: R$ 120.000,00
  Testes: R$ 80.000,00
  Validacao: R$ 50.000,00
  Entrega: R$ 30.000,00
  Custo_Medio_por_Semana: R$ 20.000,00

Rentabilidade:
  Margem_Bruta: 77%
  ROI: 143%
  Payback: 14 meses
