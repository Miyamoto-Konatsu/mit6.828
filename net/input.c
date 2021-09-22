#include "ns.h"

extern union Nsipc nsipcbuf;
void
input(envid_t ns_envid)
{
	binaryname = "ns_input";
	
	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.

	nsipcbuf.pkt.jp_data[0]=0;

	int r; // error code or packet length
	while (1) {
		r = sys_recv_packet(nsipcbuf.pkt.jp_data);
		if(r < 0) {
			sys_yield();
			continue;
		}
		nsipcbuf.pkt.jp_len = r;
		ipc_send(ns_envid, NSREQ_INPUT, &nsipcbuf, PTE_P | PTE_W | PTE_U);
		sys_yield();
		sys_yield();
		sys_yield();
	}
}
