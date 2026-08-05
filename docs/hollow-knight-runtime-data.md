# Hollow Knight runtime data map

Target: Hollow Knight 1.4.3.2b, update title `0100633007D48800`, version
`v262144`, base title `0100633007D48000`.

## Health and hit hooks

- `HealthManager.TakeDamage`: RVA `0x163F50`
- `HealthManager.ApplyExtraDamage`: RVA `0x1656B0`
- `HealthManager.OnDisable`: RVA `0x1630C0`
- `HeroController.Die`: RVA `0x172200`
- `HeroController.DieFromHazard`: RVA `0x172270`
- `HealthManager.hp`: object offset `0xE8`
- `HealthManager.enemyType`: object offset `0xEC`
- `HealthManager.battleScene`: object offset `0x110`
- `HealthManager.showGodfinderIcon`: object offset `0x16C`
- `HealthManager.unlockBossScene`: object offset `0x178`

`enemyType` is not a reliable ordinary-enemy/boss discriminator. Boss
classification uses the structural boss markers `battleScene`,
`showGodfinderIcon` and `unlockBossScene`; every target without these markers
uses an ordinary overhead bar, regardless of HP or `enemyType`.

`InstanceID` identifies the current Unity object instance. It is useful for
tracking one live target but must not be treated as a stable catalog ID across
scene reloads or game sessions. `GameObject.name` is the current stable boss
classification candidate.

## HitInstance layout

- `Source`: `0x00`
- `AttackType`: `0x08`
- `CircleDirection`: `0x0C`
- `DamageDealt`: `0x10`
- `Direction`: `0x14`
- `IgnoreInvulnerable`: `0x18`
- `MagnitudeMultiplier`: `0x1C`
- `MoveAngle`: `0x20`
- `MoveDirection`: `0x24`
- `Multiplier`: `0x28`
- `SpecialType`: `0x2C`
- `IsExtraDamage`: `0x30`

Known attack types: `0` Nail, `1` Generic, `2` Spell, `3` Acid, `4`
Splatter, `5` RuinsWater, `6` SharpShadow, `7` NailBeam.

Effective damage is `hpBefore - hpAfter`. `DamageDealt` is the raw/reported
damage and may differ because of target rules, multipliers or invulnerability.
Both player-death paths clear boss state, ordinary-enemy slots and damage
popups immediately.

## Unity object data

- `Component.get_transform`: RVA `0x981290`
- `Component.get_gameObject`: RVA `0x981300`
- `Object.get_name`: RVA `0x9984A0`
- `Object.GetInstanceID`: RVA `0x99B450`
- `Transform.get_position`: RVA `0xBE6F00`
- `Camera.get_main`: RVA `0x97F320`
- `Camera.WorldToScreenPoint`: RVA `0x97ECB0`
- `Object.op_Implicit`: RVA `0x98CC10` (safe destroyed-object check)

These provide the internal name, live instance ID and world position needed
for boss classification, per-enemy state, overhead bars and floating damage.
The current implementation keeps 32 fixed enemy slots and converts Unity's
bottom-origin screen Y to IMGUI's top-origin Y using `screenHeight - screenY`.
Ordinary-enemy bars expire six seconds after their latest hit. This is required
because environmental deaths do not consistently reach the hooked
`HealthManager.OnDisable` path.
Before following a target each frame, the implementation checks both the
`HealthManager` and its current `Transform` through `Object.op_Implicit`.

Damage popups use the effective value `hpBefore - hpAfter`, capture the
target's screen position at hit time, rise 42 pixels, and fade over 1.4
seconds. The current game data exposes the target position rather than an
exact collision contact point, so the popup originates near the target body.

## HUD methods

- `InputHandler.OnGUI`: RVA `0xD21E0`
- `GUI.DrawTexture`: RVA `0xB01230`
- `GUI.Label(Rect, String)`: RVA `0xB00AB0`
- `GUI.get_color`: RVA `0xAFF190`
- `GUI.set_color`: RVA `0xAFF2C0`
- `Texture2D.whiteTexture`: RVA `0xBE4B80`
- `Screen.get_width`: RVA `0xBDC3D0`
- `Screen.get_height`: RVA `0xBDC440`
- `String.CreateString(sbyte*)`: RVA `0x8C9ED0`

Static IL2CPP calls in this build include a hidden null `__this` argument and
the trailing null `MethodInfo` argument. Omitting either caused incorrect ABI
calls during early HUD work.

## Confirmed samples

| Name | Instance ID | enemyType | HP after/total | Effective | Raw | Attack |
|---|---:|---:|---:|---:|---:|---:|
| Climber 1 | 97208 | 0 | confirmed | 32 | 32 | 0 |
| Nightmare King Grimm | 99220 | 1 | 1468/1500 | 32 | 32 | 0 |

Instance IDs above are examples from one session only.
