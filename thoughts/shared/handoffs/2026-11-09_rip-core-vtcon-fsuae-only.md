---
date: 2026-11-09
topic: RIP engine core decided for vtcon; Amiberry dropped for fs-uae
status: idle (waits on UP-Term session)
---

# Handoff: DCTelnet after RIP-0 (2026-11-09)

## State
- RIP-0 done. Corrections folded into thoughts/spot/plans/2026-11-08-ripscrip.md (D1-D17).
- Owner decision D17: RIP engine core (parser, drawing, mouse fields) goes into vtcon, beside kitty and sixel. The UP-Term session builds it. DCTelnet RIP-1 does not start here on its own; it waits for a vtcon submodule bump. Decision copied to ~/Code/UP-Term/thoughts/spot/todos.md (GFX-PROTO section, "do not ask again").
- Owner: "dont use amiberry its buggy use fs-uae". Amiberry 8.2.0 boot freeze closed, not investigated. All rig checks run on fs-uae. Memory rig-emulator-fsuae replaces the old Amiberry rule.
- KG-11 closed on fs-uae (owner eye check PASS at 256 colours after 16 -> 256 switch). A1200 check only when the owner asks.

## Open
- UP-Term session: GFX-PROTO plan and RIP engine in vtcon (its start prompt: ~/Code/UP-Term/thoughts/spot/next-session.md, not yet used).
- After vtcon has the RIP engine: bump the vtcon submodule in DCTelnet, wire RIP into the DCTelnet front end, 3 upstream PRs core/feature/extras off upstream main (D10-D14).
- RIP-0 leftovers (for the vtcon work): 5 STILL UNKNOWN items in thoughts/shared/research/2026-11-09-ripscrip-spec-check.md (EGA default palette, $VAR$ list, templates/pick lists, .ICN layout, font behaviour); SyncTERM-vs-1.54 diff not done.

## Gotchas
- Unmerged vtcon work from another session (s48-amiberry-default) makes Amiberry the rig default. If it lands, rig briefs must name fs-uae explicitly.
- SyncTERM is GPL v2+: behaviour reference only, copy no code (D15). No board accounts: Serion + hand-written fixtures (D16).
- Ledger: thoughts/spot/todos.md sections "2026-11-09 RIPscrip" and "2026-11-09 Graphics protocols".
