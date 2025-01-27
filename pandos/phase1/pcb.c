/*

*/

#include "../h/pcb.h"

HIDDEN pcb_PTR pcbFree_h = NULL;
HIDDEN pcb_t pcbPool[MAXPROC];

/*  */
void initPcbs(void) {
	int i;
	for (i = 0; i < MAXPROC; ++i) {
		pcbPool[i].p_next = pcbFree_h;
		pcbFree_h = &pcbPool[i];
	}
}

void freePcb(pcb_PTR p) {
	if (p == NULL) return;

	p->p_next = pcbFree_h;
	pcbFree_h = p;
}

pcb_t *allocPcb(void) {
	if (pcbFree_h == NULL) return NULL;

	pcb_PTR pcbRm = pcbFree_h;

	pcbFree_h = pcbRm->p_next;

	pcbRm->p_next = NULL;
	pcbRm->p_prev = NULL;
	pcbRm->p_parent = NULL;
	pcbRm->p_child = NULL;
	pcbRm->p_sib = NULL;

	pcbRm->p_s.s_entryHI = 0;
    pcbRm->p_s.s_cause = 0;
    pcbRm->p_s.s_status = 0;
    pcbRm->p_s.s_pc = 0;

    int i;
    for (i = 0; i < STATEREGNUM; ++i) {
        pcbRm->p_s.s_reg[i] = 0;
    }

	pcbRm->p_time = 0;
	/* pcbRm->support_t = NULL; */

	return pcbRm;
}