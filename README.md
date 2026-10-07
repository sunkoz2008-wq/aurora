# Aurora — multi-effet stéréo VST3 (JUCE 8)

Chaîne de traitement : Gain In → EQ 3 bandes → Saturation (Tanh / Soft / Hard / Fold, sur-échantillonnage 2x) → Chorus → Compresseur + makeup → Delay stéréo / ping-pong (filtre dans le feedback) → Reverb → Auto-Pan + largeur M/S → Gain Out → Limiteur → Dry/Wet global.

Les réglages par défaut sont neutres (le plugin ne change rien au son tant que vous ne montez pas un Mix).

## Obtenir le .vst3 (le plus rapide : GitHub Actions)

1. Créez un dépôt GitHub vide et poussez ce dossier dedans.
2. Onglet **Actions** → le workflow « Build Aurora VST3 » démarre tout seul (5-10 min).
3. Téléchargez les artefacts : `Aurora-Windows-VST3` et `Aurora-macOS-VST3`.

## Compiler en local

Prérequis : CMake 3.22+, Visual Studio 2022 (Windows) ou Xcode (macOS).

    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release

Résultat : `build/Aurora_artefacts/Release/VST3/Aurora.vst3`

## Installation pour Ableton Live 12

- **Windows** : copiez le dossier `Aurora.vst3` dans `C:\Program Files\Common Files\VST3`.
- **macOS** : décompressez l'archive et copiez `Aurora.vst3` dans `/Library/Audio/Plug-Ins/VST3` (ou `~/Library/Audio/Plug-Ins/VST3`). Si macOS bloque le plugin : `xattr -cr /Library/Audio/Plug-Ins/VST3/Aurora.vst3`.
- Dans Live : Préférences → Plug-ins → activez « Utiliser les dossiers système VST3 », puis « Rescanner ». Aurora apparaît dans Plug-ins → VST3.
