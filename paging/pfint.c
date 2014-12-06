/* isr on page fault */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>
#include <stdio.h>

SYSCALL pfint() {
	STATWORD ps;
	unsigned long cr2;
	virt_addr_t * vraddr;
	unsigned int pd_offset;
	unsigned int pt_offset;
	unsigned int pg_offset;
	struct pentry * pptr;
	pd_t * pd;
	pt_t * pt;
	int store;
	int pageth;
	int frame; 
	int pgtbl_offset;
	bs_map_t * bsmap;
	disable(ps);
	
	cr2 = read_cr2();
	 
	//decoding virtual address
	vraddr = (virt_addr_t *)cr2; 
	//kprintf("cr2 value %d\n",cr2);
	pd_offset = cr2 >> 22;
	pt_offset = cr2 >> 12 & 1023;
	pg_offset = cr2 & 1023;
	
	//extract page dir 
	pptr = &proctab[currpid];
	pd = pptr->pdbr;

	//check for page table address in pg dir
	if (pd[pd_offset].pd_pres != 1) {
		pt = create_pt();	//return physical address of page table
		
		pd[pd_offset].pd_pres = 1;
		pd[pd_offset].pd_write = 1;	
		pd[pd_offset].pd_base = ((unsigned int)pt)/NBPG;		
	}
	else
		pt = (pd[pd_offset].pd_base)*NBPG;

	//check if the page referred to is already in memory
	if (bsm_lookup(currpid, vraddr, &store, &pageth) == SYSERR) {
		//kprintf("back store mapping not found !\n");
		kill(currpid);
		restore(ps);
		return SYSERR;
	}
	
	//kprintf("in pagefault : got bs store id %d\n",store);
	
	//finding frame in page table
	pgtbl_offset = (cr2/NBPG) - pageth;
	
	frame = frame_lookup(store,pgtbl_offset);
	
	if ( frame == -1) {	//frame doesn't exist in table
		//get a free frame
		frame = FR_PAGE;
		get_frm(&frame);
		if ((frame) == -1) {
			kprintf("frame unavailable\n");
			restore(ps);
			return OK;
		}
		
		//update frame values
		frm_tab[frame].fr_pid = currpid;
		frm_tab[frame].fr_refcnt = 1;
		frm_tab[frame].fr_type = FR_PAGE;
		frm_tab[frame].fr_vpno = ((unsigned int)vraddr)/NBPG;

		//bring frame from BS
		read_bs((frame+FRAME0)*NBPG,store,pageth);

	}
	else {
		//kprintf("in reference count incerease \n");
		
		frm_tab[frame].fr_refcnt++;
	}
		
	//check for page entry in page table
	pt[pt_offset].pt_pres = 1;
	pt[pt_offset].pt_write = 1;
	pt[pt_offset].pt_base = frame + FRAME0;
	//pt[pt_offset].pt_acc = 1;
	
	//set pdbr again??

	restore(ps);
	return OK;
}

