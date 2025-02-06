/*

*/

#include "../h/asl.h"
#include "../h/pcb.h"

HIDDEN semd_PTR semd_h;
HIDDEN semd_PTR semdFree_h;

/* return at node right brefore regardless */
static semd_PTR traverseASL(int *semAdd) {
	semd_PTR iter;

	if (NULL == semAdd) return NULL;

	for (iter = semd_h;
		 semAdd > iter->s_next->s_semAdd && iter->s_semAdd < (int*) MAX_INT; 
		 iter = iter->s_next);	

	return iter;
}

static void removeEmptySemd(semd_PTR semdLoc) {
    semd_PTR semdCurr;

    if (NULL == semdLoc) return;

    semdCurr = semdLoc->s_next;
    
    if (NULL == semdCurr || !emptyProcQ(semdCurr->s_procQ)) return;

    semdLoc->s_next = semdCurr->s_next;
    semdCurr->s_procQ = mkEmptyProcQ();
    semdCurr->s_next = semdFree_h;
    semdFree_h = semdCurr;
}

int insertBlocked(int *semAdd, pcb_PTR p) {
	semd_PTR semdIns, semdLoc;

	if (NULL == p || NULL == semAdd) return TRUE;

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
	return outBlocked(headBlocked(semAdd));
}

pcb_PTR outBlocked(pcb_PTR p) {
	semd_PTR semdLoc;
	pcb_PTR pcbRm;

	if (NULL == p || NULL == p->p_semAdd) return NULL;

    semdLoc = traverseASL(p->p_semAdd);

    if (semdLoc->s_next->s_semAdd != p->p_semAdd || emptyProcQ(semdLoc->s_next->s_procQ)) return NULL;

    pcbRm = outProcQ(&(semdLoc->s_next->s_procQ), p);
    
    if (NULL != pcbRm) {
        pcbRm->p_semAdd = NULL;
        removeEmptySemd(semdLoc);
    }
    
    return pcbRm;
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

	semdFree_h = semd_h = NULL;

	/* arrays are always stored sequentially in memory */

	for (i = 0; i < MAXPROC; ++i)
		semdTable[i] = (semd_t) {&semdTable[i + 1], (int*) NULL, mkEmptyProcQ()};

	semdTable[MAXPROC - 1].s_next = NULL;
	semdFree_h = &semdTable[0];

	semdTable[MAXPROC] = (semd_t) {&semdTable[MAXPROC + 1], (int*) 0, mkEmptyProcQ()};
	semdTable[MAXPROC + 1] = (semd_t) {NULL, (int*) MAX_INT, mkEmptyProcQ()};
	semd_h = &semdTable[MAXPROC];
}