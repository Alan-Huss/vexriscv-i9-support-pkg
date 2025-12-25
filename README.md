# Implementação de SoC RISC-V em FPGA Colorlight i9  
### Infraestrutura, Porting de Bibliotecas e Integração Multissensorial

**Autor:** Alan Huss  
**Trilha:** FPGA – EmbarcaTech  

---

## Resumo

Este projeto apresenta o desenvolvimento de um **System-on-Chip (SoC)** baseado na arquitetura **RISC-V**, implementado em uma FPGA **Colorlight i9 (Lattice ECP5)** utilizando o framework **LiteX**.  
Devido à ausência de um ecossistema maduro de bibliotecas para essa plataforma, foram realizados o **porting e adaptação de drivers** para sensores ópticos e um display TFT, criando uma base reutilizável para projetos futuros.

Sensores suportados:
- BH1750 – Luminosidade
- MAX3010x – Frequência cardíaca (PPG)
- TCS34725 – Sensor de cor
- Display LCD ST7789 (SPI)

---

### Palavras-chave

FPGA · RISC-V · Colorlight i9 · LiteX · VexRiscv · I2C · SPI · Porting de Bibliotecas

---

## 1. Introdução

O desenvolvimento em FPGAs com CPUs *softcore* impõe desafios adicionais quando comparado a microcontroladores tradicionais, principalmente pela inexistência de bibliotecas prontas e abstrações de hardware consolidadas.  

Este projeto tem como objetivo não apenas a implementação funcional de um sistema multissensorial, mas também a **documentação do processo de criação da infraestrutura básica**, reduzindo a curva de aprendizado para projetos futuros que utilizem a Colorlight i9.

---

## 2. Fundamentação Teórica

- **RISC-V:** Arquitetura aberta e modular, ideal para implementação em FPGAs.  
- **VexRiscv:** CPU RISC-V configurável em HDL, amplamente utilizada com LiteX.  
- **LiteX:** Framework para criação de SoCs customizados em FPGA.  
- **I2C e SPI:** Protocolos de comunicação utilizados para sensores e display.  
- **Fotometria e PPG:** Princípios físicos aplicados à medição de luz e batimentos cardíacos.  
- **Colorimetria:** Conversão de dados RAW em representação RGB.  

---

## 3. Metodologia

### 3.1 Arquitetura do Sistema

O SoC é composto por:
- CPU **VexRiscv**
- Barramento Wishbone
- Controlador I2C (bit-banging)
- Controlador SPI
- UART para debug e upload de firmware

### 3.2 Hardware Utilizado

- **FPGA:** Colorlight i9 (Lattice ECP5)
- **Sensores:** BH1750, MAX3010x, TCS34725
- **Display:** LCD TFT ST7789 (240×320)

---

## 4. Procedimento de Execução do Projeto

### 4.1 Conexões de Hardware

Os seguintes conectores da placa **Colorlight i9** são utilizados:

- **J2**
  - Barramento I2C
  - Conexão dos sensores BH1750, MAX3010x e TCS34725

- **CN2**
  - Interface SPI
  - Conexão do display LCD ST7789
  - Sinais: MOSI, SCLK, CS, DC e RESET

---

### 4.2 Compilação e Upload do SoC (FPGA)

```bash
python3 litex/colorlight_i5.py \
  --board i9 \
  --revision 7.2 \
  --cpu-type=vexriscv \
  --build \
  --load \
  --ecppack-compress
```

---

### 4.3 Compilação do Firmware

```bash
make clean && make
```

Arquivo gerado:
```text
main.bin
```

---

### 4.4 Upload do Firmware para SRAM

```bash
litex_term --kernel main.bin /dev/ttyACMxx
```

---

### 4.5 Inicialização da Execução

```text
litex> reboot
```

---

## 5. Resultados Obtidos

- Leituras de luminosidade e BPM consistentes com instrumentos comerciais.
- Funcionamento integrado de sensores e display.
- Limitações cromáticas observadas no TCS34725, inerentes a fatores físicos e ambientais.

---

## 6. Conclusão

O projeto resultou em um **pacote funcional de suporte à Colorlight i9**, com drivers inéditos, firmware bare-metal e documentação completa, servindo como base para projetos futuros em FPGA.

---

## 7. Código-Fonte

Repositório do projeto:

https://github.com/Alan-Huss/vexriscv-i9-support-pkg
