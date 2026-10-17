# Security

Risks found in the reference client and in this project's own practice. Secret
values are **not** reproduced anywhere in this repository — only the kind of
risk and its location.

## 1. Credential exposure in the reference client — HIGH

**Finding.** `Settings.ini` in the reference client contains an `[AutoLogin]`
section with a plaintext username, a plaintext password, and a `SaveData` flag.
The file is committed to the repository.

**Why this matters.** The credentials are in the Git history, so deleting the
file would not remove them. Anyone with read access to the repository has them.
If the password is reused on any other service, the exposure extends there.

**Recommended action, for the owner of that repository.**

1. Rotate the password immediately, and change it anywhere else it is used.
2. Remove the credential values from `Settings.ini` and ship it with empty
   fields.
3. Add `Settings.ini` to `.gitignore`, or split user credentials into a separate
   file that is never committed.
4. Note that history rewriting would be needed to fully remove the values, and
   that this may break existing clones. Rotation is the action that actually
   closes the risk; history rewriting is cosmetic in comparison.

**Field names and origin, and nothing more.** Per policy this document records
only:

| Field | Source file | Note |
|---|---|---|
| `User` | `Settings.ini`, section `[AutoLogin]` | 7 characters |
| `Password` | `Settings.ini`, section `[AutoLogin]` | 6 characters, digits only |
| `SaveData` | `Settings.ini`, section `[AutoLogin]` | Flag that persists the above |

The values are not reproduced here, in any other document, in any log, or in any
commit.

Note that the password field is a **6-digit numeric string**, i.e. a widely
known weak-password pattern. That makes rotation urgent on its own merits,
independent of the repository exposure.

### Verification that nothing leaked into this repository

Performed and recorded in
[`evidence/ciclo1_security_scan.log`](evidence/ciclo1_security_scan.log). Every
file in `ClienteRecriado` (excluding `.git` and `build/`) was searched for the
complete value of each field, and every hit inspected with the value masked:

- **`User` (7 characters, distinctive): 0 occurrences.**
- **`Password` (6 digits): 5 occurrences, all false positives.** They are the
  canonical CRC-32 test vector `"123456789"` and the hexadecimal literal
  `0x12345678`. A 6-digit value is a prefix of both, so the collision is
  arithmetic. Each line is listed in the evidence log and can be checked.

**Conclusion: no credential value is present in this repository.**

This check must be re-run whenever documentation that quotes the reference
client's configuration is added.

## 2. Protection components — do not touch

The client includes commercial licensing and anti-cheat modules: `License.hinet`,
`main.premium`, `3D.Premium`, `Checksum.premium`, `MHPClient.dll`,
`MHPVerify.dll`, `3DGuard.dll`, and `PS/License.ps`. `Main.exe` is packed with
VMProtect.

`ListaPatch.txt` carries checksums for `main.premium`, `3D.Premium` and
`License.hinet`, so the client integrity-checks its own protection files.

**Policy.** No unpacking, patching, key generation, licence check removal or
circumvention of any kind. This is a hard rule, restated in `AGENTS.md`. Tasks
that appear to require it are out of scope and get recorded as `BLOCKED`.

This is not a technical limitation being dressed up as a policy. Even setting
licensing aside, modifying a protected module would produce something that
cannot be distributed and cannot be maintained.

## 3. Untrusted input handling

Every reader in `src/mu/assets/` processes files that may be corrupt, truncated
or deliberately hostile — a patch server compromise or a malformed download both
land here. The rules the code follows:

- **No reader indexes a buffer without a length check.** Every access is guarded.
- **No reader trusts a length field read from the file.** Declared sizes are
  compared against the bytes actually present, and the smaller is used. A
  truncated download degrades instead of overflowing.
- **Multiplication is guarded against overflow.** OZT dimensions are checked
  against a cap before `width * height * bytes_per_pixel` is computed, so a
  corrupt header cannot wrap to a small value and pass the size check.
- **Readers return `mu::Status`, never throw, never abort.** A single bad file
  in a 29,000-file tree must not take down a scan.
- **Parsing continues past a bad record.** `ManifestParse` records a line number
  and moves on, because one corrupt line should not hide the other 187 entries.

Verified by: truncation tests at multiple offsets for every reader,
out-of-range dimension tests, absurd length tests, and a full run under
AddressSanitizer and UndefinedBehaviorSanitizer with no reports.

## 4. Supply chain

The library has **no third-party dependencies**. There is no `vcpkg.json`, no
vendored tarball, no download step and no package manager in the build. The
attack surface for a dependency compromise is zero, and there are no transitive
licences to track. See [`DEPENDENCIES.md`](DEPENDENCIES.md).

The trade-off is that there is no JPEG pixel decoder. That gap is recorded
honestly rather than closed with an unaudited vendored copy of something.

## 5. Secrets in this repository

None. Checked:

- No credential, token, API key or private key material in any file.
- No `Settings.ini` values copied from the reference client.
- No original asset bytes.
- `.gitignore` excludes build output.

The reference client's `mu.ini` is 111 bytes of binary content of unknown
purpose. It was not copied here and its contents are not reproduced.

## 6. Distribution integrity

`ListaPatch.txt` provides `path crc32 size` records, and `mutool verify`
implements the same CRC-32 (verified against the reference client's own recorded
value for `Data/Item/capesnowlucasnuut02.ozt`). That makes it possible to
integrity-check a distribution before shipping and after install.

**Note on CRC-32 as a security control.** CRC-32 detects accidental corruption
well. It is **not** collision resistant and must not be used to detect
deliberate tampering. A distribution pipeline that needs tamper evidence should
use SHA-256 alongside it. This is recorded so nobody later mistakes the existing
CRC manifest for a security mechanism.

**Current state of the reference client's own manifest.** Verification against
the tree returned 116 matching of 188 entries, with 9 size mismatches, 11 CRC
mismatches and 52 missing files. The shipped client does not match its own
integrity manifest. Details in [`REFERENCE_AUDIT.md`](REFERENCE_AUDIT.md)
section 6.

## 7. Privacy

Twelve `Thumbs.db` files (677,376 bytes) are present in the asset tree. These
are Windows Explorer thumbnail caches and they embed a record of the directory
listing on the machine that packaged the client. They serve no purpose in a
distribution and leak information about whoever built it. `mutool scan` counts
them separately so a packaging step can exclude them.

The `[Discord] DiscordShowPresence = 1` setting means the client reports
presence to a third-party service by default. Anyone redistributing should be
aware that this is on unless the user turns it off.

## 8. Server-side validation

Not weakened, not bypassed, not worked around, and no benchmark in this project
depends on doing so. There is no authorised test server available, so network
compatibility is recorded as `NOT EXECUTED` rather than approximated.

## 9. Riscos operacionais — o ambiente de execução não é estável

Este item existe porque o risco se materializou, não por precaução teórica.

### 9.1 Restauração de snapshot antigo

O ambiente onde este projeto roda **pode ser restaurado de um snapshot antigo
entre um turno e outro**. Durante o ciclo 3 isso aconteceu e produziu dois
efeitos simultâneos:

- o branch de trabalho local voltou ao ponto do clone, perdendo os commits
  locais que **já estavam publicados** no remoto;
- o clone de referência `Cliente_ref` desapareceu do sistema de arquivos.

O risco não é teórico nem raro: é uma propriedade do ambiente. A consequência
prática é que **trabalho não commitado pode desaparecer sem aviso**, e trabalho
commitado mas não publicado pode ficar apontando para uma base errada.

**Mitigação, e ela é a regra permanente de commit:**

| Prática | Por quê |
|---|---|
| Commit **imediatamente** após cada unidade coerente | Minimiza a janela de perda potencial. Trabalho só em memória de edição é o que se perde |
| Tentar push **após cada commit** | Commit local sobrevive ao snapshot só se o repositório sobreviver; o remoto é a cópia que o ambiente não restaura |
| Se o push falhar, **registrar o erro explicitamente** | Nunca afirmar que publicou sem confirmar o estado remoto |
| Verificar `git rev-parse HEAD` contra `git ls-remote` **antes de qualquer scan** | Detecta divergência antes de gerar evidência sobre a base errada |
| Verificar a existência e o hash de `Cliente_ref` antes de medir | Uma medição sobre referência ausente ou trocada é inválida e não se distingue de uma válida |
| `git format-patch` **antes** de qualquer `reset --hard` | O reset é o único comando destrutivo da recuperação; sem o patch salvo ele descarta o trabalho |

**O que fazer quando divergir:** salvar patch → `git fetch` → `git reset --hard
origin/<branch>` → `git apply` → commit → verificar → push. **Nunca**
force-push, nunca reescrever histórico. O remoto não é o problema; o branch
local é que está desalinhado.

Evidência completa do incidente, com reflog e diffs de verificação:
[`evidence/ciclo4_git_incident.log`](evidence/ciclo4_git_incident.log).

### 9.4 Confiabilidade do relógio do sistema — observada em outubro/2026

Durante verificação do commit externo `xulek/muonline-bmd-viewer` `3f11bf89` (auditoria ciclo 6 item 0 e ciclo 9 item 7), o ambiente de execução reportou data local **outubro/2026** (`date -u --iso-8601=seconds` → `2026-10-12T...`), enquanto o commit externo tem `AuthorDate 2026-07-24` e `CommitDate 2026-07-24`.

Em 2026-10-12, a data do commit é passado (julho), não futuro, mas o histórico do repositório externo mostra desordem temporal: commits de 2025-07-02 a 2026-07-24 com um commit 2025-11-09 fora de ordem, indicando **clock skew** no ambiente de origem.

**Risco operacional:** o relógio do ambiente de execução pode não ser confiável para validar datas de commits externos, expiração de certificados, ou ordenação temporal. Em 2026-10-12, o sistema reportou outubro/2026, que é consistente com `date` local, mas não há garantia de sincronização NTP ou de que o relógio não tenha sido manipulado ou restaurado de snapshot.

**Mitigação recomendada:**
- Validar datas de commits externos por meio independente quando possível (ex.: `git show --format=%ad --date=iso` vs `GitHub API` vs `package.json` vs histórico de releases)
- Registrar `git log -1 --format=fuller` com AuthorDate e CommitDate em evidência bruta (`ciclo6_bmdref_commit_date.log`)
- Não usar data do sistema como única fonte para decidir se commit é futuro/passado; comparar com múltiplas fontes
- Para certificados e assinaturas, validar com `openssl` e com serviço externo de timestamp quando aplicável
- Documentar desordem temporal observada (2025-11-09 fora de ordem) como evidência de clock skew

Evidência: `docs/evidence/ciclo6_bmdref_commit_date.log`, `docs/evidence/ciclo9_item6_multimesh_14cat.log` (Data: 2026-10-12T...), e `date -u` durante ciclo 9.



### 9.2 O que o incidente custou, e o que não custou

Nenhum commit foi perdido e nenhum force-push foi feito. O custo real foi de
tempo: diagnóstico, patch, realinhamento, reaplicação e re-verificação. A
verificação por diff mostrou que as duas árvores — a do commit sobre base
errada e a do reaplicado sobre base correta — diferem em **dois arquivos, ambos
logs de evidência regenerados**, com diferença apenas de cabeçalho.

Isso é o resultado esperado da mitigação funcionar, não sorte: o trabalho
estava commitado e o patch foi salvo antes do reset.

### 9.3 Efeito colateral sobre a evidência

Uma medição executada sobre a base errada produz números que parecem válidos e
não são comparáveis com os anteriores. Por isso os logs de evidência deste
projeto registram **o hash HEAD de ambos os repositórios no cabeçalho**. Sem
isso, o incidente teria passado despercebido: os números do ciclo 3 teriam sido
gerados sobre `972fabc` e comparados com os do ciclo 2 gerados sobre `c03264b`,
e a diferença teria parecido regressão de código.

### 9.5 Segurança do parser de ossos — detecção de ciclo e OOM-safe (ciclo 9 item 3 e ciclo 10 Fase A)

**Problema:** hierarquia de ossos com parent pointers pode conter ciclo A->B->A ou self-loop A->A, causando travessia infinita em código futuro que caminha a hierarquia para skinning. Além disso, `boneCount` 0xFFFF (65535) lido de payload corrompido causaria alocação de `bc*59` bytes ~3.8 MB, mas com `bc=0xFFFF` e `mc` grande poderia causar OOM ou overflow `bc*59` em `size_t`.

**Mitigação implementada (commit `f28a02d` e `0d84b0f`):**
- `TryParseBonesAt`: verifica overflow `bc > SIZE_MAX/59` antes de multiplicar, rejeita `bc==0` ou `bc>64` (observado máx 18, limite 64 seguro).
- Detecção de ciclo: para cada osso, segue parents até `bc` passos com `visited[64]` booleano, se revisita nó → ciclo detectado → rejeita arquivo. Também rejeita self-loop `parent==i` diretamente.
- Para `bc=1`, apenas parent -1 (0xFFFF) válido, parent 0 é self-loop → rejeitado. Isso reduziu falso-positivo de controle de 8,32% (2099/25218) para 0,0079% (2/25218), ratio de 5,43× para 1846×, evidência `ciclo9_item2_validation_fixed.log`.
- Testes: `TestBmdBonesCircularParent` (0->1->0 ciclo deve falhar), `ExtremeCount` bc=0xFFFF OOM-safe, `Truncated` no meio lista, `Overflow` bc=0xFFFF. 389 checks 0 falhas, ASan/UBSan 0 reports, evidência `ciclo9_item3_security_test.log`.

**Impacto segurança:** sem detecção de ciclo, código futuro de skinning que faz `while(parent!=-1) parent = bones[parent].parent` entraria em loop infinito, causando deadlock ou CPU 100%. Com detecção, arquivo com ciclo é rejeitado no parsing, não chega a skinning.

### 9.6 Otimização bux cipher — vetorização segura (ciclo 10 Fase C OPT-003)

**Mudança:** `BuxCrypt` reescrito de scalar byte-a-byte para 8 bytes por vez com `uint64` XOR e `memcpy` para loads/stores unaligned safe, sem strict-aliasing violation. Precomputa 3 padrões `key64[3]` para fases 0,1,2 (FC CF AB...), fase avança `(cur_phase+8)%3` a cada chunk.

**Riscos avaliados:**
- **Unaligned access:** usa `memcpy` para carregar/store `uint64`, não cast direto, seguro em x86 e ARM.
- **Phase handling:** testado para fases 0..2 e lengths 0..40, igualdade byte-a-byte com scalar, 389 checks 0 falhas.
- **Timing side-channel:** bux cipher não é criptografia de segurança (XOR 3-byte key), apenas obfuscação de asset, não protege segredo, então timing não é risco. Otimização não introduz branch dependente de dados secretos.
- **Corretude:** self-inverse mantido, `BuxCrypt(BuxCrypt(data)) == data` ainda verdadeiro.

**Evidência:** `ciclo10_opt002_bux_benchmark.log` mostra 880 MB/s → 3437 MB/s 3.9x, sem regressão.
