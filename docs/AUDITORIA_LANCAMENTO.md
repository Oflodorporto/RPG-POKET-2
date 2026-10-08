# Avaliação de direção — 8 de outubro de 2026

Decisão: candidato a alpha jogável, ainda não pronto para um lançamento comercial completo. A arte já promete um RPG grande; o conteúdo e a profundidade atuais não cumprem toda essa promessa. O maior risco é produzir dezenas de telas bonitas antes de fechar um ciclo de decisões, recompensa e história.

## Cinco melhorias aplicadas

1. **Orientação:** objetivo clicável no abrigo, Carvalho e Vespera. “Seguir objetivo” abre a ação ou prepara o destino sem viajar/consumir itens automaticamente. Tutorial diz onde começar. Crítica: NPCs sem consequência e objetivo escondido em Menu>Diário passam sensação de decoração.
2. **Evolução:** nível 1 e tabela D&D 1–20, HP por classe, atributos e primeira escolha de talento, com consulta por nível. Crítica: antes as classes davam pouca perspectiva de novas conquistas e o início no nível 3 apagava parte da curva. A tabela de referência deixa explícito o que ainda falta implementar.
3. **Decisão no combate:** risco máximo do próximo golpe calculado incluindo crítico/guarda, habilidades sem mana sinalizadas e bloqueio da técnica de Paladino no nível 1 sem consumir turno. Crítica: tocar Ataque repetidamente domina muitos encontros; a previsão permite decidir entre defender, curar ou finalizar. Não foi feita IA nova nem telegráficos específicos por inimigo.
4. **Bolsa intuitiva:** voltar à cidade de onde foi aberta; conservação do contexto em equipamentos; cura real de 30% HP / 50% MP descrita corretamente. Crítica: rótulos errados e volta inesperada são defeitos de produto, não detalhes visuais.
5. **Recompensa e continuidade:** vitória mostra XP, nível/barra, objetivo atual e contrato; texto deixa de afirmar que todo encontro foi nas Ruínas. Crítica: prêmio sem mostrar avanço convida o usuário a repetir sem saber para quê.

## Referências próximas e lições

[Knights of Pen & Paper 2](https://www.paradoxinteractive.com/games/knights-of-pen-and-paper-2/about) apresenta aventura pixelada por turnos com identidade de RPG de mesa. Lição de direção para POKET: a personalidade das classes e a apresentação da jornada precisam ligar cada sistema, não virar uma coleção de minijogos.

[For the King](https://store.steampowered.com/app/527230/For_The_King/) combina exploração e combate por turnos com perigos claros. Lição: decisão antes do risco; leitura de preparação, custo e chance. O POKET já tem viagem D20, mas precisa de inimigos com comportamentos que mudem escolhas.

[AdventureQuest — Quests](https://www.battleon.com/Quests) estrutura aventuras em missões oferecidas por NPCs com problemas distintos. Lição: a lore deve gerar objetivos e consequências jogáveis, em vez de apenas quatro linhas de conversa.

Estas comparações são julgamento de design com base nas descrições oficiais, não análise de retenção/vendas ou afirmação de que todos esses recursos foram copiados.

## Bloqueadores de lançamento

- Campanha: fechar Aeldra com arco, clímax, desfecho e créditos. Atos futuros anunciados sem conclusão não sustentam lançamento completo. Recomendo lançar primeiro uma aventura fechada em Aeldra, ampliar continentes depois.
- Combate: completar poderes das classes e equilibrar níveis 1–20/equipamentos/inimigos. As tabelas D&D não tornam automaticamente o combate do jogo uma implementação de D&D.
- Conteúdo: diferenciar inimigos por comportamento e recompensas, não apenas HP/cor. Uma dungeon concluível é uma boa fatia inicial; dungeon/endgame infinitos não são requisito do primeiro lançamento.
- Confiabilidade: teste físico de atualização com quedas, retomada, desligamento no checkpoint e cartão ausente. Testes em PC não confirmam bateria, toque nem estabilidade real.
- Produto: medir início com pessoas que nunca viram o jogo, legibilidade em 240×320, calor/bateria/tempo de carregamento, créditos e origem/licenças dos assets, instruções curtas de instalação e versão recuperável. Nenhuma revisão de direitos dos assets ou certificação foi feita aqui.

## Critérios de aceite e diversão

Sessões curtas com objetivo visível e final satisfatório; vitória deve entregar progresso legível; derrota deve ensinar um risco evitável. Variedade de encontros, decisões de classe e histórias com NPCs são retenção saudável. Não usar perda de recompensa por faltar um dia ou tarefas diárias obrigatórias para fabricar hábito.

Metas propostas, ainda não medidas: novo jogador chega a combate/bolsa/objetivo sem explicação externa em 5 min; sabe por que perdeu; sabe sua próxima melhoria; consegue completar uma missão em uma sessão curta. Antes de prometer lançamento, testar com 5 pessoas novas e anotar onde hesitam. Mais eventos e mapas só depois de passar isso.

## Validação e próxima sequência

Passaram 19 suítes (18 anteriores + class_progression), 80 combinações de consulta classe/nível, nova criação/atributos e importação de saves antigos. Compilação ESP32 aprovada sem upload: programa 2.627.710 bytes (83%), globais 53.532 bytes (16%). Teste físico pendente.

Próximas 5 ações: 1. Testar um herói novo sem apagar antigos; 2. Implementar juramento e magia usando a tabela; 3. Balancear progressão/recompensas e arquétipos de inimigos; 4. Fechar roteiro de Aeldra; 5. Teste externo + atualização interrompida antes de lançamento.

Atualização martiais1: Guerreiro e Bárbaro agora possuem cura/ação extra/Fúria com recursos e persistência. 21 suítes e ESP32 aprovados; publicação e hashes verificados. Ainda faltam subclasses e habilidades avançadas, balanceamento1–20, clímax/desfecho de Aeldra e os testes externos/físicos acima. Próxima sequência: teste dos quatro heróis; Punição Divina/auras e caminho de classe; tipos/comportamentos de inimigos; desfecho da campanha; teste de instalação e usabilidade com novos jogadores.
