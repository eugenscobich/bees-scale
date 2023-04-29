#ifndef __NRF24L01pMemoryMap_H__
#define __NRF24L01pMemoryMap_H__

/* Memory Map */
#define CONFIG      0x00
#define EN_AA       0x01
#define EN_RXADDR   0x02
#define SETUP_AW    0x03
#define SETUP_RETR  0x04
#define RF_CH       0x05
#define RF_SETUP    0x06
#define STATUS      0x07
#define OBSERVE_TX  0x08
#define RPD         0x09
#define RX_ADDR_P0  0x0A
#define RX_ADDR_P1  0x0B
#define RX_ADDR_P2  0x0C
#define RX_ADDR_P3  0x0D
#define RX_ADDR_P4  0x0E
#define RX_ADDR_P5  0x0F
#define TX_ADDR     0x10
#define RX_PW_P0    0x11
#define RX_PW_P1    0x12
#define RX_PW_P2    0x13
#define RX_PW_P3    0x14
#define RX_PW_P4    0x15
#define RX_PW_P5    0x16
#define FIFO_STATUS 0x17
#define DYNPD       0x1C
#define FEATURE     0x1D

#define TX_PLD     0xFF
#define RX_PLD     0xFF

/* Bit Mnemonics */
#define CONFIG_MASK_RX_DR_6     6
#define CONFIG_MASK_TX_DS_5     5
#define CONFIG_MASK_MAX_RT_4    4
#define CONFIG_EN_CRC_3         3
#define CONFIG_CRCO_2           2
#define CONFIG_PWR_UP_1         1
#define CONFIG_PRIM_RX_0        0
#define EN_AA_ENAA_P5_5         5
#define EN_AA_ENAA_P4_4         4
#define EN_AA_ENAA_P3_3         3
#define EN_AA_ENAA_P2_2         2
#define EN_AA_ENAA_P1_1         1
#define EN_AA_ENAA_P0_0         0
#define EN_RXADDR_ERX_P5_5      5
#define EN_RXADDR_ERX_P4_4      4
#define EN_RXADDR_ERX_P3_3      3
#define EN_RXADDR_ERX_P2_2      2
#define EN_RXADDR_ERX_P1_1      1
#define EN_RXADDR_ERX_P0_0      0
#define SETUP_AW_AW_1           1 
#define SETUP_AW_AW_0           0
#define SETUP_RETR_ARD_7        7
#define SETUP_RETR_ARD_6        6
#define SETUP_RETR_ARD_5        5
#define SETUP_RETR_ARD_4        4
#define SETUP_RETR_ARC_3        3
#define SETUP_RETR_ARC_2        2
#define SETUP_RETR_ARC_1        1
#define SETUP_RETR_ARC_0        0
#define RF_CH_RF_CH_6           6
#define RF_CH_RF_CH_5           5
#define RF_CH_RF_CH_4           4
#define RF_CH_RF_CH_3           3
#define RF_CH_RF_CH_2           2
#define RF_CH_RF_CH_1           1
#define RF_CH_RF_CH_0           0
#define RF_SETUP_CONT_WAVE_7    7
#define RF_SETUP_RF_DR_LOW_5    5
#define RF_SETUP_PLL_LOCK_4     4
#define RF_SETUP_RF_DR_HIGH_3   3
#define RF_SETUP_RF_PWR_2       2
#define RF_SETUP_RF_PWR_1       1
#define RF_SETUP_LNA_HCURR_0    0
#define STATUS_RX_DR_6          6
#define STATUS_TX_DS_5          5
#define STATUS_MAX_RT_4         4
#define STATUS_RX_P_NO_3        3
#define STATUS_RX_P_NO_2        2
#define STATUS_RX_P_NO_1        1
#define STATUS_TX_FULL_0        0
#define OBSERVE_TX_PLOS_CNT_7   7
#define OBSERVE_TX_PLOS_CNT_6   6
#define OBSERVE_TX_PLOS_CNT_5   5
#define OBSERVE_TX_PLOS_CNT_4   4
#define OBSERVE_TX_ARC_CNT_3    3
#define OBSERVE_TX_ARC_CNT_2    2
#define OBSERVE_TX_ARC_CNT_1    1
#define OBSERVE_TX_ARC_CNT_0    0
#define RPD_RPD                 0
#define RX_PW_P0_RX_PW_P0_5     5
#define RX_PW_P0_RX_PW_P0_4     4
#define RX_PW_P0_RX_PW_P0_3     3
#define RX_PW_P0_RX_PW_P0_2     2
#define RX_PW_P0_RX_PW_P0_1     1
#define RX_PW_P0_RX_PW_P0_0     0
#define RX_PW_P1_RX_PW_P1_5     5
#define RX_PW_P1_RX_PW_P1_4     4
#define RX_PW_P1_RX_PW_P1_3     3
#define RX_PW_P1_RX_PW_P1_2     2
#define RX_PW_P1_RX_PW_P1_1     1
#define RX_PW_P1_RX_PW_P1_0     0
#define RX_PW_P2_RX_PW_P2_5     5
#define RX_PW_P2_RX_PW_P2_4     4
#define RX_PW_P2_RX_PW_P2_3     3
#define RX_PW_P2_RX_PW_P2_2     2
#define RX_PW_P2_RX_PW_P2_1     1
#define RX_PW_P2_RX_PW_P2_0     0
#define RX_PW_P3_RX_PW_P3_5     5
#define RX_PW_P3_RX_PW_P3_4     4
#define RX_PW_P3_RX_PW_P3_3     3
#define RX_PW_P3_RX_PW_P3_2     2
#define RX_PW_P3_RX_PW_P3_1     1
#define RX_PW_P3_RX_PW_P3_0     0
#define RX_PW_P4_RX_PW_P4_5     5
#define RX_PW_P4_RX_PW_P4_4     4
#define RX_PW_P4_RX_PW_P4_3     3
#define RX_PW_P4_RX_PW_P4_2     2
#define RX_PW_P4_RX_PW_P4_1     1
#define RX_PW_P4_RX_PW_P4_0     0
#define RX_PW_P5_RX_PW_P5_5     5
#define RX_PW_P5_RX_PW_P5_4     4
#define RX_PW_P5_RX_PW_P5_3     3
#define RX_PW_P5_RX_PW_P5_2     2
#define RX_PW_P5_RX_PW_P5_1     1
#define RX_PW_P5_RX_PW_P5_0     0
#define FIFO_STATUS_TX_REUSE_6  6
#define FIFO_STATUS_FIFO_FULL_5 5
#define FIFO_STATUS_TX_EMPTY_4  4
#define FIFO_STATUS_RX_FULL_1   1
#define FIFO_STATUS_RX_EMPTY_0  0
#define DYNPD_DPL_P5_5          5
#define DYNPD_DPL_P4_4          4
#define DYNPD_DPL_P3_3          3
#define DYNPD_DPL_P2_2          2
#define DYNPD_DPL_P1_1          1
#define DYNPD_DPL_P0_0          0
#define FEATURE_EN_DPL_2        2
#define FEATURE_EN_ACK_PAY_1    1
#define FEATURE_EN_DYN_ACK_0    0

/* Instruction Mnemonics */
#define R_REGISTER          0x00 //
#define W_REGISTER          0x20
#define R_RX_PAYLOAD        0x61
#define W_TX_PAYLOAD        0xA0
#define FLUSH_TX            0xE1
#define FLUSH_RX            0xE2
#define REUSE_TX_PL         0xE3
#define ACTIVATE            0x50
#define R_RX_PL_WID         0x60
#define W_ACK_PAYLOAD       0xA8
#define W_TX_PAYLOAD_NO_ACK 0xB0
#define NOP                 0xFF


#endif // __NRF24L01pMemoryMap_H__