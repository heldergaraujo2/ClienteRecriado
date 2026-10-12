# Próxima tarefa

> **Ciclo 3 fechado** com `docs/CICLO3_RELATORIO.md`. Este arquivo foi atualizado
> no ciclo 3, conforme a regra permanente — o que não aconteceu durante o ciclo,
> e é a segunda vez que este documento fica para trás.

## Completar `DEC-008` com a comparação de custo/risco

É o que está na frente de tudo, e não é tarefa de código.

`DEC-008` registra a **intenção** (VBO/VAO como piso mínimo) mas não a comparação
de custo/risco que este projeto exige para decisões de arquitetura. Enquanto
estiver incompleta:

- **Nenhum código de backend pode ser escrito** — `DEC-010` lista o que está
  bloqueado: `wglCreateContext`, `glGenBuffers`, `glBindBuffer`, `glBufferData`,
  `glGenVertexArrays`, `glDrawElements`, `glDrawArrays`, shaders, FBOs
- Sem backend não há como medir desempenho, que é o objetivo declarado do projeto
- `PERFORMANCE_BASELINE.md` continua sem baseline de FPS, e continuará

**O que `DEC-008` precisa para ficar completa:**

1. Custo de implementação de cada alternativa considerada
2. Risco de cada uma, **incluindo o risco de não conseguir medir neste ambiente**
3. O que será medido para decidir entre elas, e em que hardware
4. O que acontece com o requisito de compatibilidade visual em cada caso

**Verificação de conformidade enquanto estiver pendente:**

```
grep -rn "glGenBuffers\|wglCreateContext\|glDrawElements\|GL/gl.h" src/
```

Deve retornar apenas comentários. Se retornar código, ele foi escrito antes da
decisão e deve ser revertido, ou a decisão completada.

## Alternativa, se houver acesso a GPU

Iniciar o pipeline VBO/VAO. Não está bloqueado por nenhuma descoberta pendente —
apenas pela ausência de hardware e pela decisão incompleta.

---

## O que NÃO adianta retomar

### Índice de UV por canto — nove hipóteses descartadas

Investigação encerrada. As nove, para não repetição:

| # | Hipótese | Por que falhou |
|---|---|---|
| 1 | Campo em +4 do registro de triângulo | padding — 64,95% zero, 18,18% 0xCDCD |
| 2 | Quarto canto em +24 | não existe — nível de ruído |
| 3 | Relação implícita por posição | refutada — 93% dos arquivos diferem |
| 4 | Bytes 24..59 como índices | platô sem pico; confirmado serem `f32` |
| 5 | `u16@48` do cabeçalho | contagem, não índice |
| 6 | `u16@50` do cabeçalho | parcialmente padding (0xCDCD) |
| 7 | Array paralelo no corpo | 11,8% com distâncias dispersas |
| 8 | `u16@12`/`@14` do vértice | teste não discrimina |
| 9 | Bytes 24..59 como coordenadas UV | 79,42% contra controle de 70,34% — razão 1,13× |

**Não reabrir sem hipótese genuinamente nova** — não uma variação das nove. O
critério de parada existe porque nove falhas seguidas são evidência de que a
pergunta pode estar mal formulada, não de que falta tentar mais uma vez.

### Escala vertical da altura — três caminhos fechados

1. Prefixo dos 29 arquivos de grade crua — inteiramente zero em 29 de 29
2. `xPixelsPerMeter`/`yPixelsPerMeter` do BMP — 2.834 em 42 de 45, que é o padrão
   de 72 DPI, valor default e não medição
3. Paleta BMP — identidade (`palette[i] = (i,i,i)`), então o byte **é** a altura

**O que resta:** procurar em outro arquivo do mundo, ou tratar a escala como
parâmetro de renderização ajustado visualmente. A segunda é provavelmente a
correta — motores desse período costumavam ter a escala como constante.

---

## Problema estrutural não resolvido, e é o mais importante

**Nada obriga o commit que muda o código a mudar o documento.**

A topologia ficou registrada como "não implementada em C++" por dezenas de commits
depois de implementada, justamente porque os testes passavam e a ferramenta
funcionava. **Não havia sinal de que algo estava errado.**

Este ciclo corrigiu quatro afirmações falsas e aplicou quatro correções
estruturais (medições viraram comandos, denominador impresso, regra de controle
da mesma classe, documentos sem hash de ponta). **Nenhuma delas impede a próxima
defasagem.**

Uma verificação automática — por exemplo, um teste que falhe se `COMPATIBILITY.md`
afirmar que algo não está implementado quando há símbolo correspondente em `src/`
— atacaria a causa. Não foi construída neste ciclo.

---

## Números de referência, do full-scan do ciclo 3

Para o próximo ciclo não repetir medição amostral:

| Medição | Valor | População |
|---|---|---|
| Faces parseáveis | 4.149 (41,12%) | 10.090, full-scan |
| Vértice + normal | 8.406 (83,31%) | 10.090, full-scan |
| Hierarquia de ossos | 3.505 = 98,68% dos que fecham | **85,09% dos 4.119 elegíveis** |
| Animação divisível por `12×bc` | 385 (77,00%) | 500, população total |
| Altura de terreno | 76 = 47 BMP + 29 sem cabeçalho | 76, 100% |
| Testes | 344 checks, 0 failures | — |
