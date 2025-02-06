/******************************* asl.c ***************************************
 *
 *  Module: Active Semaphore List (ASL) Management
 *
 *  This module implements the Active Semaphore List (ASL), which 
 *  manages process queues associated with semaphores. The ASL ensures 
 *  that each semaphore in use is associated with a queue of blocked 
 *  processes.
 *
 *  The module provides:
 *  
 *  - Sorted Active Semaphore List (ASL): Maintained as a NULL-terminated 
 *    singly linked list, sorted by semaphore address (`s_semAdd`).
 *  - Semaphore Descriptors Free List (Stack): Stores unused `semd_t` 
 *    descriptors for reuse.
 *  - Process Queue Management: Each active semaphore has a process queue 
 *    (`s_procQ`) storing blocked processes.
 *
 *  Data Structures Used:
 *  
 *  - Stack: The free list (`semdFree_h`) is a NULL-terminated singly linked 
 *    list* where newly freed semaphore descriptors are pushed and allocated 
 *    descriptors are popped.
 *  - Sorted Singly Linked List: The ASL (`semd_h`) maintains active semaphores 
 *    in ascending order, using `s_semAdd` as the sorting key.
 *  - Queue: Each semaphore has a process queue, implemented as a circular 
 *    doubly linked list.
 *
 *  This module ensures:
 *  
 *  - Proper initialization of the ASL (`initASL`).
 *  - Safe insertion (`insertBlocked`) and removal (`removeBlocked`, `outBlocked`) 
 *    of PCBs into/from semaphore queues.
 *  - Efficient retrieval of the first blocked process (`headBlocked`).
 *
 *****************************************************************************/
#include "../h/asl.h"
#include "../h/pcb.h"

HIDDEN semd_PTR semd_h;			/* Head of the Active Semaphore List (ASL) */
HIDDEN semd_PTR semdFree_h;		/* Head of the Free Semaphore Descriptor List */

/***************************************************************
 *  traverseASL - Traverses the Active Semaphore List (ASL)
 *
 *  This function searches the sorted singly linked ASL to find 
 *  the node preceding the given semaphore (`semAdd`).
 *
 *  - The ASL is sorted in ascending order using `s_semAdd`.
 *  - If `semAdd` is greater than the next node's `s_semAdd`, 
 *    traversal continues.
 *  - If `semAdd` is less than or equal to `s_semAdd` of the next node, 
 *    traversal stops, and the function returns the node before it.
 *
 *  Parameters:
 *    - semAdd: Pointer to the semaphore address.
 *
 *  Returns:
 *    - Pointer to the node preceding the target semaphore.
 *    - NULL if `semAdd` is NULL.
 ***************************************************************/
static semd_PTR traverseASL(int *semAdd) {
	semd_PTR iter;

	/* Return NULL if semAdd is invalid */
	if (NULL == semAdd) return NULL;

	/* Traverse the ASL to find the correct position */
	for (iter = semd_h;
		 semAdd > iter->s_next->s_semAdd && iter->s_semAdd < (int*) MAX_INT; 
		 iter = iter->s_next);	

	return iter;	/* Return the node before the target */
}


/***************************************************************
 *  insertBlocked - Inserts a Process into a Semaphore's Queue
 *
 *  This function blocks a process (`p`) on the given semaphore (`semAdd`). 
 *  If the semaphore is already active, `p` is inserted into its queue.
 *  Otherwise, a new semaphore descriptor is allocated from `semdFree_h`.
 *
 *  Parameters:
 *    - semAdd: Pointer to the semaphore address.
 *    - p: Pointer to the PCB to be blocked.
 *
 *  Returns:
 *    - FALSE (0) if successful.
 *    - TRUE (1) if `p` is NULL, `semAdd` is NULL, or if no free semaphore 
 *      descriptors are available.
 ***************************************************************/
int insertBlocked(int *semAdd, pcb_PTR p) {
	semd_PTR semdIns, semdLoc;

	/* Case 1: Invalid input */
	if (NULL == p || NULL == semAdd) return TRUE;

	/* Associate process with the given semaphore */
	p->p_semAdd = semAdd;

	/* Locate the correct position in the ASL */
	semdLoc = traverseASL(semAdd);

	/* Case 2: Semaphore is already active, add to its queue */
	if (semdLoc->s_next->s_semAdd == semAdd) {
        insertProcQ(&(semdLoc->s_next->s_procQ), p);

        return FALSE;
    }

	/* Case 3: No free semaphore descriptors available */
    if (NULL == semdFree_h) return TRUE;

	/* Case 4: Allocate new semaphore descriptor */
	semdIns = semdFree_h;
	semdFree_h = semdFree_h->s_next;	
	
	/* Insert new semaphore descriptor into ASL */
	semdIns->s_next = semdLoc->s_next;
	semdLoc->s_next = semdIns;

	/* Initialize semaphore descriptor */
	semdIns->s_procQ = mkEmptyProcQ();
	semdIns->s_semAdd = semAdd;
	insertProcQ(&(semdIns->s_procQ), p);

	return FALSE;
}

/***************************************************************
 *  removeBlocked - Removes the First Process from a Semaphore Queue
 *
 *  This function removes and returns the first PCB from the 
 *  queue associated with `semAdd`.
 *
 *  Parameters:
 *    - semAdd: Pointer to the semaphore address.
 *
 *  Returns:
 *    - Pointer to the removed PCB.
 *    - NULL if the semaphore is inactive or its queue is empty.
 ***************************************************************/
pcb_PTR removeBlocked(int *semAdd) {
	return outBlocked(headBlocked(semAdd));
}

/***************************************************************
 *  outBlocked - Removes a Specific Process from a Semaphore Queue
 *
 *  This function removes a specific PCB (`p`) from its semaphore queue.
 *  If the process queue becomes empty, the semaphore descriptor is returned 
 *  to the free list (`semdFree_h`).
 *
 *  Parameters:
 *    - p: Pointer to the PCB to be removed.
 *
 *  Returns:
 *    - Pointer to the removed PCB.
 *    - NULL if `p` is NULL, not blocked, or if the semaphore queue is empty.
 ***************************************************************/
pcb_PTR outBlocked(pcb_PTR p) {
	semd_PTR semdLoc, semdCurr;
	pcb_PTR pcbRm;

	/* Return NULL if process is invalid or not blocked */
	if (NULL == p || NULL == p->p_semAdd) return NULL;

	/* Find the semaphore in the ASL */
    semdLoc = traverseASL(p->p_semAdd);
	semdCurr = semdLoc->s_next;

	/* Check if the semaphore exists and has processes */
    if (semdCurr->s_semAdd != p->p_semAdd || emptyProcQ(semdCurr->s_procQ)) return NULL;

	/* Check if the semaphore exists and has processes */
    pcbRm = outProcQ(&(semdCurr->s_procQ), p);
    
	/* If the queue becomes empty, remove the semaphore from ASL */
    if (NULL != pcbRm && emptyProcQ(semdCurr->s_procQ)) {
        pcbRm->p_semAdd = NULL;

		/* Remove semaphore from ASL and return it to the free list */
		semdLoc->s_next = semdCurr->s_next;
		semdCurr->s_procQ = mkEmptyProcQ();
		semdCurr->s_next = semdFree_h;
		semdFree_h = semdCurr;
    }
    
    return pcbRm;
}

/***************************************************************
 *  headBlocked - Retrieves the First Process in a Semaphore Queue
 *
 *  This function returns (without removing) the first PCB
 *  in the queue associated with `semAdd`.
 *
 *  Parameters:
 *    - semAdd: Pointer to the semaphore address.
 *
 *  Returns:
 *    - Pointer to the first PCB in the queue.
 *    - NULL if the semaphore is inactive or its queue is empty.
 ***************************************************************/
pcb_PTR headBlocked(int *semAdd) {
	semd_PTR semdLoc;
	pcb_PTR pcbRet;

	semdLoc = traverseASL(semAdd)->s_next;

	/* Check if semaphore is active and has processes */
	if (semdLoc->s_semAdd != semAdd || emptyProcQ(semdLoc->s_procQ)) return NULL;

	pcbRet = headProcQ(semdLoc->s_procQ);
	pcbRet->p_semAdd = semAdd;

	/* Return the first process in the queue without removing it */
	return pcbRet;
}

/***************************************************************
 *  initASL - Initializes the Active Semaphore List (ASL)
 *
 *  This function initializes the Active Semaphore List (ASL)
 *  and the Free Semaphore Descriptor List (`semdFree_h`).
 *
 *  - The ASL is implemented as a NULL-terminated singly 
 *    linked list, keeping active semaphores sorted by 
 *    their memory addresses (`s_semAdd`).
 *  - The Free List is implemented as a stack (LIFO order) 
 *    and contains all unused `semd_t` descriptors.
 *  - Two dummy nodes are added at the beginning and end of 
 *    the ASL for efficient traversal and avoid the meomry loss:
 *      - Head Dummy Node: `s_semAdd = 0`
 *      - Tail Dummy Node: `s_semAdd = MAX_INT`
 *
 *  Parameters:
 *    - None
 ***************************************************************/
void initASL(void) {
	int i;
	/* Static array of semaphore descriptors */
	static semd_t semdTable[MAXPROC + DUMMYVARCOUNT];

	/* Initialize both ASL and Free List heads */	
	semdFree_h = semd_h = NULL;																													

	/* Step 1: Initialize the Free List */
	for (i = 0; i < MAXPROC; ++i){
		/* Each descriptor points to the next in the Free List */
		semdTable[i] = (semd_t) {&semdTable[i + 1], (int*) NULL, mkEmptyProcQ()};
	}
	/* Last element of the Free List should point to NULL */
	semdTable[MAXPROC - 1].s_next = NULL;		
	/* Set the head of the Free List to the first descriptor */									
	semdFree_h = &semdTable[0];														

	/* Step 2: Initialize Dummy Nodes for ASL */
	/* Dummy Head Node (s_semAdd <- 0) */
	semdTable[MAXPROC] = (semd_t) {&semdTable[MAXPROC + 1], (int*) 0, mkEmptyProcQ()};
	/* Dummy Tail Node (s_semAdd <- MAX_INT) */
	semdTable[MAXPROC + 1] = (semd_t) {NULL, (int*) MAX_INT, mkEmptyProcQ()};
	/* Set the head of ASL to the Dummy Head Node */
	semd_h = &semdTable[MAXPROC];
}