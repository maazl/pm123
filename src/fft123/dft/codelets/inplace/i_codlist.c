#include "ifftw.h"

extern void X(codelet_q1_2)(planner *);
extern void X(codelet_q1_4)(planner *);
extern void X(codelet_q1_8)(planner *);
extern void X(codelet_q1_3)(planner *);
extern void X(codelet_q1_5)(planner *);
extern void X(codelet_q1_6)(planner *);


extern const solvtab X(solvtab_dft_inplace);
const solvtab X(solvtab_dft_inplace) = {
   SOLVTAB(X(codelet_q1_2)),
   SOLVTAB(X(codelet_q1_4)),
   SOLVTAB(X(codelet_q1_8)),
   SOLVTAB(X(codelet_q1_3)),
   SOLVTAB(X(codelet_q1_5)),
   SOLVTAB(X(codelet_q1_6)),
   SOLVTAB_END
};
