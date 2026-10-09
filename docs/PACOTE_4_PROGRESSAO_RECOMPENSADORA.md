# Pacote 4 — Progressão recompensadora

Versão2026.10.08-progressao1, build2026100809. Candidata em validação.

Primeira entrega do pacote4: clareza dos ganhos reais, decisões de evolução e medição do equilíbrio. Não adiciona novos poderes avançados, bestiário ou novas recompensas; esses itens continuam como desdobramentos do pacote4. Próxima entrega de campanha: pacote5, Aurora.

## Entregue

- Personagem mostra o próximo marco real da classe, com nome e nível. Acesso Trilha de classe abre a nova tela.
- Trilha mostra XP restante até o próximo nível, níveis1–20, XP total necessário, nível alcançado/futuro, ganho de HP/MP/ataque base com os atributos atuais, e somente benefícios executáveis. Navegação não passa dos limites1/20. Nível20 mostra limite, sem pedir mais XP.
- Mago: cinco magias existentes e seus desbloqueios. Paladino: cura e sua reserva, Investida no2, juramento e canalização no3, ataques adicionais. Guerreiro: Segundo fôlego, Surto e seus usos, ataques adicionais. Bárbaro: Fúria, usos, dano e ataques adicionais. Aumentos de proficiência, pontos de atributo, sobrevivência e sorte aparecem quando a regra realmente muda.
- Atributos deixa de exibir marcos de referência como promessa de poder funcional. Mostra o próximo marco de pontos; toque abre uma prévia com custo, modificador, HP máximo, ataque, defesa e MP máximo antes/depois. Valores ímpares podem não mudar o modificador: a tela explica que ele muda nos pares.
- Talento Resistente também exige confirmação: custo2 do mesmo marco, ganho retroativo de HP e erro explícito quando não pode aprender. Cancelar, navegar e ver prévias não gastam pontos, alteram RNG ou escrevem saves. Confirmar usa as regras existentes e salva uma vez; falha de gravação permite repetir a mesma transação sem conceder outro ganho.
- Acesso direto Atributos/Poderes/Voltar na Trilha. Escolha de juramento e regras de uso/combate permanecem em Poderes. Paladino precisa firmar Devoção antes de canalizar, mesmo após alcançar o nível3.

Ganhos são adaptações já executadas do jogo. A tabela SRD de referência permanece no código/documentação, mas a Trilha não apresenta tradições, arquétipos, auras ou outros recursos ainda ausentes como disponíveis. Prévia usa atributos/equipamento atuais; não prevê escolhas futuras nem cura HP/MP automaticamente ao subir nível. Heróis antigos continuam com suas regras e indicação explícita.

## Medição

27 suítes, incluindo progressão/preview1–20 das4classes, save19, fluxo de toque real, cancelamento, confirmação, falha/repetição e limites de texto. Benchmark:40.960 combates,64 sementes por classe×nível×cidade×política. Duas políticas: ataque básico e rotação simples de poderes; inicia cada combate com HP/MP e poderes cheios, sem equipamento/forja, sem gasto de pontos ou poções. Paladino da política de poderes tem Devoção firmada no3. Usa roster/regras reais, sem alterar estado dos jogadores.

CSV em validacao/progressao-balance.csv, com vitórias, derrotas, turnos, XP/ouro de vencedores e estimativa ideal de vitórias para o próximo nível. Não é tempo de sessão nem dificuldade média do mundo: inclui regiões inadequadas ao nível, ignora viagens/baús/contratos/mortes/consumíveis e reinicia recursos por combate. Taxa agregada não serve como alvo de balanceamento.

Exemplos nas regiões apropriadas: Nv1 Carvalho ~3,3 vitórias para o2; Nv3 Carvalho15 para o4; Nv5 Ruínas ~30 para o6; Nv10 Mares ~24,7 para o11; Nv18 Aurora ~22,2 para o19. Com50% de combate na exploração, interações são mais que vitórias. Mago Nv5 Ruínas teve3 derrotas/64 só atacando e0/64 com poderes; Nv10 Mares teve5/64 e3/64. Equipamento, pontos, consumíveis e contratos podem alterar esses resultados. XP/preços permanecem iguais até medição de sessões reais.

Save19/128 bytes/leitura1–19 mantidos byte a byte. Nenhum novo campo persistido; nenhuma migração nova. Artes SD iguais (459 entradas); novas telas desenhadas a partir de fundos existentes em RAM, sem leitura por quadro. Mapa/D20/deslocamento, trancas, campanha, dungeon e animações preservados. Sem flash automático, .rpg, formatação ou apagamento de saves.

## Teste físico

1. Atualizar em Configurações > Atualização; conferir herói existente e recursos.
2. Abrir Personagem > Trilha de classe; conferir próximo marco, XP, Anterior/Próximo e níveis1/20.
3. Conferir suas quatro classes; Paladino no3 ainda precisa firmar juramento em Poderes.
4. Atributos: abrir prévia, cancelar e conferir pontos; confirmar uma melhoria disponível. No marco com2 pontos, conferir Resistente; não requer criar personagem novo.
5. Conferir retorno aos Poderes/Personagem, exploração/trancas, combate e mapa/D20. Teste físico fica com o usuário.
