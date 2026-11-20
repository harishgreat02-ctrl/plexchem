#ifndef PLEXCHEM_KINETICS_H
#define PLEXCHEM_KINETICS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define GAS_R 8.3145 /* J / (mol·K) */

/* ============================================================================
 * Module 1: Reaction Rates & Stoichiometry
 * ============================================================================ */
typedef struct {
    double c1, c2, t1, t2;
    double coeff;       /* stoichiometric coefficient a */
    bool   is_reactant; /* true -> reactant (-Δ[A]/aΔt), false -> product (+Δ[C]/cΔt) */
    double species_rate;
    double overall_rate;
} RateCalcState;

typedef struct {
    double coeffs[4];      /* a, b, c, d */
    bool   is_product[4];  /* false=reactant, true=product */
    int    known_idx;      /* 0..3 */
    double known_rate;     /* magnitude of known species rate */
    double overall_rate;
    double rates[4];       /* signed rate of change (+prod, -react) */
} StoichRateState;

void kinetics_rate_calc(RateCalcState *s);
void kinetics_stoich_rates(StoichRateState *s);
void kinetics_format_k_units(double overall_order, const char *time_unit, char *out, size_t sz);

/* ============================================================================
 * Module 2: Method of Initial Rates (Configurable 1..3 reactants, 2..8 rows)
 * ============================================================================ */
#define MAX_IR_ROWS 8
#define MAX_IR_REACT 3

typedef struct {
    int    num_reactants; /* 1..3 */
    int    num_rows;      /* 2..8 */
    double conc[MAX_IR_ROWS][MAX_IR_REACT];
    double rate[MAX_IR_ROWS];
    double pred_conc[MAX_IR_REACT];

    /* Outputs */
    bool   solved;
    double raw_order[MAX_IR_REACT];
    double order[MAX_IR_REACT];      /* snapped to integer or half-integer */
    double overall_order;
    double k_trial[MAX_IR_ROWS];
    double k_mean;
    double pred_rate;
    int    pair_r1[MAX_IR_REACT];    /* best trial pair indices for step display */
    int    pair_r2[MAX_IR_REACT];
} InitialRatesState;

bool kinetics_solve_initial_rates(InitialRatesState *s);

/* ============================================================================
 * Module 3: Integrated Rate Laws, Linear Regression Plots & Half-Life
 * ============================================================================ */
#define MAX_INT_ROWS 10

typedef struct {
    double slope;
    double intercept;
    double r2;
} LinReg;

typedef struct {
    int    num_rows; /* 3..10 */
    double t[MAX_INT_ROWS];
    double c[MAX_INT_ROWS];

    /* Regression results for 0th ([A]), 1st (ln[A]), 2nd (1/[A]) */
    LinReg reg[3];
    int    best_order; /* 0, 1, or 2 */
    double k;
    double c0_fit;
    bool   solved;
} IntegratedTableState;

typedef struct {
    int    order;    /* 0, 1, 2, or 3 */
    int    unknown;  /* 0=A0, 1=At, 2=k, 3=t */
    double A0;
    double At;
    double k;
    double t;
    double t_half_1;
    double t_half_2;
    double t_half_3;
    double pct_remaining;
    bool   valid;
} IntegratedEqState;

bool kinetics_regress_order(IntegratedTableState *s);
bool kinetics_integrated_solve(IntegratedEqState *s);
double kinetics_pseudo_k(double k_prime, double excess_conc, double excess_order);

/* ============================================================================
 * Module 4: Reaction Mechanisms & Intermediate Eliminator
 * ============================================================================ */
#define MAX_MECH_STEPS 3
#define MAX_STEP_TERMS 3
#define MAX_SPECIES    8

typedef enum {
    STEP_SLOW = 0,
    STEP_FAST_EQ = 1,
    STEP_FAST = 2
} StepSpeed;

typedef struct {
    char   formula[12];
    int    coeff;
} MechTerm;

typedef struct {
    int       n_react;
    MechTerm  react[MAX_STEP_TERMS];
    int       n_prod;
    MechTerm  prod[MAX_STEP_TERMS];
    StepSpeed speed;
} MechStep;

typedef struct {
    int      num_steps; /* 2 or 3 */
    MechStep steps[MAX_MECH_STEPS];

    /* Derived outputs */
    int    n_species;
    char   species_name[MAX_SPECIES][12];
    int    net_coeff[MAX_SPECIES];    /* <0 reactant, >0 product, 0 cancelled */
    bool   is_intermediate[MAX_SPECIES];
    bool   is_catalyst[MAX_SPECIES];
    double rate_exp[MAX_SPECIES];     /* derived overall rate law exponent */
    int    slow_step_idx;
    char   molecularity[MAX_MECH_STEPS][16];
    char   overall_eq[96];
    char   rate_law_math[96];
    char   elim_step_math[96];
    bool   valid;
} MechState;

bool kinetics_analyze_mech(MechState *m);

/* ============================================================================
 * Module 5: Arrhenius, Collision Model & Catalysis
 * ============================================================================ */
#define MAX_ARRH_ROWS 8

typedef struct {
    int    unknown;  /* 0=Ea(kJ/mol), 1=k2, 2=T2(K), 3=k1, 4=T1(K) */
    double k1;
    double k2;
    double T1;       /* Kelvin */
    double T2;       /* Kelvin */
    double Ea_kj;    /* kJ/mol */
    bool   valid;
} ArrhTwoPtState;

typedef struct {
    int    num_rows; /* 2..8 */
    double T[MAX_ARRH_ROWS]; /* Kelvin */
    double k[MAX_ARRH_ROWS];
    LinReg reg;      /* ln(k) vs 1/T */
    double Ea_kj;
    double A_factor;
    double eval_T;
    double eval_k;
    bool   solved;
} ArrhTableState;

typedef struct {
    double Ea_uncat_kj;
    double Ea_cat_kj;
    double delta_E_kj; /* negative=exothermic, positive=endothermic */
    double T_kelvin;
    double Ea_rev_kj;
    double speedup_ratio; /* k_cat / k_uncat */
} CatalystState;

bool kinetics_arrhenius_2pt(ArrhTwoPtState *s);
bool kinetics_arrhenius_regress(ArrhTableState *s);
void kinetics_catalyst_calc(CatalystState *s);

#endif /* PLEXCHEM_KINETICS_H */
