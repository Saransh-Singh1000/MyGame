#pragma once

namespace Launcher {

// Shared launcher markup and behavior for platform-specific hosts.
inline constexpr char page[] = R"MYGAME_LAUNCHER(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>MyGame Launcher</title>
  <style>
    @font-face { font-family: "Launcher Sans"; src: local("Segoe UI"); }
    :root {
      color-scheme: dark;
      font-family: "Launcher Sans", "Segoe UI", sans-serif;
      background: #090c12;
      color: #f4f6fa;
      font-synthesis: none;
      text-rendering: geometricPrecision;
    }
    * { box-sizing: border-box; }
    body {
      min-width: 760px;
      min-height: 100vh;
      margin: 0;
      overflow: hidden;
      background:
        radial-gradient(ellipse at 82% 12%, rgba(37, 113, 105, .2), transparent 35%),
        radial-gradient(ellipse at 8% 91%, rgba(60, 79, 141, .18), transparent 35%),
        #090c12;
    }
    body::after {
      position: fixed;
      inset: 0;
      z-index: -1;
      background-image: linear-gradient(rgba(255,255,255,.018) 1px, transparent 1px),
                        linear-gradient(90deg, rgba(255,255,255,.018) 1px, transparent 1px);
      background-size: 48px 48px;
      content: "";
      mask-image: linear-gradient(to bottom, black, transparent 88%);
    }
    .launcher {
      width: min(100%, 1120px);
      min-height: 100vh;
      margin: 0 auto;
      padding: 30px 72px 26px;
      display: flex;
      flex-direction: column;
    }
    .topbar, .brand, .section-heading, .api-card, .launch-row, .launch-note {
      display: flex;
      align-items: center;
    }
    .topbar { justify-content: space-between; }
    .brand { gap: 12px; font-size: 15px; font-weight: 800; letter-spacing: .22em; }
    .brand-light { color: #8f9aa9; font-weight: 500; }
    .brand-mark {
      width: 30px;
      height: 30px;
      display: flex;
      align-items: end;
      justify-content: center;
      gap: 3px;
      padding: 5px;
      border: 1px solid rgba(116, 231, 194, .22);
      border-radius: 8px;
      background: rgba(67, 200, 159, .08);
    }
    .brand-mark i { width: 4px; border-radius: 3px; background: #65dfb5; }
    .brand-mark i:nth-child(1) { height: 9px; opacity: .65; }
    .brand-mark i:nth-child(2) { height: 15px; }
    .brand-mark i:nth-child(3) { height: 11px; opacity: .8; }
    .build-tag, .single-choice { color: #788493; font-size: 10px; letter-spacing: .16em; }
    .build-tag { display: flex; align-items: center; gap: 8px; }
    .live-dot {
      width: 6px;
      height: 6px;
      border-radius: 50%;
      background: #68ddb2;
      box-shadow: 0 0 12px #68ddb2;
    }
    .hero { margin-top: 58px; }
    .eyebrow {
      margin: 0 0 12px;
      color: #74dcb8;
      font-size: 10px;
      font-weight: 700;
      letter-spacing: .2em;
    }
    h1 { margin: 0; font-size: clamp(45px, 6vw, 66px); line-height: .98; letter-spacing: -.055em; font-weight: 650; }
    h1 span { color: #73dfba; }
    .intro { margin: 15px 0 0; color: #929cab; font-size: 14px; }
    .api-section { margin-top: auto; padding-top: 44px; }
    .section-heading { justify-content: space-between; margin-bottom: 14px; }
    .section-heading .eyebrow { margin-bottom: 5px; font-size: 9px; }
    h2 { margin: 0; font-size: 19px; font-weight: 600; letter-spacing: -.02em; }
    .single-choice { padding: 7px 10px; border: 1px solid #242b35; border-radius: 5px; font-size: 9px; }
    .api-options { display: grid; grid-template-columns: 1fr 1fr; gap: 13px; }
    .api-card {
      min-height: 98px;
      position: relative;
      gap: 14px;
      padding: 17px 16px;
      border: 1px solid #252d38;
      border-radius: 9px;
      background: linear-gradient(135deg, rgba(24,31,41,.94), rgba(16,21,29,.92));
      cursor: pointer;
      transition: border-color .16s ease, background .16s ease, transform .16s ease;
    }
    .api-card:hover { transform: translateY(-2px); border-color: #465361; }
    .api-card.selected {
      border-color: rgba(105, 221, 183, .74);
      background: linear-gradient(135deg, rgba(29,58,53,.74), rgba(17,30,33,.96));
      box-shadow: inset 0 0 0 1px rgba(105,221,183,.1), 0 8px 28px rgba(0,0,0,.16);
    }
    .api-card input { position: absolute; width: 1px; height: 1px; opacity: 0; }
    .checkmark {
      width: 17px;
      height: 17px;
      flex: none;
      position: relative;
      border: 1px solid #56616e;
      border-radius: 4px;
      background: #10151c;
    }
    .selected .checkmark { border-color: #72dfb9; background: #72dfb9; }
    .selected .checkmark::after {
      position: absolute;
      top: 2px;
      left: 5px;
      width: 4px;
      height: 8px;
      border: solid #092017;
      border-width: 0 2px 2px 0;
      transform: rotate(45deg);
      content: "";
    }
    .api-card:focus-within { outline: 2px solid rgba(114,223,185,.55); outline-offset: 3px; }
    .api-icon {
      width: 40px;
      height: 40px;
      flex: none;
      display: grid;
      place-items: center;
      border-radius: 8px;
    }
    .api-icon svg { width: 23px; height: 23px; fill: currentColor; }
    .vulkan-icon { color: #f09a78; background: rgba(240,154,120,.1); }
    .opengl-icon { color: #85a8fc; background: rgba(133,168,252,.1); }
    .api-copy { display: flex; flex-direction: column; gap: 5px; }
    .api-copy strong { font-size: 14px; font-weight: 650; }
    .api-copy span { color: #8994a2; font-size: 11px; }
    .api-badge {
      margin-left: auto;
      align-self: start;
      padding: 5px 7px;
      border-radius: 4px;
      background: rgba(104,221,178,.11);
      color: #7eddbb;
      font-size: 8px;
      font-weight: 700;
      letter-spacing: .08em;
    }
    .api-badge.compatibility { background: rgba(133,168,252,.1); color: #9ab4f7; }
    .launch-row { justify-content: space-between; margin-top: 22px; padding-top: 17px; border-top: 1px solid #202630; }
    .launch-note { gap: 9px; color: #8792a0; font-size: 11px; }
    .keycap {
      width: 21px;
      height: 21px;
      display: grid;
      place-items: center;
      border: 1px solid #343d48;
      border-radius: 5px;
      color: #bdc6d1;
      font-size: 12px;
    }
    #play-button {
      min-width: 150px;
      height: 43px;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 13px;
      border: 0;
      border-radius: 6px;
      background: #71dfb8;
      color: #092018;
      font: inherit;
      font-size: 11px;
      font-weight: 800;
      letter-spacing: .12em;
      cursor: pointer;
      transition: background .15s ease, transform .15s ease, box-shadow .15s ease;
    }
    #play-button:hover { transform: translateY(-1px); background: #8aebc8; box-shadow: 0 7px 22px rgba(91,220,176,.2); }
    #play-button:active { transform: translateY(0); }
    #play-button svg { width: 15px; height: 15px; fill: currentColor; }
    #play-button:disabled { cursor: wait; opacity: .7; }
    .status { min-height: 14px; margin: 10px 0 0; color: #727e8c; text-align: right; font-size: 10px; }
    @media (max-width: 820px) {
      .launcher { padding-right: 38px; padding-left: 38px; }
      .api-options { grid-template-columns: 1fr; }
      .api-section { padding-top: 30px; }
      .api-card { min-height: 80px; }
    }
  </style>
</head>
<body>
  <main class="launcher">
    <header class="topbar">
      <div class="brand">
        <div class="brand-mark" aria-hidden="true"><i></i><i></i><i></i></div>
        <span>MY<span class="brand-light">GAME</span></span>
      </div>
      <span class="build-tag"><span class="live-dot"></span> WINDOWS EDITION</span>
    </header>
    <section class="hero">
      <p class="eyebrow">YOUR WORLD. YOUR SETTINGS.</p>
      <h1>Ready when<br>you <span>are.</span></h1>
      <p class="intro">Choose your graphics API and jump straight into the game.</p>
    </section>
    <section class="api-section" aria-labelledby="api-heading">
      <div class="section-heading">
        <div>
          <p class="eyebrow">DISPLAY</p>
          <h2 id="api-heading">Graphics API</h2>
        </div>
        <span class="single-choice">SELECT ONE</span>
      </div>
      <div class="api-options" role="radiogroup" aria-label="Graphics API">
        <label class="api-card selected" for="api-vulkan">
          <input id="api-vulkan" type="radio" name="graphics-api" value="vulkan" checked>
          <span class="checkmark" aria-hidden="true"></span>
          <span class="api-icon vulkan-icon" aria-hidden="true">
            <svg viewBox="0 0 48 48"><path d="M24 5 43 39H5L24 5Zm0 9L13 34h22L24 14Zm0 8 7 12H17l7-12Z"/></svg>
          </span>
          <span class="api-copy"><strong>Vulkan</strong><span>Low-level performance</span></span>
          <span class="api-badge">DEFAULT</span>
        </label>
        <label class="api-card" for="api-opengl">
          <input id="api-opengl" type="radio" name="graphics-api" value="opengl">
          <span class="checkmark" aria-hidden="true"></span>
          <span class="api-icon opengl-icon" aria-hidden="true">
            <svg viewBox="0 0 48 48"><path d="M24 7a17 17 0 1 0 16.7 20H34a10 10 0 1 1-2.4-9.2l5-5A16.9 16.9 0 0 0 24 7Zm14 1v9h-3V8h3Zm5 0v9h-3V8h3ZM24 19a5 5 0 1 0 0 10 5 5 0 0 0 0-10Z"/></svg>
          </span>
          <span class="api-copy"><strong>OpenGL</strong><span>Broad compatibility</span></span>
          <span class="api-badge compatibility">COMPATIBLE</span>
        </label>
      </div>
    </section>
    <footer class="launch-row">
      <div class="launch-note"><span class="keycap">↵</span><span>Selected API starts with the game</span></div>
      <button id="play-button" type="button">
        <span>PLAY NOW</span>
        <svg viewBox="0 0 20 20" aria-hidden="true"><path d="M5 3.5v13l11-6.5-11-6.5Z"/></svg>
      </button>
    </footer>
    <p id="status" class="status" aria-live="polite">Your selection is ready.</p>
  </main>
  <script>
    (() => {
      const cards = [...document.querySelectorAll(".api-card")];
      const options = [...document.querySelectorAll('input[name="graphics-api"]')];
      const playButton = document.getElementById("play-button");
      const status = document.getElementById("status");
      function updateSelection() {
        for (const card of cards) {
          const input = card.querySelector("input");
          card.classList.toggle("selected", input.checked);
        }
        const selected = document.querySelector('input[name="graphics-api"]:checked');
        status.textContent = `${selected.value === "vulkan" ? "Vulkan" : "OpenGL"} is selected.`;
      }
      for (const option of options) option.addEventListener("change", updateSelection);
      playButton.addEventListener("click", () => {
        const selected = document.querySelector('input[name="graphics-api"]:checked');
        if (!selected) return;
        playButton.disabled = true;
        status.textContent = `Starting MyGame with ${selected.value === "vulkan" ? "Vulkan" : "OpenGL"}…`;
        if (typeof window.nativePlay !== "function") {
          playButton.disabled = false;
          status.textContent = "The game launcher connection is unavailable.";
          return;
        }
        window.nativePlay(selected.value);
      });
      document.addEventListener("keydown", event => {
        if (event.key === "Enter" && !playButton.disabled) playButton.click();
      });
      updateSelection();
    })();
  </script>
</body>
</html>
)MYGAME_LAUNCHER";

} // namespace Launcher
