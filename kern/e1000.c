#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/error.h>
// LAB 6: Your driver code here
volatile uint32_t * reg_base = 0;
// packet descriptor size 16
// packet descriptor queue size 128
// max packct content size 1518
int pci_e1000_alloc_desc_array(struct pci_func *pcif);

int pci_e1000_attach(struct pci_func *pcif) {
    int r;
    pci_func_enable(pcif);
    reg_base = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
    cprintf("E1000 status register %x\n",reg_base[E1000_REG_TO_NUM(E1000_STATUS)]);
  
    reg_base[E1000_REG_TO_NUM(E1000_TCTL)] |= E1000_TCTL_EN | E1000_TCTL_PSP;
    reg_base[E1000_REG_TO_NUM(E1000_TCTL)] |= 40 << 12;
    reg_base[E1000_REG_TO_NUM(E1000_TIPG)] |= 10;
    reg_base[E1000_REG_TO_NUM(E1000_TIPG)] |= 4 << 10;
    reg_base[E1000_REG_TO_NUM(E1000_TIPG)] |= 6 << 20;
      if((r = pci_e1000_alloc_desc_array(pcif)))
        panic("Error occurs in pci_e1000_attach : %e", r);
    return 0;
}

int pci_e1000_alloc_desc_array(struct pci_func *pcif) {
    struct PageInfo* phy_page = page_alloc(ALLOC_ZERO);
    if(phy_page == NULL)
        return -E_NO_MEM;
    phy_page->pp_ref+=2;
    reg_base[E1000_REG_TO_NUM(E1000_TDBAL)] = page2pa(phy_page);
    reg_base[E1000_REG_TO_NUM(E1000_TDLEN)] = 256;
    reg_base[E1000_REG_TO_NUM(E1000_TDT)] = 0;
    reg_base[E1000_REG_TO_NUM(E1000_TDH)] = 0;
    int r;
    struct tx_desc * p_tx_desc = (struct tx_desc *)UTEMP;
    if((r = page_insert(kern_pgdir, phy_page, (void* )UTEMP, PTE_W)) < 0)
        return r;
    
    for(int i = 0; i < 256 / sizeof(struct tx_desc); ++i) {
        if((phy_page = page_alloc(0)) == 0)
            return -E_NO_MEM;
        p_tx_desc[i].addr = page2pa(phy_page);
        p_tx_desc[i].length = PGSIZE;
    }
    page_remove(kern_pgdir,(void *)UTEMP);
    return 0;
}