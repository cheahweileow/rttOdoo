/* Ethernet generated data merged from Titan_driver_eth RMAC example. */
#include "hal_data.h"

const ether_phy_lsi_cfg_t g_rmac_phy_lsi1 =
{
    .address           = 1,
    .type              = ETHER_PHY_LSI_TYPE_CUSTOM,
};

rmac_phy_instance_ctrl_t g_rmac_phy1_ctrl;

#define RA_NOT_DEFINED (1)
const rmac_phy_extended_cfg_t g_rmac_phy1_extended_cfg =
{
    .p_target_init                     = rmac_phy_target_rtl8211_initialize,
    .p_target_link_partner_ability_get = rmac_phy_target_rtl8211_is_support_link_partner_ability,
    .frame_format                      = RMAC_PHY_FRAME_FORMAT_MDIO,
    .mdc_clock_rate                    = 2500000,
    .mdio_hold_time                    = 0,
    .mdio_capture_time                 = 0,
    .p_phy_lsi_cfg_list =
    {
#if (RA_NOT_DEFINED != RA_NOT_DEFINED)
        &RA_NOT_DEFINED,
#else
        NULL,
#endif
#if (RA_NOT_DEFINED != g_rmac_phy_lsi1)
        &g_rmac_phy_lsi1,
#else
        NULL,
#endif
    },
    .default_phy_lsi_cfg_index = 1,
};
#undef RA_NOT_DEFINED

const ether_phy_cfg_t g_rmac_phy1_cfg =
{
    .channel                   = 1,
    .phy_reset_wait_time       = 0x00020000,
    .mii_bit_access_wait_time  = 0,
    .flow_control              = ETHER_PHY_FLOW_CONTROL_DISABLE,
    .mii_type                  = ETHER_PHY_MII_TYPE_RGMII,
    .p_context                 = NULL,
    .p_extend                  = &g_rmac_phy1_extended_cfg,
};

const ether_phy_instance_t g_rmac_phy1 =
{
    .p_ctrl        = &g_rmac_phy1_ctrl,
    .p_cfg         = &g_rmac_phy1_cfg,
    .p_api         = &g_ether_phy_on_rmac_phy
};

const ether_phy_lsi_cfg_t g_rmac_phy_lsi0 =
{
    .address           = 2,
    .type              = ETHER_PHY_LSI_TYPE_CUSTOM,
};

rmac_phy_instance_ctrl_t g_rmac_phy0_ctrl;

#define RA_NOT_DEFINED (1)
const rmac_phy_extended_cfg_t g_rmac_phy0_extended_cfg =
{
    .p_target_init                     = rmac_phy_target_rtl8211_initialize,
    .p_target_link_partner_ability_get = rmac_phy_target_rtl8211_is_support_link_partner_ability,
    .frame_format                      = RMAC_PHY_FRAME_FORMAT_MDIO,
    .mdc_clock_rate                    = 2500000,
    .mdio_hold_time                    = 0,
    .mdio_capture_time                 = 0,
    .p_phy_lsi_cfg_list =
    {
#if (RA_NOT_DEFINED != g_rmac_phy_lsi0)
        &g_rmac_phy_lsi0,
#else
        NULL,
#endif
#if (RA_NOT_DEFINED != RA_NOT_DEFINED)
        &RA_NOT_DEFINED,
#else
        NULL,
#endif
    },
    .default_phy_lsi_cfg_index = 0,
};
#undef RA_NOT_DEFINED

const ether_phy_cfg_t g_rmac_phy0_cfg =
{
    .channel                   = 0,
    .phy_reset_wait_time       = 0x00020000,
    .mii_bit_access_wait_time  = 0,
    .flow_control              = ETHER_PHY_FLOW_CONTROL_DISABLE,
    .mii_type                  = ETHER_PHY_MII_TYPE_RGMII,
    .p_context                 = NULL,
    .p_extend                  = &g_rmac_phy0_extended_cfg,
};

const ether_phy_instance_t g_rmac_phy0 =
{
    .p_ctrl        = &g_rmac_phy0_ctrl,
    .p_cfg         = &g_rmac_phy0_cfg,
    .p_api         = &g_ether_phy_on_rmac_phy
};

layer3_switch_instance_ctrl_t g_layer3_switch0_ctrl;

uint8_t g_layer3_switch0_mac_address_port0[6] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 };
uint8_t g_layer3_switch0_mac_address_port1[6] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 };
layer3_switch_l3_filter_t g_layer3_switch0_l3_filter_list[10];

layer3_switch_cbs_cfg_t p_cbs_cfg_list_port0 =
{
    .band_width_list    = { 0, 0, 0, 0, 0, 0, 0, 0 },
    .max_burst_num_list = { 0, 0, 0, 0, 0, 0, 0, 0 },
};

layer3_switch_cbs_cfg_t p_cbs_cfg_list_port1 =
{
    .band_width_list    = { 0, 0, 0, 0, 0, 0, 0, 0 },
    .max_burst_num_list = { 0, 0, 0, 0, 0, 0, 0, 0 },
};

layer3_switch_port_cfg_t g_layer3_switch0_port0_cfg =
{
    .p_cbs_cfg                = &p_cbs_cfg_list_port0,
    .forwarding_to_cpu_enable = true,
    .p_mac_address            = g_layer3_switch0_mac_address_port0,
    .p_callback               = NULL,
    .p_callback_memory        = NULL,
    .p_context                = NULL,
};

layer3_switch_port_cfg_t g_layer3_switch0_port1_cfg =
{
    .p_cbs_cfg                = &p_cbs_cfg_list_port1,
    .forwarding_to_cpu_enable = true,
    .p_mac_address            = g_layer3_switch0_mac_address_port1,
    .p_callback               = NULL,
    .p_callback_memory        = NULL,
    .p_context                = NULL,
};

#define RA_NOT_DEFINED (1)
const layer3_switch_extended_cfg_t g_layer3_switch0_extended_cfg =
{
    .p_ether_phy_instances =
    {
#if (RA_NOT_DEFINED != g_rmac_phy0)
        &g_rmac_phy0,
#else
        NULL,
#endif
#if (RA_NOT_DEFINED != g_rmac_phy1)
        &g_rmac_phy1,
#else
        NULL,
#endif
    },
    .fowarding_target_port_masks =
    {
        (LAYER3_SWITCH_PORT_BITMASK_PORT2 | 0U),
        (LAYER3_SWITCH_PORT_BITMASK_PORT2 | 0U),
    },
    .p_mac_addresses =
    {
        g_layer3_switch0_mac_address_port0,
        g_layer3_switch0_mac_address_port1
    },
    .ipv_queue_depth_list =
    {
        { 64, 64, 64, 64, 64, 64, 64, 64 },
        { 64, 64, 64, 64, 64, 64, 64, 64 }
    },
    .p_port_cfg_list          = { &g_layer3_switch0_port0_cfg, &g_layer3_switch0_port1_cfg },
    .l3_filter_list           = g_layer3_switch0_l3_filter_list,
    .l3_filter_list_length    = 10,
    .etha_error_irq_port_0 =
#if defined(VECTOR_NUMBER_ETHER_EAEI0)
        VECTOR_NUMBER_ETHER_EAEI0,
#else
        FSP_INVALID_VECTOR,
#endif
    .etha_error_irq_port_1 =
#if defined(VECTOR_NUMBER_ETHER_EAEI1)
        VECTOR_NUMBER_ETHER_EAEI1,
#else
        FSP_INVALID_VECTOR,
#endif
    .etha_error_ipl_port_0    = (12),
    .etha_error_ipl_port_1    = (12),
    .p_gptp_instance =
#if (RA_NOT_DEFINED != RA_NOT_DEFINED)
        &RA_NOT_DEFINED,
#else
        NULL,
#endif
    .gptp_timer_numbers       = { 0, 1 },
};
#undef RA_NOT_DEFINED

const ether_switch_cfg_t g_layer3_switch0_cfg =
{
    .channel        = 0,
#if defined(VECTOR_NUMBER_ETHER_GWDI0)
    .irq            = VECTOR_NUMBER_ETHER_GWDI0,
#else
    .irq            = FSP_INVALID_VECTOR,
#endif
    .ipl            = (12),
    .p_callback     = NULL,
    .p_context      = NULL,
    .p_extend       = &g_layer3_switch0_extended_cfg,
};

const ether_switch_instance_t g_layer3_switch0 =
{
    .p_ctrl        = &g_layer3_switch0_ctrl,
    .p_cfg         = &g_layer3_switch0_cfg,
    .p_api         = &g_ether_switch_on_layer3_switch
};

rmac_instance_ctrl_t g_ether0_ctrl;
static rmac_buffer_node_t g_ether0_buffer_node_list[24];

uint8_t g_ether0_mac_address[6] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 };

layer3_switch_ts_reception_process_descriptor_t g_ether0_ts_descriptor_array0[8];
rmac_queue_info_t g_ether0_ts_queue[1] =
{
    {
        .queue_cfg =
        {
            .array_length          = 8,
            .p_descriptor_array    = NULL,
            .p_ts_descriptor_array = g_ether0_ts_descriptor_array0,
            .ports                 = (1 << 0),
            .type                  = LAYER3_SWITCH_QUEUE_TYPE_TX,
            .write_back_mode       = LAYER3_SWITCH_WRITE_BACK_MODE_FULL,
            .descriptor_format     = LAYER3_SWITCH_DISCRIPTOR_FORMTAT_TX_TIMESTAMP,
            .rx_timestamp_storage  = LAYER3_SWITCH_RX_TIMESTAMP_STORAGE_DISABLE,
        },
    },
};

layer3_switch_descriptor_t g_ether0_tx_descriptor_array0[3 + 1];
layer3_switch_descriptor_t g_ether0_tx_descriptor_array1[3 + 1];
rmac_queue_info_t g_ether0_tx_queue_list[2] =
{
    {
        .queue_cfg =
        {
            .array_length         = 3 + 1,
            .p_descriptor_array   = g_ether0_tx_descriptor_array0,
            .p_ts_descriptor_array = NULL,
            .ports                = (1 << 0),
            .type                 = LAYER3_SWITCH_QUEUE_TYPE_TX,
            .write_back_mode      = LAYER3_SWITCH_WRITE_BACK_MODE_FULL,
            .descriptor_format    = LAYER3_SWITCH_DISCRIPTOR_FORMTAT_EXTENDED,
            .rx_timestamp_storage = LAYER3_SWITCH_RX_TIMESTAMP_STORAGE_DISABLE,
        },
    },
    {
        .queue_cfg =
        {
            .array_length         = 3 + 1,
            .p_descriptor_array   = g_ether0_tx_descriptor_array1,
            .p_ts_descriptor_array = NULL,
            .ports                = (1 << 0),
            .type                 = LAYER3_SWITCH_QUEUE_TYPE_TX,
            .write_back_mode      = LAYER3_SWITCH_WRITE_BACK_MODE_FULL,
            .descriptor_format    = LAYER3_SWITCH_DISCRIPTOR_FORMTAT_EXTENDED,
            .rx_timestamp_storage = LAYER3_SWITCH_RX_TIMESTAMP_STORAGE_DISABLE,
        },
    },
};

layer3_switch_descriptor_t g_ether0_rx_descriptor_array0[3 + 1];
layer3_switch_descriptor_t g_ether0_rx_descriptor_array1[3 + 1];
rmac_queue_info_t g_ether0_rx_queue_list[2] =
{
    {
        .queue_cfg =
        {
            .array_length         = 3 + 1,
            .p_descriptor_array   = g_ether0_rx_descriptor_array0,
            .p_ts_descriptor_array = NULL,
            .ports                = (1 << 0),
            .type                 = LAYER3_SWITCH_QUEUE_TYPE_RX,
            .write_back_mode      = LAYER3_SWITCH_WRITE_BACK_MODE_FULL,
            .descriptor_format    = LAYER3_SWITCH_DISCRIPTOR_FORMTAT_EXTENDED,
#if LAYER3_SWITCH_CFG_GPTP_ENABLE
            .rx_timestamp_storage = LAYER3_SWITCH_RX_TIMESTAMP_STORAGE_ENABLE,
#else
            .rx_timestamp_storage = LAYER3_SWITCH_RX_TIMESTAMP_STORAGE_DISABLE,
#endif
        },
    },
    {
        .queue_cfg =
        {
            .array_length         = 3 + 1,
            .p_descriptor_array   = g_ether0_rx_descriptor_array1,
            .p_ts_descriptor_array = NULL,
            .ports                = (1 << 0),
            .type                 = LAYER3_SWITCH_QUEUE_TYPE_RX,
            .write_back_mode      = LAYER3_SWITCH_WRITE_BACK_MODE_FULL,
            .descriptor_format    = LAYER3_SWITCH_DISCRIPTOR_FORMTAT_EXTENDED,
#if LAYER3_SWITCH_CFG_GPTP_ENABLE
            .rx_timestamp_storage = LAYER3_SWITCH_RX_TIMESTAMP_STORAGE_ENABLE,
#else
            .rx_timestamp_storage = LAYER3_SWITCH_RX_TIMESTAMP_STORAGE_DISABLE,
#endif
        },
    },
};

const rmac_extended_cfg_t g_ether0_extended_cfg_t =
{
    .p_ether_switch      = &g_layer3_switch0,
    .tx_queue_num        = 2,
    .rx_queue_num        = 2,
    .p_ts_queue          = g_ether0_ts_queue,
    .p_tx_queue_list     = g_ether0_tx_queue_list,
    .p_rx_queue_list     = g_ether0_rx_queue_list,
#if defined(VECTOR_NUMBER_ETHER_RMPI0)
    .rmpi_irq            = VECTOR_NUMBER_ETHER_RMPI0,
#else
    .rmpi_irq            = FSP_INVALID_VECTOR,
#endif
    .rmpi_ipl            = (BSP_IRQ_DISABLED),
    .p_buffer_node_list  = g_ether0_buffer_node_list,
    .buffer_node_num     = 24,
};

uint8_t g_ether0_ether_buffer0[1536];
uint8_t g_ether0_ether_buffer1[1536];
uint8_t g_ether0_ether_buffer2[1536];
uint8_t g_ether0_ether_buffer3[1536];
uint8_t g_ether0_ether_buffer4[1536];
uint8_t g_ether0_ether_buffer5[1536];
uint8_t g_ether0_ether_buffer6[1536];
uint8_t g_ether0_ether_buffer7[1536];
uint8_t g_ether0_ether_buffer8[1536];
uint8_t g_ether0_ether_buffer9[1536];
uint8_t g_ether0_ether_buffer10[1536];
uint8_t g_ether0_ether_buffer11[1536];
uint8_t g_ether0_ether_buffer12[1536];
uint8_t g_ether0_ether_buffer13[1536];
uint8_t g_ether0_ether_buffer14[1536];
uint8_t g_ether0_ether_buffer15[1536];
uint8_t g_ether0_ether_buffer16[1536];
uint8_t g_ether0_ether_buffer17[1536];
uint8_t g_ether0_ether_buffer18[1536];
uint8_t g_ether0_ether_buffer19[1536];
uint8_t g_ether0_ether_buffer20[1536];
uint8_t g_ether0_ether_buffer21[1536];
uint8_t g_ether0_ether_buffer22[1536];
uint8_t g_ether0_ether_buffer23[1536];

uint8_t * pp_g_ether0_ether_buffers[24] =
{
    (uint8_t *) &g_ether0_ether_buffer0[0],
    (uint8_t *) &g_ether0_ether_buffer1[0],
    (uint8_t *) &g_ether0_ether_buffer2[0],
    (uint8_t *) &g_ether0_ether_buffer3[0],
    (uint8_t *) &g_ether0_ether_buffer4[0],
    (uint8_t *) &g_ether0_ether_buffer5[0],
    (uint8_t *) &g_ether0_ether_buffer6[0],
    (uint8_t *) &g_ether0_ether_buffer7[0],
    (uint8_t *) &g_ether0_ether_buffer8[0],
    (uint8_t *) &g_ether0_ether_buffer9[0],
    (uint8_t *) &g_ether0_ether_buffer10[0],
    (uint8_t *) &g_ether0_ether_buffer11[0],
    (uint8_t *) &g_ether0_ether_buffer12[0],
    (uint8_t *) &g_ether0_ether_buffer13[0],
    (uint8_t *) &g_ether0_ether_buffer14[0],
    (uint8_t *) &g_ether0_ether_buffer15[0],
    (uint8_t *) &g_ether0_ether_buffer16[0],
    (uint8_t *) &g_ether0_ether_buffer17[0],
    (uint8_t *) &g_ether0_ether_buffer18[0],
    (uint8_t *) &g_ether0_ether_buffer19[0],
    (uint8_t *) &g_ether0_ether_buffer20[0],
    (uint8_t *) &g_ether0_ether_buffer21[0],
    (uint8_t *) &g_ether0_ether_buffer22[0],
    (uint8_t *) &g_ether0_ether_buffer23[0],
};

const ether_cfg_t g_ether0_cfg =
{
    .channel            = 0,
    .zerocopy           = ETHER_ZEROCOPY_DISABLE,
    .multicast          = ETHER_MULTICAST_ENABLE,
    .promiscuous        = ETHER_PROMISCUOUS_DISABLE,
    .flow_control       = ETHER_FLOW_CONTROL_DISABLE,
    .padding            = ETHER_PADDING_DISABLE,
    .padding_offset     = 0,
    .broadcast_filter   = 0,
    .p_mac_address      = g_ether0_mac_address,
    .num_tx_descriptors = 12,
    .num_rx_descriptors = 12,
    .pp_ether_buffers   = pp_g_ether0_ether_buffers,
    .ether_buffer_size  = 1536,
    .irq                = FSP_INVALID_VECTOR,
    .p_callback         = user_ether0_callback,
    .p_context          = NULL,
    .p_extend           = &g_ether0_extended_cfg_t,
};

const ether_instance_t g_ether0 =
{
    .p_ctrl        = &g_ether0_ctrl,
    .p_cfg         = &g_ether0_cfg,
    .p_api         = &g_ether_on_rmac,
};
