/**
 * Build Script for Tactical Console
 * 
 * Compiles both frontend (Svelte) and backend (Bun server) into
 * deployable bundles. Uses a custom Svelte plugin compatible with Bun.
 */

import { compile, preprocess } from "svelte/compiler";
import type { BunPlugin } from "bun";

const args = process.argv.slice(2);
const buildFrontend = args.includes("frontend") || args.length === 0;
const buildBackend = args.includes("backend") || args.length === 0;
const copyStatic = args.includes("static") || args.length === 0;

// TypeScript preprocessor for Svelte
const tsPreprocessor = {
  script: async ({ content, attributes }: { content: string; attributes: Record<string, string | boolean> }) => {
    if (attributes.lang !== "ts") {
      return { code: content };
    }

    // Use Bun's transpiler to convert TypeScript to JavaScript
    const transpiler = new Bun.Transpiler({
      loader: "ts",
      target: "browser",
    });

    const code = transpiler.transformSync(content);
    return { code };
  },
};

// Custom Svelte plugin for Bun
const sveltePlugin: BunPlugin = {
  name: "svelte",
  setup(build) {
    build.onLoad({ filter: /\.svelte$/ }, async (args) => {
      const source = await Bun.file(args.path).text();

      // Preprocess TypeScript in script tags
      const preprocessed = await preprocess(source, tsPreprocessor, {
        filename: args.path,
      });

      const result = compile(preprocessed.code, {
        filename: args.path,
        generate: "dom",
        css: "injected",
        dev: false,
      });

      // Log any warnings
      for (const warning of result.warnings) {
        console.warn(`[svelte] ${warning.filename}: ${warning.message}`);
      }

      return {
        contents: result.js.code,
        loader: "js",
      };
    });
  },
};

async function main() {
  // Frontend build - Svelte app
  if (buildFrontend) {
    console.log("[build] Compiling frontend...");
    
    const result = await Bun.build({
      entrypoints: ["./src/ui/main.ts"],
      outdir: "./dist/assets",
      minify: true,
      plugins: [sveltePlugin],
    });

    if (!result.success) {
      console.error("[build] Frontend build failed:");
      for (const log of result.logs) {
        console.error(log);
      }
      process.exit(1);
    }
    
    console.log("[build] Frontend compiled to dist/assets/");
  }

  // Copy static files
  if (copyStatic) {
    console.log("[build] Copying static files...");
    
    const indexHtml = await Bun.file("./static/index.html").text();
    await Bun.write("./dist/index.html", indexHtml);
    
    console.log("[build] Static files copied to dist/");
  }

  // Backend build - Bun server
  if (buildBackend) {
    console.log("[build] Compiling backend...");
    
    const result = await Bun.build({
      entrypoints: ["./src/server/index.ts"],
      outdir: "./build",
      target: "bun",
      minify: true,
    });

    if (!result.success) {
      console.error("[build] Backend build failed:");
      for (const log of result.logs) {
        console.error(log);
      }
      process.exit(1);
    }
    
    console.log("[build] Backend compiled to build/");
  }

  console.log("[build] Done!");
}

main().catch((err) => {
  console.error("[build] Error:", err);
  process.exit(1);
});
