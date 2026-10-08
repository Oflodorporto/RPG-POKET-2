# Novas telas do conceito — paineis1

Versao 2026.10.07-paineis1, build 2026100707.

Combate, habilidades, bolsa, Carvalho, personagem e tres fases do acampamento usam os seis conceitos originais fornecidos pelo usuario. Elementos fixos mantem fonte, borda, cores e icones da arte. Valores de exemplo, inimigo e mago desenhados sao substituidos por dados e sprites reais, incluindo raca/classe/cores. Areas livres da propria arte foram recortadas para remover atores fixos da arena. Inteligencia/agilidade ainda nao existem nas regras; personagem mostra sobrevivencia e sorte reais em seu lugar. Outros locais mantem seus cenarios regionais; nao sao transformados em Carvalho ou Ruinas. Retratos menores sao redimensionados a partir do sprite existente, sem trocar os assets dos personagens.

Habilidades continuam com nomes/custos/efeitos de cada classe. Bolsa possui sete consumiveis com quantidade real, selecao, descricao, Equipamentos e Usar/Voltar. Pocao consome uma unidade, cristal e racao nao sao gastos ao consultar. Equipamentos/venda continuam na tela existente. Carvalho tem Conversar/Loja/Bolsa/Heroi/Explorar/Guilda/Acampar/Mapa/Menu. Acampamento conserva escolha de racao e kit, D20, emboscada, descanso e recuperacao existentes. D20 e viagem no mapa continuam inalterados.

Cartao e PSRAM: as oito imagens sao 240x320, 256 cores, 614400 bytes adicionais. Pacote artes.pak agora possui425 recursos, 6772560 bytes totais; payload6772544 e CRC1a8982c3. O pacote inteiro e validado/carregado para PSRAM na inicializacao. Nenhuma leitura SD por quadro. Paletas e fallback pequenos ficam na flash. Saves11, IDs e particoes preservados. Sem cartao valido o fallback e simples; a arte completa requer atualizar firmware e artes pela placa. Publicacao distribui pacote completo; delta gzip no repositorio apenas reduz duplicacao com a base de417 assets. Ferramenta unpack_art monta o pacote e valida CRC. A base original e os primeiros6158144 bytes sao preservados.

Fontes originais e conversor local: assets/paineis1 (no disco, pasta painéis1), tools/prepare_panels_ui.py. A conversao e deterministica e nao usa imagens geradas novas. Paletas, delta e metadados publicados permitem reconstruir a mesma arte do firmware; fontesPNG integrais estao preservadas localmente.

Validacao em andamento: controlador real, renderizacao e compilacao ESP32. Teste fisico pelo usuario apos publicar. Nao gravar automaticamente, apagar NVS/SD, mudar particoes ou gerar.rpg.

Teste na placa: atualizar firmware+artes por Titulo>Configuracoes>Atualizacao; abrir Carvalho e testar9botoes; conferir dados de personagem e7slots da bolsa; consumir1pocao e voltar; batalhar com diferentes classes e abrirTecnicas/Bolsa; ver efeitos de ataque/raio/corte e turno inimigo; acampar com/sem racao/kit e conferir dados do D20/emboscada/progresso; testar tambem cidades/regioes anteriores e d20/viagem. Sem apagar personagens para testar.
