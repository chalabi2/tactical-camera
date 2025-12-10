<script lang="ts">
  import { onMount } from "svelte";

  interface StatusData {
    deviceId: string;
    uptimeSeconds: number;
    version: string;
    recording: boolean;
  }

  let status: StatusData | null = null;
  let error: string | null = null;

  function formatUptime(seconds: number): string {
    const hours = Math.floor(seconds / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const secs = seconds % 60;
    if (hours > 0) return `${hours}h ${minutes}m`;
    if (minutes > 0) return `${minutes}m ${secs}s`;
    return `${secs}s`;
  }

  async function fetchStatus() {
    try {
      const res = await fetch("/api/status");
      if (!res.ok) throw new Error(`${res.status}`);
      status = await res.json();
      error = null;
    } catch (e) {
      error = e instanceof Error ? e.message : "ERR";
    }
  }

  onMount(() => {
    fetchStatus();
    const interval = setInterval(fetchStatus, 3000);
    return () => clearInterval(interval);
  });
</script>

<div class="panel">
  <div class="panel-header">
    <span class="panel-title">STATUS</span>
  </div>

  <div class="panel-body">
    {#if error}
      <div class="error">ERR: {error}</div>
    {:else if status}
      <div class="stat-grid">
        <div class="stat">
          <div class="stat-label">ID</div>
          <div class="stat-value">{status.deviceId}</div>
        </div>

        <div class="stat">
          <div class="stat-label">UPTIME</div>
          <div class="stat-value">{formatUptime(status.uptimeSeconds)}</div>
        </div>

        <div class="stat">
          <div class="stat-label">VER</div>
          <div class="stat-value">{status.version}</div>
        </div>

        <div class="stat">
          <div class="stat-label">REC</div>
          <div class="stat-value" class:rec-on={status.recording} class:rec-off={!status.recording}>
            {status.recording ? "ON" : "OFF"}
          </div>
        </div>
      </div>
    {:else}
      <div class="loading">...</div>
    {/if}
  </div>
</div>

<style>
  .panel {
    background: #111;
    border: 1px solid #2a3548;
  }

  .panel-header {
    padding: 0.5rem 0.75rem;
    background: #0a0a0a;
    border-bottom: 1px solid #2a3548;
  }

  .panel-title {
    font-size: 0.7rem;
    font-weight: bold;
    letter-spacing: 0.1em;
    color: #0af;
  }

  .panel-body {
    padding: 0.75rem;
  }

  .loading,
  .error {
    font-size: 0.8rem;
    color: #666;
  }

  .error {
    color: #f44;
  }

  .stat-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 0.5rem;
  }

  .stat {
    padding: 0.5rem;
    background: #0a0a0a;
    border: 1px solid #222;
  }

  .stat-label {
    font-size: 0.55rem;
    color: #666;
    letter-spacing: 0.1em;
    margin-bottom: 0.2rem;
  }

  .stat-value {
    font-size: 0.85rem;
    color: #eee;
    font-weight: bold;
  }

  .rec-on {
    color: #0f0;
  }

  .rec-off {
    color: #f44;
  }
</style>
