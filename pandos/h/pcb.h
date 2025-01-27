#ifndef PCB
#define PCB

/************************* PROCQ.H *****************************
*
*  The externals declaration file for the Process Control Block
*    Module.
*
*  Written by Mikeyg
*/

#include "../h/types.h"

/* process control block type */
typedef struct pcb_t {
	/* process queue fields */
	struct pcb_t		*p_next,	/* ptr to next entry*/
						*p_prev,	/* ptr to prev entry*/

	/* process tree fields */
						*p_parent,	/* ptr to parent */
						*p_child,	/* ptr to 1st child */
						*p_sib;		/* ptr to sibling */

	/* process status info */
	state_t				p_s;		/* processor state */
	cpu_t				p_time;		/* cpu time used by proc */
	int 				*p_semAdd;	/* ptr to sema4 on which process blocked */

	/* support layter info */
	/*support_t			*p_supportStruct;	 ptr to support struct */
} pcb_t, *pcb_PTR;

extern void freePcb (pcb_PTR p);
extern pcb_PTR allocPcb ();
extern void initPcbs ();

/*
extern pcb_PTR mkEmptyProcQ (); 
extern int emptyProcQ (pcb_PTR tp);
extern void insertProcQ (pcb_PTR *tp, pcb_PTR p);
extern pcb_PTR removeProcQ (pcb_PTR *tp);
extern pcb_PTR outProcQ (pcb_PTR *tp, pcb_PTR p);
extern pcb_PTR headProcQ (pcb_PTR tp);

extern int emptyChild (pcb_PTR p);
extern void insertChild (pcb_PTR prnt, pcb_PTR p);
extern pcb_PTR removeChild (pcb_PTR p);
extern pcb_PTR outChild (pcb_PTR p);
*/

/***************************************************************/

#endif