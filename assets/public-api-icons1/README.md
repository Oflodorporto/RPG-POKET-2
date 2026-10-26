# Game Icons para RPG POKET — proposta offline

16 ícones, duas resoluções (24 e 32 px), 12.832 bytes indexed4/RGB565.
Nesta candidata, espada e escudo de 24 px aparecem nos resultados de vitória e fuga.
Os demais ícones seguem disponíveis, sem substituir a arte de equipamentos já usada.
O incremento real de flash é medido no build ESP32 integrado. Nada foi publicado ou enviado ao remoto.

- [Prévia dos pixels reais](preview/contact-sheet.png)
- [Créditos individuais e licença](CREDITS.md)
- [Guia de reprodução, testes e integração](../../docs/OFFLINE_API_ASSETS_PROPOSAL.md)

Original SVG → geometria Iconify fixada → máscara alpha8 auditada → redução
inteira/contorno/cor → indexed4 + paleta RGB565 → decoder existente do jogo.
O build normal é totalmente offline e usa apenas a biblioteca padrão de Python.

Preservar os créditos e a licença ao redistribuir a arte. Os ícones não são
sprites de personagens ou animações e não adicionam itens/regras ao jogo.
