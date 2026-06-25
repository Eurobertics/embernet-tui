# EmberNet-TUI Agenten-Roadmap (Nächste Kapitel)

## Kapitel 17 – Slash Commands & Modellwahl

### Ziel

Ein erstes Command-System einführen, das direkt mit Ollama integriert ist.

### Funktionen

- `/exit`
- `/model`
- `/model <name>`

### Lernziele

- Command Parsing
- String-Verarbeitung
- Trennung von UI und Command-Logik
- Ollama REST API nutzen
- Dynamische Daten statt Hardcoding

### Entwicklungsziele

- Modelle über `/api/tags` abrufen
- Aktives Modell zur Laufzeit wechseln
- Modellname aus dem Request entkoppeln
- Grundlage für spätere Commands schaffen

---

## Kapitel 18 – Dateianhänge (@)

### Ziel

Dateien direkt in den Kontext einer Anfrage einbinden.

### Beispiele

```text
@main.cpp Erkläre mir diese Datei.
@ROADMAP.md Fasse die nächsten Schritte zusammen.
```

### Lernziele

- Dateiauswahl
- Kontextaufbereitung
- Token-Bewusstsein
- UI-Ereignisse

### Entwicklungsziele

- Dateierkennung im Input
- Dateiinhalt laden
- Kontext automatisch erweitern
- Mehrere Anhänge vorbereiten

---

## Kapitel 19 – Session Persistence

### Ziel

Unterhaltungen speichern und später fortsetzen.

### Lernziele

- JSON Serialisierung
- Persistenz
- Session Management
- Zustandswiederherstellung

### Entwicklungsziele

- Chatverlauf speichern
- Chatverlauf laden
- Sessions benennen
- Vorbereitung für Projekt-Sessions

### Geplante Nutzung

```bash
embernet-tui --session embernet
```

---

## Kapitel 20 – Clipboard & Input UX

### Ziel

Moderne Texteingabe wie in echten Agenten-CLIs.

### Lernziele

- Terminal Events
- Plattformunterschiede
- Clipboard APIs
- UX Design

### Entwicklungsziele

- Copy/Paste im Inputfeld
- STRG+C Verhalten überarbeiten
- Komfortfunktionen für längere Prompts
- Vorbereitung für Power-User Workflows

---

# Ausblick

Nach diesen Kapiteln verfügt EmberNet-TUI über:

- Tool Calling
- Tool Loops
- Permission System
- Workspace Lock
- Approval UI
- Konfigurierbare Prompts
- Modellwahl
- Dateianhänge
- Session Persistence
- Moderne Eingabe-UX

Damit entwickelt sich das Projekt von einer Chat-TUI zu einem vollständigen lokalen Agentensystem.
