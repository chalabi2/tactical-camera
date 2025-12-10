<script lang="ts">
  import { onMount } from "svelte";

  interface AccelData {
    x: number;
    y: number;
    z: number;
  }

  interface GyroData {
    x: number;
    y: number;
    z: number;
  }

  interface OrientationData {
    available: boolean;
    pitch: number;
    roll: number;
    accel: AccelData;
    gyro: GyroData;
    temp: number;
  }

  interface TelemetryResponse {
    orientation: OrientationData;
    timestamp: number;
  }

  let orientation: OrientationData | null = null;
  let error: string | null = null;

  function formatAngle(deg: number): string {
    const sign = deg >= 0 ? "+" : "";
    return `${sign}${deg.toFixed(1)}`;
  }

  function getWindageDirection(roll: number): string {
    if (Math.abs(roll) < 2) return "LEVEL";
    return roll > 0 ? "RIGHT" : "LEFT";
  }

  function getPitchDirection(pitch: number): string {
    if (Math.abs(pitch) < 2) return "LEVEL";
    return pitch > 0 ? "UP" : "DOWN";
  }

  async function fetchOrientation() {
    try {
      const res = await fetch("/api/telemetry");
      if (!res.ok) throw new Error(`${res.status}`);
      const telemetry: TelemetryResponse = await res.json();
      orientation = telemetry.orientation;
      error = null;
    } catch (e) {
      error = e instanceof Error ? e.message : "ERR";
    }
  }

  onMount(() => {
    fetchOrientation();
    const interval = setInterval(fetchOrientation, 100);
    return () => clearInterval(interval);
  });
</script>

<div class="panel">
  <div class="panel-header">
    <span class="panel-title">ORIENTATION</span>
    {#if orientation}
      <span class="imu-status" class:active={orientation.available}>
        IMU {orientation.available ? "OK" : "N/A"}
      </span>
    {/if}
  </div>

  <div class="panel-body">
    {#if error}
      <div class="error">ERR: {error}</div>
    {:else if orientation}
      <div class="orientation-grid">
        <div class="horizon-container">
          <div class="horizon-ring">
            <div 
              class="horizon-ball"
              style="transform: translate({orientation.roll * 1.5}px, {orientation.pitch * 1.5}px)"
            >
              <div class="horizon-crosshair"></div>
            </div>
            <div class="horizon-lines">
              <div class="h-line"></div>
              <div class="v-line"></div>
            </div>
            <div class="horizon-labels">
              <span class="label-n">FWD</span>
              <span class="label-e">R</span>
              <span class="label-s">AFT</span>
              <span class="label-w">L</span>
            </div>
          </div>
          <div class="compass-ticks">
            {#each Array(12) as _, i}
              <div 
                class="tick" 
                style="transform: rotate({i * 30}deg)"
              ></div>
            {/each}
          </div>
        </div>

        <div class="data-columns">
          <div class="angle-box">
            <div class="angle-row">
              <span class="angle-label">PITCH</span>
              <span class="angle-value">{formatAngle(orientation.pitch)}°</span>
              <span class="angle-dir">{getPitchDirection(orientation.pitch)}</span>
            </div>
            <div class="angle-bar-container">
              <div class="angle-bar-bg"></div>
              <div 
                class="angle-bar pitch-bar" 
                style="height: {Math.min(Math.abs(orientation.pitch) * 2, 100)}%; {orientation.pitch > 0 ? 'bottom: 50%' : 'top: 50%'}"
              ></div>
              <div class="angle-bar-center"></div>
            </div>
          </div>

          <div class="angle-box">
            <div class="angle-row">
              <span class="angle-label">ROLL</span>
              <span class="angle-value">{formatAngle(orientation.roll)}°</span>
              <span class="angle-dir">{getWindageDirection(orientation.roll)}</span>
            </div>
            <div class="angle-bar-container horizontal">
              <div class="angle-bar-bg"></div>
              <div 
                class="angle-bar roll-bar" 
                style="width: {Math.min(Math.abs(orientation.roll) * 2, 100)}%; {orientation.roll > 0 ? 'left: 50%' : 'right: 50%'}"
              ></div>
              <div class="angle-bar-center horizontal"></div>
            </div>
          </div>
        </div>

        <div class="raw-data">
          <div class="raw-section">
            <span class="raw-title">ACCEL (g)</span>
            <div class="raw-values">
              <span>X: {orientation.accel.x.toFixed(2)}</span>
              <span>Y: {orientation.accel.y.toFixed(2)}</span>
              <span>Z: {orientation.accel.z.toFixed(2)}</span>
            </div>
          </div>
          <div class="raw-section">
            <span class="raw-title">GYRO (°/s)</span>
            <div class="raw-values">
              <span>X: {orientation.gyro.x.toFixed(1)}</span>
              <span>Y: {orientation.gyro.y.toFixed(1)}</span>
              <span>Z: {orientation.gyro.z.toFixed(1)}</span>
            </div>
          </div>
          <div class="temp-display">
            <span class="temp-label">TEMP</span>
            <span class="temp-value">{orientation.temp.toFixed(1)}°C</span>
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
    grid-column: span 2;
  }

  @media (max-width: 600px) {
    .panel {
      grid-column: span 1;
    }
  }

  .panel-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 0.5rem 0.75rem;
    background: #0a0a0a;
    border-bottom: 1px solid #2a3548;
  }

  .panel-title {
    font-size: 0.7rem;
    font-weight: bold;
    letter-spacing: 0.1em;
    color: #0fa;
  }

  .imu-status {
    font-size: 0.6rem;
    padding: 0.15rem 0.4rem;
    background: #400;
    color: #f44;
    border-radius: 2px;
  }

  .imu-status.active {
    background: #042;
    color: #0f8;
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

  .orientation-grid {
    display: grid;
    grid-template-columns: auto 1fr;
    grid-template-rows: auto auto;
    gap: 0.75rem;
  }

  .horizon-container {
    position: relative;
    width: 120px;
    height: 120px;
    grid-row: span 2;
  }

  .horizon-ring {
    position: absolute;
    inset: 10px;
    border: 2px solid #2a3548;
    border-radius: 50%;
    background: radial-gradient(circle at center, #0a1520 0%, #050a10 100%);
    overflow: hidden;
  }

  .horizon-ball {
    position: absolute;
    top: 50%;
    left: 50%;
    width: 20px;
    height: 20px;
    margin: -10px 0 0 -10px;
    background: radial-gradient(circle at 30% 30%, #0ff, #0aa);
    border-radius: 50%;
    box-shadow: 0 0 10px #0ff8, 0 0 20px #0ff4;
    transition: transform 0.1s ease-out;
  }

  .horizon-crosshair {
    position: absolute;
    top: 50%;
    left: 50%;
    width: 6px;
    height: 6px;
    margin: -3px 0 0 -3px;
    background: #000;
    border-radius: 50%;
  }

  .horizon-lines {
    position: absolute;
    inset: 0;
    pointer-events: none;
  }

  .h-line {
    position: absolute;
    top: 50%;
    left: 10%;
    right: 10%;
    height: 1px;
    background: #2a3548;
  }

  .v-line {
    position: absolute;
    left: 50%;
    top: 10%;
    bottom: 10%;
    width: 1px;
    background: #2a3548;
  }

  .horizon-labels {
    position: absolute;
    inset: 0;
    font-size: 0.5rem;
    color: #556;
    pointer-events: none;
  }

  .label-n {
    position: absolute;
    top: 4px;
    left: 50%;
    transform: translateX(-50%);
  }

  .label-s {
    position: absolute;
    bottom: 4px;
    left: 50%;
    transform: translateX(-50%);
  }

  .label-e {
    position: absolute;
    right: 6px;
    top: 50%;
    transform: translateY(-50%);
  }

  .label-w {
    position: absolute;
    left: 6px;
    top: 50%;
    transform: translateY(-50%);
  }

  .compass-ticks {
    position: absolute;
    inset: 0;
    pointer-events: none;
  }

  .tick {
    position: absolute;
    top: 0;
    left: 50%;
    width: 1px;
    height: 8px;
    background: #2a3548;
    transform-origin: center 60px;
  }

  .tick:nth-child(3n+1) {
    height: 10px;
    background: #3a4558;
  }

  .data-columns {
    display: flex;
    flex-direction: column;
    gap: 0.5rem;
  }

  .angle-box {
    background: #0a0a0a;
    border: 1px solid #222;
    padding: 0.5rem;
  }

  .angle-row {
    display: flex;
    align-items: baseline;
    gap: 0.5rem;
  }

  .angle-label {
    font-size: 0.6rem;
    color: #666;
    min-width: 35px;
  }

  .angle-value {
    font-size: 1rem;
    font-weight: bold;
    color: #0fa;
    font-family: monospace;
    min-width: 55px;
  }

  .angle-dir {
    font-size: 0.55rem;
    color: #888;
    letter-spacing: 0.05em;
  }

  .angle-bar-container {
    position: relative;
    height: 40px;
    margin-top: 0.4rem;
  }

  .angle-bar-container.horizontal {
    height: 12px;
  }

  .angle-bar-bg {
    position: absolute;
    inset: 0;
    background: #151515;
    border-radius: 2px;
  }

  .angle-bar {
    position: absolute;
    left: 50%;
    width: 40%;
    background: linear-gradient(to top, #0fa, #0a8);
    border-radius: 2px;
    transform: translateX(-50%);
    transition: height 0.1s ease-out;
  }

  .pitch-bar {
    left: 50%;
    width: 40%;
    transform: translateX(-50%);
  }

  .roll-bar {
    top: 0;
    bottom: 0;
    height: 100%;
    background: linear-gradient(to right, #f80, #fa0);
    transform: none;
  }

  .angle-bar-center {
    position: absolute;
    left: 10%;
    right: 10%;
    top: 50%;
    height: 1px;
    background: #333;
  }

  .angle-bar-center.horizontal {
    left: 50%;
    right: auto;
    top: 0;
    bottom: 0;
    width: 1px;
    height: auto;
  }

  .raw-data {
    grid-column: span 2;
    display: flex;
    gap: 0.75rem;
    flex-wrap: wrap;
  }

  .raw-section {
    background: #0a0a0a;
    border: 1px solid #222;
    padding: 0.4rem 0.5rem;
    flex: 1;
    min-width: 100px;
  }

  .raw-title {
    font-size: 0.55rem;
    color: #666;
    display: block;
    margin-bottom: 0.25rem;
  }

  .raw-values {
    display: flex;
    gap: 0.5rem;
    font-size: 0.65rem;
    color: #888;
    font-family: monospace;
  }

  .temp-display {
    background: #0a0a0a;
    border: 1px solid #222;
    padding: 0.4rem 0.5rem;
    display: flex;
    flex-direction: column;
    align-items: center;
    min-width: 60px;
  }

  .temp-label {
    font-size: 0.55rem;
    color: #666;
  }

  .temp-value {
    font-size: 0.8rem;
    color: #f80;
    font-family: monospace;
  }
</style>

