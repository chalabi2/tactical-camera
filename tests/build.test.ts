import { describe, expect, test, beforeAll } from "bun:test";
import { compile, preprocess } from "svelte/compiler";

describe("Build Process", () => {
  test("Svelte compiler is available", () => {
    expect(typeof compile).toBe("function");
    expect(typeof preprocess).toBe("function");
  });

  test("Can compile simple Svelte component", () => {
    const source = `
      <script>
        let count = 0;
      </script>
      <button on:click={() => count++}>{count}</button>
    `;

    const result = compile(source, {
      filename: "test.svelte",
      generate: "dom",
      css: "injected",
      dev: false,
    });

    expect(result.js).toBeDefined();
    expect(result.js.code).toContain("function");
    expect(result.warnings).toEqual([]);
  });

  test("Can preprocess TypeScript in Svelte", async () => {
    const source = `
      <script lang="ts">
        interface User {
          name: string;
          age: number;
        }
        let user: User = { name: "Test", age: 25 };
      </script>
      <p>{user.name}</p>
    `;

    const tsPreprocessor = {
      script: async ({ content, attributes }: { content: string; attributes: Record<string, string | boolean> }) => {
        if (attributes.lang !== "ts") return { code: content };
        const transpiler = new Bun.Transpiler({ loader: "ts", target: "browser" });
        return { code: transpiler.transformSync(content) };
      },
    };

    const preprocessed = await preprocess(source, tsPreprocessor, { filename: "test.svelte" });

    // TypeScript interface should be stripped
    expect(preprocessed.code).not.toContain("interface User");

    // Should compile without errors
    const result = compile(preprocessed.code, {
      filename: "test.svelte",
      generate: "dom",
      css: "injected",
      dev: false,
    });

    expect(result.js.code).toBeDefined();
  });

  test("Bun Transpiler handles TypeScript", () => {
    const ts = `
      interface Config {
        port: number;
        host: string;
      }
      const config: Config = { port: 3000, host: "localhost" };
      export default config;
    `;

    const transpiler = new Bun.Transpiler({ loader: "ts", target: "browser" });
    const js = transpiler.transformSync(ts);

    expect(js).not.toContain("interface Config");
    expect(js).not.toContain(": Config");
    expect(js).toContain("const config");
    expect(js).toContain("export default");
  });
});

describe("Build Outputs", () => {
  beforeAll(async () => {
    // Ensure build has run
    const proc = Bun.spawn(["bun", "run", "build"], {
      cwd: import.meta.dir + "/..",
      stdout: "pipe",
      stderr: "pipe",
    });
    await proc.exited;
  });

  test("dist/index.html exists", async () => {
    const file = Bun.file("dist/index.html");
    expect(await file.exists()).toBe(true);
  });

  test("dist/assets/main.js exists", async () => {
    const file = Bun.file("dist/assets/main.js");
    expect(await file.exists()).toBe(true);
  });

  test("build/index.js exists", async () => {
    const file = Bun.file("build/index.js");
    expect(await file.exists()).toBe(true);
  });

  test("Frontend bundle is reasonably sized (< 50KB)", async () => {
    const file = Bun.file("dist/assets/main.js");
    const size = file.size;
    expect(size).toBeLessThan(50 * 1024);
  });

  test("Backend bundle is reasonably sized (< 10KB)", async () => {
    const file = Bun.file("build/index.js");
    const size = file.size;
    expect(size).toBeLessThan(10 * 1024);
  });

  test("index.html references correct JS path", async () => {
    const html = await Bun.file("dist/index.html").text();
    expect(html).toContain("/assets/main.js");
  });
});

