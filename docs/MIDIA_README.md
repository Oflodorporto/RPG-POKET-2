# Mídia da apresentação

[Voltar à apresentação](../README.md) · [Estado publicado](ESTADO_ATUAL.md)

As imagens do jogo são saídas reais do renderer em testes **host** (no computador). Os GIFs são sequências de capturas host. Não são fotos nem vídeos da placa, prova de comunicação por rádio, medida de FPS ou validação de brilho físico. Não há imagem de protótipo da nova bolsa, pets ou coop nesta apresentação.

## Origem

- **Logo:** arte já versionada em `assets/titulo1/logo-original.png`. O registro existente em [assets/titulo1/proveniencia.json](../assets/titulo1/proveniencia.json) informa geração/extração com ImageGen a partir de referência fornecida pelo proprietário. É uma ilustração de marca, não uma captura do jogo. O arquivo foi apenas referenciado, sem alteração.
- **Carvalho:** captura de `tests/carvalho77_render.cpp` executado no commit-base `dfc68c49f3264d4f2d3642266d44b82ca9b7c563`, cuja diferença em relação ao fonte publicado é somente documentação. O teste aprovou 288 frames. Conversão PPM → PNG sem recorte, retoque ou alteração dos pixels RGB.
- **Ruínas e acampamento:** PNGs copiados byte a byte das capturas do build qualificado de `0541056de5dc94a48b4d750ee0cb4c040df322b1`. Fontes: [captura das Ruínas](../tools/capture_ruins7.py) e [captura do acampamento](../tools/capture_camp_meal7.py).
- **PvP:** GIF copiado byte a byte de `mago-ataque-duas-perspectivas.gif`, produzido pela [captura de apresentação PvP](../tools/capture_pvp_presentation.py). São dois participantes simulados da mesma partida; não duas placas físicas.
- **Relógio:** GIF copiado byte a byte de `Relogio-pulso-manha-velocidade-real-12s.gif`, produzido pela [captura de visibilidade](../tools/capture_clock_visibility7.py). A prévia dura 12 segundos em velocidade normal no host; não comprova a legibilidade a 10% de backlight.

As capturas da versão publicada pertencem à [qualificação do SHA exato](https://github.com/Oflodorporto/RPG-POKET-2/actions/runs/38580662456). O [registro completo da entrega](ESTADO_ATUAL.md) explica a comparação das prévias, a publicação e os limites dos testes.

## Inventário e SHA-256

Os cinco arquivos novos de mídia somam **723.522 bytes** (cerca de 707 KiB). Os dois GIFs juntos somam **490.816 bytes**. O logo de 1.106.159 bytes já existia no repositório e não foi duplicado.

### [carvalho-host.png](media/carvalho-host.png)

- Dimensões: 240 × 320
- Tamanho: 87,442 bytes
- Quadros: 1
- SHA-256: `4b69218eacf3af1453f2a6aab12c0dadd59388fc48ad60f5d18e7c04283c6c93`

### [ruinas-host.png](media/ruinas-host.png)

- Dimensões: 240 × 320
- Tamanho: 69,174 bytes
- Quadros: 1
- SHA-256: `8de9067c32ef3389dab2438b4da52ce62e9082d7cc880d320e988eeb6436e047`

### [acampamento-host.png](media/acampamento-host.png)

- Dimensões: 240 × 320
- Tamanho: 76,090 bytes
- Quadros: 1
- SHA-256: `8a1a21e93af8f2b65e51c846e8b4790aad2f2e93f9fc7243995573e75010323b`

### [pvp-duas-perspectivas-host.gif](media/pvp-duas-perspectivas-host.gif)

- Dimensões: 480 × 358
- Tamanho: 199,002 bytes
- Quadros: 19
- SHA-256: `57ef4106748a1c0759dd7b61dffef0932a0260a98edd66072242204b629bfdcd`

### [relogio-pulso-host.gif](media/relogio-pulso-host.gif)

- Dimensões: 480 × 688
- Tamanho: 291,814 bytes
- Quadros: 59
- SHA-256: `a69868c8af5710dbdff64f83a83a0ded09e5628216e2ffbebfae1b0adff438c0`

### [logo-original.png](../assets/titulo1/logo-original.png)

- Dimensões: 1774 × 887
- Tamanho: 1,106,159 bytes
- Quadros: 1
- SHA-256: `a983abe8098f9aae7469c59a73cca0e7d87262b87441543e7a432259b657af70`

## Direitos e uso

A reorganização não atribui licença nova ao jogo, à identidade visual ou a imagens de terceiros. Os arquivos e avisos de origem permanecem válidos. Confira a procedência e obtenha as permissões necessárias antes de reutilizar conteúdo fora deste projeto.
