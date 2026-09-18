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

#include "ch32v4x7.h"

/* @define */
#define GET_INT_SP()    asm("csrrw sp,mscratch,sp")
#define FREE_INT_SP()   asm("csrrw sp,mscratch,sp")

/* @enum */
typedef enum
{
    WORK_MODE_IDLE,
    WORK_MODE_USBH,
    WORK_MODE_USBD,
} work_mode_t;

/* @global */
static work_mode_t work_mode[CONFIG_USB_MAX_BUS];

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
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_High;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_High;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 921600;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

    USART_Init(USART1, &USART_InitStructure);
    USART_Cmd(USART1, ENABLE);
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
    return USART1->STATR & USART_FLAG_RXNE ? (USART1->DATAR & 0xFF) : -1;
}

static void usb_low_level_init(uint8_t busid, work_mode_t mode)
{
    switch (busid)
    {
    case 0:
        if ((RCC->CTLR & RCC_USBHSPLLRDY) == 0)
        {
            RCC_HBPeriphClockCmd(RCC_HBPeriph_USBHS1, DISABLE);
            RCC->CTLR &= ~RCC_USBHSPLLON;
            if (RCC->CTLR & RCC_HSEON)
            {
                RCC_USBHSPLLCLKConfig(RCC_USBHSPLLCLKSource_HSE);
                RCC_USBHSPLLReferConfig(RCC_USBHSPLLCKREFCLK_25M);
            }
            else
            {
                RCC_USBHSPLLCLKConfig(RCC_USBHSPLLCLKSource_HSI);
                RCC_USBHSPLLReferConfig(RCC_USBHSPLLCKREFCLK_20M);
            }
            RCC->CTLR |= RCC_USBHSPLLON;
            while (!(RCC->CTLR & RCC_USBHSPLLRDY));
        }

        RCC_UTMI1cmd(ENABLE);
        RCC_HBPeriphClockCmd(RCC_HBPeriph_USBHS1, ENABLE);

        NVIC_EnableIRQ(USBHS1_IRQn);
        work_mode[0] = mode;
        break;

    case 1:
        if ((RCC->CTLR & RCC_USBHSPLLRDY) == 0)
        {
            RCC_HBPeriphClockCmd(RCC_HBPeriph_USBHS2, DISABLE);
            RCC->CTLR &= ~RCC_USBHSPLLON;
            if (RCC->CTLR & RCC_HSEON)
            {
                RCC_USBHSPLLCLKConfig(RCC_USBHSPLLCLKSource_HSE);
                RCC_USBHSPLLReferConfig(RCC_USBHSPLLCKREFCLK_25M);
            }
            else
            {
                RCC_USBHSPLLCLKConfig(RCC_USBHSPLLCLKSource_HSI);
                RCC_USBHSPLLReferConfig(RCC_USBHSPLLCKREFCLK_20M);
            }
            RCC->CTLR |= RCC_USBHSPLLON;
            while (!(RCC->CTLR & RCC_USBHSPLLRDY));
        }

        RCC_UTMI2cmd(ENABLE);
        RCC_HBPeriphClockCmd(RCC_HBPeriph_USBHS2, ENABLE);

        NVIC_EnableIRQ(USBHS2_IRQn);
        work_mode[1] = mode;
        break;
    }
}

static void usb_low_level_deinit(uint8_t busid)
{
    switch (busid)
    {
    case 0:
        NVIC_DisableIRQ(USBHS1_IRQn);
        RCC_HBPeriphClockCmd(RCC_HBPeriph_USBHS1, DISABLE);
        RCC_UTMI1cmd(DISABLE);
        work_mode[0] = WORK_MODE_IDLE;
        break;

    case 1:
        NVIC_DisableIRQ(USBHS2_IRQn);
        RCC_HBPeriphClockCmd(RCC_HBPeriph_USBHS2, DISABLE);
        RCC_UTMI2cmd(DISABLE);
        work_mode[1] = WORK_MODE_IDLE;
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

__attribute__((interrupt())) void USBHS1_IRQHandler(void)
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

__attribute__((interrupt())) void USBHS2_IRQHandler(void)
{
    GET_INT_SP();

    rt_interrupt_enter();

    if (work_mode[1] == WORK_MODE_USBD)
    {
        USBD_IRQHandler(1);
    }
    else if (work_mode[1] == WORK_MODE_USBH)
    {
        USBH_IRQHandler(1);
    }

    rt_interrupt_leave();

    FREE_INT_SP();
}
