TODO (big stuff)
----------------
---
- Remove `setjmp` / `longjmp`. Not supported on all architectures, leaves stale
  memory, crashes on Win32 if the erroring function is within a message handler
  and other uglyness.

- ncurses based Linux server console (or at the very least a usable line editor)

- TCP support for going crazy with entities (possible use also for MVD?).

- FPS independent physics (ha!)

- Integrated master server browser.

TODO (general stuff)
--------------------
---
- Event / scripting system ala AprQ2 / mIRC "ON TEXT "\*match started\*" record...

- Regex or wildcard support for server stuff - cvarbans, cmdbans, etc.

- Multimoves support ala Q2PRO protocol 36, although I'm concerned this isn't a
  good approach due to the negative results of running too many msecs of movement
  at once. Maybe multimoves up to an msec limit before sending a packet rather
  than use a fixed rate.

- Fix player updates (if they're even broken and not a result of low packet rate
  clients screwing things up). Major perceived latency reduction if this works,
  lots of compatibility issues to work out though (eg lifts and other entities
  that move players).

- Fix OpenAL spatialization and looping sound sync.

- Rework filesystem code to support per-platform API enhancements and PKZ.
