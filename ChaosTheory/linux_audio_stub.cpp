// ChaosTheory Linux port -- Phase 1 (silent).
//
// The mvx softsynth (mvxPlayerLib.lib) is Windows-only and not yet wired up, so
// the intro runs with music off (wall-clock timing). The engine still references
// the `music` / `music_size` data symbols, so provide empty placeholders here.
// Phase 2 will replace these with the real music.mvm blob + a PulseAudio-backed
// mvx driver.
#ifdef CONSPIRACY_LINUX
extern "C" char music[1] = { 0 };
extern "C" int  music_size = 0;
#endif
