import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import test from "node:test";
import { plugins as wasm32 } from "@csound/plugins/wasm32";
import { plugins as wasm64 } from "@csound/plugins/wasm64";
import { plugins as defaults } from "@csound/plugins";
import { runtime } from "./runtime.js";

test("the package default selects wasm32", () => assert.equal(defaults, wasm32));

for (const [target, plugins, Samples] of [
  ["wasm32", wasm32, Float32Array],
  ["wasm64", wasm64, Float64Array],
]) {
  const libcsound = ({ withPlugins = [] } = {}) => runtime(target, withPlugins);
  const binaries = await Promise.all(Object.values(plugins).map((url) => readFile(new URL(url))));
  const targetTest = (name, run) => test(`${target}: ${name}`, run);

  const document = (body) => `<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1
instr 1
${body}
endin
</CsInstruments>
<CsScore>
i 1 0 0.02
e
</CsScore>
</CsoundSynthesizer>`;

  /**
   * Load plugin bytes into Csound 7 and render the given instrument body.
   * Run prepare before compilation to set options or supply files.
   * Call check with the API, Csound instance, and peak sample magnitude before
   * destroying the instance.
   */
  async function run(body, check, withPlugins = binaries, prepare = () => {}) {
    const errors = [];
    const originalError = console.error;
    console.error = (...args) => errors.push(args.join(" "));
    let api;
    try {
      api = await libcsound({ withPlugins });
    } finally {
      console.error = originalError;
    }
    assert.deepEqual(errors, [], "every plugin must load without missing imports");
    assert.ok(api.csoundGetVersion() >= 7000 && api.csoundGetVersion() < 8000);
    assert.equal(api.csoundGetSizeOfMYFLT(), Samples.BYTES_PER_ELEMENT);
    assert.equal(api.csoundGetSizeOfCsDouble(), Samples.BYTES_PER_ELEMENT);
    const csound = api.csoundCreate();
    try {
      await prepare(api, csound);
      assert.equal(api.csoundCompileCSD(csound, document(body)), 0);
      assert.equal(api.csoundStart(csound), 0);
      let peak = 0;
      let blocks = 0;
      while (api.csoundPerformKsmps(csound) === 0) {
        const samples = new Samples(api.getMemory().buffer, api.csoundGetSpout(csound), 32);
        for (const sample of samples) {
          assert.ok(Number.isFinite(sample), "audio samples must be finite");
          peak = Math.max(peak, Math.abs(sample));
        }
        assert.ok(++blocks < 100, "score must finish");
      }
      assert.ok(blocks > 0);
      await check(api, csound, peak);
    } finally {
      api.csoundDestroy(csound);
    }
  }

  targetTest("all plugins have side-module exports and the target ABI", async () => {
    for (const bytes of binaries) {
      const module = await WebAssembly.compile(bytes);
      const imports = WebAssembly.Module.imports(module);
      const exports = WebAssembly.Module.exports(module);
      assert.ok(WebAssembly.Module.customSections(module, "dylink.0").length > 0);
      assert.ok(imports.some(({ kind, name }) => kind === "memory" && name === "memory"));
      assert.ok(imports.some(({ kind }) => kind === "table"));
      assert.ok(exports.some(({ name }) => name === "__wasm_call_ctors"));
      if (target === "wasm64") {
        const [section] = WebAssembly.Module.customSections(module, "csound.abi");
        assert.ok(section, "wasm64 needs native signature and pointer metadata");
        assert.equal(JSON.parse(new TextDecoder().decode(section)).pointerSize, 8);
      }
      assert.ok(
        exports.some(({ name }) => ["csoundModuleInit", "csound_opcode_init"].includes(name)),
      );
      assert.ok(!exports.some(({ name }) => name === "_start"));
    }
  });

  targetTest("chua produces finite, nonzero audio alongside the other plugins", async () => {
    await run(
      `
    a1, a2, a3 chuap 0.018, 0, 1e-7, 1/1500, -0.0008, -0.0005, 1, 1e-8, 0, 0, 0.1, 1e-5
    out a3 * 0.1
  `,
      (_, __, peak) => assert.ok(peak > 0),
    );
  });

  for (const instrument of ["STKMandolin", "STKModalBar", "STKBeeThree", "STKDrummer"]) {
    targetTest(
      `${instrument} renders with embedded waves alongside the other plugins`,
      async () => {
        await run(
          `
      a1 ${instrument} 220, 0.5
      out a1
    `,
          (_, __, peak) => assert.ok(peak > 0),
        );
      },
    );
  }

  targetTest("STK loads on its own without a raw-wave path or extra files", async () => {
    const stk = await readFile(new URL(plugins.stk));
    await run("a1 STKMandolin 220, 0.5\nout a1", (_, __, peak) => assert.ok(peak > 0), [stk]);
  });

  targetTest("RAWWAVE_PATH uses caller-supplied samples without overwriting them", async () => {
    const silentWave = new Uint8Array(512);
    await run(
      "a1 STKModalBar 220, 0.5\nout a1",
      async (api, csound, peak) => {
        assert.equal(peak, 0);
        assert.deepEqual(await api.readFile(csound, "/custom-waves/marmstk1.raw"), silentWave);
      },
      binaries,
      async (api, csound) => {
        await api.writeFile(csound, "/custom-waves/marmstk1.raw", silentWave);
        assert.equal(api.csoundSetOption(csound, "--env:RAWWAVE_PATH=/custom-waves"), 0);
      },
    );
  });

  targetTest(
    "STK reports a missing sample through Csound instead of throwing out of WASM",
    async () => {
      const api = await libcsound({ withPlugins: binaries });
      const csound = api.csoundCreate();
      const messages = [];
      const originalLog = console.log;
      console.log = (...args) => messages.push(args.join(" "));
      try {
        assert.equal(api.csoundSetOption(csound, "--env:RAWWAVE_PATH=/missing-stk-waves"), 0);
        assert.equal(api.csoundCompileCSD(csound, document("a1 STKMandolin 220, 0.5\nout a1")), 0);
        assert.equal(api.csoundStart(csound), 0);
        let result = 0;
        for (let block = 0; block < 100 && result === 0; block++) {
          result = api.csoundPerformKsmps(csound);
        }
        assert.notEqual(result, 0, "the failed score must finish");
        assert.ok(
          messages.some((message) => message.includes("STK: FileRead::open")),
          messages.join("\n"),
        );
      } finally {
        api.csoundDestroy(csound);
        console.log = originalLog;
      }
    },
  );

  targetTest("linear algebra computes a vector norm", async () => {
    await run(
      `
    iv la_i_vr_create 2
    iv la_i_vr_set 0, 3
    iv la_i_vr_set 1, 4
    inorm la_i_norm_euclid_vr iv
    chnset inorm, "norm"
  `,
      (api, csound) => assert.equal(api.csoundGetControlChannel(csound, "norm"), 5),
    );
  });

  targetTest(
    "linear algebra prints vectors and computes eigenvalues at the engine precision",
    async () => {
      await run(
        `
    im la_i_mr_create 2, 2, 4
    iv la_i_vr_create 2
    iv la_i_qr_eigen_mr im, 0.00001
    la_i_print_vr iv
    ic la_i_vc_create 2
    la_i_print_vc ic
    inorm la_i_norm_euclid_vr iv
    chnset inorm, "norm"
  `,
        (api, csound) =>
          assert.ok(Math.abs(api.csoundGetControlChannel(csound, "norm") - Math.sqrt(32)) < 0.0001),
      );
    },
  );

  targetTest("image writes and reads a PNG through the WASI filesystem", async () => {
    await run(
      `
    ih imagecreate 2, 3
    imagesetpixel ih, 0, 0, 1, 0, 0
    imagesave ih, "roundtrip.png"
    iload imageload "roundtrip.png"
    iw, ihgt imagesize iload
    kred, kgreen, kblue imagegetpixel iload, 0, 0
    chnset iw, "width"
    chnset ihgt, "height"
    chnset kred, "red"
    chnset kgreen, "green"
  `,
      async (api, csound) => {
        assert.equal(api.csoundGetControlChannel(csound, "width"), 2);
        assert.equal(api.csoundGetControlChannel(csound, "height"), 3);
        assert.equal(api.csoundGetControlChannel(csound, "red"), 1);
        assert.equal(api.csoundGetControlChannel(csound, "green"), 0);
        const bytes = await api.readFile(csound, "/project/roundtrip.png");
        assert.deepEqual([...bytes.slice(0, 8)], [137, 80, 78, 71, 13, 10, 26, 10]);
      },
      binaries,
      async (api, csound) => {
        await api.mkdir(csound, "/project");
        assert.equal(api.chdir(csound, "/project"), 0);
      },
    );
  });

  targetTest("rejects a plugin built for the other pointer width", async () => {
    const other = target === "wasm64" ? wasm32 : wasm64;
    const errors = [];
    const originalError = console.error;
    console.error = (...args) => errors.push(args.join(" "));
    try {
      const api = await libcsound({
        withPlugins: [await readFile(new URL(other.chua))],
      });
      const csound = api.csoundCreate();
      api.csoundDestroy(csound);
    } finally {
      console.error = originalError;
    }
    assert.ok(
      errors.some((error) => error.includes("Plugin pointer width does not match")),
      errors.join("\n"),
    );
  });
}
