/* generated vector source file - do not edit */
        #include "bsp_api.h"
        /* Do not build these data structures if no interrupts are currently allocated because IAR will have build errors. */
        #if VECTOR_DATA_IRQ_COUNT > 0
        BSP_DONT_REMOVE const fsp_vector_t g_vector_table[BSP_ICU_VECTOR_NUM_ENTRIES] BSP_PLACE_IN_SECTION(BSP_SECTION_APPLICATION_VECTORS) =
        {
                        [0] = layer3_switch_gwdi_isr, /* ETHER GWDI0 (GWCA Data Interrupt 0) */
            [1] = layer3_switch_eaei_isr, /* ETHER EAEI0 (ETHA0 Error Interrupt) */
            [2] = layer3_switch_eaei_isr, /* ETHER EAEI1 (ETHA1 Error Interrupt) */
            [3] = sci_b_uart_rxi_isr, /* SCI8 RXI (Receive data full) */
            [4] = sci_b_uart_txi_isr, /* SCI8 TXI (Transmit data empty) */
            [5] = sci_b_uart_tei_isr, /* SCI8 TEI (Transmit end) */
            [6] = sci_b_uart_eri_isr, /* SCI8 ERI (Receive error) */
            [7] = iic_master_rxi_isr, /* IIC0 RXI (Receive data full) */
            [8] = iic_master_txi_isr, /* IIC0 TXI (Transmit data empty) */
            [9] = iic_master_tei_isr, /* IIC0 TEI (Transmit end) */
            [10] = iic_master_eri_isr, /* IIC0 ERI (Transfer error) */
            [11] = glcdc_line_detect_isr, /* GLCDC LINE DETECT (Specified line) */
            [12] = drw_int_isr, /* DRW INT (DRW interrupt) */
            [13] = vin_status_isr, /* VIN IRQ (Interrupt Request) */
            [14] = vin_error_isr, /* VIN ERR (Interrupt Request for SYNC Error) */
            [15] = mipi_csi_rx_isr, /* MIPICSI RX (Receive interrupt) */
            [16] = mipi_csi_dl_isr, /* MIPICSI DL (Data Lane interrupt) */
            [17] = mipi_csi_vc_isr, /* MIPICSI VC (Virtual Channel interrupt) */
            [18] = mipi_csi_pm_isr, /* MIPICSI PM (Power Management interrupt) */
            [19] = mipi_csi_gst_isr, /* MIPICSI GST (Generic Short Packet interrupt) */
            [20] = gpt_counter_overflow_isr, /* GPT0 COUNTER OVERFLOW (Overflow) */
            [21] = sdhimmc_accs_isr, /* SDHIMMC0 ACCS (Card access) */
            [22] = sdhimmc_card_isr, /* SDHIMMC0 CARD (Card detect) */
            [23] = dmac_int_isr, /* DMAC1 INT (DMAC1 transfer end) */
            [24] = rm_ethosu_isr, /* NPU IRQ (NPU IRQ) */
            [25] = ceu_isr, /* CEU CEUI (CEU interrupt) */
            [26] = mipi_dsi_seq0_isr, /* MIPIDSI SEQ0 (Sequence operation channel 0 interrupt) */
            [27] = mipi_dsi_seq1_isr, /* MIPIDSI SEQ1 (Sequence operation channel 1 interrupt) */
            [28] = mipi_dsi_vin1_isr, /* MIPIDSI VIN1 (Video-Input operation channel1 interrupt) */
            [29] = mipi_dsi_rcv_isr, /* MIPIDSI RCV (DSI packet receive interrupt) */
            [30] = mipi_dsi_ferr_isr, /* MIPIDSI FERR (DSI fatal error interrupt) */
            [31] = mipi_dsi_ppi_isr, /* MIPIDSI PPI (DSI D-PHY PPI interrupt) */
        };
        #if BSP_FEATURE_ICU_HAS_IELSR
        const bsp_interrupt_event_t g_interrupt_event_link_select[BSP_ICU_VECTOR_NUM_ENTRIES] =
        {
            [0] = BSP_PRV_VECT_ENUM(EVENT_ETHER_GWDI0,GROUP0), /* ETHER GWDI0 (GWCA Data Interrupt 0) */
            [1] = BSP_PRV_VECT_ENUM(EVENT_ETHER_EAEI0,GROUP1), /* ETHER EAEI0 (ETHA0 Error Interrupt) */
            [2] = BSP_PRV_VECT_ENUM(EVENT_ETHER_EAEI1,GROUP2), /* ETHER EAEI1 (ETHA1 Error Interrupt) */
            [3] = BSP_PRV_VECT_ENUM(EVENT_SCI8_RXI,GROUP3), /* SCI8 RXI (Receive data full) */
            [4] = BSP_PRV_VECT_ENUM(EVENT_SCI8_TXI,GROUP4), /* SCI8 TXI (Transmit data empty) */
            [5] = BSP_PRV_VECT_ENUM(EVENT_SCI8_TEI,GROUP5), /* SCI8 TEI (Transmit end) */
            [6] = BSP_PRV_VECT_ENUM(EVENT_SCI8_ERI,GROUP6), /* SCI8 ERI (Receive error) */
            [7] = BSP_PRV_VECT_ENUM(EVENT_IIC0_RXI,GROUP7), /* IIC0 RXI (Receive data full) */
            [8] = BSP_PRV_VECT_ENUM(EVENT_IIC0_TXI,GROUP0), /* IIC0 TXI (Transmit data empty) */
            [9] = BSP_PRV_VECT_ENUM(EVENT_IIC0_TEI,GROUP1), /* IIC0 TEI (Transmit end) */
            [10] = BSP_PRV_VECT_ENUM(EVENT_IIC0_ERI,GROUP2), /* IIC0 ERI (Transfer error) */
            [11] = BSP_PRV_VECT_ENUM(EVENT_GLCDC_LINE_DETECT,GROUP3), /* GLCDC LINE DETECT (Specified line) */
            [12] = BSP_PRV_VECT_ENUM(EVENT_DRW_INT,GROUP4), /* DRW INT (DRW interrupt) */
            [13] = BSP_PRV_VECT_ENUM(EVENT_VIN_IRQ,GROUP5), /* VIN IRQ (Interrupt Request) */
            [14] = BSP_PRV_VECT_ENUM(EVENT_VIN_ERR,GROUP6), /* VIN ERR (Interrupt Request for SYNC Error) */
            [15] = BSP_PRV_VECT_ENUM(EVENT_MIPICSI_RX,GROUP7), /* MIPICSI RX (Receive interrupt) */
            [16] = BSP_PRV_VECT_ENUM(EVENT_MIPICSI_DL,GROUP0), /* MIPICSI DL (Data Lane interrupt) */
            [17] = BSP_PRV_VECT_ENUM(EVENT_MIPICSI_VC,GROUP1), /* MIPICSI VC (Virtual Channel interrupt) */
            [18] = BSP_PRV_VECT_ENUM(EVENT_MIPICSI_PM,GROUP2), /* MIPICSI PM (Power Management interrupt) */
            [19] = BSP_PRV_VECT_ENUM(EVENT_MIPICSI_GST,GROUP3), /* MIPICSI GST (Generic Short Packet interrupt) */
            [20] = BSP_PRV_VECT_ENUM(EVENT_GPT0_COUNTER_OVERFLOW,GROUP4), /* GPT0 COUNTER OVERFLOW (Overflow) */
            [21] = BSP_PRV_VECT_ENUM(EVENT_SDHIMMC0_ACCS,GROUP5), /* SDHIMMC0 ACCS (Card access) */
            [22] = BSP_PRV_VECT_ENUM(EVENT_SDHIMMC0_CARD,GROUP6), /* SDHIMMC0 CARD (Card detect) */
            [23] = BSP_PRV_VECT_ENUM(EVENT_DMAC1_INT,GROUP7), /* DMAC1 INT (DMAC1 transfer end) */
            [24] = BSP_PRV_VECT_ENUM(EVENT_NPU_IRQ,GROUP0), /* NPU IRQ (NPU IRQ) */
            [25] = BSP_PRV_VECT_ENUM(EVENT_CEU_CEUI,GROUP1), /* CEU CEUI (CEU interrupt) */
            [26] = BSP_PRV_VECT_ENUM(EVENT_MIPIDSI_SEQ0,GROUP2), /* MIPIDSI SEQ0 (Sequence operation channel 0 interrupt) */
            [27] = BSP_PRV_VECT_ENUM(EVENT_MIPIDSI_SEQ1,GROUP3), /* MIPIDSI SEQ1 (Sequence operation channel 1 interrupt) */
            [28] = BSP_PRV_VECT_ENUM(EVENT_MIPIDSI_VIN1,GROUP4), /* MIPIDSI VIN1 (Video-Input operation channel1 interrupt) */
            [29] = BSP_PRV_VECT_ENUM(EVENT_MIPIDSI_RCV,GROUP5), /* MIPIDSI RCV (DSI packet receive interrupt) */
            [30] = BSP_PRV_VECT_ENUM(EVENT_MIPIDSI_FERR,GROUP6), /* MIPIDSI FERR (DSI fatal error interrupt) */
            [31] = BSP_PRV_VECT_ENUM(EVENT_MIPIDSI_PPI,GROUP7), /* MIPIDSI PPI (DSI D-PHY PPI interrupt) */
        };
        #endif
        #endif
