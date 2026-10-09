# Pacote 4.1 — Paladino: luz contra as sombras

Versão2026.10.08-paladino1, build2026100810. Publicada e verificada:28suítes e compilaçãoESP32-S3 passaram; downloads conferidos por tamanho/SHA-256/CRC. Teste físico pendente do usuário.

Poderes > Paladino passa a ter Punição divina, disponível no nível2. Ataque corpo a corpo com +2d8 de dano radiante; contra mortos-vivos do bestiário atual (IDs2/5/8/9), +1d8 adicional. Custo3MP somente se acertar; esquiva gasta o turno mas conserva mana. Não requer juramento. Arma sagrada/Expulsar continuam exigindo Devoção e compartilhando seu uso de canalização. No nível11, Punição aprimorada acrescenta automaticamente1d8 por golpe corpo a corpo, sem mana, tanto no ataque quanto na Investida; soma com Punição divina. Críticos dobram os dados radiantes.

Referência: Paladino2014/SRD5.1, https://media.wizards.com/2023/downloads/dnd/SRD_CC_v5.1.pdf. Adaptação explícita ao jogo: mana fixa3 substitui slot de magia; escolha antes do ataque, sem selecionar slot/elevar círculo. O motor existente agrega Ataque adicional numa ação e usa uma única rolagem de acerto; Punição ativa aplica uma vez, aprimorada aplica por golpe agregado (dois a partir do5). Não replica iniciativa/ações bônus/espaços ou todas as regras de mesa. Não adiciona auras nesta entrega.

Nova animação de1050ms: corte curvo dourado com núcleo branco e clarão no impacto. Funciona no combate normal e na dungeon; a dungeon mantém alvo adjacente à frente. Renderização não altera regras/save e não lê SD por quadro. Trilha de classe e próximo marco agora mostram os ganhos executáveis dos níveis2 e11. Navegar não gasta mana. Saves19/128bytes/leitura1–19 e arte459entradas mantidos, sem migração nova; heróis legados mantêm regras anteriores.

Validação:28suítes, incluindo4.990 casos de Punição divina, classe/nível/fase/mana, esquivas/críticos/mortos-vivos, bônus passivo e save sem repetir turno. Renderer verifica texto e estado sem mutação. Benchmark40.960 combates atualizado com política de Paladino que usa Punição; CSVpaladino-balance.csv. Recursos cheios a cada combate, sem equipamentos/poções/forja/atributos comprados: não é uma sessão de campanha, nem alvo de dificuldade global. XP/preços inalterados.

Teste físico: atualizar pelo menu, conferir herói existente; Paladino no2 > Poderes > avançar até Punição divina, testar acerto/esquiva/mana; mortos-vivos recebem bônus. No11 conferir bônus automático e corte de luz, inclusive na dungeon. Juramento continua disponível na cidade apenas para Arma sagrada/Expulsar. Navegar/cancelar não muda recursos. Nenhum flash automático, .rpg, eraseNVS ou formatação.

Próximos desdobramentos: identidade avançada das outras classes, bestiário e balanceamento de sessões; Pacote5 continua planejado.
