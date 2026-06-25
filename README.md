EmberNet-TUI KI TUI Chat Client
===============================

Ein kleiner KI Chatclient mit Toolings und einigen netten Features.

### Dependcies (CMake FetchContent)
- ftxui::ftxui (v6.1.9)
- ftxui::markdown-ui (main branch)
- cpr::cpr (v1.14.2)
- nlohmann::json (v3.12.0)

### Build dependcies
- cmake
- ninja

Das Projekt downloaded alle Abhaengigkeiten direkt, keine lokalen Bibliotheken noetig.

Konfiguriert wird das Projekt mit:

```bash
# cmake -B build -G Ninja
```

Gebaut im Anschluss mit:
```bash
# cmake --build build
```

Und gestartet schliesslich mit:
```bash
# ./build/embernet-tui
```

Das Projekt neu konfigurieren passiert mit:
```bash
# cmake --fresh -B build -G Ninja
```

