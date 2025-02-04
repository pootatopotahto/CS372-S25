/*

*/

#include "../h/asl.h"
#include "../h/pcb.h"

HIDDEN semd_PTR semd_h;
HIDDEN semd_PTR semdFree_h;

int insertBlocked(int *semAdd, pcb_PTR p) {
	semd_PTR semdIns, semdLoc;

	if (NULL == p) return TRUE;

	p->p_semAdd = semAdd;

	semdLoc = traverseASL(semAdd);

	if (semdLoc->s_next->s_semAdd == semAdd) {
        insertProcQ(&(semdLoc->s_next->s_procQ), p);

        return FALSE;
    }

    if (NULL == semdFree_h) return TRUE;

	semdIns = semdFree_h;
	semdFree_h = semdFree_h->s_next;	
	
	semdIns->s_next = semdLoc->s_next;
	semdLoc->s_next = semdIns;

	semdIns->s_procQ = mkEmptyProcQ();
	semdIns->s_semAdd = semAdd;
	insertProcQ(&(semdIns->s_procQ), p);

	return FALSE;
}

pcb_PTR removeBlocked(int *semAdd) {
	semd_PTR semdLoc, semdCurr;
	pcb_PTR pcbRm;

	semdLoc = traverseASL(semAdd);
	semdCurr = semdLoc->s_next;

	if (semdLoc->s_next->s_semAdd != semAdd) return NULL;

	pcbRm = removeProcQ(&(semdLoc->s_next->s_procQ));

	pcbRm->p_semAdd = NULL;

	if (emptyProcQ(semdCurr->s_procQ)) {
		semdLoc->s_next = semdCurr->s_next;
		semdCurr->s_next = semdFree_h;
		semdFree_h = semdCurr;
	}

	return pcbRm;
}

pcb_PTR outBlocked(pcb_PTR p) {
	semd_PTR semdLoc, semdCurr;

	if (NULL == p) return NULL;

	semdLoc = traverseASL(p->p_semAdd);
	semdCurr = semdLoc->s_next;

	if (semdCurr->s_semAdd != p->p_semAdd) return NULL;

	if (emptyProcQ(semdCurr->s_procQ)) {
		semdLoc->s_next = semdCurr->s_next;
		semdCurr->s_next = semdFree_h;
		semdFree_h = semdCurr;
	}

	return outProcQ(&(semdCurr->s_procQ), p);
}

pcb_PTR headBlocked(int *semAdd) {
	semd_PTR semdLoc;
	pcb_PTR pcbRet;

	semdLoc = traverseASL(semAdd)->s_next;

	if (semdLoc->s_semAdd != semAdd || emptyProcQ(semdLoc->s_procQ)) return NULL;

	pcbRet = headProcQ(semdLoc->s_procQ);
	pcbRet->p_semAdd = semAdd;

	return pcbRet;
}

void initASL(void) {
	int i;

	static semd_t semdTable[MAXPROC + DUMMYVARCOUNT];

	/* arrays are always stored sequentially in memory */

	for (i = 0; i < MAXPROC; ++i)
		semdTable[i] = (semd_t) {&semdTable[i + 1], NULL, mkEmptyProcQ()};

	semdTable[MAXPROC - 1].s_next = NULL;
	semdFree_h = &semdTable[0];

	semdTable[MAXPROC] = (semd_t) {&semdTable[MAXPROC + 1], (int*) 0, mkEmptyProcQ()};
	semdTable[MAXPROC + 1] = (semd_t) {NULL, (int*) MAX_INT, mkEmptyProcQ()};
	semd_h = &semdTable[MAXPROC];
}

/* return at node right brefore regardless */
semd_PTR traverseASL(int *semAdd) {
	semd_PTR iter;

	for (iter = semd_h;
		 semAdd > iter->s_next->s_semAdd && iter->s_semAdd < (int*) MAX_INT; 
		 iter = iter->s_next);	

	return iter;
}

/*
Need dealloc helper
*/