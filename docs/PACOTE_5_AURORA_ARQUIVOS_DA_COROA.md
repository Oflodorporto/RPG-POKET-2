# Pacote5 — Aurora: Arquivos da Coroa

Versão2026.10.09-aurora1/build2026100906. Candidata em validação.

Primeira entrega do Pacote5. Após concluir as duas missões de Sabela em Mares e alcançar nível18, viaje até Aurora e converse com Liora em Pessoas. O botão Abrir arquivos da Coroa mostra objetivo, requisito e recompensa antes de começar. O Livro das Vigílias continua obrigatório; heróis antigos com a cripta concluída conservam o reconhecimento desse marco. Sem taxa de Guilda adicional nem consumo de cristais para esta missão.

Derrote a Sentinela do bloqueio para que Liora, Seraphine e Dargan entrem nos arquivos. A mesma Sentinela existente é um encontro dedicado da campanha; vencer uma sentinela comum não conclui a missão. A criatura é registrada normalmente no bestiário. Usa bolsa/poderes/Ficha e animações atuais; não há novo inimigo nem alteração das regras de ataque.

Seis cenas com retratos mostram as ordens de Odran, a Vigília Perpétua, o desenho protegido por Dargan, um registro escrito pelo regente e a decisão de Liora de impedir o sacrifício de outras famílias. Odran não participa de uma conversa presente: sua fala está identificada como Registro do palácio. Sua perda não o absolve. As contribuições e confrontos finais seguem para a próxima entrega, não aparecem como liberados nesta versão.

Concluir as seis páginas registra a descoberta no diário e concede350ouro/+3500XP para progressão atual, ou350XP legado, além da recompensa normal da batalha. Os documentos são informação de campanha, não equipamento aleatório vendável. Pessoas, objetivo e diário reconhecem o novo progresso. Após a descoberta o objetivo diz explicitamente que reunir aliados será a próxima etapa; finais/epílogo ainda não foram implementados.

Preview/cancelamento/páginas de diálogo são somente leitura. Missão salva ao começar; após vitória, permanece pendente até registrar o diário. Reinício retoma combate/resultado, reiniciando apenas a leitura das páginas. Derrota/fuga preservam as provas de Mares e permitem tentar novamente, sem recompensa do contrato. Falha de gravação segue SaveError/repetir: não rola outro encontro nem paga duas vezes. Sem perda irreversível de NPCs por falhar.

Save21 continua128bytes/leitura1–21. Usa bit2 de campaignFlags(byte120) para arquivos recuperados e stage3(byte121) para missão ativa. Flags válidas0,1,3,7 preservam ordem de missões; formatos anteriores não aceitam os novos valores. Bestiário permanece nos bits reservados do20; dados de baús/acampamento/origem/progressão intactos. Leitura de save antigo não fabrica missão completa nem escreve automaticamente. Depois de gravar21, leitores antigos20/19 bloqueiam com segurança; não fazer downgrade para abrir21.

34suítes incluindo campanha de Aurora nas16combinações classe/raça, requisito/pureza, vitória/perda/fuga, reinício, save20, bits inválidos, isolamento de encontros comuns, recompensa única e falha/repetição. Controlador real confirma previews e seis cenas sem escrita/RNG, retorno e retry; renderer cobre missão/seis retratos/objetivo/diário/diálogos. Compilação e downloads serão conferidos antes da publicação. Artes459 mantidas, carregadas em PSRAM; nenhuma leituraSD por quadro. Mapa/D20/deslocamento preservados.

Teste físico: atualizar em Configurações > Atualização; conferir slot/bestiário. Com Livro, duas missões de Mares eNv18, Mapa > Aurora > Conversar/Pessoas > Liora > Abrir arquivos da Coroa. Ler/cancelar preview; começar, combater, conferir seis cenas/retratos e Registrar no diário. Conferir recompensa/objetivo/diário e retorno. Reiniciar em combate ou resultado deve retomar sem repetir recompensa. Sem criar heróiNv18 automaticamente. Teste físico do usuário separado dos testes de software.

Próxima etapa do Pacote5: contribuições das quatro cidades e projeto de Anwen, confrontos com Odran/Coração do Véu, aviso das condições, final temporário reversível e final pleno/epílogo; exploração continua depois. Preparar essas regras e persistência antes de prometer finais prontos. Preservar Heltec; nenhum flash automático/.rpg/eraseNVS/formataçãoSD.
