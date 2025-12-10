/**
 * Tactical Console - Bun HTTP Server
 * 
 * Self-contained server for embedded/edge deployment.
 * Serves both the Svelte frontend (static assets) and REST API endpoints.
 * 
 * In production on ESP32-S3:
 * - This is the only server process running
 * - All assets served locally (no CDN, no cloud)
 * - API endpoints return sensor data instead of mock values
 */

import { handleStatus } from "./routes/status";
import { handleTelemetry } from "./routes/telemetry";

const PORT = Number(process.env.PORT) || 3000;

// Track server start time for uptime calculation
export const serverStartTime = Date.now();

const server = Bun.serve({
  port: PORT,
  
  async fetch(req: Request): Promise<Response> {
    const url = new URL(req.url);
    const pathname = url.pathname;

    // API Routes - JSON responses for frontend consumption
    if (pathname === "/api/status") {
      return handleStatus(req);
    }
    
    if (pathname === "/api/telemetry") {
      return handleTelemetry(req);
    }

    // Static file serving for the Svelte frontend
    // In embedded mode, all UI assets come from the bundled dist/ folder
    try {
      let filePath: string;
      
      if (pathname === "/" || pathname === "/index.html") {
        filePath = "./dist/index.html";
      } else if (pathname.startsWith("/assets/")) {
        // Serve bundled JS/CSS from dist/assets/
        filePath = `./dist${pathname}`;
      } else {
        // Fallback to dist folder for any other static files
        filePath = `./dist${pathname}`;
      }

      const file = Bun.file(filePath);
      const exists = await file.exists();
      
      if (exists) {
        // Determine content type based on file extension
        const contentType = getContentType(filePath);
        return new Response(file, {
          headers: { "Content-Type": contentType }
        });
      }
    } catch (e) {
      // File not found or read error - fall through to 404
    }

    return new Response("Not Found", { status: 404 });
  },
});

function getContentType(path: string): string {
  if (path.endsWith(".html")) return "text/html; charset=utf-8";
  if (path.endsWith(".js")) return "application/javascript; charset=utf-8";
  if (path.endsWith(".css")) return "text/css; charset=utf-8";
  if (path.endsWith(".json")) return "application/json; charset=utf-8";
  if (path.endsWith(".svg")) return "image/svg+xml";
  if (path.endsWith(".png")) return "image/png";
  if (path.endsWith(".jpg") || path.endsWith(".jpeg")) return "image/jpeg";
  return "application/octet-stream";
}

console.log(`[tactical-console] Server running at http://localhost:${PORT}`);
console.log(`[tactical-console] API endpoints:`);
console.log(`  GET /api/status`);
console.log(`  GET /api/telemetry`);

