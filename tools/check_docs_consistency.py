#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verifica se os numeros citados em docs/ batem com a saida real da ferramenta.

Por que este script existe
--------------------------
Este projeto publicou numeros errados em documentos repetidas vezes, e sempre
pela mesma razao: NADA OBRIGAVA O COMMIT QUE MUDA O CODIGO A MUDAR O DOCUMENTO.
Os testes passavam, a ferramenta funcionava, e a prosa ficava para tras. Cinco
ocorrencias registradas:

  - COMPATIBILITY.md afirmou que a topologia "nao esta implementada em C++" por
    dezenas de commits depois de estar.
  - ARCHITECTURE.md sec. 9.1 afirmou que a representacao interna "ainda nao
    estava implementada" quando mesh.hpp existia com testes.
  - ARCHITECTURE.md sec. 9 dizia "oito hipoteses" de UV quando eram nove.
  - Uma tabela do ciclo 1 (1.496 / 11,80%) sobreviveu ao lado da tabela
    corrigida (8.571 / 83,31%) no mesmo documento.
  - Numeros de DEC-008 foram digitados a mao em cinco documentos; o valor nao
    existia na evidencia que os sustentava.

A ausencia desta verificacao foi registrada em DOIS relatorios de ciclo como
"o que continua nao construido". Registrar sem construir e o mesmo que nao
registrar. Este script e a construcao.

O que ele faz
-------------
Roda `mutool bmdreport` na arvore real, extrai os numeros que a ferramenta
produz, e confere cada numero correspondente citado em docs/. Falha (exit 1) se
houver divergencia alem da tolerancia.

Uso:
    python3 tools/check_docs_consistency.py [caminho/para/Data]

Sem argumento, usa $MU_DATA ou /home/user/Cliente_ref/Data.

Deve ser rodado ANTES de qualquer commit que toque documentacao com numeros.
Ver AGENTS.md, secao "Fluxo de commit".

Como adicionar uma checagem
---------------------------
Acrescente uma entrada em CHECKS com a chave como aparece na saida da
ferramenta, o regex que a captura, e os documentos onde o valor e citado. O
script aceita o numero com ou sem separador de milhar e com ponto ou virgula
decimal, porque os documentos estao em pt-BR e a ferramenta em en-US.
"""

import os
import re
import subprocess
import sys

# Tolerancia: os documentos arredondam, a ferramenta tem mais casas. Compara
# com folga de 0,01 ponto percentual ou 1 unidade absoluta.
PCT_TOL = 0.011
ABS_TOL = 1

# Cada entrada: rotulo, regex sobre a saida da ferramenta, tipo, e os arquivos
# onde o valor e citado (junto com o formato esperado no texto).
CHECKS = [
    {
        'label': 'nivel 1 - estrutura valida',
        'regex': r'^\s+valid\s+(\d+)\s+\(\s*([\d.]+)%\)',
        'abs': 10023, 'pct': 99.34,
        'docs': ['docs/COMPATIBILITY.md', 'docs/ASSET_FORMATS.md',
                 'docs/CURRENT_STATE.md'],
        'abs_fmt': '10.023', 'pct_fmt': '99,34%',
    },
    {
        'label': 'nivel 2 - payload em claro (1245+100+7226)',
        'regex': None,  # composto, calculado abaixo
        'abs': 8571, 'pct': 84.95,
        'docs': ['docs/COMPATIBILITY.md', 'docs/ASSET_FORMATS.md',
                 'docs/CURRENT_STATE.md'],
        'abs_fmt': '8.571', 'pct_fmt': '84,95%',
    },
    {
        'label': 'nivel 2 - OPAQUE',
        'regex': r'OPAQUE \(cipher not recovered\)\s+(\d+)\s+\(\s*([\d.]+)%\)',
        'abs': 1519, 'pct': 15.05,
        'docs': ['docs/COMPATIBILITY.md', 'docs/ASSET_FORMATS.md'],
        'abs_fmt': '1.519', 'pct_fmt': '15,05%',
    },
    {
        'label': 'nivel 3 - vertice + normal',
        'regex': r'^\s+parsed\s+(\d+)\s+\(\s*([\d.]+)%\)',
        'abs': 8406, 'pct': 83.31,
        'docs': ['docs/COMPATIBILITY.md', 'docs/ASSET_FORMATS.md',
                 'docs/ARCHITECTURE.md', 'docs/CURRENT_STATE.md'],
        'abs_fmt': '8.406', 'pct_fmt': '83,31%',
    },
    {
        'label': 'nivel 4 - topologia',
        'regex': r'with triangles\s+(\d+)\s+\(\s*([\d.]+)%\)',
        'abs': 4149, 'pct': 41.12,
        'docs': ['docs/COMPATIBILITY.md', 'docs/ASSET_FORMATS.md',
                 'docs/ARCHITECTURE.md', 'docs/CURRENT_STATE.md'],
        'abs_fmt': '4.149', 'pct_fmt': '41,12%',
    },
    {
        'label': 'nivel 4b - queda por multi-mesh',
        'regex': r'multi-mesh \(meshCount != 1\)\s+(\d+)\s+\(\s*([\d.]+)%',
        'abs': 4257, 'pct': 100.00,
        'docs': ['docs/COMPATIBILITY.md', 'docs/ASSET_FORMATS.md',
                 'docs/ARCHITECTURE.md', 'docs/NEXT_TASK.md'],
        'abs_fmt': '4.257', 'pct_fmt': '100,00%',
    },
    {
        'label': 'total de arquivos .bmd',
        'regex': r'total \.bmd files\s+(\d+)',
        'abs': 10090, 'pct': None,
        'docs': ['docs/COMPATIBILITY.md', 'docs/ASSET_FORMATS.md'],
        'abs_fmt': '10.090', 'pct_fmt': None,
    },
]

# Numeros que JÁ estiveram nos documentos e estavam errados. Se reaparecerem,
# alguem reverteu uma correcao. Cada um tem o motivo de estar aqui.
FORBIDDEN = [
    ('1.496', 'payload em claro do ciclo 1, anterior a DEC-007'),
    ('14,83%', 'percentual do ciclo 1 para payload em claro'),
    ('1.191', 'geometria do ciclo 1, anterior a decifragem v0x0C'),
    ('11,80%', 'percentual do ciclo 1 para geometria'),
    ('8.594', 'OPAQUE do ciclo 1, anterior a DEC-007'),
    ('85,17%', 'percentual do ciclo 1 para OPAQUE'),
    ('21.459,9', 'DEC-008: valor digitado a mao, nao medido'),
    ('20.502,7', 'DEC-008: expansao comparada a orcamento de quadro, invalido'),
    ('935,7', 'DEC-008: "submissao indexada por quadro", custo nao medido'),
    ('25,63', 'DEC-008: razao derivada de valores digitados'),
    ('21,91', 'DEC-008: razao sobre comparacao invalida'),
    ('128,8%', 'DEC-008: expansao como % de quadro, custo e de carregamento'),
    ('123,0%', 'DEC-008: idem, versao seguinte do mesmo erro'),
]

# Relatorios de ciclo FECHADO sao registro historico: eram verdadeiros quando
# escritos, e a regra permanente proibe reabrir numero de ciclo fechado. Ficam
# fora da checagem de proibidos por inteiro.
FORBIDDEN_ALLOWLIST_GLOB = ('docs/CICLO', '_RELATORIO')

# Fora desses, um numero proibido SO pode aparecer se a linha que o contem
# estiver marcada. A convencao e um marcador explicito no fim da linha:
#
#     <!-- historico -->
#
# Por que um marcador em vez de deduzir do contexto (blockquote, titulo de
# secao): deduzir e fragil. Um blockquote pode ser citação de qualquer coisa, e
# renomear uma secao quebraria a verificacao em silencio. O marcador e grepavel,
# nao deriva de nada, e obriga quem escreve a declarar que esta citando um
# numero obsoleto de proposito. Sem ele, a checagem falha.
HISTORIC_MARKER = '<!-- historico -->'


# Checagens por LINHA DE TABELA. A checagem por presenca ("o numero certo
# aparece em algum lugar do arquivo?") NAO detecta um numero errado coexistindo
# com o certo - e foi exatamente isso que um teste de injecao revelou: trocar
# 4.149 por 4.999 numa linha passou, porque 4.149 continuava aparecendo em
# outras seis linhas do mesmo documento.
#
# Estas checagens localizam a LINHA pelo rotulo e conferem o numero DENTRO
# dela. Um numero errado na tabela falha mesmo que o certo apareca alhures.
#
# Cada entrada: rotulo da linha (substring), chave da medicao, e os documentos
# que contem a tabela.
TABLE_CHECKS = [
    ('1 — estrutura', 'nivel 1 - estrutura valida'),
    ('2 — payload', 'nivel 2 - payload em claro (1245+100+7226)'),
    ('3 — vértice', 'nivel 3 - vertice + normal'),
    ('4 — topologia', 'nivel 4 - topologia'),
    ('6 — UV', None),  # sempre zero, verificado abaixo
]

TABLE_DOCS = ['docs/COMPATIBILITY.md', 'docs/ASSET_FORMATS.md']


def norm(text):
    """Normaliza para comparar: remove separador de milhar, unifica decimal."""
    return text.replace('.', '').replace(',', '.')


def run_report(data_dir):
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.dirname(here)
    binary = os.path.join(root, 'build', 'mutool')
    if not os.path.exists(binary):
        sys.stderr.write('erro: %s nao existe; rode "make" antes\n' % binary)
        return None
    p = subprocess.run([binary, 'bmdreport', data_dir],
                       capture_output=True, text=True)
    # bmdreport sai com 2 quando ha payload opaco, o que e esperado e nao e
    # falha desta verificacao.
    if p.returncode not in (0, 2):
        sys.stderr.write('erro: mutool bmdreport falhou (%d)\n%s\n'
                         % (p.returncode, p.stderr[:400]))
        return None
    return p.stdout


def measure(out):
    """Extrai abs/pct de cada CHECK a partir da saida real da ferramenta."""
    measured = {}
    # Nivel 2 e composto: plaintext + bux + v0x0C.
    parts = {}
    for key, pat in (('plaintext', r'^\s+plaintext\s+(\d+)'),
                     ('bux', r'^\s+bux-decryptable\s+(\d+)'),
                     ('v0c', r'^\s+v0x0C-decryptable\s+(\d+)')):
        m = re.search(pat, out, re.M)
        parts[key] = int(m.group(1)) if m else None
    total_m = re.search(r'total \.bmd files\s+(\d+)', out)
    total = int(total_m.group(1)) if total_m else 0

    for c in CHECKS:
        if c['regex'] is None:
            if any(v is None for v in parts.values()) or not total:
                measured[c['label']] = (None, None)
                continue
            absval = sum(parts.values())
            measured[c['label']] = (absval, 100.0 * absval / total)
            continue
        m = re.search(c['regex'], out, re.M)
        if not m:
            measured[c['label']] = (None, None)
            continue
        absval = int(m.group(1))
        pctval = float(m.group(2)) if m.lastindex and m.lastindex >= 2 else None
        measured[c['label']] = (absval, pctval)
    return measured


def main():
    data_dir = (sys.argv[1] if len(sys.argv) > 1
                else os.environ.get('MU_DATA', '/home/user/Cliente_ref/Data'))
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.dirname(here)

    if not os.path.isdir(data_dir):
        sys.stderr.write('erro: arvore de dados nao encontrada: %s\n' % data_dir)
        sys.stderr.write('      passe o caminho como argumento ou defina MU_DATA\n')
        return 2

    out = run_report(data_dir)
    if out is None:
        return 2
    measured = measure(out)

    print('check_docs_consistency: docs/ contra "mutool bmdreport %s"' % data_dir)
    print()

    failures = []
    checked = 0

    # ---- 1. Numeros citados conferem com a ferramenta?
    print('--- numeros citados vs saida da ferramenta ---')
    for c in CHECKS:
        absval, pctval = measured[c['label']]
        if absval is None:
            failures.append('%s: a ferramenta nao produziu este valor' % c['label'])
            print('  FALHOU  %s: nao encontrado na saida' % c['label'])
            continue
        print('  %-46s ferramenta: %d' % (c['label'], absval), end='')
        if pctval is not None:
            print(' (%.2f%%)' % pctval, end='')
        print()

        for doc in c['docs']:
            path = os.path.join(root, doc)
            if not os.path.exists(path):
                failures.append('%s citado em %s, que nao existe' % (c['label'], doc))
                continue
            text = open(path, encoding='utf-8').read()
            checked += 1
            if c['abs_fmt'] and c['abs_fmt'] not in text:
                failures.append('%s: %s nao cita %s (ferramenta: %d)'
                                % (c['label'], doc, c['abs_fmt'], absval))
                print('      FALHOU %s nao cita %s' % (doc, c['abs_fmt']))
            if c['pct_fmt'] and c['pct_fmt'] not in text:
                failures.append('%s: %s nao cita %s (ferramenta: %.2f%%)'
                                % (c['label'], doc, c['pct_fmt'], pctval or 0))
                print('      FALHOU %s nao cita %s' % (doc, c['pct_fmt']))

    # ---- 1b. O numero DENTRO de cada linha da tabela confere?
    print()
    print('--- numero dentro de cada linha da tabela de seis niveis ---')
    for doc in TABLE_DOCS:
        path = os.path.join(root, doc)
        if not os.path.exists(path):
            failures.append('%s nao existe' % doc)
            continue
        for line in open(path, encoding='utf-8').read().splitlines():
            if not line.lstrip().startswith('|'):
                continue
            for label, key in TABLE_CHECKS:
                if label not in line:
                    continue
                checked += 1
                cells = [c.strip() for c in line.strip().strip('|').split('|')]
                # A tabela e: | Nivel | O que mede | Arquivos | % |
                if len(cells) < 4:
                    failures.append('%s: linha "%s" tem %d celulas, esperava 4'
                                    % (doc, label, len(cells)))
                    continue
                num_txt = cells[2].replace('*', '').replace('.', '')
                pct_txt = cells[3].replace('*', '').replace('%', '').replace(',', '.')
                if key is None:
                    # Nivel 6 (UV) e zero por definicao enquanto UV nao for
                    # resolvido. Se deixar de ser zero, isto deve falhar para
                    # obrigar a atualizar a documentacao junto.
                    if num_txt != '0':
                        failures.append('%s: nivel 6 (UV) cita %s; se UV foi '
                                        'resolvido, atualize a documentacao e '
                                        'esta verificacao' % (doc, cells[2]))
                        print('  FALHOU  %s nivel 6 = %s' % (doc, cells[2]))
                    else:
                        print('  ok      %s "%s" = 0' % (doc, label))
                    continue
                absval, pctval = measured.get(key, (None, None))
                if absval is None:
                    failures.append('%s: sem medicao para "%s"' % (doc, label))
                    continue
                try:
                    got = int(num_txt)
                except ValueError:
                    failures.append('%s: "%s" nao e um inteiro na linha "%s"'
                                    % (doc, cells[2], label))
                    print('  FALHOU  %s "%s" -> %r' % (doc, label, cells[2]))
                    continue
                if abs(got - absval) > ABS_TOL:
                    failures.append('%s: linha "%s" cita %d, ferramenta diz %d'
                                    % (doc, label, got, absval))
                    print('  FALHOU  %s "%s" = %d, ferramenta = %d'
                          % (doc, label, got, absval))
                    continue
                if pctval is not None:
                    try:
                        gotp = float(pct_txt)
                    except ValueError:
                        gotp = None
                    if gotp is not None and abs(gotp - pctval) > PCT_TOL:
                        failures.append('%s: linha "%s" cita %.2f%%, ferramenta '
                                        'diz %.2f%%' % (doc, label, gotp, pctval))
                        print('  FALHOU  %s "%s" = %.2f%%, ferramenta = %.2f%%'
                              % (doc, label, gotp, pctval))
                        continue
                print('  ok      %s "%s" = %d (%.2f%%)' % (doc, label, got, pctval or 0))

    # ---- 2. Numeros ja corrigidos reapareceram?
    print()
    print('--- numeros ja corrigidos que nao podem voltar ---')
    for doc in sorted(os.listdir(os.path.join(root, 'docs'))):
        if not doc.endswith('.md'):
            continue
        rel = 'docs/' + doc
        # Relatorio de ciclo fechado: registro historico, nao e verificado.
        if all(part in rel for part in FORBIDDEN_ALLOWLIST_GLOB):
            continue
        lines = open(os.path.join(root, 'docs', doc), encoding='utf-8').read().splitlines()
        for needle, reason in FORBIDDEN:
            for lineno, line in enumerate(lines, 1):
                if needle not in line:
                    continue
                if HISTORIC_MARKER in line:
                    continue
                failures.append('%s:%d contem %s (%s) sem o marcador "%s"'
                                % (rel, lineno, needle, reason, HISTORIC_MARKER))
                print('  FALHOU  %s:%d contem %s - %s' % (rel, lineno, needle, reason))

    # ---- 3. A trava de DEC-010 continua valendo?
    print()
    print('--- trava de DEC-010: nenhum include grafico em src/ ---')
    bad = []
    for dirpath, _, files in os.walk(os.path.join(root, 'src')):
        for f in files:
            if not f.endswith(('.hpp', '.cpp', '.h', '.cc')):
                continue
            p = os.path.join(dirpath, f)
            for i, line in enumerate(open(p, encoding='utf-8'), 1):
                ls = line.strip()
                if ls.startswith('#include') and re.search(
                        r'gl\.h|glew|glad|GL/|vulkan|d3d', ls, re.I):
                    bad.append('%s:%d %s' % (os.path.relpath(p, root), i, ls))
    checked += 1
    if bad:
        for b in bad:
            failures.append('include grafico em src/: %s' % b)
            print('  FALHOU  %s' % b)
    else:
        print('  ok      zero includes graficos em src/')

    # ---- 4. A relacao de subconjunto continua verdadeira?
    print()
    print('--- nivel 4 e subconjunto do nivel 3 ---')
    a3 = measured.get('nivel 3 - vertice + normal', (None,))[0]
    a4 = measured.get('nivel 4 - topologia', (None,))[0]
    checked += 1
    if a3 is None or a4 is None:
        failures.append('nao foi possivel conferir nivel 4 <= nivel 3')
        print('  FALHOU  valores ausentes')
    elif a4 > a3:
        failures.append('nivel 4 (%d) maior que nivel 3 (%d): subconjunto violado'
                        % (a4, a3))
        print('  FALHOU  %d > %d' % (a4, a3))
    else:
        print('  ok      %d <= %d' % (a4, a3))

    print()
    print('%d verificacoes, %d falhas' % (checked, len(failures)))
    if failures:
        print()
        for f in failures:
            print('  - %s' % f)
        return 1
    print('documentacao consistente com a ferramenta.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
