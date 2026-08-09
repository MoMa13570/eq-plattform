---
title: EQ Plattform
description: DIY-Nachführplattform für Dobson-Teleskope mit Arduino, NEMA17 und TMC2209
lang: de
---

<style>
  :root {
    --eq-bg: #f2f6f5;
    --eq-surface: #ffffff;
    --eq-surface-soft: #e9f0ef;
    --eq-ink: #13272b;
    --eq-muted: #5d7074;
    --eq-line: #d4e0de;
    --eq-accent: #087e84;
    --eq-accent-dark: #075a60;
    --eq-warm: #e99b38;
    --eq-red: #c8322b;
    --eq-night: #0b2d32;
    --eq-radius: 1.25rem;
    --eq-shadow: 0 24px 60px rgba(11, 45, 50, 0.1);
  }

  html {
    scroll-behavior: smooth;
  }

  body {
    margin: 0;
    background: var(--eq-bg);
  }

  .container-lg {
    max-width: none !important;
    margin: 0 !important;
    padding: 0 !important;
  }

  .container-lg > h1:first-child {
    display: none;
  }

  .markdown-body {
    color: var(--eq-ink);
    font-family: Inter, ui-sans-serif, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
    font-size: 16px;
    line-height: 1.65;
  }

  .markdown-body h1,
  .markdown-body h2,
  .markdown-body h3,
  .markdown-body p,
  .markdown-body figure,
  .markdown-body ul {
    margin-top: 0;
  }

  .markdown-body a {
    color: var(--eq-accent-dark);
  }

  .eq-page {
    min-height: 100vh;
    overflow: hidden;
  }

  .eq-wrap {
    width: min(1120px, calc(100% - 40px));
    margin: 0 auto;
  }

  .eq-nav {
    position: relative;
    z-index: 10;
    display: flex;
    min-height: 76px;
    align-items: center;
    justify-content: space-between;
    gap: 2rem;
  }

  .eq-brand {
    display: inline-flex;
    align-items: center;
    gap: 0.75rem;
    color: var(--eq-ink) !important;
    font-weight: 800;
    letter-spacing: -0.025em;
    text-decoration: none !important;
  }

  .eq-brand-mark {
    display: grid;
    width: 2.25rem;
    height: 2.25rem;
    place-items: center;
    border-radius: 0.7rem;
    background: var(--eq-night);
    color: #ffffff;
    font-size: 0.78rem;
    letter-spacing: -0.04em;
  }

  .eq-nav-links {
    display: flex;
    align-items: center;
    gap: 1.3rem;
  }

  .eq-nav-links a {
    color: var(--eq-muted) !important;
    font-size: 0.9rem;
    font-weight: 650;
    text-decoration: none !important;
  }

  .eq-nav-links a:hover {
    color: var(--eq-accent-dark) !important;
  }

  .eq-hero {
    display: grid;
    grid-template-columns: minmax(0, 1fr) minmax(360px, 0.86fr);
    gap: clamp(2rem, 5vw, 5rem);
    align-items: center;
    padding: clamp(3rem, 7vw, 6rem) 0 clamp(4rem, 8vw, 7rem);
  }

  .eq-kicker {
    margin-bottom: 1rem !important;
    color: var(--eq-accent-dark);
    font: 750 0.78rem ui-monospace, SFMono-Regular, Menlo, monospace;
    letter-spacing: 0.13em;
    text-transform: uppercase;
  }

  .eq-hero h1 {
    max-width: 12ch;
    margin-bottom: 1.4rem;
    color: var(--eq-night);
    font-size: clamp(3.3rem, 8vw, 6.5rem);
    font-weight: 720;
    letter-spacing: -0.072em;
    line-height: 0.9;
  }

  .eq-lead {
    max-width: 42rem;
    margin-bottom: 1.6rem !important;
    color: var(--eq-muted);
    font-size: clamp(1.05rem, 2vw, 1.25rem);
    line-height: 1.6;
  }

  .eq-actions {
    display: flex;
    flex-wrap: wrap;
    gap: 0.75rem;
  }

  .eq-button {
    display: inline-flex;
    min-height: 3rem;
    align-items: center;
    justify-content: center;
    padding: 0.7rem 1.1rem;
    border: 1px solid var(--eq-accent);
    border-radius: 0.75rem;
    background: var(--eq-accent);
    color: #ffffff !important;
    font-size: 0.94rem;
    font-weight: 750;
    text-decoration: none !important;
    transition: transform 160ms ease, box-shadow 160ms ease;
  }

  .eq-button:hover {
    transform: translateY(-2px);
    box-shadow: 0 12px 24px rgba(8, 126, 132, 0.18);
  }

  .eq-button.secondary {
    border-color: var(--eq-line);
    background: var(--eq-surface);
    color: var(--eq-ink) !important;
  }

  .eq-facts {
    display: flex;
    flex-wrap: wrap;
    gap: 0.55rem;
    margin-top: 1.6rem;
  }

  .eq-fact {
    padding: 0.35rem 0.65rem;
    border: 1px solid var(--eq-line);
    border-radius: 999px;
    background: rgba(255, 255, 255, 0.55);
    color: var(--eq-muted);
    font-size: 0.78rem;
    font-weight: 650;
  }

  .eq-hero-media {
    position: relative;
    margin: 0;
  }

  .eq-hero-media::before {
    position: absolute;
    inset: -10% -12% auto auto;
    width: 75%;
    aspect-ratio: 1;
    border-radius: 50%;
    background: rgba(8, 126, 132, 0.12);
    content: "";
    filter: blur(1px);
  }

  .eq-hero-media img {
    position: relative;
    width: 100%;
    aspect-ratio: 1.05;
    border: 1px solid rgba(255, 255, 255, 0.8);
    border-radius: 2rem;
    object-fit: cover;
    box-shadow: var(--eq-shadow);
  }

  .eq-hero-media figcaption {
    position: absolute;
    right: 1rem;
    bottom: 1rem;
    max-width: 14rem;
    padding: 0.55rem 0.75rem;
    border: 1px solid rgba(255, 255, 255, 0.35);
    border-radius: 0.65rem;
    background: rgba(11, 45, 50, 0.84);
    color: #ffffff;
    font-size: 0.75rem;
    backdrop-filter: blur(8px);
  }

  .eq-intro {
    display: grid;
    grid-template-columns: minmax(220px, 0.3fr) minmax(0, 0.7fr);
    gap: clamp(1.5rem, 5vw, 4rem);
    align-items: start;
    margin-bottom: clamp(4rem, 8vw, 7rem);
    padding: clamp(1.5rem, 4vw, 2.5rem);
    border: 1px solid var(--eq-line);
    border-radius: var(--eq-radius);
    background: var(--eq-surface);
    box-shadow: 0 18px 45px rgba(11, 45, 50, 0.07);
  }

  .eq-intro h2 {
    margin-bottom: 0;
    color: var(--eq-night);
    font-size: clamp(1.8rem, 4vw, 2.7rem);
    font-weight: 720;
    letter-spacing: -0.05em;
    line-height: 1;
  }

  .eq-intro p {
    margin-bottom: 0;
    color: var(--eq-muted);
    font-size: 1.04rem;
    line-height: 1.75;
  }

  .eq-section {
    padding: clamp(4rem, 8vw, 7rem) 0;
  }

  .eq-section.soft {
    position: relative;
  }

  .eq-section.soft::before {
    position: absolute;
    z-index: -1;
    top: 0;
    right: 50%;
    bottom: 0;
    left: 50%;
    width: 100vw;
    margin-right: -50vw;
    margin-left: -50vw;
    background: var(--eq-surface-soft);
    content: "";
  }

  .eq-section.dark {
    position: relative;
    color: #ffffff;
  }

  .eq-section.dark::before {
    position: absolute;
    z-index: -1;
    top: 0;
    right: 50%;
    bottom: 0;
    left: 50%;
    width: 100vw;
    margin-right: -50vw;
    margin-left: -50vw;
    background: var(--eq-night);
    content: "";
  }

  .eq-section-heading {
    display: grid;
    grid-template-columns: minmax(0, 0.62fr) minmax(280px, 0.38fr);
    gap: 3rem;
    align-items: end;
    margin-bottom: 2.2rem;
  }

  .eq-section-heading h2,
  .eq-split-copy h2 {
    margin-bottom: 0;
    color: inherit;
    font-size: clamp(2.1rem, 5vw, 3.8rem);
    font-weight: 720;
    letter-spacing: -0.055em;
    line-height: 1;
  }

  .eq-section-heading p {
    margin-bottom: 0;
    color: var(--eq-muted);
  }

  .eq-section.dark .eq-section-heading p,
  .eq-section.dark .eq-muted {
    color: #b7c8c8;
  }

  .eq-split {
    display: grid;
    grid-template-columns: minmax(0, 0.9fr) minmax(360px, 1.1fr);
    gap: clamp(2rem, 6vw, 5rem);
    align-items: center;
  }

  .eq-split.reverse {
    grid-template-columns: minmax(360px, 1.1fr) minmax(0, 0.9fr);
  }

  .eq-split-copy > p {
    color: var(--eq-muted);
    font-size: 1.05rem;
  }

  .eq-split-copy h2 {
    margin-bottom: 1.2rem;
  }

  .eq-diagram,
  .eq-photo-card {
    margin: 0;
    overflow: hidden;
    border: 1px solid var(--eq-line);
    border-radius: var(--eq-radius);
    background: var(--eq-surface);
    box-shadow: var(--eq-shadow);
  }

  .eq-diagram {
    padding: 1.25rem;
  }

  .eq-diagram img,
  .eq-photo-card img {
    display: block;
    width: 100%;
    margin: 0;
  }

  .eq-diagram figcaption,
  .eq-photo-card figcaption {
    padding: 0.8rem 1rem 0.95rem;
    color: var(--eq-muted);
    font-size: 0.8rem;
  }

  .eq-checks {
    display: grid;
    gap: 0.8rem;
    margin: 1.5rem 0 0;
    padding: 0;
    list-style: none;
  }

  .eq-checks li {
    position: relative;
    padding-left: 1.7rem;
    color: var(--eq-ink);
  }

  .eq-checks li::before {
    position: absolute;
    top: 0.42rem;
    left: 0;
    width: 0.7rem;
    height: 0.7rem;
    border: 2px solid var(--eq-accent);
    border-radius: 50%;
    content: "";
  }

  .eq-card-grid {
    display: grid;
    grid-template-columns: repeat(3, minmax(0, 1fr));
    gap: 1rem;
  }

  .eq-card {
    padding: 1.35rem;
    border: 1px solid var(--eq-line);
    border-radius: 1rem;
    background: var(--eq-surface);
  }

  .eq-card-label {
    display: inline-block;
    margin-bottom: 1rem;
    color: var(--eq-accent-dark);
    font: 700 0.72rem ui-monospace, SFMono-Regular, Menlo, monospace;
    letter-spacing: 0.1em;
    text-transform: uppercase;
  }

  .eq-card h3 {
    margin-bottom: 0.55rem;
    color: var(--eq-ink);
    font-size: 1.15rem;
  }

  .eq-card p,
  .eq-card li {
    color: var(--eq-muted);
    font-size: 0.93rem;
  }

  .eq-card ul {
    margin-bottom: 0;
    padding-left: 1.1rem;
  }

  .eq-step-grid {
    display: grid;
    grid-template-columns: repeat(3, minmax(0, 1fr));
    gap: 1rem;
  }

  .eq-step {
    min-height: 12rem;
    padding: 1.3rem;
    border: 1px solid var(--eq-line);
    border-radius: 1rem;
    background: rgba(255, 255, 255, 0.72);
  }

  .eq-step-number {
    display: grid;
    width: 2.15rem;
    height: 2.15rem;
    margin-bottom: 1.8rem;
    place-items: center;
    border-radius: 0.65rem;
    background: var(--eq-night);
    color: #ffffff;
    font: 700 0.78rem ui-monospace, SFMono-Regular, Menlo, monospace;
  }

  .eq-step h3 {
    margin-bottom: 0.45rem;
    color: var(--eq-ink);
    font-size: 1.05rem;
  }

  .eq-step p {
    margin-bottom: 0;
    color: var(--eq-muted);
    font-size: 0.9rem;
  }

  .eq-gallery {
    display: grid;
    grid-template-columns: repeat(3, minmax(0, 1fr));
    gap: 1rem;
    margin-top: 1rem;
  }

  .eq-photo-card img {
    aspect-ratio: 1.25;
    object-fit: cover;
  }

  .eq-dark-grid {
    display: grid;
    grid-template-columns: 0.9fr 1.1fr;
    gap: 1rem;
  }

  .eq-dark-card {
    overflow: hidden;
    border: 1px solid rgba(255, 255, 255, 0.13);
    border-radius: var(--eq-radius);
    background: rgba(255, 255, 255, 0.06);
  }

  .eq-dark-card img {
    display: block;
    width: 100%;
    aspect-ratio: 1.25;
    object-fit: cover;
  }

  .eq-dark-card-copy {
    padding: 1.25rem;
  }

  .eq-dark-card h3 {
    margin-bottom: 0.5rem;
    color: #ffffff;
  }

  .eq-dark-card p {
    margin-bottom: 0;
    color: #b7c8c8;
    font-size: 0.93rem;
  }

  .eq-note {
    margin-top: 1rem;
    padding: 1rem 1.1rem;
    border-left: 0.3rem solid var(--eq-warm);
    border-radius: 0.25rem 0.75rem 0.75rem 0.25rem;
    background: #fff6e9;
    color: #614619;
    font-size: 0.92rem;
  }

  .eq-projection {
    padding: clamp(1.4rem, 4vw, 2.4rem);
    border: 1px solid var(--eq-line);
    border-radius: var(--eq-radius);
    background: var(--eq-surface);
    box-shadow: var(--eq-shadow);
  }

  .eq-projection-top,
  .eq-projection-panel-head,
  .eq-projection-control-head {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 1rem;
  }

  .eq-projection-top {
    margin-bottom: 1.5rem;
  }

  .eq-projection-top p {
    max-width: 44rem;
    margin-bottom: 0;
    color: var(--eq-muted);
  }

  .eq-projection-play {
    flex: 0 0 auto;
    cursor: pointer;
  }

  .eq-projection-play:disabled {
    cursor: wait;
    opacity: 0.65;
  }

  .eq-projection-grid {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: clamp(1rem, 3vw, 2rem);
  }

  .eq-projection-panel {
    min-width: 0;
  }

  .eq-projection-panel-head {
    align-items: baseline;
    margin-bottom: 0.4rem;
  }

  .eq-projection-panel h3 {
    margin-bottom: 0;
    color: var(--eq-ink);
    font-size: 1.05rem;
  }

  .eq-projection-value {
    color: var(--eq-muted);
    font-size: 0.82rem;
    font-variant-numeric: tabular-nums;
    white-space: nowrap;
  }

  .eq-projection-svg {
    display: block;
    width: 100%;
    min-height: 19rem;
    overflow: visible;
  }

  .eq-projection-axis {
    stroke: var(--eq-line);
    stroke-width: 1;
  }

  .eq-projection-reference {
    fill: none;
    stroke: var(--eq-line);
    stroke-width: 1.5;
    stroke-dasharray: 5 5;
  }

  .eq-projection-ellipse {
    fill: rgba(8, 126, 132, 0.1);
    stroke: var(--eq-accent);
    stroke-width: 2.5;
  }

  .eq-projection-before {
    fill: none;
    stroke: var(--eq-muted);
    stroke-width: 1.5;
    stroke-dasharray: 6 5;
  }

  .eq-projection-after {
    fill: rgba(233, 155, 56, 0.12);
    stroke: var(--eq-warm);
    stroke-width: 2.5;
  }

  .eq-projection-vns {
    fill: var(--eq-red);
    stroke: var(--eq-red);
    stroke-width: 1.5;
  }

  .eq-projection-vns-label {
    fill: var(--eq-red) !important;
    font-weight: 700;
  }

  .eq-projection-roller {
    stroke: var(--eq-accent);
    stroke-width: 4;
    stroke-linecap: round;
  }

  .eq-projection-measure,
  .eq-projection-angle {
    fill: none;
    stroke: var(--eq-ink);
    stroke-width: 1.25;
  }

  .eq-projection-svg text {
    fill: var(--eq-ink);
    font-family: Inter, ui-sans-serif, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
    font-size: 12px;
  }

  .eq-projection-svg .muted {
    fill: var(--eq-muted);
  }

  .eq-projection-svg .formula {
    font-weight: 700;
  }

  .eq-projection-controls {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 1rem 2rem;
    margin-top: 1rem;
    padding-top: 1.25rem;
    border-top: 1px solid var(--eq-line);
  }

  .eq-projection-control label {
    color: var(--eq-ink);
    font-size: 0.88rem;
    font-weight: 650;
  }

  .eq-projection-control output {
    color: var(--eq-muted);
    font-size: 0.84rem;
    font-variant-numeric: tabular-nums;
  }

  .eq-projection-control input {
    width: 100%;
    margin: 0.65rem 0 0;
    accent-color: var(--eq-accent);
  }

  .eq-projection-progress {
    height: 3px;
    margin-top: 1rem;
    overflow: hidden;
    border-radius: 999px;
    background: var(--eq-surface-soft);
  }

  .eq-projection-progress span {
    display: block;
    width: 0;
    height: 100%;
    background: var(--eq-accent);
  }

  .eq-resource-grid {
    display: grid;
    grid-template-columns: repeat(3, minmax(0, 1fr));
    gap: 1rem;
  }

  .eq-resource {
    display: flex;
    min-height: 11.5rem;
    flex-direction: column;
    justify-content: space-between;
    padding: 1.35rem;
    border: 1px solid var(--eq-line);
    border-radius: 1rem;
    background: var(--eq-surface);
    color: var(--eq-ink) !important;
    text-decoration: none !important;
    transition: transform 160ms ease, border-color 160ms ease;
  }

  .eq-resource:hover {
    transform: translateY(-3px);
    border-color: var(--eq-accent);
  }

  .eq-resource-type {
    color: var(--eq-accent-dark);
    font: 700 0.72rem ui-monospace, SFMono-Regular, Menlo, monospace;
    letter-spacing: 0.1em;
    text-transform: uppercase;
  }

  .eq-resource strong {
    display: block;
    margin: 1.4rem 0 0.35rem;
    color: var(--eq-ink);
    font-size: 1.05rem;
  }

  .eq-resource small {
    color: var(--eq-muted);
    line-height: 1.5;
  }

  .eq-resource-arrow {
    align-self: flex-end;
    color: var(--eq-accent);
    font-size: 1.2rem;
  }

  .eq-final {
    display: grid;
    grid-template-columns: minmax(0, 1fr) auto;
    gap: 2rem;
    align-items: center;
    margin: clamp(4rem, 8vw, 7rem) 0;
    padding: clamp(1.7rem, 4vw, 3rem);
    border-radius: var(--eq-radius);
    background: var(--eq-night);
    color: #ffffff;
  }

  .eq-final h2 {
    margin-bottom: 0.55rem;
    color: #ffffff;
    font-size: clamp(1.8rem, 4vw, 2.8rem);
    letter-spacing: -0.045em;
  }

  .eq-final p {
    max-width: 44rem;
    margin-bottom: 0;
    color: #b7c8c8;
  }

  .eq-footer {
    display: flex;
    padding: 1.5rem 0 2.5rem;
    border-top: 1px solid var(--eq-line);
    justify-content: space-between;
    gap: 1rem;
    color: var(--eq-muted);
    font-size: 0.82rem;
  }

  @media (max-width: 880px) {
    .eq-hero,
    .eq-split,
    .eq-split.reverse,
    .eq-intro,
    .eq-section-heading,
    .eq-dark-grid {
      grid-template-columns: 1fr;
    }

    .eq-hero {
      padding-top: 2.5rem;
    }

    .eq-hero-copy {
      order: 1;
    }

    .eq-hero-media {
      order: 2;
    }

    .eq-card-grid,
    .eq-step-grid,
    .eq-resource-grid {
      grid-template-columns: repeat(2, minmax(0, 1fr));
    }

    .eq-section-heading {
      gap: 1rem;
    }

    .eq-projection-grid {
      grid-template-columns: 1fr;
    }
  }

  @media (max-width: 620px) {
    .eq-wrap {
      width: min(100% - 24px, 1120px);
    }

    .eq-nav {
      min-height: 66px;
    }

    .eq-nav-links {
      display: none;
    }

    .eq-hero {
      gap: 2.4rem;
      padding-bottom: 4rem;
    }

    .eq-hero h1 {
      font-size: clamp(3.2rem, 17vw, 4.8rem);
    }

    .eq-hero-media img {
      border-radius: 1.25rem;
    }

    .eq-actions {
      align-items: stretch;
      flex-direction: column;
    }

    .eq-button {
      width: 100%;
    }

    .eq-card-grid,
    .eq-step-grid,
    .eq-gallery,
    .eq-resource-grid {
      grid-template-columns: 1fr;
    }

    .eq-final {
      grid-template-columns: 1fr;
    }

    .eq-footer {
      flex-direction: column;
    }

    .eq-projection-top,
    .eq-projection-panel-head {
      align-items: flex-start;
      flex-direction: column;
      gap: 0.35rem;
    }

    .eq-projection-play {
      width: 100%;
    }

    .eq-projection-controls {
      grid-template-columns: 1fr;
    }
  }

  @media (prefers-reduced-motion: reduce) {
    html {
      scroll-behavior: auto;
    }

    *,
    *::before,
    *::after {
      transition-duration: 0.01ms !important;
    }
  }
</style>

<div class="eq-page">
  <div class="eq-wrap">
    <nav class="eq-nav" aria-label="Hauptnavigation">
      <a class="eq-brand" href="./">
        <span class="eq-brand-mark">EQ</span>
        <span>EQ Plattform</span>
      </a>
      <div class="eq-nav-links">
        <a href="#prinzip">Prinzip</a>
        <a href="#projektion">Projektion</a>
        <a href="#aufbau">Aufbau</a>
        <a href="#antrieb">Antrieb</a>
        <a href="#downloads">Downloads</a>
      </div>
    </nav>

    <main>
      <header class="eq-hero">
        <div class="eq-hero-copy">
          <p class="eq-kicker">DIY-Nachführung für Dobson-Teleskope</p>
          <h1>Mehr Zeit am Objekt.</h1>
          <p class="eq-lead">
            Eine kompakte äquatoriale Plattform, die die Erdrotation ausgleicht und
            Himmelsobjekte länger im Gesichtsfeld hält. Mechanik, Elektronik und
            Firmware sind vollständig dokumentiert.
          </p>
          <div class="eq-actions">
            <a class="eq-button" href="flasher/">Firmware installieren</a>
            <a class="eq-button secondary" href="build_instructions/buildinstruction_EQ_Plattform.pdf">Bauanleitung öffnen</a>
            <a class="eq-button secondary" href="#downloads">Dateien herunterladen</a>
          </div>
          <div class="eq-facts" aria-label="Projektmerkmale">
            <span class="eq-fact">18 Seiten Anleitung</span>
            <span class="eq-fact">Firmware für Uno &amp; Nano</span>
            <span class="eq-fact">NEMA17 + TMC2209</span>
          </div>
        </div>
        <figure class="eq-hero-media">
          <img src="images/EQ-Plattform.png" alt="Fertig aufgebaute EQ Plattform" />
          <figcaption>Fertige EQ Plattform mit motorisierter Nachführung</figcaption>
        </figure>
      </header>

      <section class="eq-intro" aria-labelledby="worum-es-geht">
        <h2 id="worum-es-geht">Worum es geht</h2>
        <p>
          Das Projekt verfolgt die Idee einer Do-it-yourself-EQ-Plattform, die sich
          vergleichsweise preisgünstig herstellen lässt, ohne dass man zwangsläufig
          handwerkliche Fähigkeiten benötigt. Es ähnelt eher einem Bausatz eines gewissen Möbelhauses –
          daher nennen wir es spaßeshalber Stjärnföljare: Die Teile werden online
          bestellt, und der Benutzer baut die Plattform nach Anleitung zusammen.
          Die Kosten fallen dabei deutlich niedriger aus als bei kommerziell
          erhältlichen Plattformen. Im Folgenden werden die nötigen Teilelisten und
          Bauschritte im Einzelnen erklärt.
        </p>
      </section>

      <section class="eq-section soft" id="aufbau">
        <div class="eq-section-heading">
          <h2>Mechanik in sechs Schritten</h2>
          <p>
            Die Website zeigt den Ablauf. Maße, Bohrbilder und Detailfotos stehen
            in der PDF-Bauanleitung.
          </p>
        </div>
        <div class="eq-step-grid">
          <article class="eq-step">
            <span class="eq-step-number">01</span>
            <h3>Möbelfüße montieren</h3>
            <p>Füße in den vorgesehenen Ausfräsungen der Grundplatte verschrauben.</p>
          </article>
          <article class="eq-step">
            <span class="eq-step-number">02</span>
            <h3>Stehlager vormontieren</h3>
            <p>Vier UCP204-Lager zunächst nur handfest auf der Grundplatte befestigen.</p>
          </article>
          <article class="eq-step">
            <span class="eq-step-number">03</span>
            <h3>Linearwellen einsetzen</h3>
            <p>Wellen ausrichten, in die Lager schieben und mit Madenschrauben fixieren.</p>
          </article>
          <article class="eq-step">
            <span class="eq-step-number">04</span>
            <h3>Tischplatte vorbereiten</h3>
            <p>M10-Gewindeeinsatz setzen und das Kugelgelenk einschrauben.</p>
          </article>
          <article class="eq-step">
            <span class="eq-step-number">05</span>
            <h3>Kreissegmente montieren</h3>
            <p>M5-Gewindeeinsätze setzen und beide Segmente an der Tischplatte verschrauben.</p>
          </article>
          <article class="eq-step">
            <span class="eq-step-number">06</span>
            <h3>Südlager befestigen</h3>
            <p>Holzblock mit M8-Gewindeeinsätzen montieren und die Leichtgängigkeit prüfen.</p>
          </article>
        </div>
        <div class="eq-gallery">
          <figure class="eq-photo-card">
            <img src="images/build/parts-overview.webp" alt="Holzplatten, Lager, Wellen und weitere mechanische Bauteile" />
            <figcaption>Mechanische Bauteile vor dem Zusammenbau</figcaption>
          </figure>
          <figure class="eq-photo-card">
            <img src="images/build/linear-shafts.webp" alt="In vier Stehlagern montierte Linearwellen" />
            <figcaption>Nordlager mit UCP204-Stehlagern und Linearwellen</figcaption>
          </figure>
          <figure class="eq-photo-card">
            <img src="images/build/south-bearing.webp" alt="Montiertes Südlager aus Holzblock und Kugelgelenk" />
            <figcaption>Holzblock und Kugelgelenk bilden das Südlager</figcaption>
          </figure>
        </div>
      </section>

      <section class="eq-section soft" id="prinzip">
        <div class="eq-split">
          <div class="eq-split-copy">
            <p class="eq-kicker">Funktionsprinzip</p>
            <h2>Parallel zur Erdachse</h2>
            <p>
              Die bewegliche Tischplatte dreht sich um eine Achse, die parallel zur
              Erdachse ausgerichtet wird. Dadurch kompensiert die Plattform die
              scheinbare Bewegung des Sternenhimmels.
            </p>
            <ul class="eq-checks">
              <li>Nordlager über zwei Kreissegmente auf Linearwellen</li>
              <li>Südlager als definierter Dreh- und Neigungspunkt</li>
              <li>Feine Geschwindigkeitsregelung über Schrittmotor und Potentiometer</li>
            </ul>
          </div>
          <figure class="eq-diagram">
            <img src="images/build/principle.webp" alt="Prinzip einer äquatorialen Plattform mit Polachse, Nordlager und Südlager" />
            <figcaption>Prinzipdarstellung aus der Bauanleitung</figcaption>
          </figure>
        </div>
      </section>

      <section class="eq-section" id="projektion" aria-labelledby="projektion-title">
        <div class="eq-section-heading">
          <h2 id="projektion-title">Vom Kreis zum Rollensegment</h2>
          <p>
            Zwei Projektionen bestimmen die Form: Der Breitengrad skaliert das
            VNS-Segment vertikal, der Rollenwinkel korrigiert die horizontale Länge.
          </p>
        </div>

        <div class="eq-projection" id="eq-projection-explainer">
          <div class="eq-projection-top">
            <p>
              Die Animation zeigt zuerst die Projektion des Kreises zur Ellipse und
              anschließend die Korrektur für die um 30° verstellten Rollen.
            </p>
            <button class="eq-button eq-projection-play" id="eq-projection-play" type="button">
              Animation abspielen
            </button>
          </div>

          <div class="eq-projection-grid">
            <article class="eq-projection-panel" aria-labelledby="eq-projection-step-one">
              <div class="eq-projection-panel-head">
                <h3 id="eq-projection-step-one">1 · Kreis → Ellipse</h3>
                <span class="eq-projection-value" id="eq-projection-phi-value">φ = 50° · cos(φ) = 0,643</span>
              </div>
              <svg class="eq-projection-svg" id="eq-projection-left" role="img" aria-label="Projektion eines Kreises auf eine Ellipse; die rote VNS-Segmentfläche liegt unten rechts und wird mit Kosinus Phi skaliert"></svg>
            </article>

            <article class="eq-projection-panel" aria-labelledby="eq-projection-step-two">
              <div class="eq-projection-panel-head">
                <h3 id="eq-projection-step-two">2 · Rollenwinkel β</h3>
                <span class="eq-projection-value" id="eq-projection-beta-value">30° · Faktor 1,155</span>
              </div>
              <svg class="eq-projection-svg" id="eq-projection-right" role="img" aria-label="Darstellung der Rollenverstellung; Ellipse und rote VNS-Segmentfläche werden um Beta gedreht und horizontal mit dem Kehrwert von Kosinus Beta gestreckt"></svg>
            </article>
          </div>

          <div class="eq-projection-controls">
            <div class="eq-projection-control">
              <div class="eq-projection-control-head">
                <label for="eq-projection-phi">Breitengrad φ</label>
                <output id="eq-projection-phi-output" for="eq-projection-phi">50°</output>
              </div>
              <input id="eq-projection-phi" type="range" min="0" max="70" step="1" value="50" />
            </div>
            <div class="eq-projection-control">
              <div class="eq-projection-control-head">
                <label for="eq-projection-beta">Rollenwinkel β</label>
                <output id="eq-projection-beta-output" for="eq-projection-beta">30°</output>
              </div>
              <input id="eq-projection-beta" type="range" min="0" max="45" step="1" value="30" />
            </div>
          </div>
          <div class="eq-projection-progress" aria-hidden="true"><span id="eq-projection-progress"></span></div>
        </div>
      </section>

      <section class="eq-section">
        <div class="eq-section-heading">
          <h2>Bauteile auf einen Blick</h2>
          <p>
            Die vollständige Stückliste enthält alle Schrauben und Bezugsquellen.
            Für das Grundprinzip reichen drei Baugruppen.
          </p>
        </div>
        <div class="eq-card-grid">
          <article class="eq-card">
            <span class="eq-card-label">Mechanik</span>
            <h3>Platten und Lagerung</h3>
            <ul>
              <li>Grund- und Tischplatte</li>
              <li>Zwei Kreissegmente</li>
              <li>2 Linearwellen Ø20 × 100 mm</li>
              <li>4 UCP204-Stehlager</li>
              <li>Kugelgelenk M10</li>
            </ul>
          </article>
          <article class="eq-card">
            <span class="eq-card-label">Antrieb</span>
            <h3>Riemen und Motor</h3>
            <ul>
              <li>NEMA17-Schrittmotor</li>
              <li>Winkelhalter</li>
              <li>GT2-Riemen und Pulleys</li>
              <li>Zwei Endstops</li>
              <li>Motorisierte Referenzfahrt</li>
            </ul>
          </article>
          <article class="eq-card">
            <span class="eq-card-label">Steuerung</span>
            <h3>PCB und Bedienung</h3>
            <ul>
              <li>Arduino Nano V3</li>
              <li>TMC2209-Treiber</li>
              <li>1,3-Zoll-OLED</li>
              <li>Schalter und Home-Taster</li>
              <li>10-kΩ-Potentiometer</li>
            </ul>
          </article>
        </div>
      </section>

      <section class="eq-section dark" id="antrieb">
        <div class="eq-section-heading">
          <h2>Präziser Antrieb, einfache Bedienung</h2>
          <p>
            Die eigene Steuerplatine verbindet Arduino, TMC2209, Display,
            Bedienelemente und Endstops zu einer kompakten Einheit.
          </p>
        </div>
        <div class="eq-dark-grid">
          <article class="eq-dark-card">
            <img src="images/build/pcb-layout.webp" alt="Leiterplattenlayout der EQ Plattform" />
            <div class="eq-dark-card-copy">
              <h3>Steuerplatine</h3>
              <p>
                Alle Anschlüsse sind zentral geführt. Die Fertigungsdaten können
                direkt als ZIP heruntergeladen und bei einem Leiterplattenservice
                bestellt werden.
              </p>
            </div>
          </article>
          <article class="eq-dark-card">
            <img src="images/build/electronics-enclosure.webp" alt="Zweiteiliges 3D-gedrucktes Elektronikgehäuse" />
            <div class="eq-dark-card-copy">
              <h3>Gehäuse und Bedienung</h3>
              <p>
                Das 3D-gedruckte Gehäuse nimmt Platine, OLED, Schalter, Taster,
                Potentiometer und Stromanschluss auf und wird an der Plattform
                befestigt.
              </p>
            </div>
          </article>
        </div>
        <div class="eq-note">
          Beim Bestücken auf die Polarität des Elektrolytkondensators achten,
          offene Lötstellen isolieren und vor der Inbetriebnahme alle Verbindungen prüfen.
        </div>
      </section>

      <section class="eq-section">
        <div class="eq-section-heading">
          <h2>Standard oder Simple</h2>
          <p>
            Beide Mechanikvarianten verwenden dasselbe Funktionsprinzip und
            dieselbe Elektronik.
          </p>
        </div>
        <div class="eq-card-grid">
          <article class="eq-card">
            <span class="eq-card-label">Standard</span>
            <h3>Vollständige Ausführung</h3>
            <p>
              Mit Aussparungen für Libelle, Kompass und Windrose sowie
              Markierungen auf der Oberplatte.
            </p>
          </article>
          <article class="eq-card">
            <span class="eq-card-label">Simple</span>
            <h3>Reduzierte Fertigung</h3>
            <p>
              Weniger Aussparungen und Markierungen senken den Fräsaufwand,
              ohne das Grundprinzip zu verändern.
            </p>
          </article>
          <article class="eq-card">
            <span class="eq-card-label">Alternative</span>
            <h3>Aluminiumsegmente</h3>
            <p>
              Kreissegmente für Winkel von 40 bis 60 Grad stehen zusätzlich
              als STEP-Dateien für Aluminium bereit.
            </p>
          </article>
        </div>
      </section>

      <section class="eq-section soft" id="downloads">
        <div class="eq-section-heading">
          <h2>Alles zum Nachbauen</h2>
          <p>
            Anleitung lesen, Teile fertigen lassen und die fertige Firmware
            direkt im Browser installieren.
          </p>
        </div>
        <div class="eq-resource-grid">
          <a class="eq-resource" href="build_instructions/buildinstruction_EQ_Plattform.pdf">
            <span class="eq-resource-type">PDF · 32 Seiten</span>
            <span>
              <strong>Bauanleitung</strong>
              <small>Prinzip, Bauteile, Montage, Elektronik und Inbetriebnahme.</small>
            </span>
            <span class="eq-resource-arrow">→</span>
          </a>
          <a class="eq-resource" href="teileliste/">
            <span class="eq-resource-type">Web · CSV</span>
            <span>
              <strong>Stückliste</strong>
              <small>Übersichtlich sortiert mit Mengen und kurzen Händlerlinks.</small>
            </span>
            <span class="eq-resource-arrow">→</span>
          </a>
          <a class="eq-resource" href="flasher/">
            <span class="eq-resource-type">Chrome · Edge</span>
            <span>
              <strong>Firmware Flasher</strong>
              <small>HEX-Datei ohne Arduino IDE direkt per USB installieren.</small>
            </span>
            <span class="eq-resource-arrow">→</span>
          </a>
          <a class="eq-resource" href="https://github.com/MoMa13570/eq-plattform/releases/latest">
            <span class="eq-resource-type">Release</span>
            <span>
              <strong>Aktuelle Firmware</strong>
              <small>Geprüfte HEX-Datei für Arduino Uno und Nano.</small>
            </span>
            <span class="eq-resource-arrow">→</span>
          </a>
          <a class="eq-resource" href="hardware/circuitboard/EQ-Plattform%20Schaltplan_TMC2209%20v32_jlcpcb.zip">
            <span class="eq-resource-type">ZIP</span>
            <span>
              <strong>PCB-Fertigungsdaten</strong>
              <small>Datenpaket für die Herstellung der Steuerplatine.</small>
            </span>
            <span class="eq-resource-arrow">→</span>
          </a>
          <a class="eq-resource" href="https://github.com/MoMa13570/eq-plattform/tree/main/hardware/mechanical">
            <span class="eq-resource-type">STEP · STL · 3MF</span>
            <span>
              <strong>CAD-Dateien</strong>
              <small>Mechanikvarianten, Kreissegmente und Elektronikgehäuse.</small>
            </span>
            <span class="eq-resource-arrow">→</span>
          </a>
          <a class="eq-resource" href="https://mende-cnc.de/" target="_blank" rel="noopener">
            <span class="eq-resource-type">Beispielanbieter</span>
            <span>
              <strong>Teile fertigen lassen</strong>
              <small>Holzteile fräsen und Gehäuseteile per 3D-Druck herstellen lassen.</small>
            </span>
            <span class="eq-resource-arrow">↗</span>
          </a>
        </div>
      </section>

      <section class="eq-final">
        <div>
          <h2>Bereit für den Aufbau?</h2>
          <p>
            Die PDF führt durch alle Details. Für die Firmware reichen anschließend
            eine HEX-Datei, Chrome oder Edge und ein USB-Kabel.
          </p>
        </div>
        <a class="eq-button" href="build_instructions/buildinstruction_EQ_Plattform.pdf">Bauanleitung starten</a>
      </section>
    </main>

    <footer class="eq-footer">
      <span>EQ Plattform · Frank Sackenheim &amp; Moritz Mayer</span>
      <span><a href="https://github.com/MoMa13570/eq-plattform">GitHub Repository</a></span>
    </footer>
  </div>
</div>

<script>
  (() => {
    const root = document.getElementById("eq-projection-explainer");
    if (!root) return;

    const left = document.getElementById("eq-projection-left");
    const right = document.getElementById("eq-projection-right");
    const phiInput = document.getElementById("eq-projection-phi");
    const betaInput = document.getElementById("eq-projection-beta");
    const playButton = document.getElementById("eq-projection-play");
    const progress = document.getElementById("eq-projection-progress");
    const prefersReducedMotion = window.matchMedia("(prefers-reduced-motion: reduce)").matches;
    let phi = Number(phiInput.value);
    let beta = Number(betaInput.value);
    let animationFrame = null;

    const radians = degrees => degrees * Math.PI / 180;
    const formatFactor = value => value.toFixed(3).replace(".", ",");
    const toPath = points => points.map((point, index) =>
      `${index === 0 ? "M" : "L"}${point[0].toFixed(2)},${point[1].toFixed(2)}`
    ).join(" ") + " Z";
    function ellipsePoints(cx, cy, rx, ry, rotation, horizontalScale = 1) {
      const angle = radians(rotation);
      const points = [];
      for (let i = 0; i <= 100; i += 1) {
        const t = i / 100 * Math.PI * 2;
        const x = rx * Math.cos(t);
        const y = ry * Math.sin(t);
        points.push([
          cx + (x * Math.cos(angle) - y * Math.sin(angle)) * horizontalScale,
          cy + x * Math.sin(angle) + y * Math.cos(angle)
        ]);
      }
      return points;
    }

    function ellipseSegmentPoints(cx, cy, rx, ry, rotation = 0, horizontalScale = 1) {
      const left = 0.24;
      const right = 0.62;
      const top = ry * 0.58;
      const angle = radians(rotation);
      const transform = (x, y) => [
        cx + (x * Math.cos(angle) - y * Math.sin(angle)) * horizontalScale,
        cy + x * Math.sin(angle) + y * Math.cos(angle)
      ];
      const curve = [];
      const start = Math.acos(right);
      const end = Math.acos(left);

      for (let i = 0; i <= 30; i += 1) {
        const t = start + (end - start) * i / 30;
        curve.push(transform(rx * Math.cos(t), ry * Math.sin(t)));
      }

      return [transform(rx * left, top), transform(rx * right, top), ...curve];
    }

    function marker(id) {
      return `<defs><marker id="${id}" markerWidth="7" markerHeight="7" refX="3.5" refY="3.5" orient="auto-start-reverse"><path d="M0,0 L7,3.5 L0,7 Z" fill="var(--eq-ink)"></path></marker></defs>`;
    }

    function drawFirstProjection() {
      const width = Math.max(280, left.parentElement.clientWidth);
      const height = 304;
      const cx = width / 2 - 10;
      const cy = 146;
      const radius = Math.min(92, width * 0.27);
      const projectedRadius = radius * Math.cos(radians(phi));
      const ellipse = ellipsePoints(cx, cy, radius, projectedRadius, 0);
      const vnsLeft = 0.24;
      const vnsSegment = ellipseSegmentPoints(cx, cy, radius, projectedRadius);
      const guideTop = cy - Math.max(radius, 42) - 14;
      const guideBottom = cy + Math.max(radius, 42) + 14;

      left.setAttribute("viewBox", `0 0 ${width} ${height}`);
      left.setAttribute("height", height);
      left.innerHTML = `${marker("eq-projection-arrow-left")}
        <line class="eq-projection-axis" x1="${cx - radius - 28}" y1="${cy}" x2="${cx + radius + 28}" y2="${cy}"></line>
        <line class="eq-projection-axis" x1="${cx}" y1="${guideTop}" x2="${cx}" y2="${guideBottom}"></line>
        <circle class="eq-projection-reference" cx="${cx}" cy="${cy}" r="${radius}"></circle>
        <path class="eq-projection-ellipse" d="${toPath(ellipse)}"></path>
        <line class="eq-projection-measure" x1="${cx + 18}" y1="${cy - projectedRadius}" x2="${cx + 18}" y2="${cy + projectedRadius}" marker-start="url(#eq-projection-arrow-left)" marker-end="url(#eq-projection-arrow-left)"></line>
        <path class="eq-projection-vns" d="${toPath(vnsSegment)}"></path>
        <text class="eq-projection-vns-label" x="${cx + radius * vnsLeft}" y="${Math.min(266, cy + projectedRadius + 34)}">VNS-Segment</text>
        <text class="formula" x="${cx - radius + 5}" y="${guideTop + 17}">vertikal × cos(φ)</text>
        <text class="muted" x="${cx - radius + 5}" y="${guideTop + 34}">${formatFactor(Math.cos(radians(phi)))} · Ausgangshöhe</text>
        <text class="muted" x="${cx - radius}" y="292">Kreis (Referenz)</text>
        <text x="${cx + radius}" y="292" text-anchor="end">Ellipse</text>`;
    }

    function drawRollerProjection() {
      const width = Math.max(280, right.parentElement.clientWidth);
      const height = 304;
      const cx = width / 2;
      const cy = 146;
      const radius = Math.min(82, width * 0.23);
      const projectedRadius = radius * Math.cos(radians(phi));
      const factor = 1 / Math.max(0.01, Math.cos(radians(beta)));
      const before = ellipsePoints(cx, cy, radius, projectedRadius, -beta, 1);
      const after = ellipsePoints(cx, cy, radius, projectedRadius, -beta, factor);
      const vnsSegment = ellipseSegmentPoints(cx, cy, radius, projectedRadius, -beta, factor);
      const xValues = after.map(point => point[0]);
      const minX = Math.min(...xValues);
      const maxX = Math.max(...xValues);
      const rollerLength = radius * 1.4;
      const angle = radians(-beta);

      right.setAttribute("viewBox", `0 0 ${width} ${height}`);
      right.setAttribute("height", height);
      right.innerHTML = `${marker("eq-projection-arrow-right")}
        <line class="eq-projection-axis" x1="${cx - radius - 50}" y1="${cy}" x2="${cx + radius + 50}" y2="${cy}"></line>
        <line class="eq-projection-reference" x1="${cx}" y1="${cy - radius - 38}" x2="${cx}" y2="${cy + radius + 38}"></line>
        <path class="eq-projection-before" d="${toPath(before)}"></path>
        <path class="eq-projection-after" d="${toPath(after)}"></path>
        <path class="eq-projection-vns" d="${toPath(vnsSegment)}"></path>
        <line class="eq-projection-roller" x1="${cx - Math.cos(angle) * rollerLength}" y1="${cy - Math.sin(angle) * rollerLength}" x2="${cx + Math.cos(angle) * rollerLength}" y2="${cy + Math.sin(angle) * rollerLength}"></line>
        <path class="eq-projection-angle" d="M ${cx + 43} ${cy} A 43 43 0 0 0 ${cx + 43 * Math.cos(angle)} ${cy + 43 * Math.sin(angle)}"></path>
        <text class="formula" x="${cx + 50}" y="${cy - 17}">β = ${Math.round(beta)}°</text>
        <line class="eq-projection-measure" x1="${minX}" y1="${cy + radius + 49}" x2="${maxX}" y2="${cy + radius + 49}" marker-start="url(#eq-projection-arrow-right)" marker-end="url(#eq-projection-arrow-right)"></line>
        <text class="formula" x="${cx}" y="${cy + radius + 40}" text-anchor="middle">horizontal × 1/cos(β) = ${formatFactor(factor)}</text>
        <text class="muted" x="${cx}" y="292" text-anchor="middle">gestrichelt: davor · farbig: korrigiert</text>`;
    }

    function updateProjection() {
      const cosPhi = Math.cos(radians(phi));
      const betaFactor = 1 / Math.max(0.01, Math.cos(radians(beta)));
      document.getElementById("eq-projection-phi-value").textContent = `φ = ${Math.round(phi)}° · cos(φ) = ${formatFactor(cosPhi)}`;
      document.getElementById("eq-projection-beta-value").textContent = `${Math.round(beta)}° · Faktor ${formatFactor(betaFactor)}`;
      document.getElementById("eq-projection-phi-output").textContent = `${Math.round(phi)}°`;
      document.getElementById("eq-projection-beta-output").textContent = `${Math.round(beta)}°`;
      drawFirstProjection();
      drawRollerProjection();
    }

    phiInput.addEventListener("input", event => {
      if (animationFrame) cancelAnimationFrame(animationFrame);
      animationFrame = null;
      playButton.disabled = false;
      phi = Number(event.target.value);
      updateProjection();
    });

    betaInput.addEventListener("input", event => {
      if (animationFrame) cancelAnimationFrame(animationFrame);
      animationFrame = null;
      playButton.disabled = false;
      beta = Number(event.target.value);
      updateProjection();
    });

    playButton.addEventListener("click", () => {
      if (animationFrame) cancelAnimationFrame(animationFrame);
      const targetPhi = Number(phiInput.value);
      const targetBeta = Number(betaInput.value);
      const duration = prefersReducedMotion ? 1 : 4200;
      const start = performance.now();
      playButton.disabled = true;

      const tick = now => {
        const total = Math.min(1, (now - start) / duration);
        if (total < 0.47) {
          phi = targetPhi * (total / 0.47);
          beta = 0;
        } else if (total < 0.57) {
          phi = targetPhi;
          beta = 0;
        } else {
          phi = targetPhi;
          beta = targetBeta * ((total - 0.57) / 0.43);
        }
        progress.style.width = `${total * 100}%`;
        updateProjection();

        if (total < 1) {
          animationFrame = requestAnimationFrame(tick);
        } else {
          phi = targetPhi;
          beta = targetBeta;
          animationFrame = null;
          playButton.disabled = false;
          updateProjection();
        }
      };

      animationFrame = requestAnimationFrame(tick);
    });

    let resizeFrame = null;
    window.addEventListener("resize", () => {
      if (resizeFrame) cancelAnimationFrame(resizeFrame);
      resizeFrame = requestAnimationFrame(() => {
        resizeFrame = null;
        updateProjection();
      });
    });

    updateProjection();
  })();
</script>
