/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

//array of backing store entries
bs_map_t bsm_tab[MAX_ID];

/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_bsm()
{
	//kprintf("In bsm.c : init_bsm() - Initializing backing store\n");

	int i;
	for (i = 0; i <= MAX_ID; i++) {
		bsm_tab[i].bsid = i;
		bsm_tab[i].bs_status = BSM_UNMAPPED;
		bsm_tab[i].bs_pid = 0;
		bsm_tab[i].bs_vpno = 0;
		bsm_tab[i].bs_npages = BACKING_STORE_UNIT_SIZE/NBPG;
		bsm_tab[i].bs_sem = 0;
		bsm_tab[i].nextMap = NULL;
		bsm_tab[i].bs_isheap = 0;
	}
	return OK;
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
	
	int heapSize = *avail;
	
	if (heapSize >= BACKING_STORE_UNIT_SIZE/NBPG)
		return SYSERR;

	int i;
	for(i = 0; i <= MAX_ID; i++) {
		if (bsm_tab[i].bs_status == BSM_UNMAPPED) {
			*avail = i; //pass i
			//kprintf("In bsm.c : get_bsm() - returns free BS %d\n",i);
			return OK;
		}
	}
	return SYSERR;
} 


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{

	bs_map_t * bsmap;
	bsmap = &bsm_tab[i];
	bsmap->bsid = i;
		bsmap->bs_status = BSM_UNMAPPED;
		bsmap->bs_pid = 0;
		bsmap->bs_vpno = 0;
		bsmap->bs_npages = BACKING_STORE_UNIT_SIZE/NBPG;
		bsmap->bs_sem = 0;
		bsmap->nextMap = NULL;
		bsmap->bs_isheap = 0;
		
	return OK;
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{
	// kprintf("In bsm.c : lookup() - backing store lookup \n");
	STATWORD ps;
	int i;
	bs_map_t * bsmap;
	bs_map_t * curr;
	
	disable(ps);

	for (i = 0; i < MAX_ID; i++) {
		
		bsmap = &bsm_tab[i];
		
		if(bsmap->bs_status == BSM_UNMAPPED) 
			continue;
		
		curr = bsmap->nextMap;
		while(curr != NULL) {
			if (curr->bs_pid == pid && (vaddr/NBPG) >= curr->bs_vpno && (vaddr/NBPG) < (curr->bs_vpno + curr->bs_npages)) {
				*store = i;
				*pageth = curr->bs_vpno;
				restore(ps);
				return OK;
			}
			curr = curr->nextMap;
		}
	}
	restore(ps);
	return SYSERR;
}

/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
	//kprintf("In bsm.c : bsm_map() - add a mapping\n ");
	bs_map_t * bsmap_new;
	bs_map_t * bsmap_old;

	bsmap_old = &bsm_tab[source];
	
	bsmap_new = (bs_map_t *) getmem(sizeof(bs_map_t));
	if(!bsmap_new)
		return SYSERR;

	bsmap_new->bsid = source;
	bsmap_new->bs_status = BSM_MAPPED;
	bsmap_new->bs_pid = pid;
	bsmap_new->bs_vpno = vpno;
	bsmap_new->bs_npages = npages;
	
	//change list pointer values
	bsmap_new->nextMap = bsmap_old->nextMap;
	bsmap_old->nextMap = bsmap_new;
	//kprintf("In bsm.c : bsm_map() - mapping added with vpno %d\n",bsmap_new->bs_vpno);
	return OK;
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
	//kprintf("In bsm.c: bsm_unmap() - delete a mapping\n");
	STATWORD ps;
	int i;
	bs_map_t * bsmap;
	bs_map_t * curr;
	bs_map_t * prev;
	
	disable(ps);

	for (i = 0; i < MAX_ID; i++) {
		
		bsmap = &bsm_tab[i];
		
		if(bsmap->bs_status == BSM_UNMAPPED) 
			continue;
		
		prev = bsmap;
		curr = bsmap->nextMap;
		while(curr != NULL) {
			if (curr->bs_pid == pid && vpno >= curr->bs_vpno && vpno < (curr->bs_vpno + curr->bs_npages)) {
				if(vpno == curr->bs_vpno) {
					prev->nextMap = curr->nextMap;
					freemem((struct mblock*)curr,sizeof(bsmap));
					// kprintf("there should be match\n"); 
				}
				else {
					curr->bs_npages = curr->bs_npages - vpno;
				}
				restore(ps);
				return OK;
			}
			prev = curr;
			curr = curr->nextMap;
		}
	}
	restore(ps);
	return SYSERR;
}

/*-------------------------------------------------------------------------
 * kill_process - releases frames
 *-------------------------------------------------------------------------
 */
 int kill_process(int pid) {
	
	int i;
	bs_map_t * bsmap;
	bs_map_t * prev;
	bs_map_t * curr;
	int pdbr,frame;
	STATWORD ps;
	disable(ps);
	for (i = 0; i < MAX_ID; i++) {
		
		bsmap = &bsm_tab[i];
		
		if(bsmap->bs_status == BSM_UNMAPPED) 
			continue;
		
		prev = bsmap;
		curr = bsmap->nextMap;
		
		while(curr != NULL) {
			if (curr->bs_pid == pid ) {
				xmunmap(curr->bs_vpno);
				
			}
			prev = curr;
			curr = curr->nextMap;
		}
	}
		pdbr = &proctab[pid].pdbr;
		frame = (pdbr/NBPG)-FRAME0;
		//release frame of page directory
		free_frm(frame);
	restore(ps);
	return OK;
 }
 
 
 
 
 
 
 
 
 
 
 
 
 
 
