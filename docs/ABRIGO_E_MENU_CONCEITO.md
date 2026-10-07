# Abrigo e menu com a arte do conceito

Versao **2026.10.07-abrigo1**, build **2026100706**.

O menu usa o proprio conceito do usuario, convertido para240x320 e RGB565 com256cores. Cada um dos sete botoes preserva o recorte original, incluindo borda, icone, fonte e cores. A acao e associada ao mesmo retangulo visivel; pequena margem externa facilita o toque sem sobrepor as opcoes. Os controles sao Mapa de Aeldra, Personagens, Tela de titulo, Diario, Pessoas, Cartas e Voltar. Seus retornos e protecoes existentes foram preservados.

Nas Ruinas, o Abrigo de Viagem conserva o painel esquerdo do conceito, com dados de classe/nivel/ouro/HP/MP/XP e tres controles inferiores: Mapa, Descanso, Menu. Outros destinos conservam seus cenarios locais com o mesmo HUD ornamentado, sem fingir ruinas no porto ou castelo. Os controles adicionais usam texto do jogo; os textos originais do menu continuam rasterizados na arte.

O novo mapa usa o segundo conceito. Valdaria abre o atlas do continente e nao cria um quinto destino. Os IDs0Carvalho/1Ruinas/2Mares/3Aurora foram preservados; pontos e caminho atualizados na apresentacao. Selecionar um destino atualiza nivel, CD e farol com os dados reais. **D20 animado e deslocamento do personagem continuam existentes**, com mesma rolagem/sobrevivencia/sorte e duracao; caminho termina nos pontos do novo mapa. Viagem/rolagem utilizam o novo fundo e conservam travas de entrada e persistencia existentes.

Ruinas de Vespera usa o painel direito do segundo conceito: Iria/conversar, entrada da dungeon, guardiao, Explorar, Acampar, Mapa e Loja. Vitorias e estado do guardiao do conceito sao substituidos por dados reais; a ilustraçao nunca concede progresso. O bloqueio de3encontros e o consumo do cristal permanecem iguais. Missao ativa e avisos continuam visiveis.

O menu tem iluminacao noturna artistica fixa; abrigo/mapa/ruinas participam do ciclo local sem escurecer os paineis. Lanterna, brasas e estrelas possuem pequenos efeitos em codigo. Nenhuma consulta ao cartao ou rede por quadro. As quatro telas ocupam309248bytes adicionais de flash; pacote de artes do SD inalterado.

Fontes: assets/abrigo-menu1/conceito-original.png, conceito-mapa-ruinas.png e proveniencia.json. A alternativa gerada por imagegen foi conservada localmente como original.png, mas **nao e utilizada** depois do pedido de fidelidade ao conceito. Conversao reproduzivel em tools/prepare_scenic_ui.py; duas fontesPNG compactadas no GitHub.

Save11, leitura1..11, namespaces, particoes e Heltec permanecem iguais. Nenhuma gravacao automatica, pacote.rpg, formato de SD ou apagamento de save.

Validacao concluida em software: 18 conjuntos de testes e compilacao ESP32-S3 aprovados; controlador real, renderizacao e previas finais conferidos. CI candidata 37697988756; publicacao 37698319513. Commit testado 485fdb430ea437c10bd8b01db4768c18b8965eb7. Programa: 2605298 bytes (82% de 3 MiB); variaveis globais: 53500 bytes (16%). Release e arquivos baixados conferidos por SHA-256. Validacao fisica sera realizada pelo usuario.

Teste na placa: abrirMenu e tocar os sete botoes; voltar ao titulo e Continuar; visitarAbrigo nas Ruinas e testarMapa/Descanso/Menu; selecionar as quatro cidades e abrirValdaria; iniciarviagem e verificar d20 animado e chegada do personagem; explorarRuinas e testarIria/Dungeon/Guardiao/Acampar/Mapa/Loja. Conferir dados do heroi, carta pendente e ciclo noturno. Nao apagar personagens para testar.

Release: https://github.com/Oflodorporto/RPG-POKET-2/releases/tag/v2026.10.07-abrigo1
Firmware publicado: 2605440 bytes; SHA-256 8dc3a97a77d9049ec3d64ff45353432f386ab79c35c08111f02f22520d44c933.
