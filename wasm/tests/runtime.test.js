import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import test from "node:test";
import { plugins } from "../../index.js";

// The in-process browser API needs these names, but no audio device or DOM.
globalThis.self = globalThis;
globalThis.window = globalThis;
const { libcsound } = await import(process.env.CSOUND_BROWSER_URL || "@csound/browser/dist/csound.js");

const binaries = await Promise.all(Object.values(plugins).map((url) => readFile(new URL(url))));

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
  assert.equal(api.csoundGetSizeOfMYFLT(), 8);
  const csound = api.csoundCreate();
  try {
    await prepare(api, csound);
    assert.equal(api.csoundCompileCSD(csound, document(body)), 0);
    assert.equal(api.csoundStart(csound), 0);
    let peak = 0;
    let blocks = 0;
    while (api.csoundPerformKsmps(csound) === 0) {
      const samples = new Float64Array(api.getMemory().buffer, api.csoundGetSpout(csound), 32);
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

test("all plugins are WASI side modules with the shared-memory TLS exports", async () => {
  for (const bytes of binaries) {
    const module = await WebAssembly.compile(bytes);
    const imports = WebAssembly.Module.imports(module);
    const exports = WebAssembly.Module.exports(module);
    assert.ok(WebAssembly.Module.customSections(module, "dylink.0").length > 0);
    assert.ok(imports.some(({ kind, name }) => kind === "memory" && name === "memory"));
    assert.ok(imports.some(({ kind }) => kind === "table"));
    assert.ok(exports.some(({ name }) => name === "__wasm_call_ctors"));
    for (const [name, kind] of [
      ["__wasm_init_tls", "function"],
      ["__tls_size", "global"],
      ["__tls_align", "global"],
    ]) {
      assert.ok(
        exports.some((entry) => entry.name === name && entry.kind === kind),
        `${name} must be exported as a ${kind}`,
      );
    }
    assert.ok(exports.some(({ name }) => ["csoundModuleInit", "csound_opcode_init"].includes(name)));
    assert.ok(!exports.some(({ name }) => name === "_start"));
  }
});

test("chua produces finite, nonzero audio alongside the other plugins", async () => {
  await run(`
    a1, a2, a3 chuap 0.018, 0, 1e-7, 1/1500, -0.0008, -0.0005, 1, 1e-8, 0, 0, 0.1, 1e-5
    out a3 * 0.1
  `, (_, __, peak) => assert.ok(peak > 0));
});

for (const instrument of ["STKMandolin", "STKModalBar", "STKBeeThree", "STKDrummer"]) {
  test(`${instrument} renders with embedded waves alongside the other plugins`, async () => {
    await run(`
      a1 ${instrument} 220, 0.5
      out a1
    `, (_, __, peak) => assert.ok(peak > 0));
  });
}

test("STK loads on its own without a raw-wave path or extra files", async () => {
  const stk = await readFile(new URL(plugins.stk));
  await run("a1 STKMandolin 220, 0.5\nout a1", (_, __, peak) => assert.ok(peak > 0), [stk]);
});

test("RAWWAVE_PATH uses caller-supplied samples without overwriting them", async () => {
  const silentWave = new Uint8Array(512);
  await run("a1 STKModalBar 220, 0.5\nout a1", async (api, csound, peak) => {
    assert.equal(peak, 0);
    assert.deepEqual(await api.readFile(csound, "/custom-waves/marmstk1.raw"), silentWave);
  }, binaries, async (api, csound) => {
    await api.writeFile(csound, "/custom-waves/marmstk1.raw", silentWave);
    assert.equal(api.csoundSetOption(csound, "--env:RAWWAVE_PATH=/custom-waves"), 0);
  });
});

test("STK reports a missing sample through Csound instead of throwing out of WASM", async () => {
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
    assert.ok(messages.some((message) => message.includes("STK: FileRead::open")), messages.join("\n"));
  } finally {
    api.csoundDestroy(csound);
    console.log = originalLog;
  }
});

test("linear algebra computes a vector norm", async () => {
  await run(`
    iv la_i_vr_create 2
    iv la_i_vr_set 0, 3
    iv la_i_vr_set 1, 4
    inorm la_i_norm_euclid_vr iv
    chnset inorm, "norm"
  `, (api, csound) => assert.equal(api.csoundGetControlChannel(csound, "norm"), 5));
});

test("image writes and reads a PNG through the WASI filesystem", async () => {
  await run(`
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
  `, async (api, csound) => {
    assert.equal(api.csoundGetControlChannel(csound, "width"), 2);
    assert.equal(api.csoundGetControlChannel(csound, "height"), 3);
    assert.equal(api.csoundGetControlChannel(csound, "red"), 1);
    assert.equal(api.csoundGetControlChannel(csound, "green"), 0);
    const bytes = await api.readFile(csound, "roundtrip.png");
    assert.deepEqual([...bytes.slice(0, 8)], [137, 80, 78, 71, 13, 10, 26, 10]);
  });
});
