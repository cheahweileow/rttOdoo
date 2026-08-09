/* generated vector header file - do not edit */
        #ifndef VECTOR_DATA_H
        #define VECTOR_DATA_H
        #ifdef __cplusplus
        extern "C" {
        #endif
                /* Number of interrupts allocated */
        #ifndef VECTOR_DATA_IRQ_COUNT
        #define VECTOR_DATA_IRQ_COUNT    (32)
        #endif
        /* ISR prototypes */
        void sci_b_uart_rxi_isr(void);
        void sci_b_uart_txi_isr(void);
        void sci_b_uart_tei_isr(void);
        void sci_b_uart_eri_isr(void);
        void iic_master_rxi_isr(void);
        void iic_master_txi_isr(void);
        void iic_master_tei_isr(void);
        void iic_master_eri_isr(void);
        void glcdc_line_detect_isr(void);
        void drw_int_isr(void);
        void vin_status_isr(void);
        void vin_error_isr(void);
        void mipi_csi_rx_isr(void);
        void mipi_csi_dl_isr(void);
        void mipi_csi_vc_isr(void);
        void mipi_csi_pm_isr(void);
        void mipi_csi_gst_isr(void);
        void gpt_counter_overflow_isr(void);
        void sdhimmc_accs_isr(void);
        void sdhimmc_card_isr(void);
        void dmac_int_isr(void);
        void rm_ethosu_isr(void);
        void ceu_isr(void);
        void mipi_dsi_seq0_isr(void);
        void mipi_dsi_seq1_isr(void);
        void mipi_dsi_vin1_isr(void);
        void mipi_dsi_rcv_isr(void);
        void mipi_dsi_ferr_isr(void);
        void mipi_dsi_ppi_isr(void);
        void layer3_switch_gwdi_isr(void);
        void layer3_switch_eaei_isr(void);

        /* Vector table allocations */
        #define VECTOR_NUMBER_ETHER_GWDI0 ((IRQn_Type) 0) /* ETHER GWDI0 (GWCA Data Interrupt 0) */
        #define ETHER_GWDI0_IRQn          ((IRQn_Type) 0) /* ETHER GWDI0 (GWCA Data Interrupt 0) */
        #define VECTOR_NUMBER_ETHER_EAEI0 ((IRQn_Type) 1) /* ETHER EAEI0 (ETHA0 Error Interrupt) */
        #define ETHER_EAEI0_IRQn          ((IRQn_Type) 1) /* ETHER EAEI0 (ETHA0 Error Interrupt) */
        #define VECTOR_NUMBER_ETHER_EAEI1 ((IRQn_Type) 2) /* ETHER EAEI1 (ETHA1 Error Interrupt) */
        #define ETHER_EAEI1_IRQn          ((IRQn_Type) 2) /* ETHER EAEI1 (ETHA1 Error Interrupt) */
        #define VECTOR_NUMBER_SCI8_RXI ((IRQn_Type) 3) /* SCI8 RXI (Receive data full) */
        #define SCI8_RXI_IRQn          ((IRQn_Type) 3) /* SCI8 RXI (Receive data full) */
        #define VECTOR_NUMBER_SCI8_TXI ((IRQn_Type) 4) /* SCI8 TXI (Transmit data empty) */
        #define SCI8_TXI_IRQn          ((IRQn_Type) 4) /* SCI8 TXI (Transmit data empty) */
        #define VECTOR_NUMBER_SCI8_TEI ((IRQn_Type) 5) /* SCI8 TEI (Transmit end) */
        #define SCI8_TEI_IRQn          ((IRQn_Type) 5) /* SCI8 TEI (Transmit end) */
        #define VECTOR_NUMBER_SCI8_ERI ((IRQn_Type) 6) /* SCI8 ERI (Receive error) */
        #define SCI8_ERI_IRQn          ((IRQn_Type) 6) /* SCI8 ERI (Receive error) */
        #define VECTOR_NUMBER_IIC0_RXI ((IRQn_Type) 7) /* IIC0 RXI (Receive data full) */
        #define IIC0_RXI_IRQn          ((IRQn_Type) 7) /* IIC0 RXI (Receive data full) */
        #define VECTOR_NUMBER_IIC0_TXI ((IRQn_Type) 8) /* IIC0 TXI (Transmit data empty) */
        #define IIC0_TXI_IRQn          ((IRQn_Type) 8) /* IIC0 TXI (Transmit data empty) */
        #define VECTOR_NUMBER_IIC0_TEI ((IRQn_Type) 9) /* IIC0 TEI (Transmit end) */
        #define IIC0_TEI_IRQn          ((IRQn_Type) 9) /* IIC0 TEI (Transmit end) */
        #define VECTOR_NUMBER_IIC0_ERI ((IRQn_Type) 10) /* IIC0 ERI (Transfer error) */
        #define IIC0_ERI_IRQn          ((IRQn_Type) 10) /* IIC0 ERI (Transfer error) */
        #define VECTOR_NUMBER_GLCDC_LINE_DETECT ((IRQn_Type) 11) /* GLCDC LINE DETECT (Specified line) */
        #define GLCDC_LINE_DETECT_IRQn          ((IRQn_Type) 11) /* GLCDC LINE DETECT (Specified line) */
        #define VECTOR_NUMBER_DRW_INT ((IRQn_Type) 12) /* DRW INT (DRW interrupt) */
        #define DRW_INT_IRQn          ((IRQn_Type) 12) /* DRW INT (DRW interrupt) */
        #define VECTOR_NUMBER_VIN_IRQ ((IRQn_Type) 13) /* VIN IRQ (Interrupt Request) */
        #define VIN_IRQ_IRQn          ((IRQn_Type) 13) /* VIN IRQ (Interrupt Request) */
        #define VECTOR_NUMBER_VIN_ERR ((IRQn_Type) 14) /* VIN ERR (Interrupt Request for SYNC Error) */
        #define VIN_ERR_IRQn          ((IRQn_Type) 14) /* VIN ERR (Interrupt Request for SYNC Error) */
        #define VECTOR_NUMBER_MIPICSI_RX ((IRQn_Type) 15) /* MIPICSI RX (Receive interrupt) */
        #define MIPICSI_RX_IRQn          ((IRQn_Type) 15) /* MIPICSI RX (Receive interrupt) */
        #define VECTOR_NUMBER_MIPICSI_DL ((IRQn_Type) 16) /* MIPICSI DL (Data Lane interrupt) */
        #define MIPICSI_DL_IRQn          ((IRQn_Type) 16) /* MIPICSI DL (Data Lane interrupt) */
        #define VECTOR_NUMBER_MIPICSI_VC ((IRQn_Type) 17) /* MIPICSI VC (Virtual Channel interrupt) */
        #define MIPICSI_VC_IRQn          ((IRQn_Type) 17) /* MIPICSI VC (Virtual Channel interrupt) */
        #define VECTOR_NUMBER_MIPICSI_PM ((IRQn_Type) 18) /* MIPICSI PM (Power Management interrupt) */
        #define MIPICSI_PM_IRQn          ((IRQn_Type) 18) /* MIPICSI PM (Power Management interrupt) */
        #define VECTOR_NUMBER_MIPICSI_GST ((IRQn_Type) 19) /* MIPICSI GST (Generic Short Packet interrupt) */
        #define MIPICSI_GST_IRQn          ((IRQn_Type) 19) /* MIPICSI GST (Generic Short Packet interrupt) */
        #define VECTOR_NUMBER_GPT0_COUNTER_OVERFLOW ((IRQn_Type) 20) /* GPT0 COUNTER OVERFLOW (Overflow) */
        #define GPT0_COUNTER_OVERFLOW_IRQn          ((IRQn_Type) 20) /* GPT0 COUNTER OVERFLOW (Overflow) */
        #define VECTOR_NUMBER_SDHIMMC0_ACCS ((IRQn_Type) 21) /* SDHIMMC0 ACCS (Card access) */
        #define SDHIMMC0_ACCS_IRQn          ((IRQn_Type) 21) /* SDHIMMC0 ACCS (Card access) */
        #define VECTOR_NUMBER_SDHIMMC0_CARD ((IRQn_Type) 22) /* SDHIMMC0 CARD (Card detect) */
        #define SDHIMMC0_CARD_IRQn          ((IRQn_Type) 22) /* SDHIMMC0 CARD (Card detect) */
        #define VECTOR_NUMBER_DMAC1_INT ((IRQn_Type) 23) /* DMAC1 INT (DMAC1 transfer end) */
        #define DMAC1_INT_IRQn          ((IRQn_Type) 23) /* DMAC1 INT (DMAC1 transfer end) */
        #define VECTOR_NUMBER_NPU_IRQ ((IRQn_Type) 24) /* NPU IRQ (NPU IRQ) */
        #define NPU_IRQ_IRQn          ((IRQn_Type) 24) /* NPU IRQ (NPU IRQ) */
        #define VECTOR_NUMBER_CEU_CEUI ((IRQn_Type) 25) /* CEU CEUI (CEU interrupt) */
        #define CEU_CEUI_IRQn          ((IRQn_Type) 25) /* CEU CEUI (CEU interrupt) */
        #define VECTOR_NUMBER_MIPIDSI_SEQ0 ((IRQn_Type) 26) /* MIPIDSI SEQ0 (Sequence operation channel 0 interrupt) */
        #define MIPIDSI_SEQ0_IRQn          ((IRQn_Type) 26) /* MIPIDSI SEQ0 (Sequence operation channel 0 interrupt) */
        #define VECTOR_NUMBER_MIPIDSI_SEQ1 ((IRQn_Type) 27) /* MIPIDSI SEQ1 (Sequence operation channel 1 interrupt) */
        #define MIPIDSI_SEQ1_IRQn          ((IRQn_Type) 27) /* MIPIDSI SEQ1 (Sequence operation channel 1 interrupt) */
        #define VECTOR_NUMBER_MIPIDSI_VIN1 ((IRQn_Type) 28) /* MIPIDSI VIN1 (Video-Input operation channel1 interrupt) */
        #define MIPIDSI_VIN1_IRQn          ((IRQn_Type) 28) /* MIPIDSI VIN1 (Video-Input operation channel1 interrupt) */
        #define VECTOR_NUMBER_MIPIDSI_RCV ((IRQn_Type) 29) /* MIPIDSI RCV (DSI packet receive interrupt) */
        #define MIPIDSI_RCV_IRQn          ((IRQn_Type) 29) /* MIPIDSI RCV (DSI packet receive interrupt) */
        #define VECTOR_NUMBER_MIPIDSI_FERR ((IRQn_Type) 30) /* MIPIDSI FERR (DSI fatal error interrupt) */
        #define MIPIDSI_FERR_IRQn          ((IRQn_Type) 30) /* MIPIDSI FERR (DSI fatal error interrupt) */
        #define VECTOR_NUMBER_MIPIDSI_PPI ((IRQn_Type) 31) /* MIPIDSI PPI (DSI D-PHY PPI interrupt) */
        #define MIPIDSI_PPI_IRQn          ((IRQn_Type) 31) /* MIPIDSI PPI (DSI D-PHY PPI interrupt) */        /* The number of entries required for the ICU vector table. */
        #define BSP_ICU_VECTOR_NUM_ENTRIES (32)

        #ifdef __cplusplus
        }
        #endif
        #endif /* VECTOR_DATA_H */
