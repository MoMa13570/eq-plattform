<style>
  :root {
    --eq-bg: #f7f9fb;
    --eq-surface: #ffffff;
    --eq-ink: #17212b;
    --eq-muted: #5e6b78;
    --eq-line: #dce4ec;
    --eq-accent: #007f8f;
    --eq-accent-dark: #075e69;
    --eq-warm: #f4b24c;
  }

  .site-header {
    display: none;
  }

  .eq-page {
    color: var(--eq-ink);
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
    line-height: 1.6;
  }

  .eq-hero {
    display: grid;
    grid-template-columns: minmax(0, 1fr) minmax(280px, 0.9fr);
    gap: 2rem;
    align-items: center;
    padding: 3rem 0 2.5rem;
    border-bottom: 1px solid var(--eq-line);
  }

  .eq-kicker {
    margin: 0 0 0.75rem;
    color: var(--eq-accent-dark);
    font-size: 0.82rem;
    font-weight: 700;
    letter-spacing: 0.08em;
    text-transform: uppercase;
  }

  .eq-hero h1 {
    margin: 0 0 1rem;
    font-size: clamp(2.4rem, 7vw, 4.75rem);
    line-height: 0.95;
    letter-spacing: 0;
  }

  .eq-lead {
    max-width: 46rem;
    margin: 0 0 1.5rem;
    color: var(--eq-muted);
    font-size: 1.15rem;
  }

  .eq-actions {
    display: flex;
    flex-wrap: wrap;
    gap: 0.75rem;
    margin: 1.5rem 0 0;
  }

  .eq-button {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    min-height: 2.75rem;
    padding: 0.7rem 1rem;
    border: 1px solid var(--eq-accent);
    border-radius: 0.45rem;
    background: var(--eq-accent);
    color: #ffffff !important;
    font-weight: 700;
    text-decoration: none !important;
  }

  .eq-button.secondary {
    background: transparent;
    color: var(--eq-accent-dark) !important;
  }

  .eq-hero img,
  .eq-image-grid img {
    width: 100%;
    border: 1px solid var(--eq-line);
    border-radius: 0.5rem;
    background: var(--eq-surface);
    box-shadow: 0 16px 45px rgba(23, 33, 43, 0.1);
  }

  .eq-section {
    padding: 2.6rem 0;
    border-bottom: 1px solid var(--eq-line);
  }

  .eq-section h2 {
    margin: 0 0 0.75rem;
    font-size: 1.75rem;
  }

  .eq-section > p {
    max-width: 54rem;
    color: var(--eq-muted);
  }

  .eq-grid {
    display: grid;
    grid-template-columns: repeat(3, minmax(0, 1fr));
    gap: 1rem;
    margin-top: 1.25rem;
  }

  .eq-card {
    padding: 1.1rem;
    border: 1px solid var(--eq-line);
    border-radius: 0.5rem;
    background: var(--eq-surface);
  }

  .eq-card h3 {
    margin: 0 0 0.4rem;
    font-size: 1rem;
  }

  .eq-card p,
  .eq-card li {
    color: var(--eq-muted);
    font-size: 0.96rem;
  }

  .eq-card ul {
    margin: 0.55rem 0 0;
    padding-left: 1.1rem;
  }

  .eq-steps {
    counter-reset: eq-step;
  }

  .eq-step {
    position: relative;
    padding-left: 3.4rem;
  }

  .eq-step::before {
    counter-increment: eq-step;
    content: counter(eq-step);
    position: absolute;
    top: 1.05rem;
    left: 1.1rem;
    display: grid;
    width: 1.75rem;
    height: 1.75rem;
    place-items: center;
    border-radius: 999px;
    background: var(--eq-accent);
    color: #ffffff;
    font-size: 0.9rem;
    font-weight: 800;
  }

  .eq-component-list {
    columns: 2;
    margin: 1rem 0 0;
    padding-left: 1.1rem;
  }

  .eq-component-list li {
    break-inside: avoid;
    margin-bottom: 0.35rem;
    color: var(--eq-muted);
  }

  .eq-highlight {
    padding: 1.2rem;
    border-left: 0.35rem solid var(--eq-warm);
    background: #fff8eb;
    color: #594018;
  }

  .eq-image-grid {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 1rem;
    margin-top: 1rem;
  }

  @media (max-width: 820px) {
    .eq-hero,
    .eq-grid,
    .eq-image-grid {
      grid-template-columns: 1fr;
    }

    .eq-component-list {
      columns: 1;
    }

    .eq-hero {
      padding-top: 1.5rem;
    }
  }
</style>

<div class="eq-page">

<section class="eq-hero">
  <div>
    <p class="eq-kicker">DIY-Hardware für Dobson-Teleskope</p>
    <h1>EQ Plattform</h1>
    <p class="eq-lead">
      Eine stabile, modulare Nachführplattform für Dobson-Teleskope. Sie kompensiert die scheinbare
      Bewegung des Sternenhimmels und hält Objekte dadurch länger im Gesichtsfeld.
    </p>
    <div class="eq-actions">
      <a class="eq-button" href="flasher/">Firmware im Browser installieren</a>
      <a class="eq-button" href="build_instructions/buildinginstructions_EQ-Plattform.pdf">Bauanleitung PDF</a>
      <a class="eq-button" href="#ressourcen">Dateien ansehen</a>
      <a class="eq-button secondary" href="part%20list/Teileliste_DM%20EQ%20Plattform.csv">Stückliste ansehen</a>
      <a class="eq-button secondary" href="https://github.com/MoMa13570/eq-plattform">GitHub-Repository</a>
    </div>
  </div>
  <div>
    <img src="images/EQ-Plattform.png" alt="EQ Plattform Frontansicht">
  </div>
</section>

<section class="eq-section">
  <h2>Worum es geht</h2>
  <p>
   Das Projekt verfolgt die Idee einer Do-it-yourself-EQ-Plattform, die sich vergleichsweise preisgünstig herstellen lässt, ohne dass man zwangsläufig handwerkliche Fähigkeiten benötigt. Es ähnelt eher einem Ikea-Bausatz – daher nennen wir es spaßeshalber Stjärnföljare: Die Teile werden online bestellt, und der Benutzer baut die Plattform nach Anleitung zusammen. Die Kosten fallen dabei deutlich niedriger aus als bei kommerziell erhältlichen Plattformen. Im Folgenden werden die nötigen Teilelisten und Bauschritte im Einzelnen erklärt.
  </p>

  <div class="eq-grid">
    <div class="eq-card">
      <h3>Mechanischer Aufbau</h3>
      <p>Grundplatte, Tischplatte, Linearwellen, Stehlager, Südlager und Kreissegmente bilden die Plattform.</p>
    </div>
    <div class="eq-card">
      <h3>Präziser Schrittmotor-Antrieb</h3>
      <p>Die Plattform nutzt eine eigene PCB mit TMC2209-Treiber und NEMA17-Schrittmotor.</p>
    </div>
    <div class="eq-card">
      <h3>Offen dokumentiert</h3>
      <p>CAD-Daten, Stückliste, Schaltplan, Leiterplatte und Firmware liegen frei im Repository.</p>
    </div>
  </div>
  <p class="eq-highlight">
    Aktuell empfohlener Aufbau: NEMA17-Schrittmotor mit Steuerplatine. Die frühere alternative Motorvariante
    hat sich nicht bewährt und wird nicht weiter empfohlen.
  </p>
</section>

<section class="eq-section">
  <h2>Aufbau in 5 Schritten</h2>
  <p>
    Die Website zeigt nur den Überblick. Die genauen Maße, Bohrungen, Montageschritte und Elektronikdetails
    stehen in der PDF-Bauanleitung.
  </p>
  <div class="eq-grid eq-steps">
    <div class="eq-card eq-step">
      <h3>Grundplatte vorbereiten</h3>
      <p>Möbelfüße montieren und die Stehlager zunächst handfest auf der Grundplatte befestigen.</p>
    </div>
    <div class="eq-card eq-step">
      <h3>Linearwellen einsetzen</h3>
      <p>Linearwellen durch die Stehlager schieben, ausrichten und anschließend mit den Madenschrauben fixieren.</p>
    </div>
    <div class="eq-card eq-step">
      <h3>Tischplatte vorbereiten</h3>
      <p>Gewindeeinsatz setzen, Kugelgelenk einschrauben und die bewegliche Tischplatte für das Südlager vorbereiten.</p>
    </div>
    <div class="eq-card eq-step">
      <h3>Kreissegmente montieren</h3>
      <p>Kreissegmente mit Gewindeeinsätzen an der Tischplatte verschrauben und das Südlager montieren.</p>
    </div>
    <div class="eq-card eq-step">
      <h3>Antrieb anbauen</h3>
      <p>Motorhalter, GT2-Riemen, Elektronikgehäuse und Endstops montieren und anschließend die Plattform prüfen.</p>
    </div>
  </div>
</section>

<section class="eq-section" id="ressourcen">
  <h2>Dateien und Ressourcen</h2>
  <p>
    Alle wichtigen Dateien sind nach Mechanik, Elektronik, Software und Stückliste gegliedert.
    Die vereinfachten Mechanikdateien reduzieren Fertigungsaufwand und Kosten.
  </p>

  <div class="eq-grid">
    <div class="eq-card">
      <h3>Mechanik</h3>
      <ul>
        <li><a href="https://github.com/MoMa13570/eq-plattform/tree/main/hardware/mechanical/normal">Normale STEP-Dateien</a></li>
        <li><a href="https://github.com/MoMa13570/eq-plattform/tree/main/hardware/mechanical/simple">Vereinfachte STEP-Dateien</a></li>
        <li><a href="https://github.com/MoMa13570/eq-plattform/tree/main/hardware/mechanical/Kreissegmente%20aus%20Aluminium">Kreissegmente aus Aluminium</a></li>
      </ul>
    </div>
    <div class="eq-card">
      <h3>Elektronik</h3>
      <ul>
        <li><a href="https://github.com/MoMa13570/eq-plattform/tree/main/hardware/circuitboard">PCB, Schaltplan und Fertigungsdaten</a></li>
        <li><a href="https://github.com/MoMa13570/eq-plattform/tree/main/hardware/circuitboard/case">Gehäuse für die Elektronik</a></li>
        <li><a href="hardware/circuitboard/EQ-Plattform%20Schaltplan_TMC2209%20v32_jlcpcb.zip">JLCPCB-Daten als ZIP</a></li>
      </ul>
    </div>
    <div class="eq-card">
      <h3>Software und BOM</h3>
      <ul>
        <li><a href="flasher/">Firmware direkt im Browser installieren</a></li>
        <li><a href="https://github.com/MoMa13570/eq-plattform/releases/latest">Aktuelle HEX-Firmware herunterladen</a></li>
        <li><a href="https://github.com/MoMa13570/eq-plattform/tree/main/software">Firmware-Quellcode und PlatformIO-Projekt</a></li>
        <li><a href="part%20list/Teileliste_DM%20EQ%20Plattform.csv">Stückliste als CSV</a></li>
        <li><a href="build_instructions/buildinginstructions_EQ-Plattform.pdf">Bauanleitung als PDF</a></li>
        <li><a href="README.md">Repository-Übersicht</a></li>
      </ul>
    </div>
  </div>
</section>

<section class="eq-section">
  <h2>Benötigte Hauptkomponenten</h2>
  <p>
    Die vollständige Teileliste steht in der BOM und in der Bauanleitung. Für die erste Einschätzung sind vor
    allem diese Baugruppen wichtig:
  </p>
  <div class="eq-card">
    <ul class="eq-component-list">
      <li>CNC-gefräste Grundplatte und Tischplatte</li>
      <li>Zwei Kreissegmente aus Holz oder Aluminium</li>
      <li>Linearwellen Ø20 mm und UCP204-Stehlager</li>
      <li>Kugelgelenk M10 für das Südlager</li>
      <li>Verstellbare Möbelfüße</li>
      <li>NEMA17-Schrittmotor mit Winkelhalter</li>
      <li>GT2-Riemen, 16-Zähne-Motorpulley und 66-Zähne-Wellenpulley</li>
      <li>Steuerplatine mit Arduino Nano und TMC2209</li>
      <li>OLED-Display, Taster, Schalter und Potentiometer</li>
      <li>Endstops, Kabel und JST-XH-Steckverbinder</li>
    </ul>
  </div>
</section>



<section class="eq-section">
  <h2>Bauanleitung</h2>
  <p>
    Die Bauanleitung führt auf 17 Seiten durch Funktionsprinzip, benötigte Bauteile, mechanischen Aufbau,
    Motorisierung, Elektronikgehäuse, Endstops und Abschlussprüfung. Sie ist als PDF verfügbar und kann direkt
    im Browser geöffnet oder heruntergeladen werden.
  </p>
  <div class="eq-grid">
    <div class="eq-card">
      <h3>Bauteile</h3>
      <p>Übersicht über CNC-gefräste Holzteile, mechanische Komponenten und Teile für den NEMA17-Antrieb.</p>
    </div>
    <div class="eq-card">
      <h3>Zusammenbau</h3>
      <p>Schritte für Möbelfüße, Stehlager, Linearwellen, Tischplatte, Kreissegmente und Südlager.</p>
    </div>
    <div class="eq-card">
      <h3>Motorisierung</h3>
      <p>Hinweise zu Steuerplatine, Lötarbeiten, Gehäuse, Motormontage und Endstops.</p>
    </div>
  </div>
  <p class="eq-highlight">
    <a href="build_instructions/buildinginstructions_EQ-Plattform.pdf">Bauanleitung als PDF öffnen</a>
  </p>
</section>

<section class="eq-section">
  <h2>Varianten</h2>
  <div class="eq-grid">
    <div class="eq-card">
      <h3>Standardmechanik</h3>
      <p>
        Enthält die vollständige Ausführung mit Aussparungen für Libelle, Kompass und Windrose sowie
        Markierungen auf der Oberplatte.
      </p>
    </div>
    <div class="eq-card">
      <h3>Simple-Mechanik</h3>
      <p>
        Reduzierte Version für günstigere Fertigung. In der Unterplatte entfallen die Aussparungen,
        in der Oberplatte die Nord- und Südmarkierungen.
      </p>
    </div>
    <div class="eq-card">
      <h3>PCB-Antrieb</h3>
      <p>
        Für den Schrittmotor-Aufbau stehen Schaltplan, Platinenansicht, Gehäuse und Firmware bereit.
      </p>
    </div>
  </div>
</section>

<section class="eq-section">
  <h2>Elektronik</h2>
  <p>
    Die Leiterplatte steuert den Aufbau mit NEMA17-Schrittmotor und TMC2209-Treiber.
  </p>
  <div class="eq-image-grid">
    <img src="hardware/circuitboard/PCB_2D.png" alt="PCB-Layout der EQ Plattform">
    <img src="hardware/circuitboard/Schematic.png" alt="Schaltplan der EQ Plattform">
  </div>
</section>

<section class="eq-section">
  <h2>Software installieren</h2>
  <p>
    Für die normale Installation werden weder Arduino IDE noch Visual Studio Code benötigt.
    Die fertige HEX-Datei lässt sich direkt mit Chrome oder Edge übertragen.
  </p>
  <div class="eq-grid">
    <div class="eq-card">
      <h3>Direkt im Browser</h3>
      <ol>
        <li>Aktuelle HEX-Datei aus dem GitHub Release herunterladen.</li>
        <li>Board auswählen und Arduino per USB verbinden.</li>
        <li>Firmware mit dem Browser-Flasher installieren.</li>
      </ol>
      <p><a href="flasher/">Browser-Flasher öffnen</a></p>
    </div>
    <div class="eq-card">
      <h3>Für Entwickler</h3>
      <p>
        Der vollständige Quellcode und die PlatformIO-Konfiguration für Uno und Nano
        liegen unter <code>software/EQ-Plattform_TMC2209/</code>.
      </p>
      <p><a href="https://github.com/MoMa13570/eq-plattform/tree/main/software/EQ-Plattform_TMC2209">PlatformIO-Projekt öffnen</a></p>
    </div>
  </div>
</section>

<section class="eq-section">
  <h2>Mitmachen</h2>
  <p>
    Beiträge zur Dokumentation, Mechanik, Elektronik, Firmware oder zu neuen Anwendungsbeispielen sind willkommen.
    Fehler, Ideen und Verbesserungen können über Issues oder Pull Requests im Repository eingebracht werden.
  </p>
  <p class="eq-highlight">
    Projekt-Repository: <a href="https://github.com/MoMa13570/eq-plattform">github.com/MoMa13570/eq-plattform</a>
  </p>
</section>

</div>
