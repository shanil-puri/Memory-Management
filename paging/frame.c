/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>
#include <stdio.h>

//array of frame entries
fr_map_t frm_tab[NFRAMES];
fr_list * curr;
fr_list * last;

/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_frm()
{
  //kprintf("In frame.c : init_frame() - Initializing Frames\n");
  
  int i;
  for (i = 0;i<NFRAMES;i++)
  {
    frm_tab[i].fr_status = FRM_UNMAPPED;
    frm_tab[i].fr_pid = 0;
    frm_tab[i].fr_vpno = 0;
    frm_tab[i].fr_refcnt = 0;
    frm_tab[i].fr_type = -1;
    frm_tab[i].fr_dirty = -1;
    frm_tab[i].fr_loadtime = 0;
  }
  
  curr = NULL;
  last = NULL;
  
  return OK;
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* avail)
{
  //kprintf("In frame.c : get_frm() - Search for a free frame\n");

  //returns free frame for page table, page directory or page
  int i;
  int frame = -1;
	
  for(i=0; i<NFRAMES; i++)
  {
    if ((*avail == FR_DIR) || (*avail == FR_TBL)) {
	if( i < (1535-NFRAMES)) {
	  if (frm_tab[i].fr_status == FRM_UNMAPPED) {
		frm_tab[i].fr_status = FRM_MAPPED;
		frm_tab[i].fr_refcnt = 1;

	    frame = i;
		//kprintf("In frame.c : frame allocated to process with type %d %d\n",*avail,frame + FRAME0);
		*avail = frame;
		
		return OK;
       }
	 }
    }
    else if (*avail == FR_PAGE) {
	  if (frm_tab[i].fr_status == FRM_UNMAPPED) {
		frm_tab[i].fr_status = FRM_MAPPED;
		frm_tab[i].fr_refcnt = 1;

       	frame = i;
		*avail = frame;
		insertNode(i);
		
		//kprintf("In frame.c : frame allocated to page %d\n",frame + FRAME0);
		return OK;
       }
    }
  }
  
  if(*avail == FR_PAGE) {
  	//kprintf(" No free frame : Apply page replacement policy \n");
	if(grpolicy() == FIFO) 
		frame = FIFO_find_replacement_page();
	if(grpolicy() == LRU) frame = FIFO_find_replacement_page();
	frm_tab[frame].fr_status = FRM_MAPPED;
	frm_tab[frame].fr_refcnt = 1;
  	*avail = frame;
	insertNode(frame);
	return OK; 	
  }
  
  return SYSERR;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{
  //kprintf("Frees %d th frame\n",i);
  int store,pageth;
  //free a frame from memory
  if (i <= 0 || i > NFRAMES) {
  	//kprintf("Invalid request to free a frame !\n");
  	return SYSERR;
  }
  
  
  //reset page table entry
  if(frm_tab[i].fr_type == FR_PAGE) {
  	if (bsm_lookup(frm_tab[i].fr_pid, ((frm_tab[i].fr_vpno) * NBPG), &store, &pageth) != SYSERR) {
		//kprintf("frame part of BS frame num  %d store %d pageth %d!\n",i,store,pageth);
		// write to back store
  		//write_bs((i+FRAME0)*NBPG,store,pageth);
		//kprintf("after writing to bs\n");
  	}
  	set_pagetable_entry(i);
	//kprintf("after setting page table entry \n");
	
	//remove from list
	deleteNode(i);
  }
  //reset frame values
  frm_tab[i].fr_status = FRM_UNMAPPED;
  frm_tab[i].fr_pid = 0;
  frm_tab[i].fr_vpno = 0; 
  frm_tab[i].fr_refcnt = 0;
  frm_tab[i].fr_type = -1;
  frm_tab[i].fr_dirty = -1;
  frm_tab[i].fr_loadtime = 0;
  
  return OK;
}

/*-------------------------------------------------------------------------
 * frame_lookup() - returns if frame exists in memory or not
 *-------------------------------------------------------------------------
 */
 int frame_lookup(int bsid,int offset) {
	
	bs_map_t * bsmap;
	bs_map_t * curr_bsmap;
	fr_map_t * frame;
	int i;
	
	bsmap = &bsm_tab[bsid];
	curr_bsmap = bsmap->nextMap;
	while(curr_bsmap != NULL) {
	  for (i = 0; i < NFRAMES; i++) {
		frame = &frm_tab[i];
		
		if(frame->fr_status == FRM_UNMAPPED || frame->fr_type != FR_PAGE)
			continue;
			
		if (curr_bsmap->bs_pid == frame->fr_pid && ((frame->fr_vpno - curr_bsmap->bs_vpno) == offset)) {
			//kprintf("offset calculate %d\n",frame->fr_vpno - curr_bsmap->bs_vpno);
			
			return i;
		}
	  }
	  curr_bsmap = curr_bsmap->nextMap;
	}	
	return (-1);
 }

/*-------------------------------------------------------------------------
 * release frames
 *-------------------------------------------------------------------------
 */
int release_frames(int bsid, int offset) {
	bs_map_t * bsmap;
	bs_map_t * curr_bsmap;
	fr_map_t * frame;
	int i;
	int count;
	
	bsmap = &bsm_tab[bsid];
	curr_bsmap = bsmap->nextMap;
	while(curr_bsmap != NULL) {
	  for (i = 0; i < NFRAMES; i++) {
		frame = &frm_tab[i];
		
		if(frame->fr_status == FRM_UNMAPPED || frame->fr_type != FR_PAGE)
			continue;
			
		if (curr_bsmap->bs_pid == frame->fr_pid && ((frame->fr_vpno - curr_bsmap->bs_vpno) == offset)) {
			//kprintf("offset calculate %d\n",frame->fr_vpno - curr_bsmap->bs_vpno);
			//kprintf("frame + pages %d\n",((frame->fr_vpno)+(curr_bsmap->bs_npages)));
			//kprintf("current frame %d\n",frame->fr_vpno);
			for (count=0; count<((curr_bsmap->bs_npages)-offset); count++) {
				if((i+count+FRAME0) < 2048) {
				//kprintf("in frame.c : call to decrement count() %d\n",i+count+FRAME0);
				decrement_framecount(i+count); }
			}
		}
	  }
	  curr_bsmap = curr_bsmap->nextMap;
	}	
	return (-1);
}

/*-------------------------------------------------------------------------
 * clear or reduce reference count for frames
 *-------------------------------------------------------------------------
 */
int decrement_framecount(int frameId) {
	fr_map_t * frame;
	frame = &frm_tab[frameId];
	
	if(frame->fr_status != FRM_UNMAPPED) {
		if(frame->fr_refcnt > 0)
			frame->fr_refcnt--;
		//kprintf("reference count for frame %d\n",frame->fr_refcnt);
		if(frame->fr_refcnt == 0) 
			free_frm(frameId);
	}
	return OK;
}

/*-------------------------------------------------------------------------
 * create a node
 *-------------------------------------------------------------------------
 */
 fr_list* createNode(int frame) {
 	fr_list *ptr = getmem(sizeof(fr_list));
 	ptr->fr_id = frame;
 	ptr->prev = NULL;
 	ptr->next = NULL;
 	return(ptr);
 }
 
 /*-------------------------------------------------------------------------
 * insert a node
 *-------------------------------------------------------------------------
 */
 void insertNode(int frame) {
 	fr_list *temp = getmem(sizeof(fr_list));
 	fr_list *ptr = createNode(frame);
 	//kprintf(" inserting node in list %d\n",frame+FRAME0);
 	if(!curr) {
 		curr = ptr;
 		curr->next = curr;
 		curr->prev = curr;
 		last = curr;
 		return;
 	}
 	else {
 			ptr->next = curr;;
 			ptr->prev = curr->prev;
 			curr->prev->next = ptr;
 			curr->prev = ptr;
 	}
 }
 
 
 /*-------------------------------------------------------------------------
 * SC page replacement policy
 *-------------------------------------------------------------------------
 */
 int FIFO_find_replacement_page() {
 	
 	//kprintf("curr frame id %d\n",curr->fr_id);
 	fr_map_t * frame;
 	STATWORD ps;
 	int store, pageth;
	bs_map_t * bsmap;
	bs_map_t * curr_bsmap;
 	int pid;
	struct pentry * p;
	pd_t * pagedir;
	int flag;
	pt_t * pagetbl;
	int frameId;
	disable(ps);
	
 	while(curr) {
 		
		frame = &frm_tab[curr->fr_id];
		frameId = curr->fr_id;
		//kprintf("value of vpno n addr %d %d \n",frame->fr_vpno,((frame->fr_vpno)*NBPG));
 		if (bsm_lookup(frame->fr_pid, ((frame->fr_vpno)*NBPG), &store, &pageth) == SYSERR) {
		//kprintf("In replacement back store mapping not found !\n");
		//kill(currpid);
		restore(ps);
		return SYSERR;
		}
		
		bsmap = &bsm_tab[store];
		curr_bsmap = bsmap->nextMap;
		flag = 0;
		
		while(curr_bsmap != NULL) {
			pid = curr_bsmap->bs_pid;
			p = &proctab[pid];
			pagedir = p->pdbr;
			int k;
			for(k=4;k<(NBPG/4);k++) {
				if(pagedir[k].pd_pres != 0) {
					
					pagetbl = pagedir[k].pd_base * NBPG;
					
					int j;
					for (j=0;j<(NBPG/4);j++) {
						if(pagetbl[j].pt_pres != 0) {
							if(pagetbl[j].pt_base == (curr->fr_id + FRAME0)) {
								
								if(pagetbl[j].pt_acc == 1) {
									pagetbl[j].pt_acc = 0;
									flag = 1;
								}
							}
						}
					}					
				}
			}			
			curr_bsmap = curr_bsmap->nextMap;
		}
		
		//kprintf("flag  value is %d \n",flag);
		curr = curr->next;
		if (flag != 1) {
			free_frm(frameId);
			kprintf("\nFrame %d is replaced\n",frameId+FRAME0);
			restore(ps);
			return(frameId);
		}		
	}
	restore(ps);
	return SYSERR;
 }
 
  /*-------------------------------------------------------------------------
 * delete a node from list
 *-------------------------------------------------------------------------
 */
 void deleteNode(int frame) {
	fr_list *ptr, *temp = curr;
	if (curr == NULL) {
		kprintf("Data unavailable\n");
        return; 
	}
	else {
		while(temp->prev != curr && temp->fr_id != frame) {
			ptr = temp;
			temp = temp->prev;
		}
		ptr->prev = temp->prev;
		temp->prev->next = temp->next;
		freemem((struct mblock *)temp, sizeof(fr_list));
	}
 }
 
 
 
 