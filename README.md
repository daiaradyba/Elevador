# _Projeto de Sistemas Embarcados - Controle Elevador_

pipipipopopo este projeto tem por objetivo realizar o controle de um elevador...

## Requisitos de projeto

Realização do controle do Elevador Esquerdo
Utilização dos botões das cabines (Central e Direita como marcadores das solicitações de corredor, onde cabine central será a marcação de controle de aperto para descer e a cabine da direita a marcação de controle para subir)
Requisitos mínimos no Sistema:
-3xTarefas
-1xFila
-1xGateKeeper

## Instruções de uso
Debug feito com a utilização do Firebase para visualização dos dados, alterar SSID e senha para seu wifi em:
#define WIFI_SSID      "********" ->SSID
#define WIFI_PASS      "********" ->password

Framework utilizado: ESP-IDF (Espressif), para instalação utilizar o instalador: https://dl.espressif.com/dl/esp-idf/?idf=4.4

Para dar Build no projeto e Flashar o MC seguir os passos abaixo:
_cd_ pasta_do_projeto_
_idf.py set-target esp32_
_idf.py build_
_idf.py -p port flash_

## ANOTAÇÕES

colocar alterações feitas

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
