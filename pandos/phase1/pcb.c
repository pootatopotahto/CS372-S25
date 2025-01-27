/*

*/

#include "../h/pcb.h"

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
	support_t			*p_supportStruct;	/* ptr to support struct */
} pcb_t, *pcb_PTR;

