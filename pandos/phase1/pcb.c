/*

*/

#include "../h/pcb.h"

HIDDEN pcb_PTR pcbFree_h;

/*  */
void initPcbs(void) {
	int i;

	static pcb_t pcbPool[MAXPROC];

	pcbFree_h = NULL;

	for (i = 0; i < MAXPROC; ++i) {
		pcbPool[i].p_next = pcbPool[i].p_prev = NULL;
        pcbPool[i].p_parent = pcbPool[i].p_child = NULL;
        pcbPool[i].p_sib_next = pcbPool[i].p_sib_prev = NULL;
        pcbPool[i].p_time = 0;
        pcbPool[i].p_semAdd = NULL;

		pcbPool[i].p_next = pcbFree_h;
		pcbFree_h = &pcbPool[i];
	}
}

void freePcb(pcb_PTR p) {
	if (NULL == p) return;

	p->p_next = pcbFree_h;
	pcbFree_h = p;
}

pcb_PTR allocPcb(void) {
	pcb_PTR pcbRm = pcbFree_h;

	if (NULL == pcbFree_h) return NULL;

	pcbFree_h = pcbRm->p_next;

	/* Set all values to default */ 
	pcbRm->p_next = pcbRm->p_prev = NULL;
	pcbRm->p_parent = pcbRm->p_child = NULL;
	pcbRm->p_sib_next = pcbRm->p_sib_prev = NULL;
	pcbRm->p_time = 0;
	pcbRm->p_semAdd = NULL;

	return pcbRm;
}

pcb_PTR mkEmptyProcQ(void) {
    return (NULL);
}

int emptyProcQ(pcb_PTR tp) {
	return (NULL == tp);
}

void insertProcQ(pcb_PTR *tp, pcb_PTR p) {
	if (NULL == p) return;

	if (emptyProcQ(*tp)) {
		*tp = p;
		p->p_prev = p->p_next = p;
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

	return outProcQ(tp, (*tp)->p_next);
}

pcb_PTR outProcQ(pcb_PTR *tp, pcb_PTR p) {
	pcb_PTR iter;

	if (NULL == p || emptyProcQ(*tp)) return NULL;

	if (p == *tp) {
		*tp = NULL;
		return p;
	}

	for (iter = (*tp)->p_next; iter != *tp; iter = iter->p_next) {
		if (iter == p) {
			iter->p_prev->p_next = iter->p_next;
			iter->p_next->p_prev = iter->p_prev;
			*tp = (*tp)->p_prev;

			iter->p_next = iter->p_prev = NULL;

			return p;
		}
	}

	return NULL;
}

pcb_PTR headProcQ(pcb_PTR tp) {
	if (emptyProcQ(tp)) return NULL;

	return tp->p_next;
}

int emptyChild(pcb_PTR p) {
	return (NULL == p->p_child);
}
	
void insertChild(pcb_PTR prnt, pcb_PTR p) {
	if (NULL == prnt || NULL == p) return;

	p->p_parent = prnt;

	if (emptyChild(prnt)) {
		prnt->p_child = p;

		/* Explicit NULL */
		p->p_sib_next = p->p_sib_prev = NULL;

		return;
	}
	
	p->p_sib_next = prnt->p_child;
	p->p_sib_next->p_sib_prev = p;
	prnt->p_child = p;
	p->p_sib_prev = NULL; /* Explicit in case non NULL */
}

pcb_PTR removeChild(pcb_PTR p) {
	if (NULL == p || emptyChild(p)) return NULL;

	return outChild(p->p_child); 
}

pcb_PTR outChild(pcb_PTR p) {
	if (NULL == p || NULL == p->p_parent) return NULL;

	if (p->p_parent->p_child == p) /* Explicit in case first/only child*/
		p->p_parent->p_child = p->p_sib_next;

	if (p->p_sib_prev != NULL)
		p->p_sib_prev->p_sib_next = p->p_sib_next;

	if (p->p_sib_next != NULL)
		p->p_sib_next->p_sib_prev = p->p_sib_prev;

	p->p_sib_next = p->p_sib_prev = p->p_parent = NULL;

	return p;
}