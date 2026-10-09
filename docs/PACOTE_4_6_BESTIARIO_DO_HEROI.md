# Pacote 4.6 — Bestiário do herói

Versão 2026.10.09-bestiario1, build 2026100905. Publicada e verificada:33 suítes e compilação ESP32-S3 passaram; downloads conferidos por tamanho/SHA-256/CRC. Teste físico pendente do usuário.

Menu > Bestiário abre o registro do personagem selecionado. Cada encontro aceito registra a criatura: exploração, viagem, dungeon, campanha e eventos usam a mesma regra. Só as criaturas encontradas aparecem; Anterior/Próximo percorrem essas descobertas sem revelar inimigos futuros. Mostra retrato, nome, HP/ataque/defesa base, tipo de ataque e natureza morto-vivo. Os valores do encontro podem variar; a Ficha de combate continua mostrando os valores atuais e a intenção do adversário. Não concede XP, ouro ou outra rolagem.

Registro separado nos três slots. Novo herói começa com bestiário vazio; excluir/recriar não herda descobertas. Consulta e navegação não gastam recursos/turnos, não avançam RNG nem gravam save. A descoberta acompanha o salvamento normal do encontro, com o mesmo journal de dois registros e verificação de escrita; falha e repetição não inventam outra descoberta. Arte reutilizada em memória, sem leitura SD por quadro.

Save20 mantém128bytes e CRC no124. Dezoito bits de descoberta usam reservas: byte54 bits0–7, byte47 bits5–7, byte50 bits1–7. Flags existentes, equipamento, contratos e cidade mantêm seus bits baixos. Bytes60–63,90–91,122–123 preservados. Leitura1–20. Saves antigos recuperam apenas o inimigo do encontro salvo fora de Home e o Guardião já vencido; o histórico anterior completo não existia e não pode ser reconstruído. Apenas ler/migrar na RAM não escreve NVS; a próxima ação salva normalmente.

Compatibilidade de retorno: depois de gravar save20, firmware antigo que só lê até19 bloqueia esse slot com segurança. Não fazer downgrade esperando abrir o save20; manter versão compatível ou mais recente. Nenhuma migração apaga os personagens.

33 suítes:18bits/roundtrip, migrações, journal/falha/repetição, slots independentes/exclusão/recriação, navegação esparsa/read-only e acesso real pelo Menu. Renderer verifica54 estados vazios/conhecidos. Preservados testes de progressão/classes, contratos, lojas, baús/trancas, dungeon e viagens. Compilação ESP32-S3 e integridade dos downloads confirmadas.

Teste físico: atualizar em Configurações > Atualização; abrir Menu > Bestiário, alternar criaturas e voltar; encontrar criatura nova, reiniciar e conferir o registro. Trocar slots e verificar que o registro pertence ao herói. Não apagar personagem usado apenas para testar exclusão. Conferir combate, D20 e deslocamento no mapa. Teste físico fica com o usuário; nenhum flash automático/.rpg/eraseNVS/formataçãoSD/alteraçãoHeltec.

Esta é a última entrega planejada do Pacote4. Depois da publicação/validação segue Pacote5 — Aurora e uma campanha que chega ao fim. O pacote4 não pretende implementar todo o SRD: auras/arquetipos/outros recursos ainda ausentes seguem declarados nos guias das classes. Equilíbrio econômico e sessões reais permanecem trabalho contínuo, sem bloquear a campanha.
