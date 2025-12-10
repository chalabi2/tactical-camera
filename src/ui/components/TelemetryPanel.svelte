<script lang="ts">
  import { onMount } from "svelte";

  interface TelemetryData {
    lat: number;
    lon: number;
    speedKph: number;
    headingDeg: number;
  }

  let telemetry: TelemetryData | null = null;
  let error: string | null = null;

  function formatCoord(value: number, isLat: boolean): string {
    const dir = isLat ? (value >= 0 ? "N" : "S") : (value >= 0 ? "E" : "W");
    return `${Math.abs(value).toFixed(4)} ${dir}`;
  }

  function getHeadingLabel(deg: number): string {
    const dirs = ["N", "NE", "E", "SE", "S", "SW", "W", "NW"];
    return dirs[Math.round(deg / 45) % 8];
  }

  async function fetchTelemetry() {
    try {
      const res = await fetch("/api/telemetry");
      if (!res.ok) throw new Error(`${res.status}`);
      telemetry = await res.json();
      error = null;
    } catch (e) {
      error = e instanceof Error ? e.message : "ERR";
    }
  }

  onMount(() => {
    fetchTelemetry();
    const interval = setInterval(fetchTelemetry, 2000);
    return () => clearInterval(interval);
  });
</script>

<div class="panel">
  <div class="panel-header">
    <span class="panel-title">TELEMETRY</span>
  </div>

  <div class="panel-body">
    {#if error}
      <div class="error">ERR: {error}</div>
    {:else if telemetry}
      <div class="telem-grid">
        <div class="coord-box">
          <div class="coord-row">
            <span class="label">LAT</span>
            <span class="value">{formatCoord(telemetry.lat, true)}</span>
          </div>
          <div class="coord-row">
            <span class="label">LON</span>
            <span class="value">{formatCoord(telemetry.lon, false)}</span>
          </div>
        </div>

        <div class="metrics">
          <div class="metric">
            <div class="metric-val">{telemetry.speedKph}</div>
            <div class="metric-unit">KPH</div>
          </div>
          <div class="metric">
            <div class="metric-val">{telemetry.headingDeg}°</div>
            <div class="metric-unit">{getHeadingLabel(telemetry.headingDeg)}</div>
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
    color: #fa0;
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

  .telem-grid {
    display: flex;
    flex-direction: column;
    gap: 0.5rem;
  }

  .coord-box {
    background: #0a0a0a;
    border: 1px solid #222;
    padding: 0.5rem;
  }

  .coord-row {
    display: flex;
    justify-content: space-between;
    padding: 0.2rem 0;
  }

  .label {
    font-size: 0.65rem;
    color: #666;
  }

  .value {
    font-size: 0.75rem;
    color: #0af;
    font-family: monospace;
  }

  .metrics {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 0.5rem;
  }

  .metric {
    background: #0a0a0a;
    border: 1px solid #222;
    padding: 0.5rem;
    text-align: center;
  }

  .metric-val {
    font-size: 1.25rem;
    font-weight: bold;
    color: #eee;
  }

  .metric-unit {
    font-size: 0.6rem;
    color: #fa0;
    margin-top: 0.1rem;
  }
</style>
