# RPG POKET 2.0

RPG para Waveshare ESP32-S3-Touch-LCD-2, SKU 29667 (16 MiB flash, 8 MiB OPI PSRAM, tela 240×320).

## Instalação inicial

Abra `firmware/RPG_POKET_2/RPG_POKET_2.ino` no Arduino IDE. Core Espressif 3.3.12, ESP32S3 Dev Module, flash 16 MB, PSRAM OPI, partição `app3M_fat9M_16MB`, USB CDC ativado. Não apague a flash. Para instalar as artes pelo computador, baixe `artes.pak` da Release e coloque em `/RPGPOKET/artes.pak` num microSD FAT32. No repositorio o pacote fica compactado como `artes.pak.gz`; o workflow o descompacta e valida. A versão inicial do atualizador precisa ser gravada pelo cabo uma única vez.

## Atualizações pela placa

Conecte a uma rede 2,4 GHz. Vá a **Configurações → Atualização → Verificar versão**. Confirme **Instalar** para baixar o firmware e as artes. A placa confere SHA-256, tamanho e CRC do pacote de artes, instala o firmware na partição livre e reinicia. As artes antigas ficam intactas até a nova versão iniciar e validar o novo pacote; então os arquivos temporários e as artes obsoletas são removidos. Os três saves permanecem em NVS.

Sem uma Release publicada, a placa informa que ainda não há versão disponível. Se as artes locais faltarem, é possível baixar as da mesma versão sem regravar o firmware. Não há leituras periódicas do cartão durante o jogo. Não retire o cartão ou a alimentação durante a instalação. FAT32 não garante recuperação de toda falha de energia; não se promete rollback automático de firmware que não inicialize.

## Publicar uma versão

1. Atualize `ReleaseVersion.h`: versão e número crescente `FW_BUILD`.
2. Atualize fontes e pacote `cartao/RPGPOKET/artes.pak`, conservando os identificadores em `AssetCatalog.h`.
3. Faça os testes e envie ao repositório.
4. Em **Actions → Publicar versão → Run workflow**, informe a mesma versão de `FW_VERSION`.
5. O envio de alteracoes a `main` tambem inicia a publicacao automaticamente. O workflow compila, verifica os tamanhos e publica `firmware.bin`, `artes.pak` e `manifest.json` em uma Release. A placa consulta a última Release publicada.

## Estado atual

2026.10.06-bolsa1: bolsa em slots com itens e equipamentos acessiveis na dungeon, recompensa do bau identificada, escolha apos o Arconte, retorno a exploracao e ataques com movimentos por classe. [Controles e detalhes](docs/BOLSA_ANIMACOES_E_VITORIA.md). Saves9 e artes SD estradas1 permanecem compativeis. Dungeon1c e wifi1 foram confirmadas funcionando na placa pelo usuario; esta entrega requer novo teste fisico.

O projeto inclui GFX de terceiros em `src/GFX`; os avisos/licenças existentes acompanham a biblioteca. Nenhuma licença nova é atribuída ao projeto ou às imagens de terceiros sem decisão do proprietário.

## Estradas e cidades

Versao 2026.10.06-estradas1: cidades com niveis e comercio proprios, quatro inimigos originais, exploracao regional e D20 persistente na viagem. Download com retomada e novas tentativas automaticas. [Regras, saves e teste na placa](docs/ESTRADAS_E_CIDADES.md).

A ota1 foi confirmada funcionando na placa pelo usuario. Esta entrega nova exige validacao fisica separada. Saves escritos no formato8; nao voltar ao firmware antigo para continuar estes saves. Sem gravacao automatica ou arquivos .rpg.

## Wi-Fi e relogio

2026.10.06-wifi1: cinco redes salvas, reconexao, indicador RSSI em quatro barras, relogio preto apos um minuto com brilho reduzido, Continuar download e diagnostico HTTPS do GitHub. [Uso e limites do teste](docs/WIFI_RELOGIO_E_DIAGNOSTICO.md).

Firmware e artes mantem saves8 e o pacote estradas1 de417 recursos. Sem gravacao automatica ou .rpg. Testes fisicos de wifi1 confirmados pelo usuario.

## Cripta do Arconte

2026.10.06-dungeon1c: primeira pessoa, dois andares, selo, sete encontros e chefe, cristal por drop/300 ouro, 44 assets originais e saves9 com importacao1..8. [Controles e detalhes](docs/DUNGEON_PRIMEIRA_PESSOA.md). A arte da dungeon acompanha o firmware; artes.pak permanece compativel. Testes fisicos de wifi1 confirmados pelo usuario; dungeon1c confirmada funcionando na placa pelo usuario.
