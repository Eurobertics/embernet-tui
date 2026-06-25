#!/bin/bash

# Dieses Skript führt die Unit Tests im Build-Verzeichnis mit ctest aus.

echo "========================================"
echo "Starte Unit Tests mit ctest..."
echo "========================================"

# Wechselt sicher in das Build-Verzeichnis
if [ ! -d "build" ]; then
    echo "FEHLER: Das Verzeichnis 'build' wurde nicht gefunden. Bitte führen Sie zuerst ein Build durch, um dieses zu erstellen."
    exit 1
fi

cd build || exit

# Führt die Tests mit ausführlicher Ausgabe aus (verbose)
ctest --verbose

EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    echo ""
    echo "========================================"
    echo "✅ Alle Unit Tests wurden erfolgreich durchgeführt!"
    echo "========================================"
else
    echo ""
    echo "========================================"
    echo "❌ Achtung: Mindestens ein Unit Test ist fehlgeschlagen (Exit Code: $EXIT_CODE)."
    echo "========================================"
fi