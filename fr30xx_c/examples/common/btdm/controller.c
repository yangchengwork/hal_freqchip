#include "fr30xx.h"
#include "SWD.h"

#include "controller.h"

#define CONTROLLER_CODE_SPLIT

#define HCI_UART                UART0
#define HCI_UART_IRQn           UART0_IRQn

///default 921600, set uart ref clk to pll if baudrate set to 3M
#define HCI_UART_BAUDRATE       921600  //3000000

#define CONTROLLER_PARAM_DUMMY          0x00
#define CONTROLLER_PARAM_BAUDRATE       0x01
#define CONTROLLER_PARAM_KEY            0x02
#define CONTROLLER_PARAM_BT_ADDR        0x03
#define CONTROLLER_PARAM_BLE_ADDR       0x04
#define CONTROLLER_PARAM_TX_POWER       0x05
#define CONTROLLER_PARAM_IO_CFG         0x06
#define CONTROLLER_PARAM_FT_POWER       0x07
#define CONTROLLER_PARAM_CAPABILITIES   0x08
#define CONTROLLER_PARAM_PMU_CFG        0x09
#define CONTROLLER_PARAM_HCI_TX_DELAY   0x0a
#define CONTROLLER_PARAM_LL_PKT_SIZE    0x0b
#define CONTROLLER_PARAM_DOUBLE_RATE_EN 0x0c
#define CONTROLLER_PARAM_WKUP_PULL_DOWN 0x0d

enum rf_tx_power_t {
    RF_TX_POWER_NEG_12dBm,
    RF_TX_POWER_NEG_8dBm,
    RF_TX_POWER_NEG_4dBm,
    RF_TX_POWER_0dBm,
    RF_TX_POWER_POS_4dBm,
    RF_TX_POWER_POS_8dBm,
    RF_TX_POWER_POS_10dBm,
    RF_TX_POWER_MAX,
};

typedef __PACKED_STRUCT {
    uint8_t port_sel;
    uint16_t bit_mask;
    uint16_t int_en;
    uint16_t inout_sel;
    uint16_t pull_en;
    uint16_t pull_sel;
    uint16_t out_val;
} bt_io_pmu_cfg_t;

typedef __PACKED_STRUCT {
    uint16_t tx_len;
    uint16_t tx_time;
    uint16_t rx_len;
    uint16_t rx_time;
} bt_ll_pkt_size_t;

static const uint8_t app_boot_conn_req[] = {'f','r','e','q','c','h','i','p'};//from embedded to pc, request
static const uint8_t app_boot_conn_ack[] = {'F','R','1','0','1','0','O','K'};//from pc to embedded,ack
static const uint8_t app_boot_conn_success[] = {'o','k'};

static const uint8_t controller_param_header[] = {'f', 'r', 'e', 'q'};
static const uint8_t controller_param_tail[] = {'c', 'h', 'i', 'p'};

static const uint8_t tx_pwr_table[RF_TX_POWER_MAX] = {
    0, 1, 2, 3, 4, 5, 6,
};

static const uint8_t controller_param[] = {
    0x13, 24,
    0x4a, 0xf2, 0x5a, 0x26, 0x9e, 0xcd, 0xc7, 0x39, 0x99, 0xef, 0xcf, 0x41, 0x41, 0xba, 0xdf, 0x09, 
                            0x00, 0x5b, 0x2b, 0x0f, 0x1c, 0xd7, 0x2f, 0x0d,

    0x14, 48,
    0x32, 0x35, 0x7e, 0x69, 0x69, 0xd4, 0xff, 0x0d, 0xfd, 0xb5, 0x08, 0xce, 0x48, 0xec, 0xd2, 0x03, 
                            0x8e, 0xce, 0x5a, 0x99, 0xaa, 0x32, 0x68, 0x3b, 0x5a, 0x77, 0xb7, 0xf0, 0xbd, 0xea, 0x01, 0x46, 
                            0xf9, 0xc1, 0xc9, 0x91, 0xbc, 0x93, 0xa2, 0xcf, 0x39, 0x87, 0x23, 0x7d, 0xd2, 0x46, 0x19, 0xc6,

    0x80, 32,
    0x79, 0x87, 0x07, 0x5d, 0xa1, 0xa4, 0x11, 0x68, 0xe0, 0x65, 0x15, 0x06, 0x7c, 0xb8, 0x66, 0x47, 
                            0x1f, 0x94, 0xbd, 0x7f, 0x4f, 0x9b, 0xd3, 0x15, 0xf6, 0x71, 0x68, 0x13, 0xda, 0x91, 0x75, 0x30,

    0x81, 64,                        
    0x5e, 0xa6, 0xf6, 0x32, 0xe8, 0x4d, 0x6d, 0x22, 0xaa, 0x16, 0x95, 0x67, 0xad, 0x10, 0x4e, 0x9d, 
                            0xd3, 0xde, 0xef, 0xe3, 0x46, 0xfa, 0x68, 0xf6, 0x08, 0x17, 0x9d, 0xb6, 0x1a, 0x57, 0xe1, 0x0e, 
                            0x46, 0x5c, 0xfe, 0x4f, 0x21, 0x3c, 0x41, 0xfa, 0x9b, 0xe9, 0x27, 0xb2, 0xd3, 0x7a, 0x90, 0xcb, 
                            0x9d, 0x28, 0x16, 0x41, 0x4d, 0x63, 0xcd, 0x62, 0x95, 0x3e, 0xe1, 0x21, 0x9b, 0x0d, 0x3c, 0xb6,

    0x83, 32,
    0x18, 0xa3, 0x7b, 0x04, 0x8b, 0x00, 0xc9, 0x71, 0x4b, 0xad, 0x41, 0x67, 0x42, 0xe2, 0xa8, 0x2d, 
                            0x36, 0x8a, 0xe4, 0x26, 0x51, 0x8c, 0x12, 0x6e, 0x99, 0x84, 0x4c, 0x1c, 0x78, 0x52, 0x34, 0x65,

    0x84, 64,
    0x54, 0xd1, 0xbd, 0xf0, 0xba, 0x29, 0x84, 0x6f, 0x8c, 0x4d, 0x58, 0xfa, 0x87, 0xa2, 0x49, 0xf9, 
                            0xa3, 0x8f, 0x90, 0x4b, 0x59, 0x41, 0xcc, 0x36, 0xa0, 0x5f, 0x2e, 0x5d, 0xb4, 0xb4, 0x90, 0xa6, 
                            0x40, 0x53, 0xbb, 0xaa, 0xa2, 0xec, 0xfb, 0x74, 0x74, 0xd4, 0xb2, 0x07, 0x6b, 0xc3, 0x1f, 0x14, 
                            0xf5, 0xe1, 0xa7, 0x23, 0x49, 0x48, 0xa0, 0x0b, 0x20, 0xaa, 0x2f, 0x4b, 0x95, 0x72, 0x29, 0x89,
};

///default feature
//static uint8_t bt_feature_param[] = {
//    0x41,0x08,
//    0xaf,0x2a,0x4d,0xde,0xc3,0x2f,0x5b,0x87,
//};
// const static uint8_t btdm_internal_param[] = {
//     ///default value: 0xaf,0x2a,0x4d,0xde,0xc3,0x2f,0x5b,0x87
//     ///disable 3M(B3-2),disable sniff subrating(B5-1),enable afh capable salve(B4-3)
//     0x41,0x08,
//     0xaf,0x2a,0x4d,0xda,0xcb,0x2d,0x5b,0x87,
//     ///to add
// };

#if defined(CHIP_SEL_FR3066DQC_V1) || defined(CHIP_SEL_FR3068EC_V1) || defined(CHIP_SEL_FR3066EQC)
/* 
 * BT_PA7->MCU_PB3
 * BT_PA6->MCU_PB2
 * BT_PA5->MCU_PB1
 * BT_PA4->MCU_PB0
 */
static const bt_io_pmu_cfg_t port_cfg = {
    .port_sel = 0,
    .bit_mask = 0xffff,
    .int_en = 0x0000,
    .inout_sel = 0xffff,
    .pull_en = 0xff0f,
    .pull_sel = 0xffff,
    .out_val = 0x0000,
};
#elif  defined(CHIP_SEL_FR3091EC)
#else
#error "unsupported chip!"
#endif

static const bt_ll_pkt_size_t ll_pkt_size = {
    .tx_len = 251,
    .tx_time = 2120,
    .rx_len = 251,
    .rx_time = 2120,
};

__RAM_CODE static void clear_uart_fifo(UART_HandleTypeDef *uart_handle)
{
    while(!__UART_IS_RxFIFO_EMPTY(uart_handle->UARTx)) {
        volatile uint8_t c = uart_handle->UARTx->DATA_DLL.DATA;
        (void)c;
    }
}

__RAM_CODE static void write_reg(UART_HandleTypeDef *uart_handle, uint32_t address, uint32_t data)
{
    uint8_t opcode;
    uint8_t buffer[6];
    opcode = 0x27;  // write reg
    buffer[0] = 0;   
    buffer[1] = 0;
    buffer[2] = 0;
    buffer[3] = 0;
    buffer[4] = 8;
    buffer[5] = 0;
    uart_transmit(uart_handle, (void *)&opcode, 1);
    uart_transmit(uart_handle, (void *)&buffer, 6);
    uart_transmit(uart_handle, (void *)&address, 4);
    uart_transmit(uart_handle, (void *)&data, 4);
}

__RAM_CODE static void set_bypass(UART_HandleTypeDef *uart_handle)
{
    /* reset controller first */
    pmu_set_pin_dir(PMU_PIN_1, PMU_GPIO_MODE_OUTPUT);
    pmu_set_pin_value(PMU_PIN_1, 0);
    system_delay_us(1000);
    pmu_set_pin_dir(PMU_PIN_1, PMU_GPIO_MODE_INPUT);
    
    system_delay_us(15000);
    clear_uart_fifo(uart_handle);   // clear handshake data
    uart_transmit(uart_handle, (void *)app_boot_conn_ack, 8);
    system_delay_us(5000);
    clear_uart_fifo(uart_handle);   // clear handshake data
    
    write_reg(uart_handle, 0x500f0000,0x00a83112);
    system_delay_us(3000);
    clear_uart_fifo(uart_handle);   // clear response data
    write_reg(uart_handle, 0x500f0004,0x00000076);
    system_delay_us(3000);
    clear_uart_fifo(uart_handle);   // clear response data
    write_reg(uart_handle, 0x500f0000,0x00a83113);
    system_delay_us(3000);
    clear_uart_fifo(uart_handle);   // clear response data
    
    /* reset controller first */
    pmu_set_pin_dir(PMU_PIN_1, PMU_GPIO_MODE_OUTPUT);
    pmu_set_pin_value(PMU_PIN_1, 0);
    system_delay_us(1000);
    pmu_set_pin_dir(PMU_PIN_1, PMU_GPIO_MODE_INPUT);
}

bool controller_start(uint32_t baudrate, const uint8_t *ble_addr, const uint8_t *bt_addr, uint32_t src_addr)
{
    /* hardware handlers */
    UART_HandleTypeDef HCI_handle;
    GPIO_InitTypeDef gpio_config;
    uint8_t buffer[12];
    uint32_t length;
    uint8_t *src, *dst;
    uint16_t tx_length;
    uint8_t opcode, param_type;
    struct {
        uint32_t dst;
        uint32_t len;
    } header;

#if defined(CHIP_SEL_FR3066DQC_V1) || defined(CHIP_SEL_FR3068EC_V1) || defined(CHIP_SEL_FR3091EC) || defined(CHIP_SEL_FR3066EQC)
    /* reset controller first */
    pmu_set_pin_dir(PMU_PIN_1, PMU_GPIO_MODE_OUTPUT);
    pmu_set_pin_value(PMU_PIN_1, 0);
    system_delay_us(1000);
    pmu_set_pin_dir(PMU_PIN_1, PMU_GPIO_MODE_INPUT);

    enum_PMU_3V3_ioldo_voltage_t vol_setting = pmu_get_3V3_ioldo_voltage();
    pmu_set_3V3_ioldo_voltage(PMU_3V3_IOLDO_1_9V);
#endif
    
    /* configure PA0, PA1, PA2, PA3 to UART0 function */
    gpio_config.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3;
    gpio_config.Mode = GPIO_MODE_AF_PP;
    gpio_config.Pull = GPIO_PULLUP;
    gpio_config.Alternate = GPIO_FUNCTION_1;
    gpio_init(GPIOA, &gpio_config);
    
    /* UART0: used for Log and AT command */
    __SYSTEM_UART0_CLK_ENABLE();
    HCI_handle.UARTx = HCI_UART;
    HCI_handle.Init.BaudRate   = 115200;
    HCI_handle.Init.DataLength = UART_DATA_LENGTH_8BIT;
    HCI_handle.Init.StopBits   = UART_STOPBITS_1;
    HCI_handle.Init.Parity     = UART_PARITY_NONE;
    HCI_handle.Init.FIFO_Mode  = UART_FIFO_ENABLE;
    HCI_handle.TxCpltCallback  = NULL;
    HCI_handle.RxCpltCallback  = NULL;
    uart_init(&HCI_handle);
    /* keep RTS is inactive before HCI is ready */
    __UART_AUTO_FLOW_CONTROL_DISABLE(HCI_handle.UARTx);
    __UART_RTS_INACTIVE(HCI_handle.UARTx);
    
#if defined(CHIP_SEL_FR3066DQC_V1) || defined(CHIP_SEL_FR3068EC_V1) || defined(CHIP_SEL_FR3091EC) || defined(CHIP_SEL_FR3066EQC)
    set_bypass(&HCI_handle);
#endif
    
    /* hand shake with controller */
    uart_receive(&HCI_handle, buffer, 8);
    while (memcmp(buffer, app_boot_conn_req, 8)) {
        for (uint8_t i=0; i<7; i++) {
            buffer[i] = buffer[i+1];
        }
        uart_receive(&HCI_handle, &buffer[7], 1);
    }

    uart_transmit(&HCI_handle, (void *)app_boot_conn_ack, 8);
    uart_receive(&HCI_handle, buffer, 2);
    if (memcmp(buffer, app_boot_conn_success, 2)) {
        return false;
    }
    
    if (src_addr == 0) {
        /* read ROM code */
        opcode = 0x0a;
        buffer[0] = 4;   
        buffer[1] = 0;
        buffer[2] = 0;
        buffer[3] = 0;
        buffer[4] = 4;
        buffer[5] = 0;
        uart_transmit(&HCI_handle, (void *)&opcode, 1);
        uart_transmit(&HCI_handle, (void *)&buffer, 6);
        uart_receive(&HCI_handle, buffer, 11);
#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
        if (buffer[7] == 0x79) {
            extern uint8_t CONTROLLER_CODE_REVC_BASE;
            src_addr = (uint32_t)&CONTROLLER_CODE_REVC_BASE;
        }
        else if (buffer[7] == 0x75) {
            extern uint8_t CONTROLLER_CODE_REVB_BASE;
            src_addr = (uint32_t)&CONTROLLER_CODE_REVB_BASE;
        }
        else {
            while(1);
        }
#elif defined(__GNUC__) || defined(__ICCARM__)
        if (buffer[7] == 0x79) {
            extern const uint8_t controller_code_revc_buffer[];
            src_addr = (uint32_t)&controller_code_revc_buffer[0];
        }
        else if (buffer[7] == 0x75) {
            extern const uint8_t controller_code_buffer[];
            src_addr = (uint32_t)&controller_code_buffer[0];
        }
        else {
            while(1);
        }
#else
#error "not supported platform"
#endif
        
    }
    src_addr += 0x10;
    
        /* SWD Enable RAM */
    SWD_W_SystemReg(HCI_UART_BAUDRATE);
    
    /* change uart baudrate */
//    opcode = 0x12;
//    buffer[0] = 11;   // 921600
//    buffer[1] = 0;
//    buffer[2] = 0;
//    buffer[3] = 0;
//    buffer[4] = 0;
//    buffer[5] = 0;
//    uart_transmit(&HCI_handle, (void *)&opcode, 1);
//    uart_transmit(&HCI_handle, (void *)&buffer, 6);
//    uart_receive(&HCI_handle, buffer, 7);
//    if (buffer[0] != 0x13) {
//        return false;
//    }
#if defined(CHIP_SEL_FR3066DQC_V1) || defined(CHIP_SEL_FR3068EC_V1) || defined(CHIP_SEL_FR3091EC) || defined(CHIP_SEL_FR3066EQC)   
    pmu_set_3V3_ioldo_voltage(vol_setting);
#endif

    HCI_handle.Init.BaudRate   = HCI_UART_BAUDRATE;
    uart_config_baudRate(&HCI_handle);
    system_delay_us(5000);
    
    memcpy((void *)&header, (void *)src_addr, sizeof(header));
    src_addr += sizeof(header);
    
    /* write code into RAM */
    opcode = 0x04;
    src = (void *)src_addr;
    dst = (void *)header.dst;
    length = header.len;
    while (length) {
        tx_length = length > 256 ? 256 : length;
        uart_transmit(&HCI_handle, (void *)&opcode, 1);
        uart_transmit(&HCI_handle, (void *)&dst, 4);
        uart_transmit(&HCI_handle, (void *)&tx_length, 2);
        uart_transmit(&HCI_handle, src, tx_length);
        
        uart_receive(&HCI_handle, buffer, 7);
        if (buffer[0] != 0x05) {
            return false;
        }
        
        length -= tx_length;
        src += tx_length;
        dst += tx_length;
    }
    src_addr += header.len;

    /* write parameters to exchange memory */
    opcode = 0x04;
    dst = (void *)0x40014000;
    /* write header to remote device */
    uart_transmit(&HCI_handle, (void *)&opcode, 1);
    uart_transmit(&HCI_handle, (void *)&dst, 4);
    length = sizeof(controller_param_header);
    dst += length;
    uart_transmit(&HCI_handle, (void *)&length, 2);
    uart_transmit(&HCI_handle, (void *)&controller_param_header[0], length);
    uart_receive(&HCI_handle, buffer, 7);
    if (buffer[0] != 0x05) {
        return false;
    }
    /* write baudrate to controller */
    uart_transmit(&HCI_handle, (void *)&opcode, 1);
    uart_transmit(&HCI_handle, (void *)&dst, 4);
    length = sizeof(baudrate) + 1 + 2;
    dst += length;
    uart_transmit(&HCI_handle, (void *)&length, 2);
    param_type = CONTROLLER_PARAM_BAUDRATE;
    uart_transmit(&HCI_handle, (void *)&param_type, 1);
    length = sizeof(baudrate);
    uart_transmit(&HCI_handle, (void *)&length, 2);
    uart_transmit(&HCI_handle, (void *)&baudrate, length);
    uart_receive(&HCI_handle, buffer, 7);
    if (buffer[0] != 0x05) {
        return false;
    }

    // /* write keys to controller */
    // length = flashdb_get_length(FDB_KEY_CONTROLLER_INFO) + sizeof(btdm_internal_param);
    // if (length) {
    //     uint8_t *tmp = pvPortMalloc(length);
    //     uint16_t sub_length = length;
    //     uint16_t key_len = length - sizeof(btdm_internal_param);
    //     if(key_len){
    //         flashdb_get(FDB_KEY_CONTROLLER_INFO, tmp, key_len);
    //         memcpy(&tmp[key_len],btdm_internal_param,sizeof(btdm_internal_param));
    //     }
    //     else{
    //         memcpy(&tmp[0],btdm_internal_param,sizeof(btdm_internal_param));
    //     }
    //     uart_transmit(&HCI_handle, (void *)&opcode, 1);
    //     uart_transmit(&HCI_handle, (void *)&dst, 4);
        
    //     length = sub_length + 1 + 2;
    //     dst += length;
    //     uart_transmit(&HCI_handle, (void *)&length, 2);
    //     param_type = CONTROLLER_PARAM_KEY;
    //     uart_transmit(&HCI_handle, (void *)&param_type, 1);
    //     length = sub_length;
    //     uart_transmit(&HCI_handle, (void *)&length, 2);
    //     uart_transmit(&HCI_handle, (void *)&tmp[0], length);
    //     uart_receive(&HCI_handle, buffer, 7);
    //     if (buffer[0] != 0x05) {
    //         vPortFree(tmp);
    //         return false;
    //     }
    //     vPortFree(tmp);
    // }

    uart_transmit(&HCI_handle, (void *)&opcode, 1);
    uart_transmit(&HCI_handle, (void *)&dst, 4);
    
    length = sizeof(controller_param) + 1 + 2;
    dst += length;
    uart_transmit(&HCI_handle, (void *)&length, 2);
    param_type = CONTROLLER_PARAM_KEY;
    uart_transmit(&HCI_handle, (void *)&param_type, 1);
    length = sizeof(controller_param);
    uart_transmit(&HCI_handle, (void *)&length, 2);
    uart_transmit(&HCI_handle, (void *)&controller_param[0], length);
    uart_receive(&HCI_handle, buffer, 7);
    if (buffer[0] != 0x05) {
        return false;
    }

    /* write bt address to controller */
    uart_transmit(&HCI_handle, (void *)&opcode, 1);
    uart_transmit(&HCI_handle, (void *)&dst, 4);
    length = 6 + 1 + 2;
    dst += length;
    uart_transmit(&HCI_handle, (void *)&length, 2);
    param_type = CONTROLLER_PARAM_BT_ADDR;
    uart_transmit(&HCI_handle, (void *)&param_type, 1);
    length = 6;
    uart_transmit(&HCI_handle, (void *)&length, 2);
    uart_transmit(&HCI_handle, (void *)&bt_addr[0], length);
    uart_receive(&HCI_handle, buffer, 7);
    if (buffer[0] != 0x05) {
        return false;
    }

    /* write ble address to controller */
    uart_transmit(&HCI_handle, (void *)&opcode, 1);
    uart_transmit(&HCI_handle, (void *)&dst, 4);
    length = 6 + 1 + 2;
    dst += length;
    uart_transmit(&HCI_handle, (void *)&length, 2);
    param_type = CONTROLLER_PARAM_BLE_ADDR;
    uart_transmit(&HCI_handle, (void *)&param_type, 1);
    length = 6;
    uart_transmit(&HCI_handle, (void *)&length, 2);
    uart_transmit(&HCI_handle, (void *)&ble_addr[0], length);
    uart_receive(&HCI_handle, buffer, 7);
    if (buffer[0] != 0x05) {
        return false;
    }
    
    if(trim_get_txpower_param() != 0x7fff){
    /* write FT paramer base tx power to controller */
        uart_transmit(&HCI_handle, (void *)&opcode, 1);
        uart_transmit(&HCI_handle, (void *)&dst, 4);
        length = 2 + 1 + 2;
        dst += length;
        uart_transmit(&HCI_handle, (void *)&length, 2);
        param_type = CONTROLLER_PARAM_FT_POWER;
        uart_transmit(&HCI_handle, (void *)&param_type, 1);
        length = 2;
        uint8_t tx_power[2] = {0};
        tx_power[0] =  (trim_get_txpower_param()-200)&0xff;
        tx_power[1] = ((trim_get_txpower_param()-200)&0xff00) >> 8;
        uart_transmit(&HCI_handle, (void *)&length, 2);
        uart_transmit(&HCI_handle, tx_power, length);
        uart_receive(&HCI_handle, buffer, 7);
        if (buffer[0] != 0x05) {
            return false;
        }
    }

    /* write tx power to controller */
    uart_transmit(&HCI_handle, (void *)&opcode, 1);
    uart_transmit(&HCI_handle, (void *)&dst, 4);
    length = 1 + 1 + 2;
    dst += length;
    uart_transmit(&HCI_handle, (void *)&length, 2);
    param_type = CONTROLLER_PARAM_TX_POWER;
    uart_transmit(&HCI_handle, (void *)&param_type, 1);
    length = 1;
    uint8_t tx_power_cfg = tx_pwr_table[RF_TX_POWER_0dBm];
    uart_transmit(&HCI_handle, (void *)&length, 2);
    uart_transmit(&HCI_handle, &tx_power_cfg, length);
    uart_receive(&HCI_handle, buffer, 7);
    if (buffer[0] != 0x05) {
        return false;
    }

//    /* write pmu cfg to controller */
//    const uint8_t pmu_cfg[] = {0xa7, 0x05};
//    uart_transmit(&HCI_handle, (void *)&opcode, 1);
//    uart_transmit(&HCI_handle, (void *)&dst, 4);
//    length = sizeof(pmu_cfg) + 1 + 2;
//    dst += length;
//    uart_transmit(&HCI_handle, (void *)&length, 2);
//    param_type = CONTROLLER_PARAM_PMU_CFG;
//    uart_transmit(&HCI_handle, (void *)&param_type, 1);
//    length = sizeof(pmu_cfg);
//    uart_transmit(&HCI_handle, (void *)&length, 2);
//    uart_transmit(&HCI_handle, (uint8_t *)&pmu_cfg[0], length);
//    uart_receive(&HCI_handle, buffer, 7);
//    if (buffer[0] != 0x05) {
//        return false;
//    }    

#if !defined(CHIP_SEL_FR3091EC)
    /* write IO configuration to controller */
    uart_transmit(&HCI_handle, (void *)&opcode, 1);
    uart_transmit(&HCI_handle, (void *)&dst, 4);
    length = sizeof(port_cfg) + 1 + 2;
    dst += length;
    uart_transmit(&HCI_handle, (void *)&length, 2);
    param_type = CONTROLLER_PARAM_IO_CFG;
    uart_transmit(&HCI_handle, (void *)&param_type, 1);
    length = sizeof(port_cfg);
    uart_transmit(&HCI_handle, (void *)&length, 2);
    uart_transmit(&HCI_handle, (uint8_t *)&port_cfg, length);
    uart_receive(&HCI_handle, buffer, 7);
    if (buffer[0] != 0x05) {
        return false;
    }
#endif
    
    /* write baseband capabilities to controller */
    const uint8_t bb_cap[] = {0x00/*coded_phy_sel*/, 10/*con_max*/, 2/*adv_max*/, 37, 0x00, 8/*ble_tx*/, 8/*ble_rx*/, 6/*bt_tx*/, 6/*bt_rx*/};
    uart_transmit(&HCI_handle, (void *)&opcode, 1);
    uart_transmit(&HCI_handle, (void *)&dst, 4);
    length = sizeof(bb_cap) + 1 + 2;
    dst += length;
    uart_transmit(&HCI_handle, (void *)&length, 2);
    param_type = CONTROLLER_PARAM_CAPABILITIES;
    uart_transmit(&HCI_handle, (void *)&param_type, 1);
    length = sizeof(bb_cap);
    uart_transmit(&HCI_handle, (void *)&length, 2);
    uart_transmit(&HCI_handle, (uint8_t *)&bb_cap[0], length);
    uart_receive(&HCI_handle, buffer, 7);
    if (buffer[0] != 0x05) {
        return false;
    }
    
    if(trim_cp_get_version() == CP_VERSION_B){
        const uint8_t tx_delay = 50;
        uart_transmit(&HCI_handle, (void *)&opcode, 1);
        uart_transmit(&HCI_handle, (void *)&dst, 4);
        length = 1 + 1 + 2;
        dst += length;
        uart_transmit(&HCI_handle, (void *)&length, 2);
        param_type = CONTROLLER_PARAM_HCI_TX_DELAY;
        uart_transmit(&HCI_handle, (void *)&param_type, 1);
        length = 1;
        uart_transmit(&HCI_handle, (void *)&length, 2);
        uart_transmit(&HCI_handle, (uint8_t *)&tx_delay, length);
        uart_receive(&HCI_handle, buffer, 7);
        if (buffer[0] != 0x05) {
            return false;
        }
    }

    /* write custom link layer packet size to controller */
    uart_transmit(&HCI_handle, (void *)&opcode, 1);
    uart_transmit(&HCI_handle, (void *)&dst, 4);
    length = sizeof(ll_pkt_size) + 1 + 2;
    dst += length;
    uart_transmit(&HCI_handle, (void *)&length, 2);
    param_type = CONTROLLER_PARAM_LL_PKT_SIZE;
    uart_transmit(&HCI_handle, (void *)&param_type, 1);
    length = sizeof(ll_pkt_size);
    uart_transmit(&HCI_handle, (void *)&length, 2);
    uart_transmit(&HCI_handle, (uint8_t *)&ll_pkt_size, length);
    uart_receive(&HCI_handle, buffer, 7);
    if (buffer[0] != 0x05) {
        return false;
    }
    
//    /* enable double rate mode */
//    uart_transmit(&HCI_handle, (void *)&opcode, 1);
//    uart_transmit(&HCI_handle, (void *)&dst, 4);
//    length = 2 + 1 + 2;
//    dst += length;
//    uart_transmit(&HCI_handle, (void *)&length, 2);
//    param_type = CONTROLLER_PARAM_DOUBLE_RATE_EN;
//    uart_transmit(&HCI_handle, (void *)&param_type, 1);
//    length = 2;
//    uart_transmit(&HCI_handle, (void *)&length, 2);
//    buffer[0] = 1;
//    buffer[1] = 32;
//    uart_transmit(&HCI_handle, (uint8_t *)&buffer[0], length);
//    uart_receive(&HCI_handle, buffer, 7);
//    if (buffer[0] != 0x05) {
//        return false;
//    }
    
//    /* set wake up pin to pull down mode */
//    uart_transmit(&HCI_handle, (void *)&opcode, 1);
//    uart_transmit(&HCI_handle, (void *)&dst, 4);
//    length = 1 + 1 + 2;
//    dst += length;
//    uart_transmit(&HCI_handle, (void *)&length, 2);
//    param_type = CONTROLLER_PARAM_WKUP_PULL_DOWN;
//    uart_transmit(&HCI_handle, (void *)&param_type, 1);
//    length = 1;
//    uint8_t dummy = 1;
//    uart_transmit(&HCI_handle, (void *)&length, 2);
//    uart_transmit(&HCI_handle, &dummy, length);
//    uart_receive(&HCI_handle, buffer, 7);
//    if (buffer[0] != 0x05) {
//        return false;
//    }

    /* write tail to controller */
    uart_transmit(&HCI_handle, (void *)&opcode, 1);
    uart_transmit(&HCI_handle, (void *)&dst, 4);
    length = sizeof(controller_param_tail);
    dst += length;
    uart_transmit(&HCI_handle, (void *)&length, 2);
    uart_transmit(&HCI_handle, (void *)&controller_param_tail[0], length);
    uart_receive(&HCI_handle, buffer, 7);
    if (buffer[0] != 0x05) {
        return false;
    }
    
    /* boot from RAM */
    opcode = 0x2b;
    dst = (void *)header.dst;
    tx_length = 0;
    uart_transmit(&HCI_handle, (void *)&opcode, 1);
    uart_transmit(&HCI_handle, (void *)&dst, 4);
    uart_transmit(&HCI_handle, (void *)&tx_length, 2);
    uart_receive(&HCI_handle, buffer, 7);
    if (buffer[0] != 0x2c) {
        return false;
    }

    memcpy((void *)&header, (void *)src_addr, sizeof(header));
    src_addr += sizeof(header);
    if (header.len) {
        system_delay_us(50000);
        opcode = 0x04;
        src = (void *)src_addr;
        dst = (void *)header.dst;
        length = header.len;
        while (length) {
            tx_length = length > 256 ? 256 : length;
            uart_transmit(&HCI_handle, (void *)&opcode, 1);
            uart_transmit(&HCI_handle, (void *)&dst, 4);
            uart_transmit(&HCI_handle, (void *)&tx_length, 2);
            uart_transmit(&HCI_handle, src, tx_length);
            
            uart_receive(&HCI_handle, buffer, 7);
            if (buffer[0] != 0x05) {
                return false;
            }
            
            length -= tx_length;
            src += tx_length;
            dst += tx_length;
        }
        src_addr += header.len;
    
        /* disconnect */
        opcode = 0x10;
        dst = (void *)1;    /* normal disconnect */
        tx_length = 0;
        uart_transmit(&HCI_handle, (void *)&opcode, 1);
        uart_transmit(&HCI_handle, (void *)&dst, 4);
        uart_transmit(&HCI_handle, (void *)&tx_length, 2);
        uart_receive(&HCI_handle, buffer, 7);
        if (buffer[0] != 0x11) {
            return false;
        }
    }
//#if defined(CHIP_SEL_FR3066DQC_V1) || defined(CHIP_SEL_FR3068EC_V1) || defined(CHIP_SEL_FR3091EC)
//    system_delay_us(10000);
//    pmu_set_3V3_ioldo_voltage(vol_setting);
//#endif

    return true;
}
