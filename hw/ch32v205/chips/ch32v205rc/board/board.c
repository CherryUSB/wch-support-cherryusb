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

#include "ch32v205.h"

/* @define */
#define GET_INT_SP()    asm("csrrw sp,mscratch,sp")
#define FREE_INT_SP()   asm("csrrw sp,mscratch,sp")

/* @function declaration */
static void _usart_config(void);
static uint32_t _systick_config(rt_uint32_t ticks);

void rt_hw_board_init()
{
    /* USART Configuration */
    _usart_config();

    /* System Tick Configuration */
    _systick_config(SystemCoreClock / RT_TICK_PER_SECOND);

    /* Call components board initial (use INIT_BOARD_EXPORT()) */
#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif

#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)
    extern char _end;
    extern char _heap_end;
    rt_system_heap_init(&_end, &_heap_end);
#endif
}

static void _usart_config(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure = {0};
    USART_InitTypeDef USART_InitStructure = {0};

    RCC_PB2PeriphClockCmd(RCC_PB2Periph_AFIO | RCC_PB2Periph_USART1 | RCC_PB2Periph_GPIOA, ENABLE);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF2);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF2);

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

static uint32_t _systick_config(rt_uint32_t ticks)
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

void usb_dc_low_level_init(uint8_t busid)
{
    if (busid == 0)
    {
        RCC_HBPeriphClockCmd(RCC_HBPeriph_USBHS, DISABLE);
        RCC_USBHS_PLLCmd(DISABLE);
        RCC_USBHSPLLCLKConfig(RCC_USBHSPLLSource_HSE);
        RCC_USBHSPLLReferConfig(RCC_USBHSPLLRefer_8M);
        RCC_USBHSPLLClockSourceDivConfig(RCC_USBHSPLL_IN_Div1);
        RCC_USBHS_PLLCmd(ENABLE);
        while (!(RCC->CTLR & RCC_USBHS_PLLRDY));

        /* Enable USBHS Clock */
        RCC_HBPeriphClockCmd(RCC_HBPeriph_USBHS, ENABLE);

        /* Enable USBHS interrupt */
        NVIC_EnableIRQ(USBHS_IRQn);
    }
}

void usb_dc_low_level_deinit(uint8_t busid)
{
    if (busid == 0)
    {
        NVIC_DisableIRQ(USBHS_IRQn);
        RCC_HBPeriphClockCmd(RCC_HBPeriph_USBHS, DISABLE);
        RCC_USBHS_PLLCmd(DISABLE);
    }
}

__attribute__((interrupt())) void SysTick_Handler(void)
{
    GET_INT_SP();

    /* enter interrupt */
    rt_interrupt_enter();

    SysTick->SR = 0;
    rt_tick_increase();

    /* leave interrupt */
    rt_interrupt_leave();

    FREE_INT_SP();
}

__attribute__((interrupt())) void USBHS_IRQHandler(void)
{
    GET_INT_SP();

    /* enter interrupt */
    rt_interrupt_enter();

    void USBD_IRQHandler(uint8_t busid);
    USBD_IRQHandler(0);

    /* leave interrupt */
    rt_interrupt_leave();

    FREE_INT_SP();
}
