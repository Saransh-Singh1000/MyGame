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

  for (const option of options) {
    option.addEventListener("change", updateSelection);
  }

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
