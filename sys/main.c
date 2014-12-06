/* user.c - main */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>

void halt();

/*------------------------------------------------------------------------
 *  main  --  user main program
 *------------------------------------------------------------------------
 */

void testSC_func()
{
		int PAGE0 = 0x40000;
		int i,j,temp;
		int addrs[1200];
		int cnt = 0; 
		//can go up to  (NFRAMES - 5 frames for null prc - 1pd for main - 1pd + 1pt frames for this proc)
		//frame for pages will be from 1032-2047
		int maxpage = (NFRAMES - (5 + 1 + 1 + 1));

		srpolicy(SC); 

		for (i=0;i<=maxpage/100;i++){
				if(get_bs(i,100) == SYSERR)
				{
						kprintf("get_bs call failed \n");
						return;
				}
				if (xmmap(PAGE0+i*100, i, 100) == SYSERR) {
						kprintf("xmmap call failed\n");
						return;
				}
				for(j=0;j < 100;j++)
				{  
						//store the virtual addresses
						addrs[cnt++] = (PAGE0+(i*100) + j) << 12;
				}			
		}

		/* all of these should generate page fault, no page replacement yet
		   acquire all free frames, starting from 1032 to 2047, lower frames are acquired first
		   */
		for(i=0; i < maxpage; i++)
		{  
				*((int *)addrs[i]) = i + 1; 
		}


		//trigger page replacement, this should clear all access bits of all pages
		//expected output: frame 1032 will be swapped out
		*((int *)addrs[maxpage]) = maxpage + 1; 
		

		for(i=1; i <= maxpage; i++)
		{

				if (i != 500)  //reset access bits of all pages except this
					*((int *)addrs[i])= i+1;
				//kprintf(" value of i %d \n",i);  

		}
		//Expected page to be swapped: 1032+500 = 1532
		*((int *)addrs[maxpage+1]) = maxpage + 2;  
		temp = *((int *)addrs[maxpage+1]);


		/*for (i=0;i<=maxpage/100;i++){
				xmunmap(PAGE0+(i*100));
				release_bs(i);			
		}*/

}
/*------------------------------------------------------------------------
 *  main  --  user main program
 *------------------------------------------------------------------------
 */
 
int main()
{
	int pid1;


		kprintf("\nTest FIFO page replacement policy\n");

		pid1 = create(testSC_func, 2000, 20, "testSC_func5", 0 , NULL);

		resume(pid1);
		sleep(10);
		//kill(pid1);
}