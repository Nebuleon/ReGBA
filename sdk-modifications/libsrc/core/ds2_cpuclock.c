#include <ds2_types.h>

#include "ds2_cpuclock.h"

#define REG8(addr)  *((volatile u8 *)(addr))
#define REG16(addr) *((volatile u16 *)(addr))
#define REG32(addr) *((volatile u32 *)(addr))

#define SDRAM_TRAS      50  /* RAS# Active Time */
#define SDRAM_RCD       23  /* RAS# to CAS# Delay */
#define SDRAM_TPC       23  /* RAS# Precharge Time */
#define SDRAM_TRWL      7   /* Write Latency Time */
#define SDRAM_TREF      7813    /* Refresh period: 8192 refresh cycles/64ms */

#define EMC_BASE    0xB3010000
#define EMC_DMCR    (EMC_BASE + 0x80)  /* DRAM Control Register */
#define REG_EMC_DMCR    REG32(EMC_DMCR)
#define EMC_RTCSR   (EMC_BASE + 0x84)  /* Refresh Time Control/Status Register */
#define REG_EMC_RTCSR   REG16(EMC_RTCSR)
#define EMC_RTCOR   (EMC_BASE + 0x8c)  /* Refresh Time Constant Register */
#define REG_EMC_RTCOR   REG16(EMC_RTCOR)
#define EMC_RTCNT   (EMC_BASE + 0x88)  /* Refresh Timer Counter */
#define REG_EMC_RTCNT   REG16(EMC_RTCNT)



#define CPM_BASE    0xB0000000
#define CPM_CPCCR   (CPM_BASE+0x00)
#define CPM_CPPCR   (CPM_BASE+0x10)
//#define CPM_RSR     (CPM_BASE+0x08)
#define REG_CPM_CPCCR   REG32(CPM_CPCCR)
#define REG_CPM_CPPCR   REG32(CPM_CPPCR)
#define CPM_CPCCR_CE        (1 << 22)






#define EMC_DMCR_TRAS_BIT   13
#define EMC_DMCR_TRAS_MASK  (0x07 << EMC_DMCR_TRAS_BIT)
#define EMC_DMCR_RCD_BIT    11
#define EMC_DMCR_RCD_MASK   (0x03 << EMC_DMCR_RCD_BIT)
#define EMC_DMCR_TPC_BIT    8
#define EMC_DMCR_TPC_MASK   (0x07 << EMC_DMCR_TPC_BIT)
#define EMC_DMCR_TRWL_BIT   5
#define EMC_DMCR_TRWL_MASK  (0x03 << EMC_DMCR_TRWL_BIT)
#define EMC_DMCR_TRC_BIT    2
#define EMC_DMCR_TRC_MASK   (0x07 << EMC_DMCR_TRC_BIT)

#define EMC_RTCSR_CKS_BIT   0
#define EMC_RTCSR_CKS_MASK  (0x07 << EMC_RTCSR_CKS_BIT)


#define CPM_CPCCR_LDIV_BIT  16
#define CPM_CPCCR_LDIV_MASK (0x1f << CPM_CPCCR_LDIV_BIT)
#define CPM_CPCCR_MDIV_BIT  12
#define CPM_CPCCR_MDIV_MASK (0x0f << CPM_CPCCR_MDIV_BIT)
#define CPM_CPCCR_PDIV_BIT  8
#define CPM_CPCCR_PDIV_MASK (0x0f << CPM_CPCCR_PDIV_BIT)
#define CPM_CPCCR_HDIV_BIT  4
#define CPM_CPCCR_HDIV_MASK (0x0f << CPM_CPCCR_HDIV_BIT)
#define CPM_CPCCR_CDIV_BIT  0
#define CPM_CPCCR_CDIV_MASK (0x0f << CPM_CPCCR_CDIV_BIT)


#define CPM_CPPCR_PLLM_BIT  23
#define CPM_CPPCR_PLLM_MASK (0x1ff << CPM_CPPCR_PLLM_BIT)
#define CPM_CPPCR_PLLN_BIT  18
#define CPM_CPPCR_PLLN_MASK (0x1f << CPM_CPPCR_PLLN_BIT)
#define CPM_CPPCR_PLLOD_BIT 16
#define CPM_CPPCR_PLLOD_MASK    (0x03 << CPM_CPPCR_PLLOD_BIT)
#define CPM_CPPCR_PLLS      (1 << 10)
#define CPM_CPPCR_PLLBP     (1 << 9)
#define CPM_CPPCR_PLLEN     (1 << 8)
#define CPM_CPPCR_PLLST_BIT 0
#define CPM_CPPCR_PLLST_MASK    (0xff << CPM_CPPCR_PLLST_BIT)


#define PLL_M       0
#define PLL_N       1
#define PLL_CCLK    2
#define PLL_HCLK    3
#define PLL_MCLK    4
#define PLL_PCLK    5

#define CFG_EXTAL       24000000
#define EXTAL_CLK       CFG_EXTAL


#define __cpm_get_pllm() \
    ((REG_CPM_CPPCR & CPM_CPPCR_PLLM_MASK) >> CPM_CPPCR_PLLM_BIT)

#define __cpm_get_plln() \
    ((REG_CPM_CPPCR & CPM_CPPCR_PLLN_MASK) >> CPM_CPPCR_PLLN_BIT)

#define __cpm_get_cdiv() \
    ((REG_CPM_CPCCR & CPM_CPCCR_CDIV_MASK) >> CPM_CPCCR_CDIV_BIT)


static unsigned char pll_m_n[CPU_MAX_LEVEL_EX + 1][6] = {
        //M, N, CCLK, HCLK, MCLK, PCLK, EXT_CLK=24MHz
        {10-2, 2-2, 1, 1, 1, 1},    //0    60, 60, 1/1
        {10-2, 2-2, 0, 1, 1, 1},    //1    120, 60, 1/2
        {10-2, 2-2, 0, 0, 0, 0},    //2    120, 120, 1/1
        {12-2, 2-2, 0, 1, 1, 1},    //3    144, 72, 1/2
        {16-2, 2-2, 0, 1, 1, 1},    //4    192, 96, 1/2
        {17-2, 2-2, 0, 0, 1, 1},    //5    204, 102, 1/2
        {20-2, 2-2, 0, 1, 1, 1},    //6    240, 120, 1/2
        {22-2, 2-2, 0, 2, 2, 2},    //7    264, 88, 1/3
        {24-2, 2-2, 0, 2, 2, 2},    //8    288, 96, 1/3
        {25-2, 2-2, 0, 2, 2, 2},    //9    300, 100, 1/3
        {28-2, 2-2, 0, 2, 2, 2},    //10   336, 112, 1/3
        {30-2, 2-2, 0, 2, 2, 2},    //11   360, 120, 1/3
        {32-2, 2-2, 0, 2, 2, 2},    //12   384, 128, 1/3
        {33-2, 2-2, 0, 2, 2, 2},    //13   396, 132, 1/3

        {34-2, 2-2, 0, 2, 2, 2},    //14   404, 132, 1/3
        {35-2, 2-2, 0, 2, 2, 2},    //15   420, 132, 1/3
        {36-2, 2-2, 0, 2, 2, 2},    //16   438, 132, 1/3
        {37-2, 2-2, 0, 2, 2, 2},    //17   444, 132, 1/3
        {38-2, 2-2, 0, 2, 2, 2},    //18   456, 132, 1/3
        //{39-2, 2-2, 0, 2, 2, 2},        //468, instant crash!
    };

static int _sdram_convert(unsigned int pllin,unsigned int *sdram_dmcr, unsigned int *sdram_div, unsigned int *sdram_tref)
{
    register unsigned int ns, dmcr,tmp;

    dmcr = ~(EMC_DMCR_TRAS_MASK | EMC_DMCR_RCD_MASK | EMC_DMCR_TPC_MASK |
                EMC_DMCR_TRWL_MASK | EMC_DMCR_TRC_MASK) & REG_EMC_DMCR;

    /* Set sdram operation parameter */
    //pllin unit is KHz
    ns = 1000000*1024 / pllin;
    tmp = SDRAM_TRAS*1024/ns;
    if (tmp < 4) tmp = 4;
    if (tmp > 11) tmp = 11;
    dmcr |= ((tmp-4) << EMC_DMCR_TRAS_BIT);

    tmp = SDRAM_RCD*1024/ns;
    if (tmp > 3) tmp = 3;
    dmcr |= (tmp << EMC_DMCR_RCD_BIT);

    tmp = SDRAM_TPC*1024/ns;
    if (tmp > 7) tmp = 7;
    dmcr |= (tmp << EMC_DMCR_TPC_BIT);

    tmp = SDRAM_TRWL*1024/ns;
    if (tmp > 3) tmp = 3;
    dmcr |= (tmp << EMC_DMCR_TRWL_BIT);

    tmp = (SDRAM_TRAS + SDRAM_TPC)*1024/ns;
    if (tmp > 14) tmp = 14;
    dmcr |= (((tmp + 1) >> 1) << EMC_DMCR_TRC_BIT);

    *sdram_dmcr = dmcr;

    /* Set refresh registers */
    unsigned int div;
    tmp = SDRAM_TREF*1024/ns;
    div = (tmp + 254)/255;
    if(div <= 4) div = 1;       //  1/4
    else if(div <= 16) div = 2; //  1/16
    else div = 3;               //  1/64

    *sdram_div = ~EMC_RTCSR_CKS_MASK & REG_EMC_RTCSR | div;

    unsigned int divm= 4;
    while(--div) divm *= 4;

    tmp = tmp/divm + 1;
    *sdram_tref = tmp;

    return 0;
}


const static int FR2n[] = {
    1, 2, 3, 4, 6, 8, 12, 16, 24, 32
};

static unsigned int _pllout;
static unsigned int _iclk;

static void detect_clockNew(void){
    _pllout = (__cpm_get_pllm() + 2)* EXTAL_CLK / (__cpm_get_plln() + 2);
    _iclk = _pllout / FR2n[__cpm_get_cdiv()];
}


//udelay overclock
void ds2_udelay(unsigned int usec)
{
    unsigned int i = usec * (_iclk / 2000000);

    __asm__ __volatile__ (
        "\t.set noreorder\n"
        "1:\n\t"
        "bne\t%0, $0, 1b\n\t"
        "addi\t%0, %0, -1\n\t"
        ".set reorder\n"
        : "=r" (i)
        : "0" (i)
    );
}

//mdelay overclock
void ds2_mdelay(unsigned int msec)
{
    int i;
    for(i=0;i<msec;i++)
    {
        ds2_udelay(1000);
    }
}




int ds2_getCPUClock(void){
    return (_pllout/1000/1000);
}



/* convert pll while program is running */
int ds2_setCPULevel(unsigned int level){
    unsigned int freq_b;
    unsigned int dmcr;
    unsigned int rtcsr;
    unsigned int tref;
    unsigned int cpccr;
    unsigned int cppcr;

    if(level > CPU_MAX_LEVEL_EX) return -1;

    freq_b = (pll_m_n[level][PLL_M]+2)*(EXTAL_CLK/1000)/(pll_m_n[level][PLL_N]+2);

    //freq_b unit is KHz
    _sdram_convert(freq_b/pll_m_n[level][PLL_MCLK], &dmcr, &rtcsr, &tref);

    cpccr = REG_CPM_CPCCR;
    cppcr = REG_CPM_CPPCR;

    REG_CPM_CPCCR = ~CPM_CPCCR_CE & cpccr;

    cppcr &= ~(CPM_CPPCR_PLLM_MASK | CPM_CPPCR_PLLN_MASK);
    cppcr |= (pll_m_n[level][PLL_M] << CPM_CPPCR_PLLM_BIT) | (pll_m_n[level][PLL_N] << CPM_CPPCR_PLLN_BIT);

    cpccr &= ~(CPM_CPCCR_CDIV_MASK | CPM_CPCCR_HDIV_MASK | CPM_CPCCR_PDIV_MASK |
                CPM_CPCCR_MDIV_MASK | CPM_CPCCR_LDIV_MASK);
    cpccr |= (pll_m_n[level][PLL_CCLK] << CPM_CPCCR_CDIV_BIT) | (pll_m_n[level][PLL_HCLK] << CPM_CPCCR_HDIV_BIT) |
                (pll_m_n[level][PLL_MCLK] << CPM_CPCCR_MDIV_BIT) | (pll_m_n[level][PLL_PCLK] << CPM_CPCCR_PDIV_BIT) |
                (31 << CPM_CPCCR_LDIV_BIT);

    REG_CPM_CPCCR = cpccr;
    REG_CPM_CPPCR = cppcr;

    REG_CPM_CPCCR |= CPM_CPCCR_CE;
    //Wait PLL stable
    while(!(CPM_CPPCR_PLLS & REG_CPM_CPPCR));

    //REG_EMC_DMCR = dmcr;
    REG_EMC_RTCOR = tref;
    REG_EMC_RTCNT = tref;

    detect_clockNew();
    return 0;
}


