# Contador bidirecional de pessoas

Protótipo para uma passagem em fila única, com um **Arduino Uno R3** e **dois HC-SR04 de 5 V**. O Monitor Serial mostra os totais de entradas e saídas desde que a placa foi ligada ou reiniciada.

## Montagem

No diagrama, considere a sala à direita como a sala monitorada. O sensor A (sensor 1) fica mais perto do lado externo, e o sensor B (sensor 2), mais perto do lado interno. Monte os dois na mesma lateral, na altura do tronco de quem vai passar. Aponte cada sensor transversalmente para uma superfície rígida, plana e fixa na lateral oposta. Mantenha a folha da porta fora dos feixes.

```text
                         Lateral com os sensores
                             [A]         [B]
                              ↓           ↓
Lado externo           ← saída           entrada →      Sala monitorada
                              ↓           ↓
                         Lateral oposta fixa
```

Comece com cerca de **15 a 25 cm entre os eixos** dos sensores e ajuste na passagem real. A pessoa precisa ser vista primeiro por uma zona, depois pelas duas e por fim só pela outra. No arranjo em X do desenho, a ordem pode mudar conforme a posição da pessoa no corredor.

| Pino do HC-SR04 | Sensor A, externo | Sensor B, interno |
| --- | --- | --- |
| VCC | 5V do Uno | 5V do Uno |
| GND | GND do Uno | GND do Uno |
| TRIG | D4 | D6 |
| ECHO | D5 | D7 |

As ligações da tabela são para o Uno R3, que usa lógica de 5 V. Se adaptar o projeto a uma placa de 3,3 V, adapte o nível de tensão de cada saída ECHO antes de conectá-la à placa. [Ficha do HC-SR04](https://cdn.sparkfun.com/datasheets/Sensors/Proximity/HCSR04.pdf) · [Arduino Uno R3](https://docs.arduino.cc/hardware/uno-rev3/)

## Bibliotecas e pacote da Arduino IDE

O contador com dois HC-SR04 dispensa bibliotecas externas. O único cabeçalho usado pelo programa, `Arduino.h`, já vem com o pacote oficial **Arduino AVR Boards**. O programa mede o pulso de retorno com a função padrão `pulseIn()`.

Se **Arduino Uno** ainda não aparecer na lista de placas, abra **Ferramentas → Placa → Gerenciador de Placas**, procure **Arduino AVR Boards** e instale o pacote publicado pela Arduino. Em seguida, selecione **Ferramentas → Placa → Arduino AVR Boards → Arduino Uno**. [Instalação de pacotes de placas](https://support.arduino.cc/hc/en-us/articles/360016119519-Add-boards-to-Arduino-IDE) · [Seleção de placa e porta](https://support.arduino.cc/hc/en-us/articles/4406856349970-Select-board-and-port-in-Arduino-IDE)

## Carregar e conferir

1. Abra [ContadorPessoas.ino](ContadorPessoas/ContadorPessoas.ino) na Arduino IDE. Selecione **Arduino Uno** e a porta USB da placa.
2. Esvazie a passagem, envie o programa e abra o **Monitor Serial a 115200 baud**. Mantenha a passagem vazia até aparecer `Calibrado`. Reinicie com a passagem vazia se mover um sensor ou mudar a superfície de referência.
3. Faça uma passagem em cada sentido e confira se cada movimento incrementa apenas o total correspondente.

| Teste | Resultado esperado |
| --- | --- |
| De fora para dentro, em fila única | Livre → A → A+B → B → livre. Soma uma entrada. |
| De dentro para fora, em fila única | Livre → B → A+B → A → livre. Soma uma saída. |
| Entrar na zona A e voltar para fora | Nenhum incremento. |
| Começar com as duas zonas ocupadas ao mesmo tempo | Nenhum incremento quando a primeira leitura ocupada já mostra A+B. |
| Perder um eco durante a travessia ou exceder 3 segundos | Descarta a sequência e espera a passagem ficar livre. |
| Duas pessoas próximas ou lado a lado | Podem ser contadas como uma só ou passar sem registro. |

Repita os dois sentidos perto das laterais e no centro, em velocidades diferentes, e compare os totais com a contagem manual. Para acompanhar os estados, mude `MOSTRAR_LEITURAS` para `true` e carregue o programa novamente. O Monitor Serial mostra `0` para livre, `1` para A, `2` para B, `3` para ambas e `?` quando algum eco é inválido. Se faltar o estado `3`, aproxime as zonas. Se faltar um dos estados exclusivos `1` ou `2`, revise o alinhamento e o espaçamento. Confira sempre a sequência completa nos dois sentidos após cada ajuste.

## Como a contagem funciona

O programa alterna os disparos a cada 40 ms. Cada módulo mede a cada 80 ms. Na calibração, a média de 20 leituras de cada sensor vira sua distância de referência com a passagem vazia. A calibração exige um fundo entre 30 cm e 3 m, com variação de até 6 cm ao longo das amostras.

Por padrão, uma zona passa a ocupada quando a leitura fica pelo menos 18 cm mais perto que o fundo. Ela volta a livre quando a diferença cai abaixo de 10 cm. Esses dois limites evitam oscilações perto do ponto de decisão. Ajuste `LIMIAR_OCUPAR_MM` e `LIMIAR_LIBERAR_MM` pelas leituras reais, mantendo o segundo menor que o primeiro. A contagem só é confirmada quando a travessia segue a ordem esperada e os dois sensores registram dois pares consecutivos de leituras livres.

Cada HC-SR04 fornece uma distância por disparo. A média descreve a referência de uma zona. Uma pessoa, duas pessoas juntas ou um objeto podem provocar a mesma sequência de ocupação. A lógica de duas zonas segue o princípio documentado pela ST para contagem em uma passagem estreita. [Manual de contagem em duas zonas](https://www.st.com/resource/en/user_manual/um2600-counting-people-with-the-vl53l1x-longdistance-ranging-timeofflight-sensor-stmicroelectronics.pdf)

## Alternativas de sensor

Os preços são referências de anúncios consultados em novembro de 2026.

| Opção | Quando faz sentido | Custo e montagem |
| --- | --- | --- |
| Dois HC-SR04 | Prototipar em fila única com o menor custo de sensores. | Cerca de R$ 20 pelo par, sem placa ou frete. [Ficha técnica](https://cdn.sparkfun.com/datasheets/Sensors/Proximity/HCSR04.pdf) · [Eletrogate](https://www.eletrogate.com/modulo-sensor-de-distancia-ultrassonico-hc-sr04) |
| Duas barreiras infravermelhas | Uma faixa de passagem em fila única, com acesso às duas laterais. Cada feixe fornece um sinal de bloqueio. | Requer dois pares emissor/receptor e alcance maior que a largura do vão. O modelo hobby Adafruit de 5 mm alcança cerca de 50 cm. O Intelbras IVA 3015 X cobre vãos maiores e usa alimentação de 12 a 24 V. [Adafruit](https://www.adafruit.com/product/2168) · [Intelbras](https://backend.intelbras.com/sites/default/files/integration/180416_datasheet_a4_iva_3015_x-rev04.pdf) |
| Um VL53L1X | Passagem estreita com montagem no alto. Um módulo óptico ToF alterna duas regiões de leitura. A ST fornece um exemplo de contagem. | Cerca de R$ 70 pelo módulo. [Manual da ST](https://www.st.com/resource/en/user_manual/um2600-counting-people-with-the-vl53l1x-longdistance-ranging-timeofflight-sensor-stmicroelectronics.pdf) · [Código de referência](https://www.st.com/en/embedded-software/stsw-img010.html) · [MakerHero](https://www.makerhero.com/produto/sensor-de-distancia-a-laser-vl53l1x-de-alta-precisao/) |
| Um VL53L5CX ou VL53L7CX | Quando é necessário distinguir pessoas lado a lado. O sensor no teto mede uma matriz de profundidade 8×8 para separar e acompanhar alvos ao longo do tempo. | A placa VL53L5CX da SparkFun aparece por US$ 32,50, antes de frete e impostos. Use uma placa da classe do ESP32 para a biblioteca multizona. [DigiKey Brasil](https://www.digikey.com.br/pt/products/detail/sparkfun-electronics/18642/15775141) · [Guia SparkFun](https://learn.sparkfun.com/tutorials/qwiic-tof-imager---vl53l5cx-hookup-guide/hardware-hookup) |

Para testar o multizona, meça a cobertura na altura da cabeça. Com o sensor a 2,40 m do piso e uma pessoa de 1,75 m, o VL53L5CX cobre nominalmente cerca de **54 cm** nessa altura, usando 45° no eixo horizontal. O VL53L7CX, com 60° por eixo, cobre cerca de **75 cm** nas mesmas condições. Um vão mais largo pode exigir outra altura de montagem ou mais de um módulo. A demonstração da ST usa exemplos de uma ou duas pessoas indo no mesmo sentido. Valide separadamente os cruzamentos em sentidos opostos e as trajetórias nas bordas. [Ficha do VL53L5CX](https://www.st.com/resource/en/datasheet/vl53l5cx.pdf) · [Ficha do VL53L7CX](https://www.st.com/resource/en/datasheet/vl53l7cx.pdf) · [Demonstração da ST](https://www.st.com/content/st_com/en/st-edge-ai-suite/case-studies/people-counting-with-a-ranging-sensor.html)
