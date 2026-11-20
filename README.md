# PlexChem — AP Chemistry Ch. 12 (Chemical Kinetics) for TI-84 Plus CE

A single, zero-dependency, PlexMath-inspired native C application for the **TI-84 Plus CE** (including **OS 5.8.4.0058** with **arTIfiCE v2.1**), covering every calculation, graph, mechanism, and concept in AP Chemistry Chapter 12 (Chemical Kinetics).

## Zero-Crash Memory Architecture
- **1 Unified App (`PLEXCHEM`)** containing all 5 Kinetics modules + Summary Chart 12.6 (no switching/fragmenting RAM across 5 separate programs).
- **Zero External `.8xv` Libraries**: Uses bare-metal PL111 8bpp LCD VRAM double-buffering (`0xD40000` / `0xD52C00`) and direct keypad matrix polling (`0xF50010`) — no `graphx.8xv`, `fontlibc.8xv`, or `keypadc.8xv` required, and restores 16bpp TI-OS mode cleanly on exit.
- **Zero `malloc()` / Heap Allocation**: Entire app state fits in **2,184 bytes of static BSS RAM**; all 9 baked 2-bit anti-aliased proportional fonts (Inter + STIX Two / TeX Gyre Termes Math) and 6 minimalist 18×18 vector icons live in **Flash ROM (`const`, ~27 KB)**.
- **True Math Typesetting**: Real superscripts (`[A]²`, `10⁻⁵`, `s⁻¹`), subscripts (`[A]₀`, `t₁/₂`, `Eₐ`), stacked horizontal fractions, arrows (`→`, `⇌`), Greek `Δ`, and italic math variables — never `^` carets.

## Modules Included (Jump Anytime with `Y=` – `GRAPH`)
1. **Rates (`Y=`)** — Average & instantaneous rates (`±Δ[A]/aΔt`), 4-species stoichiometry rate converter, and exact `k` units for any order (integer, fractional, or negative).
2. **Initial Rates (`WINDOW`)** — Configurable table (`1–3` reactants, `2–8` rows via `+`/`-` and `MODE`), log-linear least-squares order solver (handles non-constant rows, fractional orders like `1.5`, and negative orders like `-1`), `k_mean`, rate predictor, and PlexMath-style step walkthrough (`APPS` to solve).
3. **Integrated Law (`ZOOM`)** — Configurable `(t, [A])` table (`3–10` rows) with side-by-side 3-plot linearization graphs (`[A]`, `ln[A]`, `1/[A]` vs `t` with `R²` auto-order winner), equation solver (`0th`/`1st`/`2nd`/`3rd` order, solve any unknown), and 1st/2nd/3rd half-life tracker.
4. **Mechanisms (`TRACE`)** — Elementary step analyzer with automatic species cancellation (Overall Equation), Intermediate vs. Catalyst classification, Molecularity, and algebraic intermediate elimination through fast-equilibrium steps (`⇌`), including denominator terms (`[O₂]⁻¹`) and fractional powers (`[Cl₂]^(3/2)`).
5. **Arrhenius (`GRAPH`)** — Two-point Arrhenius solver (`ln(k₂/k₁) = (Eₐ/R)(1/T₁ − 1/T₂)` for any unknown), live Potential Energy Profile (uncatalyzed vs. catalyzed curve, `Eₐ,rev`, `ΔE`), and catalyst speedup calculator (`k_cat / k_un`).
6. **Chart 12.6** — Full typeset Table 12.6 quick-reference card.

## Building & Installing on TI-84 Plus CE (OS 5.8.4.0058)
### Option A — Build with GitHub Actions (No Local Compiler Needed)
1. Upload this folder to a free GitHub repository; GitHub Actions automatically runs `.github/workflows/build.yml` and outputs `PLEXCHEM.8xp` under **Actions → Artifacts**.
2. Transfer `PLEXCHEM.8xp` to your calculator using **TI Connect CE**.
3. On OS **5.8.4.0058**, launch via **arTIfiCE v2.1** (do **not** update to OS 5.8.5 if using v2.1).

### Option B — Build Locally with CE C/C++ Toolchain & `app_tools`
1. Install [CE-Programming/toolchain](https://github.com/CE-Programming/toolchain) (and optionally [CE-Programming/app_tools](https://github.com/CE-Programming/app_tools) to package as a Flash App in the `APPS` menu).
2. *(Optional)* Drop official `STIXTwoMath-Regular.otf`, `STIXTwoText-Italic.otf`, `Inter-Regular.otf`, and `Inter-SemiBold.otf` into `fonts/` and run `make fonts`.
3. Run `make` (for `bin/PLEXCHEM.8xp`) or `make app` (for `app_tools` Flash App).
4. Run `make sim` on any PC/Mac/Linux to run all 18 Zumdahl Ch. 12 unit tests and regenerate 320×240 previews.
