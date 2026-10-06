/* Bench diagnostics default on; customer builds set DOMOVOI_DIAG=0. */
#ifndef DOMOVOI_DIAG_H
#define DOMOVOI_DIAG_H
#ifndef DOMOVOI_DIAG
#define DOMOVOI_DIAG 1
#endif
#if DOMOVOI_DIAG
#define DOMOVOI_DIAG_INC(value) (++(value))
#else
#define DOMOVOI_DIAG_INC(value) ((void)0)
#endif
#endif
