# EmberNet-TUI Agenten-Roadmap (Phase 2)

## Leitprinzipien

### Keine Wegwerfarchitektur

Wir bauen keine Zwischenlösungen, die später vollständig ersetzt werden müssen.

Wenn das Endziel bereits bekannt ist:

- Permission System
- Workspace Lock
- Approval UI
- Prompt Dateien

dann wird die Architektur von Anfang an darauf vorbereitet.

### Kleine Schritte, aber richtige Richtung

Komplexe Themen dürfen in kleine Lernschritte zerlegt werden.

Nicht:

Allowlist
→ Subcommand Allowlist
→ Permission System
→ Approval UI

Sondern:

Permission System
→ Workspace Lock
→ Approval UI

Jeder Schritt soll später weiterverwendet werden.

### Sicherheit vor Komfort

Ein Agent darf nicht automatisch alles ausführen.

Die Sicherheitsarchitektur ist Teil des Designs und kein späteres Add-on.

---

# Kapitel 13 – Permission System & Konfiguration

## Ziel

Ein zentrales Sicherheitsmodell einführen.

Grundlage für alle zukünftigen Agentenfunktionen.

## Neue Konzepte

```cpp
enum class PermissionMode
{
    ReadOnly,
    WorkspaceWrite,
    Dangerous
};
```

```cpp
struct AppConfig
{
    PermissionMode permission_mode;
};
```

## CLI Parameter

Vorbereitung für:

- --allow-write
- --allow-outside-workdir
- --dangerously-skip-approval
- --prompt-file <file>

## Ergebnis

Eine zentrale Konfiguration existiert.

Tools können zukünftig Berechtigungen prüfen.

---

# Kapitel 14 – Workspace Lock & Dateischreibzugriff

## Status

Abgeschlossen

## Ergebnis

Implementiert:

- read_file
- read_directory
- write_file
- create_directory
- Workspace Lock
- Permission Integration

Der Agent kann Dateien und Verzeichnisse innerhalb des Workspace sicher erzeugen und lesen.

---

# Kapitel 15 – Approval UI & Dateimanipulation

## Ziel

Kritische Agentenaktionen benötigen eine Benutzerbestätigung.

Gleichzeitig werden die verbleibenden Dateitools implementiert.

## Neue Tools

- replace_text
- delete_file
- delete_directory

## Sicherheitsmodell

Lesen:

- Workspace Lock

Schreiben:

- Workspace Lock
- PermissionMode

Ändern:

- Workspace Lock
- PermissionMode
- Approval UI

Löschen:

- Workspace Lock
- PermissionMode
- Approval UI

Kommandos:

- PermissionMode
- Approval UI

## Approval UI

Beispiele:

Gemma möchte ausführen:

replace_text src/App.cpp

[A] Allow Once
[D] Deny

Gemma möchte ausführen:

delete_file docs/old_notes.md

[A] Allow Once
[D] Deny

Gemma möchte ausführen:

git commit -m "Fix"

[A] Allow Once
[D] Deny

## Lernziele

- Modal Dialoge in FTXUI
- Blockierende Benutzerentscheidungen
- Synchronisation zwischen Agent und UI
- Approval Workflow
- Sichere Dateimanipulation

## Ergebnis

Der Benutzer bleibt jederzeit in Kontrolle.

Die Tooling-Basis ist anschließend weitgehend vollständig:

- read_file
- read_directory
- write_file
- create_directory
- replace_text
- delete_file
- delete_directory
- exec_command

Kapitel 15 markiert den Abschluss der grundlegenden Agenten-Tooling-Infrastruktur.

---

# Kapitel 16 – System Prompt & Agentenidentität

## Ziel

Prompts werden konfigurierbar.

## Reihenfolge

1. --prompt-file
2. ~/.config/embernet-tui/system_prompt.md
3. eingebauter Standardprompt

## Vorteile

- Unterschiedliche Agentenrollen
- Projektbezogene Prompts
- Eigene Arbeitsweisen

## Ergebnis

Der Agent wird anpassbar und projektfähig.

---

# Langfristiges Ziel

Am Ende soll EmberNet-TUI über ein Sicherheitsmodell verfügen, das sich an modernen Agentensystemen orientiert:

- Tool Calling
- Tool Loop
- Lokale Prozessausführung
- Permission System
- Workspace Lock
- Approval Workflow
- Konfigurierbare Prompts

→ Grundlage für autonome Agenten

Nicht maximale Autonomie ist das Ziel.

Das Ziel ist kontrollierte Autonomie mit nachvollziehbaren Entscheidungen und klaren Sicherheitsgrenzen.
