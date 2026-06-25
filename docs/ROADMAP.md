# EmberNet-TUI Lernprojekt

## Ziel

Dieses Projekt dient nicht primär dazu, möglichst schnell einen fertigen KI-Client zu entwickeln.

Das Hauptziel ist es, C++, moderne Softwarearchitektur und das FTXUI-Framework anhand eines realen Projekts zu lernen.

Die Anwendung soll sich funktional an Codex CLI oder Claude Code orientieren und zunächst lokale Ollama-Modelle unterstützen. Später kann die Architektur um weitere Backends wie EmberNet, OpenAI oder Anthropic erweitert werden.

---

# Lernprinzipien

## Verstehen vor Geschwindigkeit

Jeder Schritt soll nachvollziehbar sein.

Code wird nicht einfach übernommen, sondern verstanden:

* Was macht der Code?
* Warum wird er benötigt?
* Welche Alternativen gäbe es?
* Welche Designentscheidung steckt dahinter?

## Kleine Schritte

Neue Konzepte werden einzeln eingeführt.

Beispiel:

* Erst Layout
* Dann Eingabe
* Dann Nachrichtenverwaltung
* Dann Netzwerkzugriffe

Nicht alles gleichzeitig.

## Eigenes Tippen

Der Code wird bewusst selbst geschrieben.

Dadurch werden:

* Syntax
* Struktur
* Wiederholungen
* typische Fehler

deutlich besser verinnerlicht als durch Copy & Paste.

## Kapitelweise Entwicklung

Jedes größere Thema erhält einen eigenen Chat.

Dadurch entsteht gleichzeitig eine Dokumentation und ein Nachschlagewerk.

---

# Geplante Kapitel

## Kapitel 01 – Projektsetup

Ziele:

* Projektstruktur erstellen
* CMake verstehen
* FTXUI einbinden
* Erste lauffähige Anwendung

Ergebnis:

Ein Fenster mit statischem Inhalt.

---

## Kapitel 02 – Das Layout

Ziele:

* Header
* Chatbereich
* Eingabebereich
* Statuszeile

Lernen:

* Renderer
* Elemente
* Layoutsystem
* Container

Ergebnis:

Eine feste Benutzeroberfläche ohne Funktionalität.

---

## Kapitel 03 – Benutzerinteraktion

Ziele:

* Texteingabe
* Tastaturereignisse
* Enter zum Senden

Lernen:

* Input-Komponenten
* Event Handling
* Zustandsverwaltung

Ergebnis:

Nachrichten können eingegeben werden.

---

## Kapitel 04 – Nachrichtenmodell

Ziele:

* Chat-Nachrichten speichern
* Rollen definieren

Lernen:

* Klassen
* Structs
* Vektoren
* Zustandsverwaltung

Ergebnis:

Ein einfacher Chatverlauf.

---

## Kapitel 05 – Architektur

Ziele:

* Projekt aufteilen
* Klassenstruktur entwickeln

Mögliche Komponenten:

* App
* ChatView
* StatusBar
* OllamaClient
* MessageStore

Lernen:

* Header-Dateien
* Trennung von Interface und Implementierung
* Verantwortlichkeiten

---

## Kapitel 06 – Textdarstellung, Input & Scrolling

Ziele:

* Textdarstellung verbessern
* Zeilenumbruch im Chatbereich
* Mehrzeiliges Inputfeld
* Dynamische Chat-Höhe
* Sauberes Scrolling

Lernen:

* Text-Wrapping
* Layoutberechnung
* Flexible Höhenverteilung
* Scroll-Positionen
* Sichtbare vs. tatsächliche Nachrichtenhöhe

Ergebnis:

Der Chatbereich verhält sich deutlich näher an einer echten KI-TUI. Längere Nachrichten werden lesbar dargestellt, das Eingabefeld kann mehrere Zeilen enthalten und der Verlauf lässt sich sauber bedienen.

---

## Kapitel 07 – Ollama Integration

Ziele:

* HTTP Requests
* JSON verarbeiten
* Modelle ansprechen
* Erste lokale KI-Antworten anzeigen

Lernen:

* Netzwerkkommunikation
* Fehlerbehandlung
* APIs
* Trennung zwischen UI und KI-Backend

Ergebnis:

Die Anwendung kann eine Anfrage an Ollama senden und eine Antwort im Chat anzeigen.

---

## Kapitel 08 – Streaming

Ziele:

* Antworten tokenweise anzeigen

Lernen:

* Asynchronität
* Threads
* Event-Schleifen
* UI-Aktualisierung während laufender Arbeit

Ergebnis:

Antworten erscheinen während der Generierung.

---

## Kapitel 09 – Slash Commands

Beispiele:

* /help
* /clear
* /exit
* /model

Lernen:

* Parser
* Befehlsverarbeitung
* UI-Interaktion

---

## Kapitel 10 – Dateianhänge

Beispiele:

* @datei.txt
* Dateiauswahl

Lernen:

* Dateisystem
* Dateidialoge
* Kontextaufbereitung

---

## Kapitel 11 – Konfiguration

Ziele:

* Einstellungen speichern

Beispiele:

* Standardmodell
* API-Endpunkte
* Theme

Lernen:

* JSON-Dateien
* Konfigurationen
* Persistenz

---

## Kapitel 12 – Multi-Backend

Unterstützung für:

* Ollama
* EmberNet
* OpenAI
* Anthropic

Lernen:

* Interfaces
* Abstraktion
* Erweiterbare Architektur

---

## Kapitel 13 – Polishing

Mögliche Themen:

* Scrollbars
* Popup-Fenster
* Modellauswahl
* Farbgestaltung
* Performance
* Logging

---

# Langfristiges Ziel

Am Ende soll eine vollständig nutzbare TUI-Anwendung entstehen, die:

* lokal mit Ollama arbeitet
* mehrere Backends unterstützt
* Streaming beherrscht
* Dateianhänge verarbeiten kann
* eine moderne Terminal-Oberfläche besitzt

Wichtiger als das fertige Programm ist jedoch das Verständnis der Konzepte, die während der Entwicklung gelernt wurden.

Das Projekt ist daher gleichzeitig Anwendung, Lernplattform und persönliches Nachschlagewerk.

