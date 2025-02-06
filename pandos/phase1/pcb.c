/******************************* pcb.c ***************************************
 *
 *  Module: Process Control Block (PCB) Management
 *
 *  This module implements the management of Process Control Blocks (PCBs).
 *  PCBs represent individual processes and contain links for both queue 
 *  management and hierarchical parent-child relationships.
 *
 *  The module provides:
 *  
 *  - A free list (Stack) of unused PCBs, allowing dynamic allocation and deallocation.
 *  - FIFO process queues (Queue) for scheduling and process management.
 *  - Parent-child relationships (Tree), supporting hierarchical process structures.
 *
 *  Data Structures Used:
 *  
 *  - Stack The free list is implemented as a NULL-terminated singly linked list, 
 *    where newly freed PCBs are pushed onto the stack and allocated PCBs are popped off.
 *  - Queue: Process queues are implemented as circular doubly linked lists, using 
 *    a tail pointer for efficient insertion and removal (FIFO order).
 *  - Tree: Parent-child relationships are represented as a tree structure, where 
 *    each parent maintains a linked list of its children.
 *
 *  This module ensures:
 *  
 *  - Proper initialization of the free list (`initPcbs`).
 *  - Safe allocation (`allocPcb`) and deallocation (`freePcb`) of PCBs.
 *  - Efficient insertion (`insertProcQ`) and removal (`removeProcQ`, `outProcQ`) 
 *    from process queues.
 *  - Hierarchical process management (`insertChild`, `removeChild`, `outChild`).
 *
 *****************************************************************************/


#include "../h/pcb.h"

/* Head of the free PCB list (stores unused PCBs) */
HIDDEN pcb_PTR pcbFree_h;

/***************************************************************
 *  initPcbs - Initializes the Free PCB List
 *  
 *  This function initializes the PCB free list by linking 
 *  all `MAXPROC` PCBs into a NULL-terminated singly linked list. 
 *  This ensures that all PCBs are available for allocation at 
 *  system startup.
 * 
 *  - The function must be called once during system initialization.
 *  - It uses a static array to store all PCBs since dynamic 
 *    allocation (e.g., `malloc()`) is not available.
 * 
 *  Parameters:
 *    - None
 ***************************************************************/
void initPcbs(void) {
	int i;
	static pcb_t pcbPool[MAXPROC];	/* Statically allocated PCB pool */
	pcbFree_h = NULL;				/* Ensure list starts empty */

	/* Link all PCBs into the free list */
	for (i = 0; i < MAXPROC; ++i) {
		pcbPool[i].p_next = pcbFree_h;	/* New PCB points to the current head */
		pcbFree_h = &pcbPool[i];		/* Move head to the new PCB */
    }		

}


/***************************************************************
 *  freePcb - Returns a PCB to the Free List
 *
 *  This function inserts a PCB (`p`) back into the free list, 
 *  making it available for reuse.
 *
 *  - The PCB is added to the head of `pcbFree_h`, maintaining 
 *    the Last-In-First-Out (LIFO) order (stack behavior).
 *  - If `p` is `NULL`, the function does nothing.
 *
 *  Parameters:
 *    - p: Pointer to the PCB to be freed.
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
 *  This function removes and returns the first available PCB 
 *  from `pcbFree_h`. If no PCBs are available, it returns `NULL`.
 *
 *  - The removed PCB is disconnected from the free list.
 *  - All its fields are reset to default values before use.
 *
 *  Parameters:
 *    - None
 *
 *  Returns:
 *    - Pointer to the allocated PCB.
 *    - NULL if no PCBs are available.
 ***************************************************************/
pcb_PTR allocPcb(void) {
	pcb_PTR pcbRm;
	pcbRm = pcbFree_h; /* Get the first available PCB */

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
 *  This function initializes a new process queue by returning 
 *  `NULL`, which represents an empty queue.
 *
 *  The queue is a doubly circularly linked list, and a tail 
 *  pointer (`tp`) is used to track its last element.
 *  
 *  Parameters:
 *    - None
 *
 *  Returns:
 *    - NULL (indicating an empty queue).
 ***************************************************************/
pcb_PTR mkEmptyProcQ(void) {
    return (NULL);
}

/***************************************************************
 *  emptyProcQ - Checks if a Process Queue is Empty
 *
 *  This function checks if the process queue is empty by verifying 
 *  whether `tp` (tail pointer) is `NULL`.
 *
 *  Parameters:
 *    - tp: Pointer to the tail of the queue.
 *
 *  Returns:
 *    - 1 (TRUE) if the queue is empty.
 *    - 0 (FALSE) if the queue contains processes.
 ***************************************************************/
int emptyProcQ(pcb_PTR tp) {
	return (NULL == tp);
}

/***************************************************************
 *  insertProcQ - Inserts a PCB into a Process Queue (FIFO)
 *
 *  This function inserts a PCB (`p`) into the doubly circularly 
 *  linked process queue whose tail pointer is `tp`.
 *
 *  - If the queue is empty, `p` becomes both the head and tail.
 *  - Otherwise, `p` is inserted after the current tail, 
 *    ensuring FIFO (First-In-First-Out) order.
 *
 *  Parameters:
 *    - tp: Pointer to the tail of the queue.
 *    - p:  PCB to be inserted.
 ***************************************************************/
void insertProcQ(pcb_PTR *tp, pcb_PTR p) {
	/* Ignore NULL process */
	if (NULL == p || NULL == tp) return;

	/* If the queue is empty, initialize it with the new process */
	if (emptyProcQ(*tp)) {
		*tp = p;
		p->p_prev = p->p_next = p;	/* Circular list: p points to itself */
        return;
	}

	/* Insert new PCB at the tail */
	p->p_next = (*tp)->p_next;		/* New PCB's next points to the head */
	p->p_prev = *tp;				/* New PCB's prev points to the current tail */
	(*tp)->p_next->p_prev = p;		/* Update the head's prev to point to new PCB */
	(*tp)->p_next = p;				/* Update the tail's next to point to new PCB */
	*tp = p;						/* Update tail pointer to the newly inserted PCB */
}
	
/***************************************************************
 *  removeProcQ - Removes the Head PCB from the Queue
 *
 *  This function removes the first PCB (head) from the process 
 *  queue whose tail pointer is `tp`.
 *
 *  - If the queue is empty, it returns `NULL`.
 *  - Otherwise, it removes the head of the queue and updates 
 *    the queue's structure accordingly.
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
	if (emptyProcQ(*tp) || NULL == tp) return NULL;
	
	/* Remove and return the first PCB (head) from the queue */
	return outProcQ(tp, headProcQ(*tp));
}

/***************************************************************
 *  outProcQ - Removes a Specific PCB from a Process Queue
 *
 *  This function removes a specific PCB (`p`) from the doubly 
 *  circularly linked process queue whose tail pointer is `tp`.
 *
 *  - If `p` is not in the queue, the function returns `NULL`.
 *  - If `p` is found, it is removed while maintaining the 
 *    circular structure of the queue.
 *  - If `p` was the tail, the tail pointer is updated.
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

	/* Case 1: Queue is empty, tp is NULL, or p is NULL */
	if (NULL == p || emptyProcQ(*tp) || NULL == tp) return NULL;

	/* Case 2: p is the only element in the queue */
	if (*tp == (*tp)->p_next) {
		if (*tp == p) {	/* Only element matches */
			*tp = NULL;	/* Queue is now empty */
			return p;
		}

		return NULL;	/* p was not found in the queue */
	}

	/* Case 3: p is the tail of the queue */
	if (p == *tp) {
		(*tp)->p_prev->p_next = (*tp)->p_next;	/* Bypass p in forward direction */
		(*tp)->p_next->p_prev = (*tp)->p_prev;	/* Bypass p in backward direction */

		iter = *tp;								/* Use iter as dummy for resetting links */
		iter->p_next = iter->p_prev = NULL;

		*tp = (*tp)->p_prev;					/* Update tail pointer */

		return p;
	}

	/* Case 4: p is somewhere in the queue (not the tail) */
	for (iter = (*tp)->p_next; iter != *tp && iter != p; iter = iter->p_next);

	/* If p is found, remove it from the queue */
	if (iter == p) {
		iter->p_prev->p_next = iter->p_next;	/* Bypass p in forward direction */
		iter->p_next->p_prev = iter->p_prev;	/* Bypass p in backward direction */

		/* Reset removed pcb's links */
		iter->p_next = iter->p_prev = NULL;

		/* Return the removed PCB */
		return p;
	}

	/* Case 5: p was not found in the queue */
	return NULL;
}


/***************************************************************
 *  headProcQ - Retrieves the First PCB Without Removing It
 *
 *  This function returns a pointer to the first PCB (head) 
 *  in the queue without removing it.
 *
 *  - If the queue is empty, it returns `NULL`.
 *  - Otherwise, it returns the first PCB pointed to by `tp->p_next`.
 *
 *  Parameters:
 *    - tp: Tail pointer of the queue.
 *
 *  Returns:
 *    - Pointer to the first PCB (head).
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
 *  This function checks whether a given process (`p`) has any 
 *  children by verifying if `p->p_child` is `NULL`.
 *
 *  The process tree is implemented as a doubly linked list, 
 *  where each process keeps track of its first child. If a 
 *  process has no children, `p_child` is `NULL`.
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
 *  This function adds `p` as the **first child** of the parent 
 *  process `prnt`, forming a parent-child relationship.
 *
 *  - The linked list of children is implemented as a Stack, 
 *    meaning new children are always added to the front
 *    (Last-In-First-Out, LIFO order).
 *  - If `prnt` has no children, `p` becomes the first child.
 *  - Otherwise, `p` is inserted at the front of the sibling list.
 *  - Each child has doubly linked sibling pointers (`p_sib_next` 
 *    and `p_sib_prev`) for efficient removal.
 *
 *  Parameters:
 *    - prnt: Pointer to the parent PCB.
 *    - p:    Pointer to the child PCB to insert.
 ***************************************************************/
	
void insertChild(pcb_PTR prnt, pcb_PTR p) {
	/* Do nothing if parent or child is NULL */
	if (NULL == prnt || NULL == p) return;

	/* Set the parent of the new child */
	p->p_parent = prnt;

	/* If the parent has no children, insert p as the first child */
	if (emptyChild(prnt)) {
		prnt->p_child = p;
		p->p_sib_next = p->p_sib_prev = NULL;	/* No siblings */
		return;
	}
	/* Insert p at the front of the sibling list*/
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
 *  the function returns `NULL`.
 *
 *  - The first child is always stored in `p->p_child`.
 *  - Calls `outChild` to remove the first child from the list.
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
 *  This function removes a specific child process (`p`) from its 
 *  parent's list of children, regardless of its position in 
 *  the sibling list.
 *
 *  - If `p` has no parent, the function returns `NULL`.
 *  - If `p` is the first child, `p_parent->p_child` is updated.
 *  - If `p` is in the middle or end, its sibling pointers 
 *    (`p_sib_next`, `p_sib_prev`) are adjusted to maintain the 
 *    doubly linked list structure.
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
		p->p_parent->p_child = p->p_sib_next;	/* The next sibling becomes the first child */

	/* Update sibling pointers to remove p from the sibling list */
	if (p->p_sib_prev != NULL)
		p->p_sib_prev->p_sib_next = p->p_sib_next;	/* Skip p in the previous sibling's next */

	if (p->p_sib_next != NULL)
		p->p_sib_next->p_sib_prev = p->p_sib_prev;	/* Skip p in the next sibling's previous */

	/* Disconnect p from the parent and sibling links */
	p->p_sib_next = p->p_sib_prev = p->p_parent = NULL;

	/* Return the removed child */
	return p;
}