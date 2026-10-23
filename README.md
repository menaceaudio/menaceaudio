# Menace Audio — Vocal Chain (VST3)

Compressor → Air (harmônicos estilo Fresh Air) → Delay ping-pong → Reverb → Output.

| Seção    | Knobs            |
|----------|------------------|
| Dynamics | Amount, Speed    |
| Air      | Air, Tone        |
| Delay    | Mix, Time        |
| Reverb   | Mix, Size        |
| Master   | Output (±12 dB)  |

## Opção A — compilar sem instalar nada (GitHub Actions)

1. Crie um repositório no GitHub e suba esta pasta inteira (inclusive `.github`).
2. Aba **Actions → Build Menace Audio → Run workflow**.
3. Quando terminar (uns 10 min), baixe o artefato **MenaceAudio-Setup** (zip com o instalador).
4. Execute `MenaceAudio-Setup.exe` (Next, Install, Finish). Ele instala em `C:\Program Files\Common Files\VST3\`.

## Opção B — compilar no seu PC (Windows)

Requisitos: Visual Studio 2022 (carga "Desenvolvimento para desktop com C++") e CMake.

```
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

O plugin sai em `build\MenaceAudio_artefacts\Release\VST3\Menace Audio.vst3`.
Também sai um `.exe` Standalone para testar sem a DAW.

## No FL Studio

Options → Manage plugins → Find installed plugins. Abra o "Menace Audio" num slot de mixer do vocal.
