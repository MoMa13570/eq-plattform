# EQ Plattform

![EQ Plattform Frontansicht](images/EQ-Plattform.png)

## Über das Projekt

Die **EQ Plattform** st zum Nachführen von Dobson teleskopen, um die visuelle Beobachtung zu verbessern oder fotogarfisch nutzbar zu machen.

Unsere EQ-Plattform ist vergleihsweise massiv gebaut, damit sie auch schwere Dobson tragen kann. 
Für den Antrieb gibt es 2 Möglichkeiten:
- mit einem EQ2 Motor Kit
- mit einer PCB und einem Nema17 Motor

## Funktionen & Eigenschaften

- Modularer mechanischer Aufbau mit austauschbaren Komponenten  
- Open-Source-Hardware und Software  
- Frei verfügbare Fertigungsdaten, Schaltpläne und Layouts 
- Teilweise Nachbau mit handelsüblichen Bauteilen möglich  
- Klare Dokumentation für Aufbau und Erweiterung  
- Geeignet für Schulen, Sternwarten oder Privatanwender

## Dateien & Ressourcen

Alle relevanten Dateien für den Nachbau und die Weiterentwicklung sind im GitHub-Repository organisiert und frei zugänglich.



- **STEP-Dateien (Mechanik)**  
  CAD-Daten für mechanische Bauteile  
  [Mechanische Hardware](https://github.com/MoMa13570/eq-plattform/tree/main/hardware/mechanical/normal)
  [Mechanische Hardware_simple](https://github.com/MoMa13570/eq-plattform/tree/main/hardware/mechanical/simple)
  [Krteissegmente](https://github.com/MoMa13570/eq-plattform/tree/main/hardware/mechanical/Kreissegmente aus Aluminium)
  Die Dateien im ordner sind vereinfacht, damit diese kostengünstiger produziert werden können. Im Vergleich zur normalen Version fehlen die Einsparungn für die Libelle, den Kompass und die Windrose in der Unterplatte. In der Oberplatte fehlen die Nord- und Südmarkierungen.

- **Stückliste (BOM)**  
  Übersicht aller benötigten Bauteile  
  [BOM (CSV)](BOM/Teileliste_DM%20EQ%20Plattform.csv)

- **Bauanleitung / Assembly**  
  Mechanischer und elektrischer Aufbau  
  [Assembly](https://github.com/MoMa13570/eq-plattform/tree/main/Assembly)

## optional:

- **Gerber-Daten (PCB-Fertigung)**  
  Leiterplatten-Layouts, Fertigungsdaten und Gehäuse
  [Platine](https://github.com/MoMa13570/eq-plattform/tree/main/hardware/circuitboard)  

- **Software**  
  Firmware für Arduino
  [Software](https://github.com/MoMa13570/eq-plattform/tree/main/software)

## Aufbau

Der Aufbau der EQ Plattform erfolgt modular und ist in der Bauanleitung detailliert beschrieben.  
Die Dokumentation umfasst:

- mechanischen Aufbau  
- Bestückung der Leiterplatten (optional) 
- grundlegende Inbetriebnahme  

➡️ **Zur Aufbauanleitung:**  
[Assembly / Bauanleitung](https://github.com/MoMa13570/eq-plattform/tree/main/Assembly)

## Softwareinstallation

Am Einfachsten geht die installation über Visual Studio Code. 
Schritt für Schritt (Visual Studio Code im Folgenden: VS):
- VS installieren
- Im Extension Manager von VS die Extension PlatformIO installieren
- Von Github das Projekt als Zip herunterladen und entpacken
- In VS im Reiter PlatformIO auf Pick a Folder und den entsprechenden Treiberordner in dem Ordner Software wählen
- Atdiono anstecken und über Plattformio das Programm hochladen

## Mitmachen & Weiterentwicklung

Die EQ Plattform ist ein offenes Projekt und lebt von Weiterentwicklung und Feedback.  
Beiträge sind ausdrücklich willkommen, z. B.:

- Verbesserung der Dokumentation  
- Erweiterungen der Hardware oder Software  
- Fehlerkorrekturen  
- neue Module oder Anwendungsbeispiele  

Beiträge können über **Issues** oder **Pull Requests** im GitHub-Repository eingebracht werden.

---

**Projekt-Repository:**  
https://github.com/MoMa13570/eq-plattform