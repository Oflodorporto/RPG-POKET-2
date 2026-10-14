<p align="center">
  <img src="assets/titulo1/logo-original.png" width="500" alt="RPG POKET: logo medieval em pixel art, com espada e escudo">
</p>

<h1 align="center">RPG POKET 2.0</h1>
<p align="center"><strong>De Carvalho às Ruínas de Vespera. Uma aventura na palma da mão.</strong></p>
<p align="center">Fantasia medieval em português · Pixel art inspirada nos 16 bits · ESP32-S3 · Tela de toque</p>

<p align="center">
  <a href="https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.14-pvp77"><strong>Baixar a versão atual</strong></a> ·
  <a href="docs/GUIA_RAPIDO.md">Instalar e jogar</a> ·
  <a href="#um-pequeno-mundo-para-explorar">Ver o jogo</a> ·
  <a href="#documentação">Documentação</a>
</p>

Crie seu aventureiro, converse com os habitantes de Aeldra, atravesse estradas, explore ruínas e enfrente combates por turnos. Escolha uma classe, personalize sua aparência e acompanhe uma campanha com diário, cartas, personagens e progressão até o nível 20.

**Versão publicada:** 7.7 · Duelo PvP · `2026.10.14-pvp77` · build `2026101404`.
Firmware, artes e downloads verificados. **A validação física desta versão, incluindo rádio entre duas placas, ainda está pendente.** [Estado completo e limites](docs/ESTADO_ATUAL.md).

## As Cinzas da Primeira Aurora

Em Aeldra, um sino toca numa torre vazia, a cinza adoece as raízes e nomes desaparecem das lembranças. Você chega a Carvalho quase sem ouro, em busca de abrigo e trabalho. Uma carta da Guilda conduz seus passos às ruínas de Véspera e ao mistério dos quatro faróis que protegem o reino. Sua história nasce das pessoas que você decide ajudar.

### Pelas estradas de Aeldra

O mapa atual percorre Aeldra, no oeste de Valdária, entre o mar e as montanhas. As estradas ligam quatro destinos:

- **Carvalho:** vila junto ao bosque, onde Nara oferece acolhimento e a jornada começa.
- **Ruínas de Véspera:** cidade abandonada, com expedições, mortos inquietos e a Cripta do Arconte.
- **Mares:** porto de mercadores e construtores navais, ameaçado por desaparecimentos e cargas suspeitas.
- **Aurora:** cidade da Coroa, dos arquivos e das forjas, onde antigos juramentos estão em disputa.

## Um pequeno mundo para explorar

<p align="center">
  <img src="docs/media/carvalho-host.png" width="240" alt="Carvalho: objetivo da jornada e botões Conversar, Loja, Bolsa, Explorar e Menu">
  <img src="docs/media/ruinas-host.png" width="240" alt="Ruínas de Vespera: praça medieval, exploração, Cripta e Guardião">
  <img src="docs/media/acampamento-host.png" width="240" alt="Aventureiro descansando ao lado da fogueira em um acampamento noturno">
</p>
<p align="center"><strong>Carvalho · Ruínas de Vespera · Acampamento</strong></p>

As imagens e animações desta página são **capturas do renderer do jogo em testes no computador (host)**. Não são fotos da placa, vídeo de hardware ou medições de FPS. O logo é uma arte do projeto. [Origem dos arquivos e hashes](docs/MIDIA_README.md).

### Escolha seu caminho

- **Um herói seu.** Quatro raças, quatro classes: Mago, Cavaleiro/Paladino, Guerreiro e Bárbaro. Cores, atributos, equipamentos e poderes acompanham a jornada.
- **Uma campanha para descobrir.** Uma jornada por Carvalho, Mares e Aurora, com conversas, objetivos, cartas, diário e uma ilha opcional.
- **Exploração e decisões.** Viagens por dias, provisões, encontros, baús, trancas e D20. A Cripta tem visão em primeira pessoa; novas entradas na Cripta das Ruínas usam geração procedural.
- **Combate por turnos.** Ataques, defesa, magias e poderes de classe, com HP/MP, intenção do inimigo, recompensas e bestiário.
- **Pausas que fazem parte da aventura.** Fadiga, refeições, acampamento personalizado e um relógio medieval que acompanha a hora local.
- **Três jornadas salvas.** Três slots em NVS. As transições relevantes do jogo são gravadas automaticamente; se houver falha, a tela oferece **Tentar salvar**.

## Duelo na Arena da Guilda

<p align="center">
  <img src="docs/media/pvp-duas-perspectivas-host.gif" width="576" alt="Animação host de um ataque de Mago, mostrando as duas perspectivas da mesma partida PvP">
</p>

Duas perspectivas da mesma partida, com os dois aventureiros completos, HP/MP, guarda, custos de mana e animações das ações confirmadas. A versão atual também permite desistir durante o turno do adversário.

**Atualize as duas placas para `pvp77`.** O protocolo v5, de 52 bytes, é incompatível com o v4 anterior. A animação acima usa dois participantes simulados no host; o teste físico de ESP-NOW/RF entre as placas está pendente.

Se a comunicação cair **depois do início**, sem resultado confirmado, o duelo termina em **NAO CONFIRMADO**, sem prêmio e com a entrada já consumida. Antes do início, permanece a devolução integral. Uma vitória ainda não gravada pode ser perdida ao desligar. [Regras de interrupção e evidências](docs/ESTADO_ATUAL.md).

## O tempo também passa por aqui

<p align="center">
  <img src="docs/media/relogio-pulso-host.gif" width="288" alt="Prévia host de 12 segundos do relógio medieval, com pulso do sol em velocidade normal">
</p>

Sol e lua acompanham a hora local, com pulso e mudanças graduais de luz. A prévia mostra 12 segundos em velocidade normal no host. A legibilidade real da tela em repouso, com backlight a 10%, ainda precisa ser conferida no aparelho.

## Comece sua jornada

### A placa certa

**Waveshare ESP32-S3-Touch-LCD-2, SKU 29667:** flash de 16 MiB, PSRAM OPI de 8 MiB e tela de 240 × 320. Use um microSD em **FAT32**, cabo USB de dados e alimentação estável. Este firmware foi preparado para essa placa; não há versão de jogo em navegador lançada.

### Instalação e atualização

1. **Primeira instalação:** abra o sketch completo da [fonte da versão atual](https://github.com/Oflodorporto/RPG-POKET-2/tree/v2026.10.14-pvp77) no Arduino IDE e envie pelo USB com a configuração do [guia de instalação](docs/GUIA_RAPIDO.md#instalar-pelo-usb). Mantenha **EraseFlash=none / Erase All Flash desativado**.
2. **Artes:** baixe `artes.pak` da [mesma Release](https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.14-pvp77) e copie para `/RPGPOKET/artes.pak` no microSD FAT32. O pacote atual contém 666 artes e ocupa cerca de 7,5 MB.
3. **Já usa scenes77 ou posterior?** Conecte ao Wi-Fi **2,4 GHz** e abra **Configurações → Atualização → Verificar versão → Instalar**. Mantenha cartão e alimentação conectados até terminar.

> **Atenção aos saves:** a versão atual grava Save30, lê Save29/30 e mantém época 1, **sem novo reset**. A primeira passagem de **touch77 ou anterior** exige USB e reinicia uma única vez os três slots antigos, sem importar Save1…28; Wi-Fi e artes ficam fora desse reset. Não tente contornar a recusa do OTA nem apagar toda a flash. [Confira o seu caso antes de instalar](docs/GUIA_RAPIDO.md#compatibilidade-dos-saves).

### Como jogar

Use **toques nos botões da tela** e solte o dedo entre comandos. Em Carvalho, **Mais** abre Herói, Guilda, Acampar e Mapa; **Menu** continua acessível. Toque no objetivo para acompanhar o próximo passo. Na dungeon, os botões movem ou giram o aventureiro; tocar na cena interage ou ataca quando disponível. [Controles, saves e solução de problemas](docs/GUIA_RAPIDO.md).

## O que vem depois

- **Validação em hardware:** PvP entre duas placas, toque, relógio a 10% de brilho, USB/OTA, energia e medições reais de desempenho.
- **Nova bolsa visual:** frente separada; não integrada nesta versão.
- **Pets e cooperação:** planejamento/protótipos separados; não são recursos lançados do `pvp77`.

Sem datas prometidas. O que está publicado e suas limitações ficam registrados no [estado atual](docs/ESTADO_ATUAL.md).

## Documentação

- [Guia rápido: instalar, atualizar, jogar e recuperar](docs/GUIA_RAPIDO.md)
- [Estado publicado, hashes, compatibilidade e testes](docs/ESTADO_ATUAL.md)
- [Mapa dos guias de história, mecânicas e desenvolvimento](docs/INDICE.md)
- [Histórico técnico completo do README anterior](docs/historico/README-2026-10-14.md)
- [Mídia desta apresentação: origem e integridade](docs/MIDIA_README.md)

Para relatar um problema, [abra uma issue](https://github.com/Oflodorporto/RPG-POKET-2/issues) com a versão/build, placa, passos para reproduzir e resultado esperado. Não inclua senhas, credenciais ou dados pessoais.

**Créditos e licenças:** as artes e os avisos existentes acompanham o projeto. A biblioteca GFX de terceiros mantém seus próprios avisos em [src/GFX](firmware/RPG_POKET_2/src/GFX). Esta apresentação não concede uma licença nova ao código ou às imagens. Consulte os arquivos de origem antes de reutilizá-los.
