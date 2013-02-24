#ifndef __JZ_ETH_H__
#define __JZ_ETH_H__

#define ETH_BASE	0xB3100000

#define ETH_BMR		(ETH_BASE + 0x1000)
#define ETH_TPDR	(ETH_BASE + 0x1004)
#define ETH_RPDR	(ETH_BASE + 0x1008)
#define ETH_RAR		(ETH_BASE + 0x100C)
#define ETH_TAR		(ETH_BASE + 0x1010)
#define ETH_SR		(ETH_BASE + 0x1014)
#define ETH_OMR		(ETH_BASE + 0x1018)
#define ETH_IER		(ETH_BASE + 0x101C)
#define ETH_MFCR	(ETH_BASE + 0x1020)
#define ETH_CTAR	(ETH_BASE + 0x1050)
#define ETH_CRAR	(ETH_BASE + 0x1054)
#define ETH_MCR		(ETH_BASE + 0x0000)
#define ETH_MAHR	(ETH_BASE + 0x0004)
#define ETH_MALR	(ETH_BASE + 0x0008)
#define ETH_HTHR	(ETH_BASE + 0x000C)
#define ETH_HTLR	(ETH_BASE + 0x0010)
#define ETH_MIAR	(ETH_BASE + 0x0014)
#define ETH_MIDR	(ETH_BASE + 0x0018)
#define ETH_FCR		(ETH_BASE + 0x001C)
#define ETH_VTR1	(ETH_BASE + 0x0020)
#define ETH_VTR2	(ETH_BASE + 0x0024)
#define ETH_WKFR	(ETH_BASE + 0x0028)
#define ETH_PMTR	(ETH_BASE + 0x002C)

// Bus Mode Register (DMA_BMR)
#define BMR_PBL		0x00003f00	// Programmable Burst Length
#define BMR_DSL		0x0000007c	// Descriptor Skip Length
#define BMR_BAR		0x00000002	// Bus ARbitration
#define BMR_SWR		0x00000001	// Software Reset

#define DMA_BURST       4

// Status Register (DMA_STS)
#define STS_BE		0x03800000	// Bus Error Bits
#define STS_TS		0x00700000	// Transmit Process State
#define STS_RS		0x000e0000	// Receive Process State

#define TS_STOP		0x00000000	// Stopped
#define TS_FTD		0x00100000	// Running Fetch Transmit Descriptor
#define TS_WEOT		0x00200000	// Running Wait for End Of Transmission
#define TS_QDAT		0x00300000	// Running Queue skb data into TX FIFO
#define TS_RES		0x00400000	// Reserved
#define TS_SPKT		0x00500000	// Reserved
#define TS_SUSP		0x00600000	// Suspended
#define TS_CLTD		0x00700000	// Running Close Transmit Descriptor

#define RS_STOP		0x00000000	// Stopped
#define RS_FRD		0x00020000	// Running Fetch Rx Descriptor
#define RS_CEOR		0x00040000	// Running Check for End of Rx Packet
#define RS_WFRP		0x00060000	// Running Wait for Rx Packet
#define RS_SUSP		0x00080000	// Suspended
#define RS_CLRD		0x000a0000	// Running Close Rx Descriptor
#define RS_FLUSH	0x000c0000	// Running Flush RX FIFO
#define RS_QRFS		0x000e0000	// Running Queue RX FIFO into RX Skb

// Operation Mode Register (DMA_OMR)
#define OMR_TTM		0x00400000	// Transmit Threshold Mode
#define OMR_SF		0x00200000	// Store and Forward
#define OMR_TR		0x0000c000	// Threshold Control Bits
#define OMR_ST		0x00002000	// Start/Stop Transmission Command
#define OMR_SR		0x00000002	// Start/Stop Receive

#define TR_18		0x00000000	// Threshold set to 18 (32) bytes
#define TR_24		0x00004000	// Threshold set to 24 (64) bytes
#define TR_32		0x00008000	// Threshold set to 32 (128) bytes
#define TR_40		0x0000c000	// Threshold set to 40 (256) bytes

// Missed Frames Counters (DMA_MFC)
#define MFC_CNT1	0xffff0000	// Missed Frames Counter Bits by
					// application
#define MFC_CNT2	0x0000ffff	// Missed Frames Counter Bits by
					// controller

// Mac control  Register (MAC_MCR)
#define MCR_RA		0x80000000	// Receive All
#define MCR_HBD		0x10000000	// HeartBeat Disable
#define MCR_PS		0x08000000	// Port Select
#define MCR_DRO		0x00800000	// Receive own Disable
#define MCR_OM		0x00600000	// Operating(loopback) Mode
#define MCR_FDX		0x00100000	// Full Duplex Mode
#define MCR_PM		0x00080000	// Pass All Multicast
#define MCR_PR		0x00040000	// Promiscuous Mode
#define MCR_IF		0x00020000	// Inverse Filtering
#define MCR_PB		0x00010000	// Pass Bad Frames
#define MCR_HO		0x00008000	// Hash Only Filtering Mode
#define MCR_HP		0x00002000	// Hash/Perfect Receive Filtering Mode
#define MCR_LCC		0x00001000	// Late Collision control
#define MCR_DBF		0x00000800	// Boardcast frame Disable
#define MCR_DTRY	0x00000400	// Retry Disable
#define MCR_ASTP	0x00000100	// Automatic pad stripping
#define MCR_BOLMT	0x000000c0	// Back off Limit
#define MCR_DC		0x00000020	// Deferral check
#define MCR_TE		0x00000008	// Transmitter enable
#define MCR_RE		0x00000004	// Receiver enable

#define MCR_MII_10	( OMR_TTM | MCR_PS)
#define MCR_MII_100	( MCR_HBD | MCR_PS)

// Constants for the intr mask and intr status registers. (DMA_SIS and DMA_IER)
#define DMA_INT_NI	0x00010000	// Normal interrupt summary
#define DMA_INT_AI	0x00008000	// Abnormal interrupt summary
#define DMA_INT_ER	0x00004000	// Early receive interrupt
#define DMA_INT_FB	0x00002000	// Fatal bus error
#define DMA_INT_ET	0x00000400	// Early transmit interrupt
#define DMA_INT_RW	0x00000200	// Receive watchdog timeout
#define DMA_INT_RS	0x00000100	// Receive stop
#define DMA_INT_RU	0x00000080	// Receive buffer unavailble
#define DMA_INT_RI	0x00000040	// Receive interrupt
#define DMA_INT_UN	0x00000020	// Underflow 
#define DMA_INT_TJ	0x00000008	// Transmit jabber timeout
#define DMA_INT_TU	0x00000004	// Transmit buffer unavailble 
#define DMA_INT_TS	0x00000002	// Transmit stop
#define DMA_INT_TI	0x00000001	// Transmit interrupt

#define DMA_TX_DEFAULT 	( DMA_INT_TI | DMA_INT_TS | DMA_INT_TU | DMA_INT_TJ | DMA_INT_FB )

#define DMA_RX_DEFAULT 	( DMA_INT_RI | DMA_INT_RS | DMA_INT_RU | DMA_INT_RW | DMA_INT_FB )

#define DMA_ENABLE	(DMA_INT_NI | DMA_INT_AI)


// Receive Descriptor Bit Summary
#define R_OWN		0x80000000	// Own Bit
#define RD_FF		0x40000000	// Filtering Fail
#define RD_FL		0x3fff0000	// Frame Length
#define RD_ES		0x00008000	// Error Summary
#define RD_DE		0x00004000	// Descriptor Error
#define RD_LE		0x00001000	// Length Error
#define RD_RF		0x00000800	// Runt Frame
#define RD_MF		0x00000400	// Multicast Frame
#define RD_FS		0x00000200	// First Descriptor
#define RD_LS		0x00000100	// Last Descriptor
#define RD_TL		0x00000080	// Frame Too Long
#define RD_CS		0x00000040	// Collision Seen
#define RD_FT		0x00000020	// Frame Type
#define RD_RJ		0x00000010	// Receive Watchdog timeout
#define RD_RE		0x00000008	// Report on MII Error
#define RD_DB		0x00000004	// Dribbling Bit
#define RD_CE		0x00000002	// CRC Error

#define RD_RER		0x02000000	// Receive End Of Ring
#define RD_RCH		0x01000000	// Second Address Chained
#define RD_RBS2		0x003ff800	// Buffer 2 Size
#define RD_RBS1		0x000007ff	// Buffer 1 Size

// Transmit Descriptor Bit Summary
#define T_OWN		0x80000000	// Own Bit
#define TD_ES		0x00008000	// Frame Aborted (error summary)
#define TD_LO		0x00000800	// Loss Of Carrier
#define TD_NC		0x00000400	// No Carrier
#define TD_LC		0x00000200	// Late Collision
#define TD_EC		0x00000100	// Excessive Collisions
#define TD_HF		0x00000080	// Heartbeat Fail
#define TD_CC		0x0000003c	// Collision Counter
#define TD_UF		0x00000002	// Underflow Error
#define TD_DE		0x00000001	// Deferred

#define TD_IC		0x80000000	// Interrupt On Completion
#define TD_LS		0x40000000	// Last Segment
#define TD_FS		0x20000000	// First Segment
#define TD_FT1		0x10000000	// Filtering Type
#define TD_SET		0x08000000	// Setup Packet
#define TD_AC		0x04000000	// Add CRC Disable
#define TD_TER		0x02000000	// Transmit End Of Ring
#define TD_TCH		0x01000000	// Second Address Chained
#define TD_DPD		0x00800000	// Disabled Padding
#define TD_FT0		0x00400000	// Filtering Type
#define TD_TBS2		0x003ff800	// Buffer 2 Size
#define TD_TBS1		0x000007ff	// Buffer 1 Size

#define PERFECT_F  	0x00000000
#define HASH_F     	TD_FT0
#define INVERSE_F  	TD_FT1
#define HASH_O_F   	(TD_FT1 | TD_F0)

// ------------------------------------------------------------------------
// MII Registers and Definitions
// ------------------------------------------------------------------------
#define MII_CR       0x00          /* MII Management Control Register */
#define MII_SR       0x01          /* MII Management Status Register */
#define MII_ID0      0x02          /* PHY Identifier Register 0 */
#define MII_ID1      0x03          /* PHY Identifier Register 1 */
#define MII_ANA      0x04          /* Auto Negotiation Advertisement */
#define MII_ANLPA    0x05          /* Auto Negotiation Link Partner Ability */
#define MII_ANE      0x06          /* Auto Negotiation Expansion */
#define MII_ANP      0x07          /* Auto Negotiation Next Page TX */

// MII Management Control Register
#define MII_CR_RST  0x8000         /* RESET the PHY chip */
#define MII_CR_LPBK 0x4000         /* Loopback enable */
#define MII_CR_SPD  0x2000         /* 0: 10Mb/s; 1: 100Mb/s */
#define MII_CR_10   0x0000         /* Set 10Mb/s */
#define MII_CR_100  0x2000         /* Set 100Mb/s */
#define MII_CR_ASSE 0x1000         /* Auto Speed Select Enable */
#define MII_CR_PD   0x0800         /* Power Down */
#define MII_CR_ISOL 0x0400         /* Isolate Mode */
#define MII_CR_RAN  0x0200         /* Restart Auto Negotiation */
#define MII_CR_FDM  0x0100         /* Full Duplex Mode */
#define MII_CR_CTE  0x0080         /* Collision Test Enable */

// MII Management Status Register
#define MII_SR_T4C  0x8000         /* 100BASE-T4 capable */
#define MII_SR_TXFD 0x4000         /* 100BASE-TX Full Duplex capable */
#define MII_SR_TXHD 0x2000         /* 100BASE-TX Half Duplex capable */
#define MII_SR_TFD  0x1000         /* 10BASE-T Full Duplex capable */
#define MII_SR_THD  0x0800         /* 10BASE-T Half Duplex capable */
#define MII_SR_ASSC 0x0020         /* Auto Speed Selection Complete*/
#define MII_SR_RFD  0x0010         /* Remote Fault Detected */
#define MII_SR_ANC  0x0008         /* Auto Negotiation capable */
#define MII_SR_LKS  0x0004         /* Link Status */
#define MII_SR_JABD 0x0002         /* Jabber Detect */
#define MII_SR_XC   0x0001         /* Extended Capabilities */

// MII Management Auto Negotiation Advertisement Register
#define MII_ANA_TAF  0x03e0        /* Technology Ability Field */
#define MII_ANA_T4AM 0x0200        /* T4 Technology Ability Mask */
#define MII_ANA_TXAM 0x0180        /* TX Technology Ability Mask */
#define MII_ANA_FDAM 0x0140        /* Full Duplex Technology Ability Mask */
#define MII_ANA_HDAM 0x02a0        /* Half Duplex Technology Ability Mask */
#define MII_ANA_100M 0x0380        /* 100Mb Technology Ability Mask */
#define MII_ANA_10M  0x0060        /* 10Mb Technology Ability Mask */
#define MII_ANA_CSMA 0x0001        /* CSMA-CD Capable */

// MII Management Auto Negotiation Remote End Register
#define MII_ANLPA_NP   0x8000      /* Next Page (Enable) */
#define MII_ANLPA_ACK  0x4000      /* Remote Acknowledge */
#define MII_ANLPA_RF   0x2000      /* Remote Fault */
#define MII_ANLPA_TAF  0x03e0      /* Technology Ability Field */
#define MII_ANLPA_T4AM 0x0200      /* T4 Technology Ability Mask */
#define MII_ANLPA_TXAM 0x0180      /* TX Technology Ability Mask */
#define MII_ANLPA_FDAM 0x0140      /* Full Duplex Technology Ability Mask */
#define MII_ANLPA_HDAM 0x02a0      /* Half Duplex Technology Ability Mask */
#define MII_ANLPA_100M 0x0380      /* 100Mb Technology Ability Mask */
#define MII_ANLPA_10M  0x0060      /* 10Mb Technology Ability Mask */
#define MII_ANLPA_CSMA 0x0001      /* CSMA-CD Capable */

// Media / mode state machine definitions
// User selectable:
#define TP              0x0040     /* 10Base-T (now equiv to _10Mb)        */
#define TP_NW           0x0002     /* 10Base-T with Nway                   */
#define BNC             0x0004     /* Thinwire                             */
#define AUI             0x0008     /* Thickwire                            */
#define BNC_AUI         0x0010     /* BNC/AUI on DC21040 indistinguishable */
#define _10Mb           0x0040     /* 10Mb/s Ethernet                      */
#define _100Mb          0x0080     /* 100Mb/s Ethernet                     */
#define AUTO            0x4000     /* Auto sense the media or speed        */

// Internal states
#define NC              0x0000     /* No Connection                        */
#define ANS             0x0020     /* Intermediate AutoNegotiation State   */
#define SPD_DET         0x0100     /* Parallel speed detection             */
#define INIT            0x0200     /* Initial state                        */
#define EXT_SIA         0x0400     /* External SIA for motherboard chip    */
#define ANS_SUSPECT     0x0802     /* Suspect the ANS (TP) port is down    */
#define TP_SUSPECT      0x0803     /* Suspect the TP port is down          */
#define BNC_AUI_SUSPECT 0x0804     /* Suspect the BNC or AUI port is down  */
#define EXT_SIA_SUSPECT 0x0805     /* Suspect the EXT SIA port is down     */
#define BNC_SUSPECT     0x0806     /* Suspect the BNC port is down         */
#define AUI_SUSPECT     0x0807     /* Suspect the AUI port is down         */
#define MII             0x1000     /* MII on the 21143                     */

#endif /* __JZ_ETH_H__ */
