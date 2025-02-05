/***************************************************************
 *  pcb.c - Process Control Block (PCB) Management
 *  
 *  This module manages the allocation, deallocation, and 
 *  organization of process control blocks (PCBs). It provides:
 *  
 *  - A free list of unused PCBs.
 *  - FIFO process queues for scheduling.
 *  - Parent-child relationships for process trees.
 *
 *  Each PCB represents a process and contains links for both 
 *  queue management and hierarchical parent-child relationships.
 ***************************************************************/

#include "../h/pcb.h"

/* Head of the free PCB list (stores unused PCBs) */
HIDDEN pcb_PTR pcbFree_h;

/***************************************************************
 *  initPcbs - Initializes the Free PCB List
 *  
 *  Initializes the free list of PCBs by linking all `MAXPROC` 
 *  PCBs into a single NULL-terminated linked list. It ensures 
 *  that all PCBs are available for allocation at the start of 
 *  execution.
 * 
 *	Parameters:
 * 		- none
 * 
 *  This function is called once during system initialization.
 ***************************************************************/
void initPcbs(void) {
	int i;
	static pcb_t pcbPool[MAXPROC];	/* Statically allocated PCB pool */
	pcbFree_h = NULL;				/* Ensure list starts empty */

	/* Link all PCBs into the free list */
	for (i = 0; i < MAXPROC; ++i) {
		pcbPool[i].p_next = pcbFree_h;
		pcbFree_h = &pcbPool[i];
	}
}

/***************************************************************
 *  freePcb - Returns a PCB to the Free List
 *
 *  Inserts the given PCB (`p`) at the head of the free list, 
 *  making it available for reuse.
 *
 *  Parameters:
 * 		- p: Pointer to the PCB to be freed.
 *
 *  If `p` is NULL, the function does nothing.
 ***************************************************************/
void freePcb(pcb_PTR p) {
	if (NULL == p) return;	/* Ignore NULL input */

	/* Insert PCB back into the free list */
	p->p_next = pcbFree_h;
	pcbFree_h = p;
}

/***************************************************************
 *  allocPcb - Allocates a PCB from the Free List
 *
 *  Removes and returns the first available PCB from `pcbFree_h`. 
 *  If no PCBs are available, returns NULL.
 *
 *  Returns:
 *    - Pointer to the allocated PCB.
 *    - NULL if no PCBs are available.
 *
 *	Parameters:
 * 		- none
 * 
 *  The allocated PCB has all its fields reset to default values.
 ***************************************************************/
pcb_PTR allocPcb(void) {
	pcb_PTR pcbRm;
	pcbRm = pcbFree_h;

	/* No PCBs available */
	if (NULL == pcbRm) return NULL;

	/* Remove the first PCB from the free list */
	pcbFree_h = pcbRm->p_next;

	/* Reset all PCB fields */
	pcbRm->p_next = pcbRm->p_prev = NULL;
	pcbRm->p_parent = pcbRm->p_child = NULL;
	pcbRm->p_sib_next = pcbRm->p_sib_prev = NULL;
	pcbRm->p_time = 0;
	pcbRm->p_semAdd = NULL;

	return pcbRm;
}

/***************************************************************
 *  mkEmptyProcQ - Creates an Empty Process Queue
 *
 *	Parameters:
 *		- none
 *
 *  Returns an empty process queue (NULL).
 ***************************************************************/
pcb_PTR mkEmptyProcQ(void) {
    return (NULL);
}

/***************************************************************
 *  emptyProcQ - Checks if a Process Queue is Empty
 *
 *  Parameters:
 *    - tp: Tail pointer of the queue.
 *
 *  Returns:
 *    - 1 (TRUE) if the queue is empty.
 *    - 0 (FALSE) if the queue has processes.
 ***************************************************************/
int emptyProcQ(pcb_PTR tp) {
	return (NULL == tp);
}

/***************************************************************
 *  insertProcQ - Inserts a PCB into a Process Queue (FIFO)
 *
 *  Inserts `p` at the tail of the FIFO queue pointed to by `tp`. 
 *  Updates `tp` to point to the newly added tail.
 *
 *  Parameters:
 *    - tp: Pointer to the queue's tail.
 *    - p:  PCB to be inserted.
 *
 *  If `p` is NULL, the function does nothing.
 ***************************************************************/
void insertProcQ(pcb_PTR *tp, pcb_PTR p) {
	/* Ignore NULL process */
	if (NULL == p) return;

	/* If the queue is empty, initialize it with the new process */
	if (emptyProcQ(*tp)) {
		*tp = p;
		p->p_prev = p->p_next = p;
		return;
	}

	/* Insert new PCB at the tail */
	p->p_next = (*tp)->p_next;	/* New PCB's next points to the head */
	p->p_prev = *tp;			/* New PCB's prev points to the current tail */
	(*tp)->p_next->p_prev = p;	/* Update the head's prev to point to new PCB */
	(*tp)->p_next = p;			/* Update the tail's next to point to new PCB */
	*tp = p;					/* Update tail pointer to the newly inserted PCB */
}

/***************************************************************
 *  removeProcQ - Removes the Head PCB from the Queue
 *
 *  Removes the first PCB from the queue and returns it.
 *
 *  Parameters:
 *    - tp: Pointer to the tail of the queue.
 *
 *  Returns:
 *    - Pointer to the removed PCB.
 *    - NULL if the queue is empty.
 ***************************************************************/
pcb_PTR removeProcQ(pcb_PTR *tp) {
	/* Return NULL if the queue is empty */
	if (emptyProcQ(*tp)) return NULL;
	
	/* Remove and return the first PCB (head) from the queue */
	return outProcQ(tp, (*tp)->p_next);
}

/***************************************************************
 *  outProcQ - Removes a Specific PCB from a Process Queue
 *
 *  This function removes a specific process (PCB) `p` from the 
 *  process queue whose tail pointer is `tp`. If `p` is not in 
 *  the queue, the function returns NULL.
 *
 *  Parameters:
 *    - tp: Pointer to the tail of the process queue.
 *    - p:  Pointer to the PCB to be removed.
 *
 *  Returns:
 *    - Pointer to the removed PCB if found.
 *    - NULL if `p` is not in the queue or if the queue is empty.
 ***************************************************************/
pcb_PTR outProcQ(pcb_PTR *tp, pcb_PTR p) {
	pcb_PTR iter;

	/* Return NULL if queue is empty or p is NULL */
	if (NULL == p || emptyProcQ(*tp)) return NULL;

	/* If p is the only element in the queue, set queue to empty */
	if (p == *tp) {
		*tp = NULL;
		return p;
	}

	/* Traverse the queue to find p */
	for (iter = (*tp)->p_next; iter != *tp && iter != p; iter = iter->p_next);

	/* If p is found, remove it from the queue */
	if (iter == p) {
		iter->p_prev->p_next = iter->p_next;
		iter->p_next->p_prev = iter->p_prev;
		*tp = (*tp)->p_prev;

		/* Disconnect p from the queue */
		iter->p_next = iter->p_prev = NULL;

		return p;
	}

	return NULL;
}

/***************************************************************
 *  headProcQ - Retrieves the First PCB Without Removing It
 *
 *  Returns a pointer to the first PCB (head) in the queue.
 *
 *  Parameters:
 *    - tp: Tail pointer of the queue.
 *
 *  Returns:
 *    - Pointer to the head PCB.
 *    - NULL if the queue is empty.
 ***************************************************************/
pcb_PTR headProcQ(pcb_PTR tp) {
	/* Return NULL if the queue is empty */
	if (emptyProcQ(tp)) return NULL;

	/* Return a pointer to the first PCB (head) without removing it */
	return tp->p_next;
}

/***************************************************************
 *  emptyChild - Checks if a Process Has Children
 *
 *  Parameters:
 *    - p: Pointer to the process.
 *
 *  Returns:
 *    - 1 (TRUE) if `p` has no children.
 *    - 0 (FALSE) if `p` has at least one child.
 ***************************************************************/
int emptyChild(pcb_PTR p) {
	return (NULL == p->p_child);
}

/***************************************************************
 *  insertChild - Inserts a PCB as a Child of Another PCB
 *
 *  Adds `p` as the first child of `prnt`.
 *
 *  Parameters:
 *    - prnt: Parent PCB.
 *    - p: Child PCB to insert.
 *
 *  If `prnt` has no children, `p` becomes the first child.
 *  Otherwise, `p` is inserted at the front of the sibling list.
 ***************************************************************/	
void insertChild(pcb_PTR prnt, pcb_PTR p) {
	/* Do nothing if parent or child is NULL */
	if (NULL == prnt || NULL == p) return;

	/* Set the parent of the new child */
	p->p_parent = prnt;

	/* If the parent has no children, insert p as the first child */
	if (emptyChild(prnt)) {
		prnt->p_child = p;
		p->p_sib_next = p->p_sib_prev = NULL;
		return;
	}
	/* Insert p at the front of the sibling list */
	p->p_sib_next = prnt->p_child;	/* New child's next sibling is the current first child */
	p->p_sib_next->p_sib_prev = p;	/* Update previous pointer of the old first child */
	prnt->p_child = p;				/* Update parent's first child pointer to p */
	p->p_sib_prev = NULL;			/* New first child has no previous sibling */
}

/***************************************************************
 *  removeChild - Removes the First Child of a Process
 *
 *  This function removes and returns the first child (oldest) 
 *  of the given process `p`. If `p` has no children or is NULL, 
 *  the function returns NULL.
 *
 *  Parameters:
 *    - p: Pointer to the parent PCB.
 *
 *  Returns:
 *    - Pointer to the removed child PCB.
 *    - NULL if `p` is NULL or has no children.
 ***************************************************************/
pcb_PTR removeChild(pcb_PTR p) {
	/* Return NULL if no children */
	if (NULL == p || emptyChild(p)) return NULL;

	/* Remove and return the first (oldest) child */
	return outChild(p->p_child); 
}

/***************************************************************
 *  outChild - Removes a Specific PCB from the Parent's Children
 *
 *  Removes `p` from its parent's child list.
 *
 *  Parameters:
 *    - p: Pointer to the child PCB to be removed.
 *
 *  Returns:
 *    - Pointer to `p` if successfully removed.
 *    - NULL if `p` is NULL or has no parent.
 ***************************************************************/
pcb_PTR outChild(pcb_PTR p) {
	/* Return NULL if p is NULL or has no parent */
	if (NULL == p || NULL == p->p_parent) return NULL;

	/* If p is the first child, update the parent's child pointer */
	if (p->p_parent->p_child == p) 
		p->p_parent->p_child = p->p_sib_next;

	/* Update sibling pointers to remove p from the sibling list */
	if (p->p_sib_prev != NULL)
		p->p_sib_prev->p_sib_next = p->p_sib_next;

	if (p->p_sib_next != NULL)
		p->p_sib_next->p_sib_prev = p->p_sib_prev;

	/* Disconnect p from the parent and sibling links */
	p->p_sib_next = p->p_sib_prev = p->p_parent = NULL;

	return p;
}