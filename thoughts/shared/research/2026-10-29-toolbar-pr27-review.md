---
date: 2026-10-29
topic: PR 27 tool bar icons - Bruno's A500 measurements (slow open, red->magenta, soft outlines)
status: research, no edits; unverified on hardware
---
# Evidence base
Worktree dctelnet-toolbar, head 5be62b1. Base 5942e99 = merge-base. 5be62b1 only changes tests (tests/test_iconpens.c, tests/Makefile), so 11.35 s -> 3.30 s is f18a074 (identity skip + 16-pixel remap); 3.30 s is the code in src/ today.
SCIcons .info files are the same icons as the base (1574 -> 1651 bytes: only the DCTELNET_PALETTE tool type added).

# A. Where the remaining ~1.8 s go
Open path (src/guis.c OpenToolBarWindow ~2200-2230), per icon i (7 icons):
1. GetDiskObjectNew: same as base, not new cost.
2. RemapToolIcon (guis.c:2130): FindToolType + strlen + IconPens_Parse (iconpens.c:12-39, 48 hex chars): ~1 ms.
3. ToolPaletteFor (guis.c:2099-2127), once per distinct value (once for SCIcons): 7 x ObtainNearestPen (utils.c:316-327) = ObtainBestPen + on -1 FindColor, each a pass over 16 entries: a few ms total. Pen 0 = dri_Pens[BACKGROUNDPEN], no search.
4. IconPens_IsIdentity (guis.c:2151, iconpens.c:92): trivial.
5. RemapToolImage -> IconPens_Remap (iconpens.c:109-164) ONLY when the pen map is not the identity. Per 16 pixels: 8 pens x (up to 3 plane AND + up to 4 plane OR, loop-carried, no unrolling) ~ 2-3k cycles. A 3-plane icon of ~48 words/plane x 14 images (gadget + select) = ~2M cycles = ~0.3 s at 7 MHz in ideal code, 0.7-1.5 s with SAS/C-style C and 68000 shifts. This is the only step big enough to explain 1.8 s.
6. Pen reservation: DCTelnet.c:4223-4243, 16 ObtainPen at SCREEN OPEN (not tool bar open), ~ms. Screen work: none new in the tool bar path (OpenWindow same as base).
Does the skip fire? Only if tp->pen[k]==k for k<8 and depth equal (iconpens.c:96-100). Pen k comes from FindColor/ObtainBestPen of SCIcons colour k (000000,DDDDDD,00DD00,DDDD00,0000DD,DD00DD,00DDDD,DD0000) against the SCREEN's palette. The screen palette is prefs.DeviceColors (DCTelnet.c:4166-4184, LoadRGB via SA_Colors), NOT the Workbench palette. With default DeviceColors (palette.c:27-40) it is identity (verified by tests/test_iconpens.c:210-228, which is host only). BACKGROUNDPEN is 0 in devicePens/defaultPens (prefs.c:71,79), so pen 0 is fine. => If Bruno still pays 1.8 s, his own-screen pens 0-7 differ from the default terminal palette (custom DeviceColors, or ibmcon deep-pen layout), so the skip does NOT fire and all 14 images are remapped. Not provable from source; discriminator: IconPens_RemapCount (iconpens.c:90) after open, or his prefs file. "Turning off: no difference" is consistent (remap is only in open path).
Open: nothing in the diff other than step 5 can cost seconds; if RemapCount==0 on his machine the 1.8 s is outside this PR's code (e.g. disk/CPU variance) - unverified.

# B. Red -> magenta
Facts:
- WinUAE test was OS 2.04 = graphics.library V37. Both new V39 branches are gated on lib_Version >= 39 (guis.c:2118, DCTelnet.c:4228). On 2.04 the code takes IconPens_Nearest over 4 fixed WB colours (guis.c:2071,2121), luma-weighted. The A500 with WB 3.2 (V47) takes ObtainBestPen/FindColor. So Bruno ran a code path the WinUAE test never executed. This is the likeliest reason it was not seen there.
- Red is SCIcons pen 7 (111b), magenta pen 5 (101b). Red is only reachable as magenta if map[7]==5 (nearest colour search landed there) or plane 1 is dropped in a remap.
- Pen search (utils.c:316-327): ObtainBestPen(PRECISION_IMAGE) then FindColor(cm,..,-1) = plain RGB distance, no luma weighting (the luma fix, iconpens.c:41-51, is only on the V<39 branch). With a palette whose pen 7 is not near DD0000 (custom DeviceColors / palette) the nearest to DD0000 can be the magenta slot. SA_SharePens plus the reservation (DCTelnet.c:4235-4242) does not change the colour map, only blocks recolouring.
- Reservation shifting colours: ObtainPen(PEN_EXCLUSIVE) result is ignored (DCTelnet.c:4237-4242). Pens Intuition already allocated from SA_Pens (devicePens uses pens 0,1,4,5,6) cannot be taken exclusive (call fails), so those stay shareable and ObtainBestPen can return and share them; pens that did go exclusive are invisible to ObtainBestPen, so the match falls to FindColor over the whole map. Both outcomes pick pens by colour distance, not by index - this is the mechanism for a neighbour pen. Not an off-by-one in plane order: Remap plane order (iconpens.c:122-163) is consistent with the tests.
Most likely: nearest-colour match against a non-default palette on the V39+ path (never tested on 2.04). Unverified: needs his DeviceColors and RemapCount.
Fix (root cause): on the own screen the SCIcons are authored per terminal pen index, and the terminal palette IS the screen palette. Draw them as authored (identity, no ObtainBestPen/FindColor/remap) when STATE_IS(APP_FULLSCREEN); i.e. in RemapToolIcon return early for the own screen. That removes A and B together; icon colours then follow the user's terminal palette exactly like the text does. Keep matching only for the Workbench.

# C. Soft outlines on Workbench with custom palette
ToolBar icons carry AAAAAA,000000,FFFFFF,6688BB,717171,383838,99B0D2,CCD7E8 (antialias greys). On V39+ each goes ObtainBestPen(PRECISION_IMAGE) -> may ALLOCATE spare pens (changing the shared WB palette use) or fall to plain-RGB FindColor. A custom WB palette without 383838/717171 greys gets a lighter/coloured neighbour for the dark outline: soft. The comment at iconpens.c:41-43 says plain RGB picks the blue for dark grey edges; that fix is not used on V39+.
Fix: map by role first, not by distance: 000000 -> dri_Pens[SHADOWPEN], FFFFFF -> SHINEPEN, AAAAAA -> BACKGROUNDPEN, 6688BB -> FILLPEN; match only the antialias colours, using IconPens_Nearest (luma-weighted) over the screen's actual palette read with GetRGB32, without allocating pens (Workbench palette is shared). Outline = darkest available pen.

# D. Needs real hardware
- Count of IconPens_RemapCount and time of OpenToolBarWindow on A500 3.2.3 with default vs his DeviceColors (confirms A).
- Red icon on 3.2.3 OCS 16-colour own screen after the identity-by-design change (B); also WB 3.2.3 custom palette outline appearance (C). WinUAE must run WB 3.x (V39+) to cover the new branches; 2.04 does not.
