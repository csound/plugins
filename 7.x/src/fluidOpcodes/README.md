# FluidSynth opcodes

`fluidEngine` creates a new FluidSynth engine each time it is initialized.
For an engine that should survive orchestra recompiles, use
`fluidEngineReuse` with a stable, non-empty name:

```csound
iEngine fluidEngineReuse "main", 1, 1, 16, 256
iSoundFont fluidLoad "soundfont.sf2", iEngine
```

Calls to `fluidEngineReuse` with the same name and settings in one Csound
instance return the same engine. The settings must match the first call.
SoundFonts loaded into that engine are also reused by normalized file path, so
recompiling does not load another copy. Different names create independent
engines; the existing `fluidEngine` opcode continues to create a new engine
on each initialization.

On Windows, paths are normalized to absolute paths; symlink aliases may still
be treated as different paths.
