# Próxima tarefa

## (EXECUTADA) Decodificar o prefixo duplicado dos arquivos de altura

**Resolvida.** O cabeçalho BMP real está no **offset 4**, após um prefixo
duplicado de quatro bytes — o mesmo padrão que o formato OZJ deste cliente usa.
Medido nos 47 arquivos BMP: cabeçalho válido no offset 4 em 44, no offset 0 em 1.

Resultado: 42 arquivos agora são lidos pela estrutura em vez de por fallback.
O campo `header_ignored` distingue decodificação de palpite.

**O que isso revelou:** a versão anterior lia o cabeçalho no offset 0, obtinha
`bpp = 0`, caía no fallback — e **o fallback produzia campo suave, então o defeito
era invisível**. Só localizar o cabeçalho de verdade mostrou que 46 arquivos
estavam sendo lidos por adivinhação.

## Próxima tarefa: escala vertical da altura de terreno

Sem ela os valores de altura são relativos, não métricos, e o terreno não fica
renderizável — independentemente de o campo estar legível.

**O que procurar:** um fator de escala no prefixo dos arquivos de altura (os 1.082
bytes antes do corpo nos arquivos de grade crua, ou os 1.078 do cabeçalho BMP), ou
um valor constante em outro arquivo do mundo.

**Alternativa de maior impacto, se houver acesso a GPU:** iniciar o pipeline
VBO/VAO (DEC-008). É pré-requisito de todo o requisito de desempenho e não está
bloqueado por nenhuma descoberta pendente — apenas pela ausência de hardware.

---



> **Índice de UV: investigação ENCERRADA no ciclo 2.** Sete hipóteses descartadas
> e documentadas em `ASSET_FORMATS.md`. Não retomar sem uma hipótese nova e
> diferente das sete — repetir uma delas é desperdício. A próxima tarefa é a que
> está abaixo.

## Decodificar ossos e animação, ou iniciar o pipeline de renderização

Duas opções, e a escolha depende de um julgamento que deve ser explícito:

**Ossos e animação.** Nenhum registro de osso foi localizado. É o último grande
bloco do formato BMD não examinado. Se for decodificado, o formato fica completo
exceto pelo índice de UV — e ainda assim sem texturização.

**Pipeline de renderização (DEC-008).** VBO/VAO não tem nenhuma linha de código.
Pode avançar com geometria sintética, rotulada como **"teste sintético de
pipeline"** conforme DEC-009, sem depender do bloqueio nº 1.

Argumento para o pipeline: ele não está bloqueado por nada, e é pré-requisito de
todo o requisito de desempenho. Argumento para ossos: é o último bloco do formato
e sem ele o requisito de reaproveitar 100% dos assets fica inatingível mesmo que
UV seja resolvido.

---

## (Encerrada) Localizar o índice de UV por canto do triângulo

> **Atualizado no fim do ciclo 2.** A tarefa abaixo foi executada e o registro de
> triângulo foi decodificado em sua parte essencial. O que resta dela está na
> seção seguinte. Os números citados no texto original continuam válidos como
> registro do método.

## Localizar o índice de UV por canto do triângulo

Este é agora **o único bloqueio entre a malha montada e a malha texturizada**.

O array de UV está confirmado e legível (`52+vc*16+nc*20`, `ntc` entradas de 8 B
`[f32 u][f32 v]`; zero não-finitos em 250.450 leituras, quando o acaso daria
~976). Falta apenas saber **qual entrada pertence a cada canto**.

**Quatro hipóteses já descartadas — não repetir:**

1. Campo em +4 do registro: é **padding** (64,95% zero, 18,18% 0xCDCD, o
   preenchimento de memória não inicializada do MSVC).
2. Quarto canto em +24: **não existe** (offsets +24..+56 dão 50–81% de validade,
   nível de ruído, contra 94–100% em +0/+8/+16).
3. Relação implícita por posição: **refutada** — `numTexCoords` difere de
   `numVertices` e `numNormals` em 93% dos 802 arquivos medidos.
4. Bytes 24..59 do registro: **platô sem pico**. Os 39 deslocamentos de 24 a 62
   dão 40–94% com razão de 7,3× a 17,1× sobre o controle, variando de forma
   gradual. Um índice verdadeiro daria pico nítido num offset par, como os de
   vértice deram em +0/+8/+16 (94–100%). Provavelmente são `f32`, não índices.

**Seis hipóteses descartadas — não repetir:**

4. Bytes 24..59 do registro: **platô sem pico**, e confirmado serem `f32`
   (0,055% de não-finitos contra 0,39% esperado por acaso). Não há índices ali.
5. `u16@48` do cabeçalho: dominado por inteiros pequenos (0 em 57,4%, depois
   1, 2, 4, 3) — assinatura de **contagem**, não de índice.
6. `u16@50` do cabeçalho: zero em 60,0% e 0xCDCD em 62 arquivos — ao menos
   parcialmente padding.

**O que resta, e é a única via não examinada:** que o índice de UV esteja num
**array paralelo de `ntri` entradas** em outra parte do corpo, fora do registro
de triângulo. Procurar por um bloco de `ntri` valores `u16` todos menores que
`numTexCoords`, varrendo o corpo a partir do fim do array de triângulos.

Se isso também falhar, o índice de UV deve ser registrado como
**NÃO RESOLVIDO** com as seis tentativas documentadas, e o projeto deve seguir
para ossos/animação ou para o pipeline de renderização — não insistir.

---

## (Executada) Decodificar os 58 bytes restantes do registro de triângulo

### Por que esta tarefa

O bloqueio nº 1 mudou de natureza no ciclo 2. A cifra foi quebrada e o array de
triângulos foi **localizado** com controle estatístico — razão de 138,3× sobre o
acaso, mais confirmação independente pela posição do nome de textura em 87,9% dos
arquivos de malha única. O problema deixou de ser *onde estão os triângulos* e
passou a ser *o que há dentro de cada registro*.

É a única tarefa que separa 83,31% de "arquivos com vértices lidos" de "modelos
renderizáveis", e é pré-requisito do requisito de reaproveitar 100% dos assets
originais. Também destrava UV e LOD de assets originais.

### O que já se sabe

```
tstart = 52 + numVertices*16 + numNormals*20 + numTexCoords*8
stride = 64 bytes
índices de vértice em +2, +6 e +10   -> 98,05% válidos (controle limpo 2,35%)
```

Vale para **arquivos de malha única** (586 medidos). Para malhas múltiplas o
corpo se repete e não foi medido.

### O que já foi refutado — não repetir

- **Primeiro byte = tamanho do polígono.** Valores 3 e 4 somam 3,01% em 35.902
  registros, indistinguível de aleatório.
- **As tabelas de registro do projeto `xulek/muonline-bmd-viewer`.** Não
  correspondem a estes arquivos para vértice nem para normal; não há razão para
  esperar que correspondam para triângulo.
- **Stride 8/10/12/14/16.** Testados no ciclo 1, todos descartados. O stride é 64.
- **Busca ancorada em `uv_end` derivado de hipótese não confirmada.** Foi o erro
  de método que invalidou as tentativas anteriores.

### Teste concreto a executar

1. Para cada um dos 64 deslocamentos do registro, ler `u16` e medir a fração
   abaixo de `numVertices`, nos mesmos 586 arquivos de malha única.
2. **Controle obrigatório:** a mesma medição na região `f32` de vértices, onde não
   há índice. Deu 2,35%; acaso puro deu 0,71%.
3. Aceitar um campo como índice de vértice só com razão **acima de 10×** sobre o
   controle. Abaixo disso é ruído.
4. Repetir para índice de normal, comparando com `numNormals`.
5. Repetir o item 1 para os 6 primeiros bytes, que são os únicos já
   identificados, como verificação de que o método recupera o que já se sabe.

Reprodutível por `python3 tools/bmd_topology_probe.py <Data>`; o docstring
documenta o método e as duas armadilhas (controle contaminado dentro do próprio
array, e base de payload 4 vs 8 por versão).

### Regras que se aplicam

- Separar sempre **estrutura válida** de **payload utilizável** de **geometria
  renderizável**.
- Toda medição exige linha de base calculada, não apenas "parece bater".
- Se não houver progresso após esforço razoável, registrar como
  **topologia BMD: NÃO RESOLVIDO — tentativas documentadas: [lista]** e avançar.

## Tarefas seguintes, em ordem

1. Coordenadas UV — dependem de topologia para validação.
2. Ossos e animação — nenhum registro localizado ainda.
3. Estender a topologia para malhas múltiplas.
4. Investigar os 1.519 arquivos v0x0C cuja decifragem foi rejeitada.
5. Implementar VBO/VAO (DEC-008) — só depois que houver malha para desenhar, ou
   com geometria sintética **rotulada como "teste sintético de pipeline"**
   (DEC-009).

## O que NÃO fazer

- Não declarar renderização funcionando com base em geometria sintética.
- Não declarar ganho de desempenho: **não há baseline de FPS/frametime/VRAM**
  neste projeto, porque não há Windows nem GPU no ambiente.
- Não extrair topologia parcial e renderizar com ela. Malha silenciosamente
  errada é pior que nenhuma malha.
- Não adotar tabela de layout de fonte externa sem medir contra os arquivos.
