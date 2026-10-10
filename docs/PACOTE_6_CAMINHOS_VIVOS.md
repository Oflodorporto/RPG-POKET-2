# Pacote 6 — Caminhos Vivos

Versão candidata2026.10.10-mundovivo1/build2026101001. Não gravada na placa.

Mares passa a ter Saqueador, Caranguejo Férreo e Mão de Cinza. Aurora: Sentinela, Autômato Dourado e Eco do Véu. Caranguejo recompõe a carapaça; Mão de Cinza prepara e drena mana; Autômato prepara golpe pesado; Eco drena sem preparação. Intenções visíveis; danos físicos/mágicos e mortos-vivos coerentes. Dia/noite muda os pesos (50% inimigo-base de dia;50% nova criatura noturna à noite); nenhum objetivo depende da hora.

Doze contratos novos, três por cidade: combate específico, recuperação de carga/cadernos/registros ao explorar e escolta entre locais reais. Carvalho Nv1+, Ruínas5+, Mares10+, Aurora18+. Cadastro da Guilda100ouro permanece. Maelis coordena; Iria, Tomás e Liora assinam os contratos locais. Uma missão ativa; ela aparece primeiro em qualquer quadro. Região e destino são derivados do ID permanente, não da cidade atual. Viajar não muda o objetivo.

Recuperação: encontros locais têm10% de chance por exploração e viram carga somente se o contrato correspondente estiver ativo na origem; a distribuição permanece 50%combate,20%achado,20%baú,10%encontro local. Cada coleta incrementa uma de duas partes; carga vinculada ao contrato, sem criar ração na bolsa. Desistir deixa a carga; após coletar, recibo salvo e nenhuma coleta repetida. Escolta: inicia na origem, termina na chegada ao destino; D20 e animação de viagem preservados. Em falha enfrenta inimigo da estrada. Perder ou fugir durante escolta cancela a viagem, não entrega o passageiro; pode tentar novamente. Contratos1..3 antigos continuam com objetivos/recompensas idênticos.

XP/ouro fixados pelo nível de aceitação; pagamento único e abandono não sorteia recompensa. Contratos novos dão12/15/18% do XP até o próximo nível conforme categoria;a campanha não depende desses contratos. Repetição permitida como trabalho da Guilda, sem progressão retroativa. Carregamento do cartão/OTA e animação do papel mantidos.

Save25 continua128bytes e lê1..25. IDs cidades0..3/enemies0..24 preservados; novos25..28. Byte67bit0guild, bits1..7descobertas20..26;119bits6..7descobertas27..28, demaisbitsilha/intenções preservados. QuestId bits0..1 no packed51, bits2..3 em116bits2..3;surgebits0..1 preservados. Discovery6carga/7recibo;ambos cabem nos3bits existentes60. Antigos≤24 rejeitam extensões; nenhum save físico artificial. Bytes90/91camp,122/123origem e124CRC preservados. NÃO voltar ao firmware leitor24 após gravar25.

Artes488/carga6991936bytes;81920bytes incrementais após480anteriores. Oito poses originais geradas pela ferramenta imagegen integrada, fontes/prompts/prévia em assets/mundovivo1. Cartão libera espaço flash, carga únicaPSRAMno início, sem leituraSD por quadro. Arte+framebuffer7145536bytes, abaixo8MiB antes dos demais buffers; conferir a placa física.

Limites deste pacote: fundação regional implementada, eventos diários continuam o piloto do hipogrifo. As oito categorias e duas ofertas/dia ainda são próxima etapa. Não anunciar isso como pronto. Ilha pós-final pleno continua oculta e sem spoilers.

Teste físico: atualizar firmware+artes em Configurações>Atualização; confirmar3inimigos em Mares/Aurora, contratos por cidade e nível, coletar2cargas, escolta comD20/viagem/combate, reiniciar no encontro/contrato/viagem e receber uma vez. Visitar bestiário/ficha de cada inimigo, confirmar direção esquerda, poses de ataque. Software não substitui estes testes.
