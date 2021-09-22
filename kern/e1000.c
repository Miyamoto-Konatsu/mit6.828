#include <inc/error.h>
#include <inc/string.h>
#include <inc/types.h>
#include <kern/e1000.h>
#include <kern/pmap.h>
// LAB 6: Your driver code here
#define TX_DESC_QUEUE_BYTES 256
#define RX_DESC_QUEUE_BYTES 4096

int tx_desc_struct_length = sizeof(struct tx_desc);
int rx_desc_struct_length = sizeof(struct rx_desc);

int tx_desc_queue_legnth;
int rx_desc_queue_legnth;

volatile uint32_t *reg_base = 0;
struct tx_desc *tx_desc_queue_base;
struct rx_desc rx_desc_queue_base[128]__attribute__((aligned(PGSIZE)));
// struct tx_desc * tx_desc_queue_base[16]__attribute__((aligned(128)));
static unsigned char e1000_send_packet_buf[16][PGSIZE]
    __attribute__((aligned(PGSIZE)));
static unsigned char e1000_recv_packet_buf[128][PGSIZE]
    __attribute__((aligned(PGSIZE)));


// packet descriptor size 16
// packet descriptor queue size 128
// max packct content size 1518
int pci_e1000_send_init(struct pci_func *pcif);
int pci_e1000_recv_init(struct pci_func *pcif);


int pci_e1000_attach(struct pci_func *pcif) {
    int r;

    pci_func_enable(pcif);
    reg_base = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
    cprintf("E1000 status register %x\n",
            reg_base[E1000_REG_TO_NUM(E1000_STATUS)]);

    if ((r = pci_e1000_send_init(pcif)))
        panic("Error occurs in pci_e1000_attach : %e", r);

    reg_base[E1000_REG_TO_NUM(E1000_TCTL)] |= E1000_TCTL_EN | E1000_TCTL_PSP;
    reg_base[E1000_REG_TO_NUM(E1000_TCTL)] |=
        (E1000_TCTL_CT & (0x10 << 4)) | (E1000_TCTL_COLD & (0x40 << 12));

    reg_base[E1000_REG_TO_NUM(E1000_TIPG)] = 10;
    reg_base[E1000_REG_TO_NUM(E1000_TIPG)] |= 8 << 10;
    reg_base[E1000_REG_TO_NUM(E1000_TIPG)] |= 12 << 20;
    pci_e1000_recv_init(pcif);
    return 0;
}

int pci_e1000_send_init(struct pci_func *pcif) {
    tx_desc_queue_legnth = TX_DESC_QUEUE_BYTES / tx_desc_struct_length;

    struct PageInfo *phy_page = page_alloc(ALLOC_ZERO);
    if (phy_page == NULL)
        return -E_NO_MEM;
    tx_desc_queue_base =
        (struct tx_desc *)mmio_map_region(page2pa(phy_page), PGSIZE);

    reg_base[E1000_REG_TO_NUM(E1000_TDBAL)] = page2pa(phy_page);
    reg_base[E1000_REG_TO_NUM(E1000_TDBAH)] = 0;

    reg_base[E1000_REG_TO_NUM(E1000_TDLEN)] = TX_DESC_QUEUE_BYTES;
    reg_base[E1000_REG_TO_NUM(E1000_TDT)] = 0;
    reg_base[E1000_REG_TO_NUM(E1000_TDH)] = 0;

    for (int i = 0; i < tx_desc_queue_legnth; ++i) {
        //  struct PageInfo * phy_page = page_alloc(0);
        // if(phy_page == NULL)
        // return -E_NO_MEM;
        tx_desc_queue_base[i].addr = PADDR(e1000_send_packet_buf[i]);
        //   page_insert(kern_pgdir, (phy_page), va ,PTE_W);
        tx_desc_queue_base[i].cmd = 0x8 | 0x01;
        tx_desc_queue_base[i].status = E1000_TXD_STAT_DD;
    }
    return 0;
}
 
int pci_e1000_recv_init(struct pci_func *pcif) {
   // 128 = RX_DESC_QUEUE_BYTES / rx_desc_struct_length;
//52:54:00:12:34:56
    //reg_base[E1000_REG_TO_NUM(E1000_MTA)] = 0;
    
   // struct PageInfo *phy_page = page_alloc(ALLOC_ZERO);
    //if (phy_page == NULL)
      //  return -E_NO_MEM;
    //rx_desc_queue_base =
     //   (struct rx_desc *)mmio_map_region(page2pa(phy_page), PGSIZE);
    reg_base[E1000_REG_TO_NUM(E1000_RA)] = 0x12005452;
    reg_base[E1000_REG_TO_NUM(E1000_RA + 4)] = 0x00005634 | E1000_RAH_AV;
  
    
    for (int i = 0; i < 128; ++i) {
        rx_desc_queue_base[i].addr =(uint32_t) PADDR(e1000_recv_packet_buf[i]);
        //rx_desc_queue_base[i].cmd = 0x8 | 0x01;
        //rx_desc_queue_base[i].status = E1000_TXD_STAT_DD;
    }
    
    //    e1000[E1000_LOCATE(E1000_RCTL)] = E1000_RCTL_EN | E1000_RCTL_SECRC | E1000_RCTL_BAM | E1000_RCTL_SZ_2048;
    reg_base[E1000_REG_TO_NUM(E1000_RDBAL)] = PADDR(rx_desc_queue_base);
    reg_base[E1000_REG_TO_NUM(E1000_RDBAH)] = 0;

    reg_base[E1000_REG_TO_NUM(E1000_RDLEN)] = RX_DESC_QUEUE_BYTES;

    reg_base[E1000_REG_TO_NUM(E1000_RDT)] = 127;
    reg_base[E1000_REG_TO_NUM(E1000_RDH)] = 0;
    //reg_base[E1000_REG_TO_NUM(E1000_RCTL)] = E1000_RCTL_LBM_NO;
    reg_base[E1000_REG_TO_NUM(E1000_RCTL)] = E1000_RCTL_EN;
   // reg_base[E1000_REG_TO_NUM(E1000_RCTL)] |= E1000_RCTL_RDMTS_QUAT;
    //reg_base[E1000_REG_TO_NUM(E1000_RCTL)] |= E1000_RCTL_MO_0;
    reg_base[E1000_REG_TO_NUM(E1000_RCTL)] |= E1000_RCTL_BAM;
    //reg_base[E1000_REG_TO_NUM(E1000_RCTL)] |= E1000_RCTL_BSEX;
    reg_base[E1000_REG_TO_NUM(E1000_RCTL)] |= E1000_RCTL_SECRC;
    reg_base[E1000_REG_TO_NUM(E1000_RCTL)] |= E1000_RCTL_SZ_2048;
    return 0;
}
int e1000_send_packet(void *data, uint32_t len) {
    int tail_pos = reg_base[E1000_REG_TO_NUM(E1000_TDT)];
    struct tx_desc *tail_desc =
         &((struct tx_desc *)tx_desc_queue_base)[tail_pos];
    if (tail_desc->status & E1000_TXD_STAT_DD) {
        memcpy((void *)e1000_send_packet_buf[tail_pos], data, len);
        tail_desc->length = len;
        tail_desc->status &= (~E1000_TXD_STAT_DD);
        reg_base[E1000_REG_TO_NUM(E1000_TDT)] =
            (tail_pos + 1) % tx_desc_queue_legnth;
        return 0;
    }
    return -E_NO_PACKET_MEM;
}

int
e1000_recv_packet(void *buf)
{
    int tail_pos = reg_base[E1000_REG_TO_NUM(E1000_RDT)];
    int next_pos = (tail_pos + 1) % 128;
    struct rx_desc *tail_desc = 
        &((struct rx_desc *)rx_desc_queue_base)[next_pos];
    if(tail_desc->status & E1000_RXD_STAT_DD) {
        memcpy(buf, (void*) e1000_recv_packet_buf[next_pos], tail_desc->length);
        tail_desc->status &= ~E1000_RXD_STAT_DD;
        int length = (int) tail_desc->length;
        reg_base[E1000_REG_TO_NUM(E1000_RDT)] = next_pos;
        return length;
    }
    return -E_NO_PACKET_MEM;
}
