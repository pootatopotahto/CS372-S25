/*

*/

#include "../h/pcb.h"

HIDDEN pcb_t pcbPool[MAXPROC];
HIDDEN pcb_PTR pcbFree_h = NULL;

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

pcb_PTR allocPcb(void) {
	if (pcbFree_h == NULL) return NULL;

	pcb_PTR pcbRm = pcbFree_h;

	pcbFree_h = pcbRm->p_next;

	pcbRm->p_next = NULL;
	pcbRm->p_prev = NULL;
	pcbRm->p_parent = NULL;
	pcbRm->p_child = NULL;
	pcbRm->p_sib = NULL;

	pcbRm->p_time = 0;
	/* pcbRm->support_t = NULL; */

	return pcbRm;
}

pcb_PTR mkEmptyProcQ(void) {
	pcb_PTR t_procQ;
	t_procQ = NULL;
	return t_procQ;
}

int emptyProcQ(pcb_PTR tp) {
	if (tp == NULL)
		return TRUE;
	else
		return FALSE;
}

void insertProcQ(pcb_PTR *tp, pcb_PTR p) {
	if (p == NULL) return;

	if (emptyProcQ(*tp)) {
		*tp = p;
		p->p_prev = p;
		p->p_next = p;
		return;
	}

	p->p_next = (*tp)->p_next;
	p->p_prev = *tp;
	(*tp)->p_next->p_prev = p;
	(*tp)->p_next = p;
	*tp = p;
}

pcb_PTR removeProcQ(pcb_PTR *tp) {
	if (emptyProcQ(*tp)) return NULL;

	pcb_PTR pcbRemoved;
	pcbRemoved = (*tp)->p_next;

	if (pcbRemoved == *tp)
		*tp = NULL;
	else {
		pcbRemoved->p_prev->p_next = pcbRemoved->p_next;
		pcbRemoved->p_next->p_prev = pcbRemoved->p_prev;
		*tp = pcbRemoved->p_prev;
	}

	pcbRemoved->p_next = pcbRemoved->p_prev = NULL;

	return pcbRemoved;
}

pcb_PTR outProcQ(pcb_PTR *tp, pcb_PTR p) {
	if (p == NULL || emptyProcQ(*tp)) return NULL;

	pcb_PTR iter;
	iter = *tp;

	do {
	 	if (iter == p) {
	 		if (iter == *tp) {
	 			if (iter->p_next == iter)
	 				*tp = NULL;
	 			else
	 				*tp = (*tp)->p_prev;
	 		}

	 		iter->p_prev->p_next = iter->p_next;
	 		iter->p_next->p_prev = iter->p_prev;
	 		iter->p_next = NULL;
	 		iter->p_prev = NULL;

	 		return p;
	 	}

	 	iter = iter->p_next;

	} while (iter != *tp);

	return NULL;
}


pcb_PTR headProcQ(pcb_PTR tp) {
	if (emptyProcQ(tp)) return NULL;

	return tp->p_next;
}