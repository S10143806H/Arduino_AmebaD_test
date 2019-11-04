/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      main.c
   * @brief     Source file for BLE scatternet project, mainly used for initialize modules
   * @author    jane
   * @date      2017-06-12
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <stdlib.h>
#include <os_sched.h>
#include <string.h>
#include <app_task.h>
#include <trace_app.h>
#include <gap.h>
#include <gap_bond_le.h>
#include <gap_scan.h>
#include <gap_msg.h>
#include <bte.h>
#include <gap_config.h>
#include "mesh_api.h"
#if MESH_GATT_CLIENT_SUPPORT
#include <profile_client.h>
#include <gaps_client.h>
#endif
#include <gap_adv.h>
#include <profile_server.h>
#include <gatt_builtin_services.h>
#include <platform_utils.h>
#include <bt_flags.h>

#include "mesh_cmd.h"
#include "device_app.h"
#include "health.h"
#include "ping.h"
#include "ping_app.h"
#include "tp.h"
#include "datatrans_server.h"
#include "health.h"
#include "datatrans_server_app.h"
#include "app_flags.h"
#include "vendor_cmd.h"
#include "vendor_cmd_bt.h"
#include "generic_on_off.h"
#include "gpio_api.h"
#include "gpio_irq_api.h"
#include "osdep_service.h"
#include "bt_mesh_app_lib_intf.h"

#define GPIO_LED_PIN      PA_19
#define GPIO_IRQ_PIN      PA_13

#define COMPANY_ID        0x005D
#define PRODUCT_ID        0x0000
#define VERSION_ID        0x0000

_timer device_publish_timer = NULL;

mesh_model_info_t health_server_model;
mesh_model_info_t generic_on_off_server_model;

gpio_t     gpio_led;
gpio_irq_t gpio_btn;

uint32_t last_push_button_time = 0;
generic_on_off_t current_on_off = GENERIC_OFF;

void device_publish_api(generic_on_off_t on_off)
{
    T_IO_MSG io_msg;

    io_msg.type = API_GENERIC_ON_OFF_PUBLISH;
    io_msg.subtype = on_off;
    bt_mesh_demo_send_io_msg(&io_msg);
}

void device_publish_timer_handler(void *FunctionContext)
{
    device_publish_api(current_on_off);
}

static int32_t generic_on_off_server_data(const mesh_model_info_p pmodel_info, uint32_t type, void *pargs)
{
    switch (type)
    {
        case GENERIC_ON_OFF_SERVER_GET:
            {
                generic_on_off_server_get_t *pdata = pargs;
                pdata->on_off = current_on_off;
            }
            break;
        case GENERIC_ON_OFF_SERVER_GET_DEFAULT_TRANSITION_TIME:
            break;
        case GENERIC_ON_OFF_SERVER_SET:
            {
                generic_on_off_server_set_t *pdata = pargs;
                if (pdata->total_time.num_steps == pdata->remaining_time.num_steps)
                {
                    if (pdata->on_off != current_on_off)
                    {
                        current_on_off = pdata->on_off;
                        if (current_on_off == GENERIC_OFF)
                        {
                            gpio_write(&gpio_led, 0);
                            data_uart_debug("Provisioner turn light OFF!\r\n");
                            rtw_set_timer(&device_publish_timer, 200);
                        }
                        else if (current_on_off == GENERIC_ON)
                        {
                            gpio_write(&gpio_led, 1);
                            data_uart_debug("Provisioner turn light ON!\r\n");
                            rtw_set_timer(&device_publish_timer, 200);
                        }
                    }
                }
            }
            break;
        default:
            break;
    }

    return 0;
}

void generic_on_off_server_model_init(void)
{
    generic_on_off_server_model.model_data_cb = generic_on_off_server_data;
    generic_on_off_server_reg(0, &generic_on_off_server_model);
}

void push_button_handler(uint32_t id, gpio_irq_event event)
{
    gpio_t *gpio_led = (gpio_t *)id;
    uint32_t current_time = rtw_get_current_time();
    mesh_model_info_t pmodel_info = generic_on_off_server_model;

    gpio_irq_disable(&gpio_btn);

    if (current_time > last_push_button_time && rtw_systime_to_ms(current_time - last_push_button_time) > 100) {
        if (current_on_off == GENERIC_OFF) {
            current_on_off = GENERIC_ON;
            gpio_write(gpio_led, 1);
            printf("User turn light ON!\r\n");
            device_publish_api(GENERIC_ON);
        } else if (current_on_off == GENERIC_ON) {
            current_on_off = GENERIC_OFF;
            gpio_write(gpio_led, 0);
            printf("User turn light OFF!\r\n");
            device_publish_api(GENERIC_OFF);
        }
    }

    last_push_button_time = current_time;
    gpio_irq_enable(&gpio_btn);
}

void light_button_init(void)
{
    gpio_init(&gpio_led, GPIO_LED_PIN);
    gpio_dir(&gpio_led, PIN_OUTPUT);
    gpio_mode(&gpio_led, PullNone);

    gpio_irq_init(&gpio_btn, GPIO_IRQ_PIN, push_button_handler, (uint32_t)(&gpio_led));
    gpio_irq_set(&gpio_btn, IRQ_FALL, 1);
    gpio_irq_enable(&gpio_btn);

    if (current_on_off == GENERIC_OFF)
        gpio_write(&gpio_led, 0);
    else if (current_on_off == GENERIC_ON)
        gpio_write(&gpio_led, 1);
}

/******************************************************************
 * @fn          Initial gap parameters
 * @brief      Initialize peripheral and gap bond manager related parameters
 *
 * @return     void
 */
void mesh_stack_init(void)
{
    /** set ble stack log level, disable nonsignificant log */
    log_module_bitmap_trace_set(0xFFFFFFFFFFFFFFFF, TRACE_LEVEL_TRACE, 0);
    log_module_bitmap_trace_set(0xFFFFFFFFFFFFFFFF, TRACE_LEVEL_INFO, 0);
    log_module_trace_set(TRACE_MODULE_LOWERSTACK, TRACE_LEVEL_ERROR, 0);
    log_module_trace_set(TRACE_MODULE_SNOOP, TRACE_LEVEL_ERROR, 0);

    /** set mesh stack log level, default all on, disable the log of level LEVEL_TRACE */
    uint32_t module_bitmap[MESH_LOG_LEVEL_SIZE] = {0};
    diag_level_set(TRACE_LEVEL_TRACE, module_bitmap);

    /** mesh stack needs rand seed */
    plt_srand(platform_random(0xffffffff));

    /** set device name and appearance */
    char *dev_name = "Mesh Device";
    uint16_t appearance = GAP_GATT_APPEARANCE_UNKNOWN;
    gap_sched_params_set(GAP_SCHED_PARAMS_DEVICE_NAME, dev_name, GAP_DEVICE_NAME_LEN);
    gap_sched_params_set(GAP_SCHED_PARAMS_APPEARANCE, &appearance, sizeof(appearance));

    /** configure provisioning parameters */
    prov_capabilities_t prov_capabilities =
    {
        .algorithm = PROV_CAP_ALGO_FIPS_P256_ELLIPTIC_CURVE,
        .public_key = 0,
        .static_oob = 0,
        .output_oob_size = 0,
        .output_oob_action = 0,
        .input_oob_size = 0,
        .input_oob_action = 0
    };
    prov_params_set(PROV_PARAMS_CAPABILITIES, &prov_capabilities, sizeof(prov_capabilities_t));
    prov_params_set(PROV_PARAMS_CALLBACK_FUN, (void *)prov_cb, sizeof(prov_cb_pf));

    /** config node parameters */
    mesh_node_features_t features =
    {
        .role = MESH_ROLE_DEVICE,
        .relay = 1,
        .proxy = 1,
        .fn = 0,
        .lpn = 0,
        .prov = 1,
        .udb = 1,
        .snb = 1,
        .bg_scan = 1,
        .flash = 1,
        .flash_rpl = 1
    };
    mesh_node_cfg_t node_cfg =
    {
        .dev_key_num = 2,
        .net_key_num = 3,
        .app_key_num = 3,
        .vir_addr_num = 3,
        .rpl_num = 20,
        .sub_addr_num = 10,
        .proxy_num = 1
    };
    mesh_node_cfg(features, &node_cfg);
    mesh_node.net_trans_count = 5;
    mesh_node.relay_retrans_count = 5;
    mesh_node.trans_retrans_count = 7;

    /** create elements and register models */
    mesh_element_create(GATT_NS_DESC_UNKNOWN);
    health_server_reg(0, &health_server_model);
    health_server_set_company_id(&health_server_model, COMPANY_ID);
    ping_control_reg(ping_app_ping_cb, pong_receive);
    trans_ping_pong_init(ping_app_ping_cb, pong_receive);
    tp_control_reg();
    generic_on_off_server_model_init();
    light_button_init();
    rtw_init_timer(&device_publish_timer, NULL, device_publish_timer_handler, NULL, "device_publish_timer");
    datatrans_server_model_init();
    compo_data_page0_header_t compo_data_page0_header = {COMPANY_ID, PRODUCT_ID, VERSION_ID};
    compo_data_page0_gen(&compo_data_page0_header);

    /** init mesh stack */
    mesh_init();
    /** register proxy adv callback */
    device_info_cb_reg(device_info_cb);
    hb_init(hb_cb);
}

/**
  * @brief  Initialize gap related parameters
  * @return void
  */
void app_le_gap_init(void)
{
    /* GAP Bond Manager parameters */
    uint8_t  auth_pair_mode = GAP_PAIRING_MODE_PAIRABLE;
    uint16_t auth_flags = GAP_AUTHEN_BIT_BONDING_FLAG;
    uint8_t  auth_io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
    uint8_t  auth_oob = false;
    uint8_t  auth_use_fix_passkey = false;
    uint32_t auth_fix_passkey = 0;
    uint8_t  auth_sec_req_enable = false;
    uint16_t auth_sec_req_flags = GAP_AUTHEN_BIT_BONDING_FLAG;

    /* Setup the GAP Bond Manager */
    gap_set_param(GAP_PARAM_BOND_PAIRING_MODE, sizeof(auth_pair_mode), &auth_pair_mode);
    gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(auth_flags), &auth_flags);
    gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(auth_io_cap), &auth_io_cap);
#if F_BT_LE_SMP_OOB_SUPPORT
    gap_set_param(GAP_PARAM_BOND_OOB_ENABLED, sizeof(auth_oob), &auth_oob);
#endif
    le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY, sizeof(auth_fix_passkey), &auth_fix_passkey);
    le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY_ENABLE, sizeof(auth_use_fix_passkey),
                      &auth_use_fix_passkey);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(auth_sec_req_enable), &auth_sec_req_enable);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(auth_sec_req_flags),
                      &auth_sec_req_flags);

    /* register gap message callback */
    le_register_app_cb(app_gap_callback);

#if F_BT_LE_5_0_SET_PHY_SUPPORT
    uint8_t  phys_prefer = GAP_PHYS_PREFER_ALL;
    uint8_t  tx_phys_prefer = GAP_PHYS_PREFER_1M_BIT | GAP_PHYS_PREFER_2M_BIT |
                              GAP_PHYS_PREFER_CODED_BIT;
    uint8_t  rx_phys_prefer = GAP_PHYS_PREFER_1M_BIT | GAP_PHYS_PREFER_2M_BIT |
                              GAP_PHYS_PREFER_CODED_BIT;
    le_set_gap_param(GAP_PARAM_DEFAULT_PHYS_PREFER, sizeof(phys_prefer), &phys_prefer);
    le_set_gap_param(GAP_PARAM_DEFAULT_TX_PHYS_PREFER, sizeof(tx_phys_prefer), &tx_phys_prefer);
    le_set_gap_param(GAP_PARAM_DEFAULT_RX_PHYS_PREFER, sizeof(rx_phys_prefer), &rx_phys_prefer);
#endif

    vendor_cmd_init(app_vendor_callback);
}

/**
 * @brief  Add GATT services, clients and register callbacks
 * @return void
 */
void app_le_profile_init(void)
{
    server_init(MESH_GATT_SERVER_COUNT + 1);
    /* Add Server Module */
    datatrans_server_add((void *)app_profile_callback);

    /* Register Server Callback */
    server_register_app_cb(app_profile_callback);

#if MESH_GATT_CLIENT_SUPPORT
    client_init(MESH_GATT_CLIENT_COUNT);
    /* Add Client Module */

    /* Register Client Callback--App_ClientCallback to handle events from Profile Client layer. */
    client_register_general_client_cb(app_client_callback);
#endif
}

/**
 * @brief    Contains the initialization of pinmux settings and pad settings
 * @note     All the pinmux settings and pad settings shall be initiated in this function,
 *           but if legacy driver is used, the initialization of pinmux setting and pad setting
 *           should be peformed with the IO initializing.
 * @return   void
 */
void board_init(void)
{

}

/**
 * @brief    Contains the initialization of peripherals
 * @note     Both new architecture driver and legacy driver initialization method can be used
 * @return   void
 */
void driver_init(void)
{

}

/**
 * @brief    Contains the power mode settings
 * @return   void
 */
void pwr_mgr_init(void)
{
}

/**
 * @brief    Contains the initialization of all tasks
 * @note     There is only one task in BLE Scatternet APP, thus only one APP task is init here
 * @return   void
 */
void task_init(void)
{
    app_task_init();
}

void bt_stack_config_init(void)
{
    gap_config_max_le_link_num(APP_MAX_LINKS);
}

/**
 * @brief    Entry of APP code
 * @return   int (To avoid compile warning)
 */
int ble_app_main(void)
{
    rtw_msleep_os(platform_random(2000));

    bt_trace_init();
    //bt_stack_config_init();
    bte_init();
    board_init();
    driver_init();
    le_gap_init(APP_MAX_LINKS);
    app_le_gap_init();
    app_le_profile_init();
    mesh_stack_init();
    pwr_mgr_init();
    task_init();

    return 0;
}