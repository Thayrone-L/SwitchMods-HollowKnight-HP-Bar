# SwitchMods — Hollow Knight HP Bar

Mod nativo para Hollow Knight no Nintendo Switch que adiciona barras de vida
e indicadores de dano ao HUD do jogo.

## Recursos

- barra grande para bosses, identificados por `HealthManager.enemyType == 1`;
- nome do boss e HP atual/total;
- barras pequenas que acompanham inimigos comuns atingidos;
- registro simultâneo de até 32 inimigos;
- números de dano efetivo que sobem e desaparecem em 1,4 segundo;
- limpeza segura de inimigos destruídos e expiração após seis segundos;
- suporte a dano normal e `ApplyExtraDamage`.

## Versão suportada

- base Title ID: `0100633007D48000`;
- update Title ID: `0100633007D48800`;
- Hollow Knight `1.4.3.2b`, update `v262144`;
- engine Unity IL2CPP.

Os RVAs, offsets, layouts e dados confirmados durante a engenharia reversa
estão documentados em
[`docs/hollow-knight-runtime-data.md`](docs/hollow-knight-runtime-data.md).

## Compilação

Requisitos:

- devkitA64;
- libnx;
- PowerShell;
- clone do [exlaunch](https://github.com/shadowninja108/exlaunch) em
  `tools/exlaunch`.

Execute:

```powershell
.\scripts\build.ps1
```

O resultado é gerado em:

```text
dist/atmosphere/contents/0100633007D48000/exefs/
```

Copie `subsdk9` e `main.npdm` para o mesmo caminho em `atmosphere/contents`
no cartão SD.

## Arquivos não distribuídos

`dump/`, `tools/`, `logs/` e `dist/` são locais e ignorados pelo Git. Dumps do
jogo, chaves e outros conteúdos protegidos não fazem parte deste repositório.

## Aviso

Projeto experimental e não afiliado à Team Cherry ou Nintendo. Use somente
com uma cópia adquirida legalmente do jogo.
