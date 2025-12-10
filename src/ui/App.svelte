<script lang="ts">
  import StatusPanel from "./components/StatusPanel.svelte";
  import CameraFeed from "./components/CameraFeed.svelte";
  import OrientationPanel from "./components/OrientationPanel.svelte";

  let isStreaming = false;
</script>

<div class="console">
  <header class="header">
    <div class="logo">
      <span class="logo-icon">*</span>
      <span class="logo-text">TACTICAL CONSOLE</span>
    </div>
    <div class="header-info">
      <span class="status-dot"></span>
      <span>ONLINE</span>
    </div>
  </header>

  <main class="panels">
    <StatusPanel {isStreaming} />
    <CameraFeed bind:isStreaming />
    <OrientationPanel />
  </main>

  <footer class="footer">
    <span>ESP32-S3 CAM</span>
    <span class="separator">|</span>
    <span>OFFLINE MODE</span>
  </footer>
</div>

<style>
  .console {
    display: flex;
    flex-direction: column;
    height: 100%;
    background: #0a0e14;
  }

  .header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 0.75rem 1rem;
    background: #000;
    border-bottom: 1px solid #2a3548;
  }

  .logo {
    display: flex;
    align-items: center;
    gap: 0.5rem;
  }

  .logo-icon {
    font-size: 1.25rem;
    color: #0f0;
  }

  .logo-text {
    font-size: 1rem;
    font-weight: bold;
    letter-spacing: 0.1em;
    color: #eee;
  }

  .header-info {
    display: flex;
    align-items: center;
    gap: 0.5rem;
    font-size: 0.7rem;
    color: #888;
    letter-spacing: 0.1em;
  }

  .status-dot {
    width: 8px;
    height: 8px;
    background: #0f0;
    border-radius: 50%;
  }

  .panels {
    flex: 1;
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 0.75rem;
    padding: 0.75rem;
    overflow: auto;
  }

  @media (max-width: 600px) {
    .panels {
      grid-template-columns: 1fr;
    }
  }

  .footer {
    display: flex;
    justify-content: center;
    align-items: center;
    gap: 0.5rem;
    padding: 0.5rem;
    background: #000;
    border-top: 1px solid #2a3548;
    font-size: 0.65rem;
    color: #666;
    letter-spacing: 0.1em;
  }

  .separator {
    color: #333;
  }
</style>
