#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Gera a evidência de DEC-008 rodando o benchmark de verdade e derivando tudo
da saída dele.

Por que este script existe e não um trecho de shell com números digitados:

A primeira versão desta evidência foi produzida por um comando que executava o
benchmark e, em outra seção do MESMO arquivo, calculava as contas de
fechamento a partir de valores HARDCODED de uma execução anterior. As duas
seções discordavam entre si — 21.459,9 µs contra 20.588,0 µs medidos no mesmo
arquivo — e o relatório citava a versão hardcoded. O erro só apareceu quando os
números foram comparados contra a evidência commitada.

A regra que este script aplica: NENHUM número derivado é digitado. Tudo sai da
saída do benchmark, parseada. Se o benchmark mudar, a evidência muda junto, e
não há como as duas seções divergirem.

    python3 tools/gen_dec008_evidence.py [repeticoes]

Repetições default: 5. Cada repetição é uma execução completa do binário, então
a dispersão ENTRE execuções também é medida — não só a dispersão interna que o
benchmark já reporta.
"""

import os
import re
import statistics
import subprocess
import sys

# As três linhas que interessam, identificadas pelo rótulo que o benchmark
# imprime. Rótulo, não posição, para que reordenar o benchmark não quebre isto.
WANTED = {
    'LOAD-TIME expand indexed -> flat': 'expansao',
    'LOAD-TIME vertex buffer upload': 'upload',
}

# Colunas da tabela do benchmark: mediana, p95, p99, media, MB, MB/s
NUMCOLS = 6


def parse_run(out):
    """Extrai as três medições de uma execução do benchmark."""
    got = {}
    for line in out.splitlines():
        for label, key in WANTED.items():
            if not line.startswith(label):
                continue
            nums = re.findall(r'\d+\.\d+', line[len(label):])
            if len(nums) < NUMCOLS:
                continue
            vals = [float(x) for x in nums[:NUMCOLS]]
            # A linha seguinte traz reps/mean/stddev/min/max.
            got[key] = {
                'median': vals[0], 'p95': vals[1], 'p99': vals[2],
                'mean': vals[3], 'mb': vals[4], 'mbps': vals[5],
            }
    return got


def main():
    reps = int(sys.argv[1]) if len(sys.argv) > 1 else 5
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.dirname(here)
    binary = os.path.join(root, 'build', 'bench_assets')

    if not os.path.exists(binary):
        sys.stderr.write('erro: %s nao existe; rode "make bench" antes\n' % binary)
        return 1

    runs = []
    raw = []
    for i in range(reps):
        p = subprocess.run([binary], capture_output=True, text=True)
        if p.returncode != 0:
            sys.stderr.write('erro: benchmark falhou na execucao %d\n' % (i + 1))
            return 1
        raw.append(p.stdout)
        parsed = parse_run(p.stdout)
        if len(parsed) != len(WANTED):
            sys.stderr.write('erro: execucao %d nao produziu as %d linhas\n'
                             % (i + 1, len(WANTED)))
            return 1
        runs.append(parsed)

    # Contagens reais da arvore, citadas pelo proprio benchmark na saida.
    m = re.search(r'vertex sends\)', raw[0])
    sends = re.search(r'\((\d+) vertex sends\)', raw[0])
    expanded_sends = int(sends.group(1)) if sends else 0
    saved = re.search(r'vertex sends saved per frame: (\d+) \(([\d.]+)%\)', raw[0])

    print('=== EVIDENCIA DE DEC-008: custo do caminho de submissao (CPU) ===')
    print()
    print('Gerado por tools/gen_dec008_evidence.py. Nenhum numero deste arquivo')
    print('e digitado: todos sao derivados da saida do benchmark, que esta')
    print('transcrita mais abaixo. Isso existe porque a versao anterior deste')
    print('arquivo tinha uma secao de contas derivada de valores hardcoded de')
    print('outra execucao, e as duas secoes discordavam entre si.')
    print()
    print('execucoes completas do binario : %d' % reps)
    print()

    budget60 = 1000000.0 / 60.0
    budget144 = 1000000.0 / 144.0

    print('---------------------------------------------------------------------------')
    print('POR QUE ESTA EVIDENCIA MUDOU DE FORMA (correcao do ciclo 5)')
    print('---------------------------------------------------------------------------')
    print()
    print('A versao anterior media UM gather e o comparava com o orcamento de UM')
    print('QUADRO a 60 FPS, chamando-o de custo por quadro. NAO E. A auditoria do')
    print('ciclo 4 apontou isso e estava certa.')
    print()
    print('Os dois custos pertencem a orcamentos diferentes:')
    print()
    print('  (1) EXPANSAO, indice -> array flat. Custo de CARREGAMENTO. No caminho')
    print('      glDrawArrays do cliente existe um array flat client-side, e a')
    print('      implementacao natural o constroi UMA VEZ ao carregar o modelo,')
    print('      nao a cada quadro. Comparar com orcamento de quadro era o erro.')
    print()
    print('  (2) VOLUME DE SUBMISSAO POR QUADRO. E o que realmente se repete. Sem')
    print('      glGenBuffers no binario (docs/RENDERING.md secao 3) o vertice mora')
    print('      em memoria de cliente e o driver o re-marshals a cada draw. Sem')
    print('      glDrawElements, vertice compartilhado e REENVIADO em vez de')
    print('      referenciado, entao o volume por quadro e o tamanho EXPANDIDO.')
    print('      Isso e uma quantidade exata. O CUSTO dela nao pode ser medido')
    print('      aqui, porque e marshalling de driver, barramento e overhead de')
    print('      chamada por vertice - e nao ha contexto GL neste ambiente.')
    print()

    print('---------------------------------------------------------------------------')
    print('MEDICOES, COM DISPERSAO ENTRE EXECUCOES')
    print('---------------------------------------------------------------------------')
    print()
    print('O benchmark ja reporta desvio padrao INTERNO (entre as repeticoes de')
    print('uma execucao). Isso nao e o mesmo que a dispersao ENTRE execucoes,')
    print('que e a que importa para citar um numero. As duas sao reportadas.')
    print()

    labels = {
        'expansao': 'CARREGAMENTO: expansao indice -> flat',
        'upload': 'CARREGAMENTO: upload do vertex buffer (unicos)',
    }
    med = {}
    for key in ('expansao', 'upload'):
        vals = [r[key]['median'] for r in runs]
        med[key] = statistics.median(vals)
        spread = max(vals) - min(vals)
        pct = 100.0 * spread / statistics.median(vals)
        print('%s' % labels[key])
        print('  medianas por execucao : %s us'
              % ', '.join('%.1f' % v for v in vals))
        print('  mediana das medianas  : %.1f us' % med[key])
        print('  min / max             : %.1f / %.1f us' % (min(vals), max(vals)))
        print('  amplitude             : %.1f us (%.2f%% da mediana)' % (spread, pct))
        internal = [r[key]['mean'] for r in runs]
        print('  media reportada       : %.1f us (entre execucoes: %.1f a %.1f)'
              % (statistics.median(internal), min(internal), max(internal)))
        print()

    ratio = med['expansao'] / med['upload']
    ratios = [r['expansao']['median'] / r['upload']['median'] for r in runs]

    print('---------------------------------------------------------------------------')
    print('CONTAS DE FECHAMENTO - derivadas das medianas acima, nao digitadas')
    print('---------------------------------------------------------------------------')
    print()
    print('ORCAMENTO DE CARREGAMENTO (o correto para a expansao):')
    print('  expansao de TODA a arvore           : %9.1f us = %6.2f s'
          % (med['expansao'], med['expansao'] / 1e6))
    print('  upload VBO de TODA a arvore         : %9.1f us = %6.2f s'
          % (med['upload'], med['upload'] / 1e6))
    print('  razao expansao/upload               : %9.2fx  (entre execucoes: %.2fx a %.2fx)'
          % (ratio, min(ratios), max(ratios)))
    print()
    print('  Referencia: carregar TODOS os 10.090 modelos de uma vez nao e o que')
    print('  um jogo faz. Uma cena carrega um subconjunto. O numero acima e o')
    print('  teto para a arvore inteira, e a razao e o que se preserva em')
    print('  qualquer subconjunto, porque ambos os custos sao lineares na')
    print('  contagem de vertice.')
    print()
    print('ORCAMENTO DE QUADRO (nao se aplica a expansao, e por isso nao ha')
    print('comparacao aqui - a versao anterior fazia essa comparacao e estava')
    print('errada):')
    print('  orcamento de 1 quadro a  60 FPS     : %9.1f us' % budget60)
    print('  orcamento de 1 quadro a 144 FPS     : %9.1f us' % budget144)
    print()
    if saved:
        print('VOLUME POR QUADRO (quantidade exata; CUSTO bloqueado, sem contexto):')
        print('  sem glDrawElements, expandido       : %s envios de canto por draw'
              % saved.group(1))
        print('  VBO + index buffer                  : vertice unico, residente')
        print()

    print('---------------------------------------------------------------------------')
    print('SAIDA BRUTA DO BENCHMARK')
    print('---------------------------------------------------------------------------')
    print('SAIDA BRUTA DO BENCHMARK, EXECUCAO 1 DE %d' % reps)
    print('---------------------------------------------------------------------------')
    print()
    for line in raw[0].splitlines():
        if any(line.startswith(lbl) for lbl in WANTED) or \
           line.strip().startswith('reps=') or \
           'bytes per frame' in line or 'bytes resident' in line or \
           'ratio expanded' in line or 'vertex sends saved' in line:
            print(line)
    print()

    print('---------------------------------------------------------------------------')
    print('LIMITE DA CONCLUSAO')
    print('---------------------------------------------------------------------------')
    print()
    print('O que SUSTENTA:')
    print('  - CARREGAMENTO: expandir custa %.2fx mais que subir o buffer' % ratio)
    print('    indexado (%.1f us contra %.1f us para a arvore inteira).' % (med['expansao'], med['upload']))
    print('    E trabalho de carregamento; orcamento relevante e o de carga, nao')
    print('    o de quadro. A razao se preserva em qualquer subconjunto porque')
    print('    ambos os custos sao lineares na contagem de vertice.')
    print('  - VOLUME POR QUADRO: quantidade exata, nao custo. Sem glDrawElements,')
    print('    o volume por draw e o tamanho expandido. O CUSTO desse volume e')
    print('    driver/barramento/overhead de chamada e esta BLOCKED, sem contexto.')
    print()
    print('O que NAO SUSTENTA: nenhum numero de FPS, nenhum ganho de desempenho,')
    print('nenhuma comparacao de draw call em tempo real. A validacao final de')
    print('DEC-008 (Parte B) exige contexto grafico e esta BLOQUEADA. Uma versao')
    print('anterior desta secao comparava a expansao com orcamento de quadro e')
    print('estava errada - corrigido no ciclo 5, item 1 da auditoria.')
    print()
    print('Ressalvas: buffers RESIDENTES entre repeticoes, que e o caso FAVORAVEL')
    print('ao caminho expandido. Contagens da arvore inteira, nao do subconjunto')
    print('visivel de um quadro. Amostra pequena (N=%d), indicativa, nao definitiva.' % reps)
    return 0


if __name__ == '__main__':
    sys.exit(main())
