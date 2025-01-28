/*

*/

#include "../h/pcb.h"

HIDDEN pcb_t pcbPool[MAXPROC];
HIDDEN pcb_PTR pcbFree_h;

/*  */
void initPcbs(void) {
	int i;

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
	if (NULL == pcbFree_h) return NULL;

	pcb_PTR pcbRm = pcbFree_h;

	pcbFree_h = pcbRm->p_next;

	pcbRm->p_next = pcbRm->p_prev = NULL;
	pcbRm->p_parent = pcbRm->p_child = NULL;
	pcbRm->p_sib_next = pcbRm->p_sib_prev = NULL;

	pcbRm->p_time = 0;
	pcbRm->p_semAdd = NULL;
	/* pcbRm->support_t = NULL; */

	return pcbRm;
}

pcb_PTR mkEmptyProcQ(void) {
	pcb_PTR t_procQ;
	t_procQ = NULL;
	return t_procQ;
}

int emptyProcQ(pcb_PTR tp) {
	if (NULL == tp) return TRUE;

	return FALSE;
}

void insertProcQ(pcb_PTR *tp, pcb_PTR p) {
	pcb_PTR iter;

	if (NULL == p) return;

	if (emptyProcQ(*tp)) {
		*tp = p;
		p->p_prev = p->p_next = p;
		return;
	}

	iter = *tp;
	do {
		if (iter == p) return;
		iter = iter->p_next;
	} while (iter != *tp);

	p->p_next = (*tp)->p_next;
	p->p_prev = *tp;
	(*tp)->p_next->p_prev = p;
	(*tp)->p_next = p;
	*tp = p;
}

pcb_PTR removeProcQ(pcb_PTR *tp) {
	pcb_PTR pcbRemoved;

	if (emptyProcQ(*tp)) return NULL;
	
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
	pcb_PTR iter;

	if (NULL == p || emptyProcQ(*tp)) return NULL;
	
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
	 		iter->p_next = iter->p_prev = NULL;

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

int emptyChild(pcb_PTR p) {
	if (NULL == p->p_child) return TRUE;

	return FALSE;
}

void insertChild(pcb_PTR prnt, pcb_PTR p) {
	pcb_PTR iter;

	if (NULL == prnt || NULL == p) return;

	p->p_parent = prnt;

	if (emptyChild(prnt)) {
		prnt->p_child = p;
		/* Explicit NULL */
		p->p_sib_next = NULL;
		p->p_sib_prev = NULL;
		return;
	}
	
	iter = prnt->p_child;

	while (iter->p_sib_next != NULL) {
		if (iter == p) return;
		iter = iter->p_sib_next;
	}
	if (iter == p) return; /* Check last */
	
	iter->p_sib_next = p;
	p->p_sib_prev = iter;
	p->p_sib_next = NULL; /* Explicit in case non NULL */
}

pcb_PTR removeChild(pcb_PTR p) {
	pcb_PTR childRm;

	if(NULL == p || emptyChild(p)) return NULL;
	
	childRm = p->p_child;
	childRm->p_parent = NULL;

	if (NULL == p->p_child->p_sib_next)
		p->p_child = NULL;
	else {
		p->p_child = p->p_child->p_sib_next;
		childRm->p_sib_next = NULL;
		p->p_child->p_sib_prev = NULL;
	}

	return childRm; 
}

pcb_PTR outChild(pcb_PTR p) {
	if (NULL == p || NULL == p->p_parent) return NULL;

	if (p->p_parent->p_child == p) /* Explicit in case first/only child*/
		p->p_parent->p_child = p->p_sib_next;

	if (p->p_sib_prev)
		p->p_sib_prev->p_sib_next = p->p_sib_next;

	if (p->p_sib_next)
		p->p_sib_next->p_sib_prev = p->p_sib_prev;

	p->p_sib_next = p->p_sib_prev = p->p_parent = NULL;

	return p;
}