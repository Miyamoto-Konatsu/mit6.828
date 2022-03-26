#include "ns.h"
#include <inc/lib.h>
extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";
	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
	int from_envid, perm, nsreq_value, r;
	uint32_t len;
	while(1) {
		nsreq_value = ipc_recv(&from_envid, &nsipcbuf, &perm);
		if(nsreq_value != NSREQ_OUTPUT || ns_envid != from_envid) {
			continue;
		}
		len = (uint32_t) nsipcbuf.pkt.jp_len;
		while(1) {
			r = sys_send_packet(&nsipcbuf.pkt.jp_data[0], len);
			if(r == -E_NO_PACKET_MEM) {
				sys_yield();
				continue;
			}
			break;
		}
	}

}
