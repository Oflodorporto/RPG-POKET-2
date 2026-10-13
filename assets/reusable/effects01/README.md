# Efeitos 01 — biblioteca de fontes reutilizáveis

Oito efeitos, 64 quadros RGBA de 64×64 e 24 variantes de mapa de 32×32.
Esta cópia contém somente sprites, atlases, metadados e fontes reprodutíveis.
Não é carregada pelo firmware, não está no artes.pak e não altera poderes,
dano, regras ou orçamento de PSRAM. Gelo permanece reservado para uso futuro.

Os metadados definem duração, âncora constante e repetição. Alpha é direto
(não pré-multiplicado); compor por source-over. Selecionar/converter somente
os efeitos que uma futura integração realmente usar. Não tratar brilho
semitransparente como simples chave de cor sem revisão visual.

Os ZIPs, GIFs e folhas de apresentação do pacote entregue ficam fora deste
repositório. Os geradores originais abaixo produzem também essas prévias:

    python3 source/generate_effects.py /tmp/rpg-effects-review
    python3 source/generate_map_variants.py /tmp/rpg-effects-review
    python3 source/validate_effects.py /tmp/rpg-effects-review

Dependências: Python 3.10+ e Pillow; requirements.txt preservado em source/.
O validador original completo espera GIFs gerados no diretório de saída.
Para conferir somente esta cópia de fontes/sprites, usar o verificador
check_library.py nesta pasta. Hashes dos quadros permanecem nos manifests.
As formas foram geradas por código, sem arte de terceiros ou imagegen.

Variantes de mapa: reservar 32×32 pixels livres e manter 6 px de distância
de rótulos e ícones. Suprimir o efeito se não houver espaço. A presença
destes arquivos não ativa efeitos novos no mapa nem no combate.
