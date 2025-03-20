#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "fr30xx.h"

#include "btdm_host.h"
#include "FreeRTOS.h"
#include "timers.h"
#include "semphr.h"

#include "fdb_app.h"
#include "host.h"

#include "me_api.h"
#include "bt_types.h"
#include "app_config.h"

#define HOST_DDB_RECORD_COUNT       8
#define HOST_DDB_INFOR_VERSION      0x01

#define HCI_UART            UART0
#define HCI_UART_IRQn       UART0_IRQn

/* Uart backup/restore */
typedef struct
{
    volatile uint8_t BAK_DLL;
    volatile uint8_t BAK_DLH;
    volatile uint8_t BAK_DLF;
    volatile uint8_t BAK_IER;
    volatile uint8_t BAK_FCR;
    volatile uint8_t BAK_LCR;
    volatile uint8_t BAK_MCR;
}struct_UartRES_t;

struct host_ddb_info {
    uint8_t version;
    uint8_t used_count;
    BtDeviceRecord ddb[HOST_DDB_RECORD_COUNT];
};

static TimerHandle_t btdm_host_timer = NULL;
static bool btdm_host_timer_inited = false;

static const uint8_t *write_buffer = 0;
static uint32_t write_length = 0;
static void (*write_callback)(void) = 0;

static uint8_t *read_buffer = 0;
static uint32_t read_length = 0;
static void (*read_callback)(void *, uint8_t) = 0;
static void *read_dummy = 0;

/* host semaphore handle */
SemaphoreHandle_t host_Semaphore_handle;

/* host task handle */
static TaskHandle_t btdm_host_handle;

/* hardware handlers */
static UART_HandleTypeDef HCI_handle;

/* structure used to save and restore uart configuration when sleep mode is enabled */
static struct_UartRES_t uart_regs;

#define READ_REG(__ADDR__)                (*(volatile uint32_t *)(__ADDR__))
#define WRTIE_REG(__ADDR__, __VALUE__)    (*(volatile uint32_t *)(__ADDR__) = (__VALUE__))

/************************************************************************************
 * @fn      Uart_backup/Uart_restore
 *
 * @brief   Uart low-power backup/restore
 */
__RAM_CODE static void __Uart_backup(struct_UART_t *Uartx, struct_UartRES_t *UartRES)
{
    volatile uint32_t UartxBase = (uint32_t)Uartx;

    UartRES->BAK_LCR = READ_REG(UartxBase + 0x0C);
    UartRES->BAK_MCR = READ_REG(UartxBase + 0x10);
    UartRES->BAK_FCR = 0x09;
    UartRES->BAK_IER = READ_REG(UartxBase + 0x04);

    WRTIE_REG(UartxBase + 0x0C, UartRES->BAK_LCR | 0x80);

    UartRES->BAK_DLL = READ_REG(UartxBase);
    UartRES->BAK_DLH = READ_REG(UartxBase + 0x04);
    UartRES->BAK_DLF = READ_REG(UartxBase + 0xC0);

    WRTIE_REG(UartxBase + 0x0C, UartRES->BAK_LCR);
}
__RAM_CODE static void __Uart_restore(struct_UART_t *Uartx, struct_UartRES_t *UartRES)
{
    volatile uint32_t UartxBase = (uint32_t)Uartx;

    WRTIE_REG(UartxBase + 0x04, UartRES->BAK_IER);
    WRTIE_REG(UartxBase + 0x08, UartRES->BAK_FCR);
    WRTIE_REG(UartxBase + 0x10, UartRES->BAK_MCR);
    WRTIE_REG(UartxBase + 0x0C, UartRES->BAK_LCR | 0x80);
    WRTIE_REG(UartxBase,        UartRES->BAK_DLL);
    WRTIE_REG(UartxBase + 0x04, UartRES->BAK_DLH);
    WRTIE_REG(UartxBase + 0xC0, UartRES->BAK_DLF);
    WRTIE_REG(UartxBase + 0x0C, UartRES->BAK_LCR);
}

static void hci_uart_rx_callback(UART_HandleTypeDef *h)
{
    read_length = 0;
    read_callback(read_dummy, 0);
}

static void hci_uart_tx_callback(UART_HandleTypeDef *h)
{
    write_length = 0;
    write_callback();
    system_prevent_sleep_clear(SYSTEM_PREVENT_SLEEP_TYPE_HCI_TX);
}

static void vTimerCallback( TimerHandle_t pxTimer )
{
    btdm_timer_trigger();
}

void btdm_timer_start(uint32_t ms)
{
    if (!btdm_host_timer_inited) {
        btdm_host_timer = xTimerCreate( "Timer", ms / portTICK_PERIOD_MS, pdTRUE, NULL, vTimerCallback);
        btdm_host_timer_inited = true;
    }
    xTimerStop(btdm_host_timer, portMAX_DELAY);
    xTimerChangePeriod(btdm_host_timer, ms / portTICK_RATE_MS, portMAX_DELAY);
    xTimerStart(btdm_host_timer, portMAX_DELAY);
}

void btdm_timer_stop(void)
{
    if(btdm_host_timer) {
        xTimerStop(btdm_host_timer, portMAX_DELAY);
    }
}

uint32_t btdm_get_system_time(void)
{
    return xTaskGetTickCount() * portTICK_RATE_MS;
}

int btdm_time_diff(uint32_t a_time, uint32_t b_time)
{
    return b_time - a_time;
}

void hci_if_do_send(const uint8_t *buffer, uint32_t length, void (*func)(void))
{
    struct app_task_event * event;
    uint32_t print_size = length > 8 ? 8 : length;
    
    assert(write_length == 0);

    write_buffer = buffer;
    write_length = length;
    write_callback = func;

#if 1
    system_prevent_sleep_set(SYSTEM_PREVENT_SLEEP_TYPE_HCI_TX);
    uart_transmit_IT(&HCI_handle, (void *)buffer, length);
#else
    uart_transmit(&HCI_handle, (void *)buffer, length);
    write_callback();
    write_length = 0;
#endif
}

void hci_if_do_recv(uint8_t *bufptr, uint32_t size, void *arg, void* dummy)
{
    struct app_task_event * event;
    
    assert(read_length == 0);

    read_buffer = bufptr;
    read_length = size;
    read_callback = (void (*)(void *, uint8_t))arg;
    read_dummy = dummy;
    
    uart_receive_IT(&HCI_handle, bufptr, size);
}

void btdm_host_notify_schedule(void)
{
    if(xPortIsInsideInterrupt() || __get_BASEPRI()) {
        BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(btdm_host_handle, &pxHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
    }
    else {
        xTaskNotifyGive(btdm_host_handle);
    }
}

__WEAK void host_ready_cb(void)
{
//    struct app_task_event *event;
//    /* notify application BTDM stack is ready. */
//    event = app_task_event_alloc(APP_TASK_EVENT_HOST_INITED, 0, true);
//    app_task_event_post(event, false);
}

static void host_hw_init(uint32_t baudrate)
{
    GPIO_InitTypeDef gpio_config;
    /* configure PA0 and PA1 to UART0 function */
    gpio_config.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3;
    gpio_config.Mode = GPIO_MODE_AF_PP;
    gpio_config.Pull = GPIO_PULLUP;
    gpio_config.Alternate = GPIO_FUNCTION_1;
    gpio_init(GPIOA, &gpio_config);
    
    /* UART0: used for Log and AT command */
    __SYSTEM_UART0_RESET();
    HCI_handle.UARTx = HCI_UART;
    HCI_handle.Init.BaudRate   = baudrate;
    HCI_handle.Init.DataLength = UART_DATA_LENGTH_8BIT;
    HCI_handle.Init.StopBits   = UART_STOPBITS_1;
    HCI_handle.Init.Parity     = UART_PARITY_NONE;
    HCI_handle.Init.FIFO_Mode  = UART_FIFO_ENABLE;
    HCI_handle.TxCpltCallback  = hci_uart_tx_callback;
    HCI_handle.RxCpltCallback  = hci_uart_rx_callback;
    uart_init(&HCI_handle);
    /* keep RTS is inactive before HCI is ready */
    __UART_AUTO_FLOW_CONTROL_DISABLE(HCI_handle.UARTx);
    __UART_RTS_INACTIVE(HCI_handle.UARTx);    
//    __UART_RxFIFO_THRESHOLD((&HCI_handle), 2);
    NVIC_SetPriority(HCI_UART_IRQn, 2);
    NVIC_EnableIRQ(HCI_UART_IRQn);
}

static void host_btdm_task(void *ble_static_addr)
{
    struct host_ddb_info *info;
    size_t size;

    /* Initialize BLE stack */
    struct ble_host_param param;
    if (ble_static_addr) {
        param.own_addr_type = 1;
        memcpy(&param.own_addr.addr[0], ble_static_addr, 6);
        vPortFree(ble_static_addr);
    }
    else {
        param.own_addr_type = 0;
    }

    host_Semaphore_handle = xSemaphoreCreateRecursiveMutex();

    ble_host_init(&param);

    /* HCI is ready */
    __UART_AUTO_FLOW_CONTROL_ENABLE(HCI_handle.UARTx);
    __UART_RTS_ACTIVE(HCI_handle.UARTx);
    while(ble_host_ready() == false) {
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        btdm_host_schedule_ble();
    }

    /* check whether link key information stored in flashdb is valid or not */
    info = pvPortMalloc(sizeof(struct host_ddb_info));
    size = flashdb_get(FDB_KEY_BT_LINKKEY, (void *)info, sizeof(struct host_ddb_info));
    if ((size != 0)
        && ((sizeof(struct host_ddb_info) != size)
            || (info->version != HOST_DDB_INFOR_VERSION)
            || (info->used_count > HOST_DDB_RECORD_COUNT))) {
        flashdb_del(FDB_KEY_BT_LINKKEY);
    }
    vPortFree(info);
    
    // Initialize BT stack
    bt_host_init();
#if BTDM_STACK_ENABLE_A2DP_SNK | BTDM_STACK_ENABLE_A2DP_SRC
    bt_a2dp_init();
#endif
#if BTDM_STACK_ENABLE_AVRCP
    bt_avrcp_init();
#endif
#if BTDM_STACK_ENABLE_HF
    bt_hf_init();
#endif
#if BTDM_STACK_ENABLE_AG
    bt_hfg_init();
#endif

#if BTDM_STACK_ENABLE_PAN
    bt_pan_init();
#endif
#if BTDM_STACK_ENABLE_PBAP
    bt_pbap_init();
#endif
#if BTDM_STACK_ENABLE_SPP
    bt_spp_init();
#endif
#if BTDM_STACK_ENABLE_HID
    bt_hid_init();
#endif
#if BTDM_STACK_ENABLE_MAP
    bt_map_init();
#endif

    while(bt_host_ready() == false) {
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        btdm_host_schedule();
    }

    host_ready_cb();

    while(1) {
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        btdm_host_schedule();
    }
}

static void host_ble_task(void *ble_static_addr)
{
    struct app_task_event *event;
    struct host_ddb_info *info;
    size_t size;

    /* Initialize BLE stack */
    struct ble_host_param param;
    if (ble_static_addr) {
        param.own_addr_type = 1;
        memcpy(&param.own_addr.addr[0], ble_static_addr, 6);
        vPortFree(ble_static_addr);
    }
    else {
        param.own_addr_type = 0;
    }
    ble_host_init(&param);
    printf("HCI is ready.\r\n");
    /* HCI is ready */
    __UART_AUTO_FLOW_CONTROL_ENABLE(HCI_handle.UARTx);
    __UART_RTS_ACTIVE(HCI_handle.UARTx);
    while(ble_host_ready() == false) {
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        btdm_host_schedule_ble();
    }

    host_ready_cb();

    while(1) {
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        btdm_host_schedule_ble();
    }
}

/************************************************************************************
 * @fn      host_btdm_start
 *
 * @brief   Initializes bt dual mode host, host task will be created in this function.
 *          host_ble_start and host_btdm_start should not be called together.
 *
 * @param   baudrate: uart baudrate of HCI
 *          stack_size: stack size of host task
 *          priority: priority of host task
 *          ble_static_addr: If public address will be used as identity address, this field
 *                          should be set NULL. Otherwise random static address stored in 
 *                          ble_static_addr will be used as identity address.
 */
void host_btdm_start(uint32_t baudrate, uint32_t stack_size, uint8_t priority, const uint8_t *ble_static_addr)
{
    uint8_t *static_addr;
    
    if (ble_static_addr) {
        static_addr = pvPortMalloc(6);
        memcpy(static_addr, ble_static_addr, 6);
    }
    else {
        static_addr = NULL;
    }

    /* initialize the seed of a pseudorandom number with TRNG */
    __SYSTEM_TRNG_CLK_ENABLE();
    uint32_t seed;
    trng_init();
    trng_read_rand_num((void *)&seed, sizeof(uint32_t));
    srand(seed);
    __SYSTEM_TRNG_CLK_DISABLE();

    host_hw_init(baudrate);
    xTaskCreate(host_btdm_task, "host", stack_size, (void *)static_addr, priority, &btdm_host_handle);
}

/************************************************************************************
 * @fn      host_ble_start
 *
 * @brief   Initializes ble host, host task will be created in this function.
 *          host_ble_start and host_btdm_start should not be called together.
 *
 * @param   baudrate: uart baudrate of HCI
 *          stack_size: stack size of host task
 *          priority: priority of host task
 *          ble_static_addr: If public address will be used as identity address, this field
 *                          should be set NULL. Otherwise random static address stored in 
 *                          ble_static_addr will be used as identity address.
 */
void host_ble_start(uint32_t baudrate, uint32_t stack_size, uint8_t priority, const uint8_t *ble_static_addr)
{
    uint8_t *static_addr;
    
    if (ble_static_addr) {
        static_addr = pvPortMalloc(6);
        memcpy(static_addr, ble_static_addr, 6);
    }
    else {
        static_addr = NULL;
    }

    /* initialize the seed of a pseudorandom number with TRNG */
    __SYSTEM_TRNG_CLK_ENABLE();
    uint32_t seed;
    trng_init();
    trng_read_rand_num((void *)&seed, sizeof(uint32_t));
    srand(seed);
    __SYSTEM_TRNG_CLK_DISABLE();

    host_hw_init(baudrate);
    xTaskCreate(host_ble_task, "host", stack_size, (void *)static_addr, priority, &btdm_host_handle);
}

//void host_stop(void)
//{
//    uint32_t *dst, *src, *end;

//    if(btdm_host_timer_inited) {
//        xTimerDelete(btdm_host_timer, portMAX_DELAY);
//        btdm_host_timer = NULL;
//        btdm_host_timer_inited = false;
//    }
//    vTaskDelete(btdm_host_handle);
//    
//    btdm_mem_uninit();

//    write_buffer = 0;
//    write_length = 0;
//    write_callback = 0;

//    read_buffer = 0;
//    read_length = 0;
//    read_callback = 0;
//    read_dummy = 0;
//    
//    dst = (uint32_t *)&Image$$HOST_DATA$$RW$$Base;
//    src=(uint32_t *)&Load$$HOST_DATA$$RW$$Base;
//    end = (uint32_t *)&Load$$HOST_DATA$$RW$$Limit;
//    for(; (uint32_t)src<(uint32_t)end;)
//    {
//        *dst++ = *src++;
//    }

//    dst = (uint32_t *)&Image$$HOST_DATA$$ZI$$Base;
//    end = (uint32_t *)&Image$$HOST_DATA$$ZI$$Limit;
//    for(; dst < end;)
//    {
//        *dst++ = 0;
//    }
//}

BtStatus DDB_AddRecord(const BtDeviceRecord* record)
{
    fdb_err_t err;
    struct host_ddb_info *info;
    size_t size;
    
    info = pvPortMalloc(sizeof(struct host_ddb_info));
    if (info) {
        uint32_t index = 0;
        size = flashdb_get(FDB_KEY_BT_LINKKEY, (void *)info, sizeof(struct host_ddb_info));
        if (size == 0) {
            info->version = HOST_DDB_INFOR_VERSION;
            info->used_count = 0;
        }
        else {
            if ((size != sizeof(struct host_ddb_info))
                    || (info->version != HOST_DDB_INFOR_VERSION)
                    || (info->used_count > HOST_DDB_RECORD_COUNT)) {
                if (flashdb_del(FDB_KEY_BT_LINKKEY) != FDB_NO_ERR) {
                    vPortFree(info);
                    return BT_STATUS_FAILED;
                }
                else {
                    info->version = HOST_DDB_INFOR_VERSION;
                    info->used_count = 0;
                }
            }
        }
        
        /* search for duplicated record according to bluetooth device address */
        for (index = 0; index < info->used_count; index++) {
            if (memcmp((void *)&info->ddb[index].bdAddr.A, (void *)&record->bdAddr, sizeof(BD_ADDR)) == 0) {
                break;
            }
        }
        if (index < info->used_count) {
            /* duplicated device is found, delete the found device and move forward the information of subsequence devices */
            memcpy((void *)&info->ddb[index], (void *)&info->ddb[index+1], sizeof(BtDeviceRecord) * (info->used_count-1-index));
            index = info->used_count - 1;
        }
        else {
            if (info->used_count == HOST_DDB_RECORD_COUNT) {
                /* the table is full, remove the oldest device information */
                memcpy((void *)&info->ddb[0], (void *)&info->ddb[1], sizeof(BtDeviceRecord) * (HOST_DDB_RECORD_COUNT-1));
                index = HOST_DDB_RECORD_COUNT-1;
            }
            else {
                index = info->used_count;
                info->used_count++;
            }
        }
        memcpy((void *)&info->ddb[index], (void *)record, sizeof(BtDeviceRecord));
        
        printf("linkey: ");
        for(uint8_t i=0; i<16; i++){
            printf("%02x ",record->linkKey[i]);
        }
        printf("\r\n");
        
        err = flashdb_set(FDB_KEY_BT_LINKKEY, (void *)info, sizeof(struct host_ddb_info));
        vPortFree(info);
        if (err != FDB_NO_ERR) {
            return BT_STATUS_FAILED;
        }
        else {
            return BT_STATUS_SUCCESS;
        }
    }
    else {
        return BT_STATUS_FAILED;
    }
}

BtStatus DDB_FindRecord(const BD_ADDR *bdAddr, BtDeviceRecord* record)
{
    fdb_err_t err;
    struct host_ddb_info *info;
    size_t size;
    BtStatus status;
    
    info = pvPortMalloc(sizeof(struct host_ddb_info));
    if (info) {
        uint32_t index = 0;
        size = flashdb_get(FDB_KEY_BT_LINKKEY, (void *)info, sizeof(struct host_ddb_info));
        if (size == 0) {
            vPortFree(info);
            return BT_STATUS_FAILED;
        }
        else {
            if ((size != sizeof(struct host_ddb_info))
                    || (info->version != HOST_DDB_INFOR_VERSION)
                    || (info->used_count > HOST_DDB_RECORD_COUNT)) {
                flashdb_del(FDB_KEY_BT_LINKKEY);
                vPortFree(info);
                return BT_STATUS_FAILED;
            }
        }
        
        /* search for duplicated record according to bluetooth device address */
        for (index = 0; index < info->used_count; index++) {
            if (memcmp((void *)&info->ddb[index].bdAddr.A, (void *)bdAddr, sizeof(BD_ADDR)) == 0) {
                break;
            }
        }
        if (index < info->used_count) {
            /* information is found */
            memcpy((void *)record, (void *)&info->ddb[index], sizeof(BtDeviceRecord));
            status = BT_STATUS_SUCCESS;
            if ((index+1) != info->used_count) {
                memcpy((void *)&info->ddb[index], (void *)&info->ddb[index+1], sizeof(BtDeviceRecord) * (info->used_count-1-index));
                memcpy((void *)&info->ddb[info->used_count - 1], (void *)record, sizeof(BtDeviceRecord));
                
                err = flashdb_set(FDB_KEY_BT_LINKKEY, (void *)info, sizeof(struct host_ddb_info));
                if (err != FDB_NO_ERR) {
                    status = BT_STATUS_FAILED;
                }
            }
        }
        else {
            status = BT_STATUS_FAILED;
        }
        
        vPortFree(info);
        return status;
    }
    else {
        return BT_STATUS_FAILED;
    }
}

BtStatus DDB_EnumRecord(void)
{
    fdb_err_t err;
    struct host_ddb_info *info;
    size_t size;
    BtStatus status = BT_STATUS_SUCCESS;
    
    info = pvPortMalloc(sizeof(struct host_ddb_info));
    if (info) {
        uint32_t index = 0;
        size = flashdb_get(FDB_KEY_BT_LINKKEY, (void *)info, sizeof(struct host_ddb_info));
        if (size == 0) {
            vPortFree(info);
            return BT_STATUS_FAILED;
        }
        else {
            if ((size != sizeof(struct host_ddb_info))
                    || (info->version != HOST_DDB_INFOR_VERSION)
                    || (info->used_count > HOST_DDB_RECORD_COUNT)) {
                flashdb_del(FDB_KEY_BT_LINKKEY);
                vPortFree(info);
                return BT_STATUS_FAILED;
            }
        }
        printf("DDB record, total count = %d \r\n",info->used_count);
        BD_ADDR *addr;
        BtDeviceRecord* record;
        /* search for duplicated record according to bluetooth device address */
        for (index = 0; index < info->used_count; index++) {
            addr = &info->ddb[index].bdAddr;
            record = &info->ddb[index];
            printf("record index %d, trusted %d:\r\n",index,info->ddb[index].trusted);
            printf("bd addr: 0x%02x%02x%02x%02x%02x%02x\r\n",addr->A[0],addr->A[1],addr->A[2],addr->A[3],addr->A[4],addr->A[5]);
            printf("linkey: ");
            for(uint8_t i=0; i<16; i++){
                printf("%02x ",record->linkKey[i]);
            }
            printf("\r\n");
            printf("-------------------------------------------\r\n");
        }
        vPortFree(info);
        return status;
    }
    else {
        return BT_STATUS_FAILED;
    }
}

BtStatus DDB_DeleteRecord(const BD_ADDR *bdAddr)
{
    fdb_err_t err;
    struct host_ddb_info *info;
    size_t size;
    BtStatus status;
    
    info = pvPortMalloc(sizeof(struct host_ddb_info));
    if (info) {
        uint32_t index = 0;
        size = flashdb_get(FDB_KEY_BT_LINKKEY, (void *)info, sizeof(struct host_ddb_info));
        if (size == 0) {
            vPortFree(info);
            return BT_STATUS_FAILED;
        }
        else {
            if ((size != sizeof(struct host_ddb_info))
                    || (info->version != HOST_DDB_INFOR_VERSION)
                    || (info->used_count > HOST_DDB_RECORD_COUNT)) {
                flashdb_del(FDB_KEY_BT_LINKKEY);
                vPortFree(info);
                return BT_STATUS_FAILED;
            }
        }
        
        /* search for duplicated record according to bluetooth device address */
        for (index = 0; index < info->used_count; index++) {
            if (memcmp((void *)&info->ddb[index].bdAddr.A, (void *)bdAddr, sizeof(BD_ADDR)) == 0) {
                break;
            }
        }
        printf("ddb deleting index: %d,total %d ...\r\n",index,info->used_count);
        if (index < info->used_count) {
            /* information is found */
            status = BT_STATUS_SUCCESS;
            if((index+1) < info->used_count){
                memcpy((void *)&info->ddb[index], (void *)&info->ddb[index+1], sizeof(BtDeviceRecord) * (info->used_count-1-index));
                index++;
            }
            info->used_count --;
            err = flashdb_set(FDB_KEY_BT_LINKKEY, (void *)info, sizeof(struct host_ddb_info));
            if (err != FDB_NO_ERR) {
                status = BT_STATUS_FAILED;
            }
        }
        else {
            status = BT_STATUS_FAILED;
        }
        printf("delete ok\r\n");
        vPortFree(info);
        return status;
    }
    else {
        return BT_STATUS_FAILED;
    }
}

enum btdm_nvds_status btdm_nvds_put(uint8_t tag, uint16_t length, uint8_t *data)
{
    enum btdm_nvds_status status = BTDM_NVDS_STATUS_FAILED;
    uint32_t fdb_key;
    
    if (tag == BTDM_NVDS_TAG_CONTROLLER_INFO) {
        fdb_key = FDB_KEY_CONTROLLER_INFO;
    }
    else {
        fdb_key = FDB_KEY_BTDM_LIB_BASE | tag;
    }
    if (flashdb_set(fdb_key, data, length) == FDB_NO_ERR) {
        status = BTDM_NVDS_STATUS_OK;
    }
    else {
        status = BTDM_NVDS_STATUS_FAILED;
    }
    
    return status;
}

enum btdm_nvds_status btdm_nvds_get(uint8_t tag, uint16_t *length, uint8_t *buffer)
{
    *length = flashdb_get(FDB_KEY_BTDM_LIB_BASE | tag, buffer, *length);

    if (*length == 0) {
        return BTDM_NVDS_STATUS_FAILED;
    }
    else {
        return BTDM_NVDS_STATUS_OK;
    }
}

enum btdm_nvds_status btdm_nvds_del(uint8_t tag)
{
    enum btdm_nvds_status status = BTDM_NVDS_STATUS_FAILED;

    if (flashdb_del(FDB_KEY_BTDM_LIB_BASE | tag) == FDB_NO_ERR) {
        status = BTDM_NVDS_STATUS_OK;
    }
    else {
        status = BTDM_NVDS_STATUS_FAILED;
    }
        
    return status;
}

void uart0_irq(void)
{
    uart_IRQHandler(&HCI_handle);
}

/************************************************************************************
 * @fn      host_before_sleep_check
 *
 * @brief   user should call this function to check whether HCI is in busy state before
 *          enter sleep mode. When HCI is in IDLE state, this function will return True and 
 *          RTS will be changed to GPIO mode and output high level to suspend HCI transfer.
 *
 * @return  True: HCI is in IDLE state, user is allowed to enter sleep mode.
 *          False: HCI is in busy state, user is not allowed to enter sleep mode.
 */
__RAM_CODE bool host_before_sleep_check(void)
{
    if (__UART_IS_TxFIFO_EMPTY(HCI_handle.UARTx) == 0) {
        return false;
    }
    /* set RTS to notice host device that UART is not available*/
    __SYSTEM_GPIOA_CLK_ENABLE();
    GPIOA->GPIO_OutputEN &= (~GPIO_PIN_3);
    GPIOA->GPIO_BIT_SET = GPIO_PIN_3;
    SYSTEM->PortA_L_FuncMux &= (~(0xf<<12));
    system_delay_us(100);
    if (__UART_IS_RxFIFO_EMPTY(HCI_handle.UARTx)) {
        /* keep RTS pin is controllered by GPIO */
        __Uart_backup(HCI_handle.UARTx, &uart_regs);
        return true;
    }
    else {
        /* recover RTS pin is controllered by UART */
        SYSTEM->PortA_L_FuncMux |= (GPIO_FUNCTION_1<<12);
        return false;
    }
}

/************************************************************************************
 * @fn      host_hci_reinit
 *
 * @brief   When system wake up from sleep mode, user should call this function to recover
 *          HCI.
 */
__RAM_CODE void host_hci_reinit(void)
{
    GPIO_InitTypeDef gpio_config;
    
    __SYSTEM_UART0_CLK_ENABLE();
    __Uart_restore(HCI_handle.UARTx, &uart_regs);
        
    /* configure PA0 and PA1 to UART0 function */
    gpio_config.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3;
    gpio_config.Mode = GPIO_MODE_AF_PP;
    gpio_config.Pull = GPIO_PULLUP;
    gpio_config.Alternate = GPIO_FUNCTION_1;
    gpio_init(GPIOA, &gpio_config);

    NVIC_SetPriority(HCI_UART_IRQn, 2);
    NVIC_EnableIRQ(HCI_UART_IRQn);
}

void btdm_host_lock(void)
{
//    volatile uint32_t address;
//    __asm("MOV %[result], LR":[result] "=r" (address));

    if(xPortIsInsideInterrupt() == 0){
        xSemaphoreTakeRecursive(host_Semaphore_handle,portMAX_DELAY);
    }
    else{
        while(1);
        //xSemaphoreTakeFromISR(host_Semaphore_handle,NULL);    
    }
}

void btdm_host_unlock(void)
{
    if(xPortIsInsideInterrupt() == 0){
        xSemaphoreGiveRecursive(host_Semaphore_handle);
    }
    else{
        while(1);
    }
}

bool host_get_bt_last_device(BD_ADDR *addr)
{
    struct host_ddb_info *info;
    size_t size;
    bool ret = false;
    info = pvPortMalloc(sizeof(struct host_ddb_info));
    size = flashdb_get(FDB_KEY_BT_LINKKEY, (void *)info, sizeof(struct host_ddb_info));
    printf("size = %d,version=%d,count=%d\r\n",size,info->version,info->used_count);
    if((size != 0) && (info->version == HOST_DDB_INFOR_VERSION) && (info->used_count > 0)){
        memcpy(addr,&info->ddb[info->used_count -1].bdAddr,sizeof(BD_ADDR));
        ret = true;
    }
    vPortFree(info);
    return ret;
}