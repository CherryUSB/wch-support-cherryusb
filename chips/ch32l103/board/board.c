/**
 * @file board.c
 * @author Links (lhd@wch.cn)
 * @brief Board functional configuration.
 * @version 0.1
 * @date 2026-09-05
 *
 * @copyright Copyright (c) 2026
 *
 */

/* @include */
#include <rtthread.h>

#include "usbh_core.h"

#include "ch32l103.h"

/* @define */
#define GET_INT_SP()    asm("csrrw sp,mscratch,sp")
#define FREE_INT_SP()   asm("csrrw sp,mscratch,sp")

#define CONSOLE_MB_SIZE 64

/* @enum */
typedef enum
{
    WORK_MODE_IDLE,
    WORK_MODE_USBH,
    WORK_MODE_USBD,
} work_mode_t;

/* @global */
static struct rt_mailbox console_mb;
static work_mode_t work_mode[CONFIG_USB_MAX_BUS];
static rt_ubase_t console_mb_pool[CONSOLE_MB_SIZE];

/* @function declaration */
static void usart_config(void);
static uint32_t systick_config(rt_uint32_t ticks);
static void usb_low_level_init(uint8_t busid, work_mode_t mode);
static void usb_low_level_deinit(uint8_t busid);
void USBD_IRQHandler(uint8_t busid);
void USBH_IRQHandler(uint8_t busid);

void rt_hw_board_init()
{
    /* USART Configuration */
    usart_config();

    /* System Tick Configuration */
    systick_config(SystemCoreClock / RT_TICK_PER_SECOND);

    /* Call components board initial (use INIT_BOARD_EXPORT()) */
#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif

#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)
    extern char _end;
    extern char _heap_end;
    rt_system_heap_init(&_end, &_heap_end);
#endif

    rt_mb_init(&console_mb, "console", console_mb_pool, CONSOLE_MB_SIZE, RT_IPC_FLAG_PRIO);

    for (int i = 0; i < sizeof(work_mode) / sizeof(work_mode[0]); i++)
    {
        work_mode[i] = WORK_MODE_IDLE;
    }
}

static void usart_config(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure = {0};
    USART_InitTypeDef USART_InitStructure = {0};

    RCC_PB2PeriphClockCmd(RCC_PB2Periph_USART1 | RCC_PB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 921600;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

    USART_Init(USART1, &USART_InitStructure);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART1, ENABLE);

    NVIC_SetPriority(USART1_IRQn, 0xF0);
    NVIC_EnableIRQ(USART1_IRQn);
}

static uint32_t systick_config(rt_uint32_t ticks)
{
    NVIC_SetPriority(SysTick_IRQn, 0xF0);
    NVIC_SetPriority(Software_IRQn, 0xF0);
    NVIC_EnableIRQ(SysTick_IRQn);
    NVIC_EnableIRQ(Software_IRQn);
    SysTick->CTLR = 0;
    SysTick->SR = 0;
    SysTick->CNT = 0;
    SysTick->CMP = ticks - 1;
    SysTick->CTLR = 0x0F;
    return 0;
}

void rt_hw_console_output(const char *str)
{
    rt_size_t size = rt_strlen(str);

    for (rt_size_t i = 0; i < size; i++)
    {
        if (str[i] == '\n')
        {
            while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
            USART_SendData(USART1, '\r');
        }

        while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
        USART_SendData(USART1, str[i]);
    }
}

char rt_hw_console_getchar(void)
{
    rt_ubase_t ch;

    if (rt_mb_recv(&console_mb, &ch, RT_WAITING_FOREVER) != RT_EOK)
    {
        return -1;
    }

    return (char)ch;
}

static void usb_low_level_init(uint8_t busid, work_mode_t mode)
{
    switch (busid)
    {
    case 0:
        if (SystemCoreClock == 96000000)
        {
            RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_Div2);
        }
        else if (SystemCoreClock == 72000000)
        {
            RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_Div1_5);
        }
        else if (SystemCoreClock == 48000000)
        {
            RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_Div1);
        }
        RCC_HBPeriphClockCmd(RCC_HBPeriph_USBFS, ENABLE);
        NVIC_EnableIRQ(USBFS_IRQn);
        work_mode[0] = mode;
        break;
    }
}

static void usb_low_level_deinit(uint8_t busid)
{
    switch (busid)
    {
    case 0:
        NVIC_DisableIRQ(USBFS_IRQn);
        RCC_HBPeriphClockCmd(RCC_HBPeriph_USBFS, DISABLE);
        work_mode[0] = WORK_MODE_IDLE;
        break;
    }
}

void usb_dc_low_level_init(uint8_t busid)
{
    usb_low_level_init(busid, WORK_MODE_USBD);
}

void usb_dc_low_level_deinit(uint8_t busid)
{
    usb_low_level_deinit(busid);
}

void usb_hc_low_level_init(struct usbh_bus *bus)
{
    usb_low_level_init(bus->busid, WORK_MODE_USBH);
}

void usb_hc_low_level_deinit(struct usbh_bus *bus)
{
    usb_low_level_deinit(bus->busid);
}

__attribute__((interrupt())) void SysTick_Handler(void)
{
    GET_INT_SP();

    rt_interrupt_enter();

    SysTick->SR = 0;
    rt_tick_increase();

    rt_interrupt_leave();

    FREE_INT_SP();
}

__attribute__((interrupt())) void USART1_IRQHandler(void)
{
    GET_INT_SP();

    rt_interrupt_enter();

    if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET)
    {
        rt_mb_send(&console_mb, (rt_ubase_t)(USART_ReceiveData(USART1) & 0xFF));
    }

    rt_interrupt_leave();

    FREE_INT_SP();
}

__attribute__((interrupt())) void USBFS_IRQHandler(void)
{
    GET_INT_SP();

    rt_interrupt_enter();

    if (work_mode[0] == WORK_MODE_USBD)
    {
        USBD_IRQHandler(0);
    }
    else if (work_mode[0] == WORK_MODE_USBH)
    {
        USBH_IRQHandler(0);
    }

    rt_interrupt_leave();

    FREE_INT_SP();
}
