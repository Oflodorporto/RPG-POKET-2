# Pacote 5 — O Legado de Anwen

Versão 2026.10.09-anwen1/build2026100908. Candidata em validação.

Depois de Arquivos da Coroa, reunir quatro ajudas concretas, em qualquer ordem. Requer Livro, tutorial e nível18 como a missão anterior. Conversas com os contatos locais abrem uma confirmação; consultar ou cancelar não consome recursos. Objetivo mostra as quatro contribuições e conduz ao próximo aliado faltante pelo mapa normal: mantém D20 e deslocamento. Diário, capítulo4, registra pronta/pendente.

| Local/aliado | Ação | Registro e recompensa |
|---|---|---|
| Carvalho/Elarin | Entregar3 rações; provisões para quem sustenta as raízes vivas | Raízes vivas; +1000XP atual/+100legado |
| Vespera/Iria | Proteger a arqueóloga de um Espectro ao abrir os registros | Memórias libertas; +100ouro/+1000XP atual/+100legado, além da batalha |
| Mares/Nilsa | Escoltar voluntários contra o Saqueador do cais | Rotas abertas; mesma recompensa da escolta de Iria |
| Aurora/Dargan | Financiar componentes,150ouro; oficina oferece o trabalho | Estrutura renovada/projeto; +1000XP atual/+100legado |

Contribuições permanecem com os aliados; são progresso de missão no diário, não equipamento vendável/equipável na bolsa. Não se obtêm apenas visitando cidades, lendo falas ou vencendo um inimigo aleatório. Raízes/Nomes/Rotas/Estrutura formam o projeto de Anwen sem novas vítimas; Liora reconhece a equipe quando todas estão prontas. Odran/Coração do Véu e finais ainda são a próxima entrega do Pacote5, não estão implementados nesta release. A variedade regional/contratos continua início do Pacote6.

NPCs falam em texto contínuo, retratos e pergaminho regional. Resultados de escolta pedem Registrar no diário ao final da leitura antes de liberar prova/recompensa. Entregas de suprimentos/componentes gravam o custo e a prova juntos e depois mostram agradecimento; fechar a fala não repete pagamento. Perder/fugir da escolta deixa a contribuição pendente, aliados em segurança e nova tentativa disponível. SaveError: repetir a gravação mantém o mesmo custo/resultado; reboot antes de confirmar gravação retorna ao checkpoint. Nunca marcar memória/rota como obtida só por restaurar um combate normal.

Save22,128bytes,leitura1–22: bits3–6 de campaignFlags no byte120 registram quatro contribuições; bits0–2 preservam Mares/arquivos; bit7 rejeitado. Flags0/1/3 ou base7 com qualquer subconjunto dos bits3–6;127 completo. Byte121 aceita batalhas1/2/3/5/6, nunca estados transitórios de doação4/7. Saves<=21 rejeitam flags>7/stage>3; leitores antigos bloqueiam22. Não ocupa bytes de acampamento/origem/bestiário/CRC, não aumenta payload, não migra fatos por inferência.

36suítes:384campanhas/permutação/classe/raça, custos exatos, ordem livre, gating, perda/fuga/retomada, saves21/marcadores adulterados, falha de gravação/doação e recompensa uma vez; controlador real cobre contato/cancelar/retry/paginação; renderer percorre confirmação/agradecimentos/progresso/diário/perda e limites de texto. Sem alterar artes459, sem leituraSD por quadro, D20/deslocamento e XP1–20 preservados. Compilar/downloads antes de publicar; físico depende do usuário. Sem flash automático/.rpg/eraseNVS/formatação/Heltec.

Teste físico: Configurações>Atualização, continuar personagem após Arquivos da Coroa. Objetivo>Seguir: Carvalho>Elarin, Vespera>Iria, Mares>Nilsa, Aurora>Dargan. Conferir custo e cancelar; entregar/escoltar e ler; diário muda só após ação concluída. Reiniciar para conferir provas/custos, sem preparar saves físicos artificialmente. Pode fazer em outra ordem. Após quatro, Objetivo>Encontrar Liora: reconhecimento da equipe. Próxima etapa é o confronto e os finais, preservando estas provas.
