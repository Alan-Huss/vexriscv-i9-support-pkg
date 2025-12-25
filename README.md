# RELATÓRIO DO PROJETO FINAL – TRILHA FPGA

**TÍTULO DO PROJETO:** Implementação de SoC RISC-V em FPGA Colorlight i9: Desenvolvimento de Infraestrutura e Desafios de Integração Multissensorial.  

**AUTOR:** Alan Huss 

---

### RESUMO
Este projeto apresenta o desenvolvimento de um System-on-Chip (SoC) baseado na arquitetura RISC-V (VexRiscv) na FPGA Colorlight i9 via LiteX. A principal motivação foi suprir a escassez de drivers e bibliotecas prontas para esta plataforma, criando uma base sólida para projetos futuros. Foram adaptadas bibliotecas para os sensores BH1750, MAX3010x, TCS34725 e o display ST7789. Os resultados de luminosidade e frequência cardíaca apresentaram alta fidelidade em comparação com instrumentos de referência. No caso do sensor de cor TCS34725, foram implementadas correções via firmware para mitigar divergências cromáticas, evidenciando os desafios técnicos de calibração em sistemas digitais integrados.

**Palavras-chave:** FPGA. RISC-V. Colorlight i9. LiteX. Porting de Bibliotecas.

---

### 1. INTRODUÇÃO
O desenvolvimento em FPGAs como a Colorlight i9 enfrenta o desafio da inexistência de um ecossistema de bibliotecas "plug-and-play" para CPUs *softcore*. Este projeto visa não apenas criar um sistema funcional de monitoramento, mas documentar o processo de porting de drivers e os limites físicos e de software na reconstrução de sinais sensoriais. A relevância reside na criação de um pacote de suporte (BSP) que reduz o tempo de desenvolvimento para a comunidade acadêmica que utiliza esta placa, permitindo que projetos futuros foquem na aplicação e não na infraestrutura básica.

---

### 2. FUNDAMENTAÇÃO TEÓRICA
O projeto fundamenta-se na arquitetura **RISC-V**, utilizando a CPU **VexRiscv** através do framework **LiteX**. 

* **SoC (System-on-Chip):** Integração de processador, memória e periféricos em um único chip (FPGA).
* **Protocolos de Comunicação:** Utilização de **I2C** para barramento de sensores e **SPI** para interface de alta velocidade com o display.
* **Fotometria e PPG:** Princípios físicos para medição de luz e batimentos cardíacos via sensores ópticos.
* **Colorimetria:** Teoria de conversão de dados brutos (Raw) para o espaço de cores RGB.

---

### 3. METODOLOGIA 

**3.1 Arquitetura do Sistema**
O SoC foi configurado com um barramento principal interconectando a CPU VexRiscv aos controladores periféricos. O mestre I2C gerencia o barramento compartilhado pelos sensores, enquanto um mestre SPI dedicado controla o display LCD.

**3.2 Hardware Utilizado**
* **FPGA:** Colorlight i9 (Lattice ECP5).
* **Sensores:** BH1750 (Luz), MAX3010x (Frequência Cardíaca), TCS34725 (Cor).
* **Display:** LCD TFT ST7789 (240x320) com suporte à biblioteca GFX.

**3.3 Software e Tecnologias**
O desenvolvimento do firmware foi realizado em linguagem C (bare-metal), focado no porting de bibliotecas originalmente escritas para outras plataformas (C++/Arduino), adaptando-as para a estrutura de memória e barramentos do LiteX.

**3.4 Estratégias de Teste e Validação**
* **BH1750:** Comparado com um luxímetro profissional para aferir precisão lumínica.
* **MAX3010x:** Comparado com um smartwatch comercial para validação de BPM.
* **TCS34725:** Teste qualitativo com palheta de cores física e espelhamento no display LCD em 24 bits.

---

### 4. DESENVOLVIMENTO E RESULTADOS OBTIDOS
O SoC foi sintetizado e implementado com sucesso na Colorlight i9.
* **Sucesso em Sensores de Precisão:** O BH1750 e o MAX3010x apresentaram leituras consistentes, validando o porting.
* **Desafio Cromático (TCS34725):** O sensor teve sua saída convertida para 24 bits. Observou-se que a representação visual no display não atingiu a fidelidade absoluta das cores reais. Mesmo com correções de ganho e integração via firmware, fatores como a temperatura da luz ambiente e a resposta não-linear do sensor causaram variações tonais, sendo um ponto de melhoria para iterações futuras.

---

### 5. CONCLUSÃO
Os objetivos foram plenamente atingidos, resultando em um repositório funcional que provê drivers inéditos para a Colorlight i9. O projeto demonstra que o porting de hardware para FPGA exige uma compreensão profunda tanto da arquitetura digital quanto das características analógicas dos sensores, deixando um legado de infraestrutura para a trilha de FPGA.

---

### 6. REFERÊNCIAS
* **Datasheets:** ROHM BH1750, Maxim MAX30102, AMS TCS34725, Sitronix ST7789.
* **Bibliotecas Base:** Adafruit TCS34725, SparkFun MAX3010x, Adafruit GFX.
* **Ferramental:** LiteX & VexRiscv Ecosystem.

---

### 7. APÊNDICE: CÓDIGO-FONTE
O código completo, scripts de build e bibliotecas adaptadas estão disponíveis publicamente em:
**[https://github.com/Alan-Huss/vexriscv-i9-support-pkg](https://github.com/Alan-Huss/vexriscv-i9-support-pkg)**