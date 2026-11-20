#ifndef PLEXCHEM_THEORY_DATA_H
#define PLEXCHEM_THEORY_DATA_H

#define NUM_THEORIES 12

typedef struct {
    const char *title;
    const char *section;
    const char *def_lines[6];
    const char *how_lines[6];
    const char *ex_lines[6];
} TheoryTopic;

static const TheoryTopic THEORY_DB[NUM_THEORIES] = {
    {
        "Reaction Rates & Stoich", "12.1",
        {
            "Reaction rate = change in conc. (mol/L) per",
            "unit time: Rate = -d[A]/(a*dt) = +d[C]/(c*dt).",
            "Overall rate is ALWAYS positive. Reactants use",
            "a minus sign because [reactant] decreases.",
            "Instantaneous rate = slope of tangent line at",
            "a specific time t (initial rate is at t = 0)."
        },
        {
            "1) Divide each species rate by its coeff:",
            "   -(1/a)d[A]/dt = (1/c)d[C]/dt.",
            "2) Why rate slows over time: [reactants] drop",
            "   (fewer collisions) & reverse rxn speeds up.",
            "3) Measure rate via: color/light absorbance",
            "   (Beer's Law), gas pressure (dP/dt), or mass."
        },
        {
            "Ex (Notes 12.1): 2 NO2 -> 2 NO + O2.",
            "If [NO2] drops 0.0100 -> 0.0079 M in 50 s:",
            "d[NO2]/dt = (0.0079 - 0.0100)/50 = -4.2e-5.",
            "Overall Rate = -(-4.2e-5)/2 = 2.1e-5 M/s.",
            "NO forms at +4.2e-5 M/s (same magnitude).",
            "O2 forms at +2.1e-5 M/s (half of NO rate)."
        }
    },
    {
        "Rate Laws & Reaction Order", "12.2-12.3",
        {
            "Differential rate law: Rate = k[A]^n[B]^m.",
            "k = rate constant (depends ONLY on T & Ea,",
            "NOT on concentration!). n, m = reactant orders.",
            "Overall order = n + m. Orders MUST be found",
            "experimentally - NEVER from balanced coeffs",
            "(unless a step is explicitly elementary)."
        },
        {
            "Method of Initial Rates:",
            "1) Pick 2 trials where ONLY [A] changes.",
            "2) Rate2/Rate1 = ([A]2/[A]1)^n -> solve n.",
            "   2x [A] -> 1x (n=0), 2x (n=1), 4x (n=2).",
            "3) Negative order (-1): doubling [X] HALVES",
            "   rate (inhibitor). Units of k: M^(1-n)/time."
        },
        {
            "Ex (Notes 12.3): [A] doubles (B const) ->",
            "Rate quadruples (4 = 2^n -> n = 2).",
            "[B] triples (A const) -> Rate triples (m = 1).",
            "Rate = k[A]^2[B], overall order = 2 + 1 = 3.",
            "Plug Trial 1: k = 1.53e-4 / (0.10^2 * 0.10)",
            "k = 0.153 L^2/(mol^2*s). Use Tab 2 to solve!"
        }
    },
    {
        "Integrated Laws & Proof Plots", "12.4",
        {
            "Relates [A] directly to time t by linearizing",
            "y = mx + b so the straight-line plot proves",
            "the reaction order and gives k from slope:",
            "0th: [A] = -kt + [A]0    (slope = -k)",
            "1st: ln[A] = -kt + ln[A]0 (slope = -k)",
            "2nd: 1/[A] = kt + 1/[A]0  (slope = +k)"
        },
        {
            "FRQ Proof-Plot Strategy:",
            "1) Plot [A] vs t, ln[A] vs t, and 1/[A] vs t.",
            "2) Whichever plot is LINEAR (R^2 closest to 1)",
            "   gives the order (0th, 1st, or 2nd).",
            "3) For gases, P is proportional to [A], so",
            "   partial pressure P works directly in 1st ord."
        },
        {
            "Ex (Zumdahl Q39): H2O2 decomposes; ln[H2O2]",
            "vs t is linear with slope = -8.3e-4 s^-1.",
            "-> 1st order, k = +8.3e-4 s^-1.",
            "Find [H2O2] at t = 4000 s ([A]0 = 1.00 M):",
            "ln([A]/1.00) = -(8.3e-4)(4000) = -3.32.",
            "[A] = e^(-3.32) = 0.036 M. Use Tab 3!"
        }
    },
    {
        "Half-Life & Decay Patterns", "12.4",
        {
            "Half-life (t1/2) = time for [A] to drop to",
            "half its value ([A]t = [A]0 / 2).",
            "0th order: t1/2 = [A]0 / (2k)  (depends on [A]0)",
            "1st order: t1/2 = 0.693 / k    (INDEPENDENT!)",
            "2nd order: t1/2 = 1 / (k[A]0)  (inverse [A]0)",
            "All radioactive decay follows 1st-order law."
        },
        {
            "FRQ Shortcut (Successive Half-Lives):",
            "- Constant t1/2 (10m, 10m, 10m) -> 1st ORDER.",
            "- Halving t1/2  (10m,  5m, 2.5m) -> 0th ORDER.",
            "- Doubling t1/2 (10m, 20m, 40m) -> 2nd ORDER.",
            "For % reacted (e.g. 87.5% reacted = 12.5%",
            "left = 1/8 = 3 half-lives for 1st order!)."
        },
        {
            "Ex (Notes 12.4 I-131): k = 0.086 day^-1.",
            "t1/2 = 0.693 / 0.086 = 8.06 days.",
            "Time for 99% to decay (1% remains -> 0.01):",
            "ln(100 / 1) = (0.086) * t -> t = 53.5 days.",
            "After 2.00 days from 0.100 M:",
            "[I]t = 0.100 * e^(-0.086*2.00) = 0.084 M."
        }
    },
    {
        "Pseudo-Order Conditions", "12.4",
        {
            "For multi-reactant reactions (Rate=k[A]^n[B]^m)",
            "making [B]0 >> [A]0 (flooding) keeps [B]",
            "effectively constant throughout the reaction.",
            "Rate simplifies to: Rate = k'[A]^n, where",
            "k' = k[B]0^m is the pseudo rate constant.",
            "Lets you use single-reactant integrated plots!"
        },
        {
            "How to Solve on AP FRQs:",
            "1) Verify [B]0 >> [A]0 (at least 10x-100x).",
            "2) Find order n in [A] and k' from linear plot",
            "   (e.g. slope of ln[A] vs t = -k').",
            "3) Extract true k: k = k' / ([B]0^m).",
            "4) If 2 runs use different [B]0, compare k'2/k'1."
        },
        {
            "Ex (Zumdahl Q60): Rate = k[A][B]^2, [B]0=3.0M",
            "in large excess. From 1st-order ln[A] vs t,",
            "k' = 0.12 s^-1.",
            "Since k' = k[B]0^2:",
            "k = 0.12 / (3.0)^2 = 0.0133 L^2/(mol^2*s).",
            "Use Tab 3 -> Pseudo softkey to compute!"
        }
    },
    {
        "Mechanisms & Molecularity", "12.6",
        {
            "A reaction mechanism is the sequence of",
            "elementary steps summing to the overall rxn.",
            "Molecularity = # of colliding species in ONE",
            "elementary step: Unimolecular (1), Bimolecular",
            "(2), Termolecular (3 - very rare, low prob.).",
            "ONLY elementary steps use coeffs as exponents!"
        },
        {
            "2 Mandatory FRQ Validity Criteria:",
            "1) Sum of elementary steps MUST equal the",
            "   overall balanced chemical equation.",
            "2) Rate law derived from the mechanism MUST",
            "   match the experimentally determined law.",
            "NOTE: A mechanism can NEVER be proved 100%!"
        },
        {
            "Ex (Notes 12.6): 2 NO2 -> NO3 + NO (slow)",
            "                 NO3 + CO -> NO2 + CO2 (fast)",
            "Sum: NO2 + CO -> NO + CO2 (NO3 cancels!).",
            "Step 1 is bimolecular & slow (RDS):",
            "Rate = k1[NO2]^2. Matches exp. rate law ->",
            "Mechanism is PLAUSIBLE (consistent)."
        }
    },
    {
        "Slow Step & Intermediates", "12.6",
        {
            "Slow step = Rate-Determining Step (RDS).",
            "Overall rate cannot exceed the slowest step.",
            "Intermediate: produced in an earlier step and",
            "consumed in a later step (never in overall eq).",
            "Catalyst: consumed FIRST, regenerated LATER.",
            "Intermediates MUST be eliminated from Rate Law!"
        },
        {
            "Eliminating Intermediates (Fast Eq. Step 1):",
            "1) Write Rate = k_slow * [reactants of slow].",
            "2) For fast reversible step 1: rate_f = rate_r",
            "   k1[R] = k-1[Int] -> solve for [Int].",
            "3) Substitute [Int] into slow-step rate law.",
            "4) Combine all k constants into one overall k."
        },
        {
            "Ex (Notes 12.6 Ozone): O3 <=> O2 + O (fast eq)",
            "                       O3 + O -> 2 O2 (slow)",
            "Slow step: Rate = k2[O3][O] (O is interm.).",
            "Fast eq: k1[O3] = k-1[O2][O] -> [O]=(k1/k-1)[O3]/[O2].",
            "Sub in: Rate = k * [O3]^2 / [O2].",
            "Use Tab 4 (Custom Mech Builder) to verify!"
        }
    },
    {
        "Collision Model & Orientation", "12.7",
        {
            "Collision Model: Molecules must collide to",
            "react, but ONLY a tiny fraction of collisions",
            "produce products (effective collisions).",
            "Two requirements for an effective collision:",
            "1) Collision energy >= Activation Energy (Ea).",
            "2) Proper relative molecular orientation."
        },
        {
            "FRQ Explanation Checklist:",
            "- Higher [reactant] -> more particles/volume ->",
            "  higher collision frequency -> faster rate.",
            "- Higher surface area -> more contact sites.",
            "- Orientation (steric factor p): colliding atoms",
            "  must align so new bonds can actually form."
        },
        {
            "Ex (Notes 12.7): 2 BrNO -> 2 NO + Br2.",
            "Must break 2 Br-N bonds & form 1 Br-Br bond.",
            "Collision where Br hits Br -> SUCCESS (forms",
            "Br-Br bond). Collision where O hits O or Br",
            "hits O -> FAIL (no reaction, bounces off).",
            "In k = z*p*e^(-Ea/RT), p = steric factor."
        }
    },
    {
        "Activation Energy & Profiles", "12.7",
        {
            "Ea = minimum threshold energy needed to reach",
            "the transition state (activated complex) at",
            "the top of the potential energy barrier.",
            "Delta E = E(products) - E(reactants).",
            "Exo: Delta E < 0 (P lower). Endo: Delta E > 0.",
            "CRITICAL: Delta E does NOT affect rxn rate!"
        },
        {
            "Reading Potential Energy Diagrams:",
            "- Ea(fwd) = Peak - Reactants (controls rate).",
            "- Ea(rev) = Peak - Products = Ea(fwd) - dE.",
            "- Multi-step diagram: # of peaks = # of steps,",
            "  valleys = intermediates, HIGHEST barrier = RDS.",
            "- Smaller Ea -> exponentially faster rate!"
        },
        {
            "Ex (Zumdahl Q69): Exothermic rxn with",
            "Ea(fwd) = 125 kJ/mol, Delta E = -216 kJ/mol.",
            "Ea(rev) = Ea(fwd) - Delta E",
            "        = 125 - (-216) = 341 kJ/mol.",
            "Because products sit 216 kJ lower than R,",
            "reverse rxn must climb both 216 + 125 kJ."
        }
    },
    {
        "Temperature & Maxwell-Boltzmann", "12.7",
        {
            "Maxwell-Boltzmann curve plots # of molecules",
            "(or collisions) vs kinetic energy at temp T.",
            "Area under the curve to the RIGHT of Ea equals",
            "the fraction of effective collisions: e^(-Ea/RT).",
            "Increasing T increases BOTH collision speed",
            "AND (far more importantly) fraction >= Ea."
        },
        {
            "How to Answer Temp FRQs (Full Credit):",
            "1) At higher T2 > T1, curve shifts RIGHT and",
            "   flattens (lower peak, broader tail).",
            "2) Ea line stays FIXED (temp does NOT lower Ea!).",
            "3) Shaded area past Ea grows EXPONENTIALLY,",
            "   increasing rate constant k and rxn rate."
        },
        {
            "Ex (Zumdahl Q12): Compare T2 > T1 on curve.",
            "State BOTH: (a) T2 curve has lower peak and",
            "extends further right (higher avg KE);",
            "(b) Larger fraction of collisions at T2 have",
            "KE >= Ea, so more collisions overcome barrier",
            "-> larger k. See Guide 2 for the live graph!"
        }
    },
    {
        "Arrhenius Equation & Plots", "12.7",
        {
            "Arrhenius Equation: k = A * e^(-Ea / RT).",
            "A = frequency factor (z*p), R = 8.3145 J/mol*K,",
            "T MUST be in Kelvin (C + 273.15), Ea in J/mol.",
            "Linear form: ln(k) = (-Ea/R)*(1/T) + ln(A).",
            "Plot ln(k) vs 1/T -> straight line with",
            "slope = -Ea/R and y-intercept = ln(A)."
        },
        {
            "Two-Point Form & Conceptual FRQs:",
            "ln(k2/k1) = (Ea/R) * (1/T1 - 1/T2).",
            "- Steeper negative slope = LARGER Ea.",
            "- Adding a catalyst lowers Ea -> slope of",
            "  ln(k) vs 1/T becomes LESS negative!",
            "- Always match units: convert kJ -> J (*1000)."
        },
        {
            "Ex (Zumdahl Q73): k1=3.52e-7 at 555 K,",
            "Ea = 186 kJ/mol. Find k2 at 645 K:",
            "ln(k2/3.52e-7) = (186000/8.3145)*(1/555-1/645)",
            "= 5.62 -> k2 = 3.52e-7 * e^5.62 = 9.7e-5.",
            "Use Tab 5 (Arrhenius) for 2-pt or (T,k) plot!"
        }
    },
    {
        "Catalysts, Enzymes & Inhibitors", "12.8",
        {
            "Catalyst: speeds up a reaction WITHOUT being",
            "consumed overall, by providing an ALTERNATIVE",
            "pathway/mechanism with a LOWER activation",
            "energy (Ea). Increases k for BOTH fwd & rev.",
            "Inhibitor: slows rxn (raises Ea or blocks sites).",
            "Catalyst NEVER changes Delta E or equilibrium!"
        },
        {
            "Key AP Chem Catalyst Distinctions:",
            "- In a mechanism: consumed FIRST, regenerated",
            "  LATER (opposite of an intermediate!).",
            "- Homo (same phase) vs Hetero (solid surface).",
            "- Surface/enzyme saturation -> 0th order rate!",
            "- Speedup: k_cat/k_un = exp((Ea,un-Ea,cat)/RT)."
        },
        {
            "Ex (Zumdahl Q79/80): Cl + O3 -> ClO + O2 (slow)",
            "                     ClO + O -> Cl + O2 (fast)",
            "Cl = catalyst (used in Step 1, made in Step 2).",
            "ClO = intermediate (made in Step 1, used in 2).",
            "If catalyst lowers Ea by 2.1 kJ/mol at 298 K:",
            "k_cat/k_un = e^(2100/(8.3145*298)) = 2.3x!"
        }
    }
};

#endif
