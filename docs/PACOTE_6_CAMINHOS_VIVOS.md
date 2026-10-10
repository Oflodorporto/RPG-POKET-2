# Pacote 6 — Caminhos Vivos

Versão2026.10.10-mundovivo1/build2026101001 publicada e verificada.39 suítes, controle real, compilação ESP32-S3, SHA-256/tamanho/CRC/catálogo/480 entradas anteriores preservadas. Teste físico pendente do usuário; não gravada automaticamente.

Mares passa a ter Saqueador, Caranguejo Férreo e Mão de Cinza. Aurora: Sentinela, Autômato Dourado e Eco do Véu. Caranguejo recompõe a carapaça; Mão de Cinza prepara e drena mana; Autômato prepara golpe pesado; Eco drena sem preparação. Intenções visíveis; danos físicos/mágicos e mortos-vivos coerentes. Dia/noite muda os pesos (50% inimigo-base de dia;50% nova criatura noturna à noite); nenhum objetivo depende da hora.

Doze contratos novos, três por cidade: combate específico, recuperação de carga/cadernos/registros ao explorar e escolta entre locais reais. Carvalho Nv1+, Ruínas5+, Mares10+, Aurora18+. Cadastro da Guilda100 ouro permanece. Maelis coordena; Iria, Tomás e Liora assinam os contratos locais. Uma missão ativa; ela aparece primeiro em qualquer quadro. Região e destino são derivados do ID permanente, não da cidade atual. Viajar não muda o objetivo.

Recuperação: quando o contrato correspondente está ativo na origem, os achados (20%) e encontros locais (10%) passam a oferecer a carga: 30% de chance por exploração. Fora desse contrato, permanece a distribuição de 50% combate, 20% achado, 20% baú e 10% encontro local. Cada coleta incrementa uma de duas partes; carga vinculada ao contrato, sem criar ração na bolsa. Desistir deixa a carga; após coletar, recibo salvo e nenhuma coleta repetida. Escolta: inicia na origem, termina na chegada ao destino; D20 e animação de viagem preservados. Em falha enfrenta inimigo da estrada. Perder ou fugir durante escolta cancela a viagem, não entrega o passageiro; pode tentar novamente. Contratos1..3 antigos continuam com objetivos/recompensas idênticos.

XP/ouro fixados pelo nível de aceitação; pagamento único e abandono não sorteia recompensa. Contratos novos dão12/15/18% do XP até o próximo nível conforme categoria;a campanha não depende desses contratos. Repetição permitida como trabalho da Guilda, sem progressão retroativa. Carregamento do cartão/OTA e animação do papel mantidos.

Save25 continua128 bytes e lê1..25. IDs cidades0..3/enemies0..24 preservados; novos25..28. Byte67 bit0 guild, bits1..7 descobertas20..26;119 bits6..7 descobertas27..28, demaisbitsilha/intenções preservados. QuestId bits0..1 no packed51, bits2..3 em116 bits2..3;surgebits0..1 preservados. Discovery6 carga/7 recibo;ambos cabem nos3 bits existentes60. Antigos≤24 rejeitam extensões; nenhum save físico artificial. Bytes90/91 camp,122/123 origem e124 CRC preservados. NÃO voltar ao firmware leitor24 após gravar25.

Artes488/carga6991936 bytes;81920 bytes incrementais após480 anteriores. Oito poses originais geradas pela ferramenta imagegen integrada, fontes/prompts/prévia em assets/mundovivo1. Cartão libera espaço flash, carga únicaPSRAMno início, sem leituraSD por quadro. Arte+framebuffer7145536 bytes, abaixo8 MiB antes dos demais buffers; conferir a placa física.

Limites deste pacote: fundação regional implementada, eventos diários continuam o piloto do hipogrifo. As oito categorias e duas ofertas/dia ainda são próxima etapa. Não anunciar isso como pronto. Ilha pós-final pleno continua oculta e sem spoilers.

Teste físico: atualizar firmware+artes em Configurações>Atualização; confirmar3 inimigos em Mares/Aurora, contratos por cidade e nível, coletar2 cargas, escolta comD20/viagem/combate, reiniciar no encontro/contrato/viagem e receber uma vez. Visitar bestiário/ficha de cada inimigo, confirmar direção esquerda, poses de ataque. Software não substitui estes testes.

Validação de balanceamento:480batalhas das quatro classes em níveis10/18 com equipamento tier3, seis poções de vida e três de mana;nenhuma derrota, média5passos de ação. Este é um cenário equipado, não prova de dificuldade com personagem sem equipamentos. Prévias reais:validacao/previa-caminhos-vivos.png. Fontesoriginais/imagegenintegrado/prompts completos:assets/mundovivo1.
