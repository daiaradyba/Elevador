# _Projeto de Sistemas Embarcados - Controle Elevador_

pipipipopopo este projeto tem por objetivo realizar o controle de um elevador...<br>

## Requisitos de projeto

Realização do controle do Elevador Esquerdo<br>
Utilização dos botões das cabines (Central e Direita como marcadores das solicitações de corredor, onde cabine central será a marcação de controle de aperto para descer e a cabine da direita a marcação de controle para subir)<br>
Requisitos mínimos no Sistema:<br>
- 3xTarefas
- 1xFila
- 1xGateKeeper

## Instruções de uso
Debug feito com a utilização do Firebase para visualização dos dados, alterar SSID e senha para seu wifi em:<br>
#define WIFI_SSID      "********" ->SSID<br>
#define WIFI_PASS      "********" ->password<br>

Framework utilizado: ESP-IDF (Espressif), para instalação utilizar o instalador: https://dl.espressif.com/dl/esp-idf/?idf=4.4

Para dar Build no projeto e Flashar o MC seguir os passos abaixo:<br>
```
cd_ pasta_do_projeto
idf.py set-target esp32
idf.py build
idf.py -p port flash
```
## ANOTAÇÕES

- colocar alterações feitas

## Estrutura do projeto
Below is short explanation of remaining files in the project folder.

```
├── CMakeLists.txt
├── main
│   ├── CMakeLists.txt
│   └── main.c
└── README.md                  This is the file you are currently reading
```
Additionally, the sample project contains Makefile and component.mk files, used for the legacy Make based build system. 
They are not used or needed when building with CMake and idf.py.
