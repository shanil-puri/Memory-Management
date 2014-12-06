/* page.c - manage page tables, page dir and bs pages */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>
#include <stdio.h>

//array of page table entries
pt_t * pt_tab[4];

/*-------------------------------------------------------------------------
 * init_page_table - initialize global page table pt_tab[i]
 *-------------------------------------------------------------------------
 */
int init_page_table() {
	
	//kprintf(" page.c : init_page_table() \n");
	int i;
	pt_t * pt;
	for (i = 0; i < 4; i++) {
		pt_tab[i] = create_pt();
		if (pt_tab[i] == NULL) {
			kprintf("page table creation unsuccessful..\n");
			return SYSERR;
		}
		
		int j;
		pt = pt_tab[i];
		for (j = 0; j < (NBPG/4); j++) {
			pt[j].pt_pres = 1;
			pt[j].pt_write = 1;
			pt[j].pt_base = (i*(NBPG/4)) + j;
		}	
	}
	//kprintf(" page.c : First 4 page tables initialized \n");
	return OK;
}

/*-------------------------------------------------------------------------
 * create_pt - returns page table address
 *-------------------------------------------------------------------------
 */
pt_t * create_pt() {
	
	int frame;
	frame = FR_TBL;
	get_frm(&frame);
	
	if ((frame) == -1) 
		return NULL;
		
	pt_t * pgtbl;

	frm_tab[frame].fr_type = FR_TBL;
	
	pgtbl = (pt_t *)(((frame)+FRAME0)*NBPG);
	
	int i;
	//initialize page table with number of entries = pagesize(4096 bytes)/size of page table entry (4 bytes)
	for (i = 0; i < (NBPG/4); i++) {
		pgtbl[i].pt_pres = 0;
		pgtbl[i].pt_write = 0;
		pgtbl[i].pt_user = 0;
		pgtbl[i].pt_pwt = 0;
		pgtbl[i].pt_pcd = 0;
		pgtbl[i].pt_acc = 0;
		pgtbl[i].pt_dirty = 0;
		pgtbl[i].pt_mbz = 0;
		pgtbl[i].pt_global = 0;
		pgtbl[i].pt_avail = 0;
		pgtbl[i].pt_base = 0;
	}	
	
	//kprintf("In page.c : create_pt()- page table created at frame location %d\n",((*frame)+FRAME0));
	return pgtbl;
}

/*-------------------------------------------------------------------------
 * init_page_dir - initialize page dir
 *-------------------------------------------------------------------------
 */
pd_t * init_page_dir(int pid) {

	int i;
	pd_t * pd;
	pd = create_pd(pid);
	if (pd == NULL) {
		kprintf("page directory creation unsuccessful..\n");
		return NULL;
	}
		
	int j;
	for (j = 0; j < 4; j++) {
		pd[j].pd_pres = 1;
		pd[j].pd_write = 1;	
		pd[j].pd_base = ((unsigned int)pt_tab[j])/NBPG;
		//kprintf(" In page.c: init_page_dir() - %d th page directory points to page table %d\n",j,pd[j].pd_base);
	}	
	
	return pd;
}

/*-------------------------------------------------------------------------
 * create_pd - returns page dir address
 *-------------------------------------------------------------------------
 */
pd_t * create_pd(int pid) {

	int frame;
	frame = FR_DIR;
	get_frm(&frame);
	
	if ((frame) == -1) 
		return NULL;
		
	pd_t * pgdir;

	frm_tab[frame].fr_type = FR_DIR;
	frm_tab[frame].fr_pid = pid;
	pgdir = (((frame)+FRAME0)*NBPG);
	
	int i;
	//initialize page dir with number of entries = pagesize(4096 bytes)/size of page table entry (4 bytes)
	for (i = 0; i < (NBPG/4); i++) {
		pgdir[i].pd_pres = 0;
		pgdir[i].pd_write = 0;
		pgdir[i].pd_user = 0;
		pgdir[i].pd_pwt = 0;
		pgdir[i].pd_pcd = 0;
		pgdir[i].pd_acc = 0;
		pgdir[i].pd_mbz = 0;
		pgdir[i].pd_fmb = 0;
		pgdir[i].pd_global = 0;
		pgdir[i].pd_avail = 0;
		pgdir[i].pd_base = 0;
	}	
	
	//kprintf("In page.c: create_pd() - page directory created at frame location %d\n",((*frame)+FRAME0));
	
	return pgdir;	
}

/*-------------------------------------------------------------------------
 *  reset page table entries when releasing frame
 *-------------------------------------------------------------------------
 */
 int set_pagetable_entry(int frameId) {
 	
 	int i;
 	struct pentry * p;
 	pd_t * pagedir;
 	pt_t * pagetbl;
 	int flag = 0;
 	for (i = 0; i<NPROC; i++) {
 		p = &proctab[i];
 		
 		if(p->pstate == PRFREE) continue;
 		
 		pagedir = p->pdbr;
 		int k;
 		for(k=4;k<(NBPG/4);k++) {
 			if(pagedir[k].pd_pres != 0) {
 				flag = 0;
 				pagetbl = pagedir[k].pd_base * NBPG;
 				
 				int j;
 				for (j=0;j<(NBPG/4);j++) {
 					if(pagetbl[j].pt_pres != 0) {
 						if(pagetbl[j].pt_base == (frameId + FRAME0)) {
 						    //kprintf("in the end frame num is %d\n",frameId);
 							pagetbl[j].pt_pres = 0;
							pagetbl[j].pt_base = 0;
 						}
 					}
 				}
 				for (j=0;j<(NBPG/4);j++)
 					if(pagetbl[j].pt_pres != 0) 
 						flag = 1;
 				
 				if (flag != 1) 
 					free_pagetbl(pagetbl,i);
 			}
 		}
 		
 	}
 	return OK;
 }
 
/*-------------------------------------------------------------------------
 *  free a page table
 *-------------------------------------------------------------------------
 */
 free_pagetbl(pt_t * pt,int pid) {
 	//kprintf("in free page table\n");
 	
 	unsigned int frame = ((unsigned int)pt)/NBPG;
 	struct pentry * p;
 	pd_t * pagedir;
 	p = &proctab[pid]; 
 	pagedir = p->pdbr;
 	int k;
 	for(k=4;k<(NBPG/4);k++) {
 		if(pagedir[k].pd_pres != 0) {
 			if(pagedir[k].pd_base == frame) {
 				// kprintf("frame value of table is %d\n",frame);
 				pagedir[k].pd_base = 0;
 				pagedir[k].pd_pres = 0;
 				free_frm(frame-FRAME0);
 			}
 		}
 	}
 	
 }
 