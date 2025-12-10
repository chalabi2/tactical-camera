<script lang="ts">
  import { onMount } from "svelte";

  let streamActive = false;
  let streamError: string | null = null;
  let imgRef: HTMLImageElement;

  function startStream() {
    if (imgRef) {
      imgRef.src = "/api/stream?" + Date.now();
      streamActive = true;
      streamError = null;
    }
  }

  function stopStream() {
    if (imgRef) {
      imgRef.src = "";
      streamActive = false;
    }
  }

  async function captureFrame() {
    try {
      const res = await fetch("/api/capture");
      if (!res.ok) throw new Error(`${res.status}`);
      const blob = await res.blob();
      const url = URL.createObjectURL(blob);
      if (imgRef) {
        imgRef.src = url;
        streamActive = false;
      }
    } catch (e) {
      streamError = e instanceof Error ? e.message : "Capture failed";
    }
  }

  onMount(() => {
    return () => stopStream();
  });
</script>

<div class="panel">
  <div class="panel-header">
    <span class="panel-title">CAMERA</span>
    <div class="controls">
      {#if streamActive}
        <button class="btn" on:click={stopStream}>STOP</button>
      {:else}
        <button class="btn" on:click={startStream}>STREAM</button>
      {/if}
      <button class="btn" on:click={captureFrame}>SNAP</button>
    </div>
  </div>

  <div class="panel-body">
    {#if streamError}
      <div class="error">{streamError}</div>
    {/if}
    <div class="feed-container">
      <img
        bind:this={imgRef}
        alt="Camera Feed"
        class="feed"
        on:error={() => { streamError = "Stream unavailable"; streamActive = false; }}
      />
      {#if !streamActive && !imgRef?.src}
        <div class="placeholder">NO SIGNAL</div>
      {/if}
    </div>
  </div>
</div>

<style>
  .panel {
    background: #111;
    border: 1px solid #2a3548;
    grid-column: 1 / -1;
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
    color: #f44;
  }

  .controls {
    display: flex;
    gap: 0.5rem;
  }

  .btn {
    background: #222;
    border: 1px solid #444;
    color: #eee;
    padding: 0.25rem 0.5rem;
    font-size: 0.6rem;
    font-family: monospace;
    cursor: pointer;
    letter-spacing: 0.05em;
  }

  .btn:hover {
    background: #333;
  }

  .btn:active {
    background: #444;
  }

  .panel-body {
    padding: 0.5rem;
  }

  .error {
    color: #f44;
    font-size: 0.7rem;
    margin-bottom: 0.5rem;
  }

  .feed-container {
    position: relative;
    background: #000;
    min-height: 200px;
    display: flex;
    align-items: center;
    justify-content: center;
  }

  .feed {
    max-width: 100%;
    max-height: 300px;
    display: block;
  }

  .placeholder {
    position: absolute;
    color: #444;
    font-size: 0.8rem;
    letter-spacing: 0.1em;
  }
</style>

