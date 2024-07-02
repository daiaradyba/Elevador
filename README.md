# _Projeto de Sistemas Embarcados - ELF74 - Controle Elevador_
 <b>Daiara Dyba, 1508965</b><br>
 <b>Polianna Beatriz Basso, 2302578</b><br>

## Requisitos de projeto

Realização do controle do Elevador Esquerdo<br>
Utilização dos botões da cabine esquerda como marcadores das solicitações de corredor de controle de aperto para descer e para subir<br>
Requisitos mínimos no Sistema:<br>
- 3xTarefas
- 1xFila
- 1xGateKeeper

## Instruções de uso
Debug feito com a utilização do Firebase para visualização dos dados, alterar SSID e senha para seu wifi em:<br>
#define WIFI_SSID      your_ssid" ->SSID<br>
#define WIFI_PASS      password" ->password<br>

Framework utilizado: ESP-IDF (Espressif), para instalação utilizar o instalador: <br> https://dl.espressif.com/dl/esp-idf/?idf=4.4

Para dar Build no projeto e Flashar o MC seguir os passos abaixo:<br>
```
cd pasta_do_projeto
idf.py set-target esp32
idf.py build
idf.py -p port flash
```
## PROTOCOLO BÁSICO

| Código |   Hex   | Ação |
|--------|---------|-------------|
| a      | 61      | Aberto      |
| f      | 66      | Fechado      |
| s      | 73      | Sobe     |
| d      | 64      | Desce      |
| \r      | 0A      | terminação      |
| \n      | 0D      | terminação      |

## ANOTAÇÕES

- utilizar apenas _Building View_ pois alterar para _Technical View_ altera a velocidade
- alterar a contagem na linha 427 do código para a parada (varia entre PCs)

## Estrutura do projeto

```
├── CMakeLists.txt
├── main
│   ├── CMakeLists.txt
│   └── main.c
└── README.md                  This is the file you are currently reading
```
