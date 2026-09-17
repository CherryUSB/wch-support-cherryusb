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

#include "ch32h417.h"

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
work_mode_t work_mode = WORK_MODE_IDLE;

/* @function declaration */
static void _usart_config(void);
static uint32_t _systick_config(rt_uint32_t ticks);
static void _usbhs_rcc_config(bool enable);

void rt_hw_board_init()
{
    /* System Clock Configuration */
    SystemInit();

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

    RCC_HB2PeriphClockCmd(RCC_HB2Periph_AFIO | RCC_HB2Periph_USART1 | RCC_HB2Periph_GPIOA, ENABLE);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF7);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF7);

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
    NVIC_SetPriority(SysTick0_IRQn, 0xF0);
    NVIC_SetPriority(Software_IRQn, 0xF0);
    NVIC_EnableIRQ(SysTick0_IRQn);
    NVIC_EnableIRQ(Software_IRQn);
    SysTick0->CTLR = 0;
    SysTick0->ISR = 0;
    SysTick0->CNT = 0;
    SysTick0->CMP = ticks - 1;
    SysTick0->CTLR = 0x0F;
    return 0;
}

static void _usbhs_rcc_config(bool enable)
{
    if (enable)
    {
        /* Disable JTAG Port */
        RCC_HB2PeriphClockCmd(RCC_HB2Periph_AFIO, ENABLE);
        GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);

        if ((RCC->PLLCFGR & RCC_SYSPLL_SEL) != RCC_SYSPLL_USBHS)
        {
            /* Initialize USBHS 480M PLL */
            RCC_USBHS_PLLCmd(DISABLE);
            RCC_USBHSPLLCLKConfig(RCC_USBHSPLLSource_HSE);
            RCC_USBHSPLLReferConfig(RCC_USBHSPLLRefer_25M);
            RCC_USBHSPLLClockSourceDivConfig(RCC_USBHSPLL_IN_Div1);
            RCC_USBHS_PLLCmd(ENABLE);
            while (!(RCC->CTLR & RCC_USBHS_PLLRDY));
        }
        /* Enable UTMI Clock */
        RCC_UTMIcmd(ENABLE);

        /* Enable USBHS Clock */
        RCC_HBPeriphClockCmd(RCC_HBPeriph_USBHS, ENABLE);

        /* Enable USBHS interrupt */
        NVIC_EnableIRQ(USBHS_IRQn);
    }
    else
    {
        NVIC_DisableIRQ(USBHS_IRQn);
        RCC_HBPeriphClockCmd(RCC_HBPeriph_USBHS, DISABLE);
        RCC_UTMIcmd(DISABLE);
        if ((RCC->PLLCFGR & RCC_SYSPLL_SEL) != RCC_SYSPLL_USBHS)
        {
            RCC_USBHS_PLLCmd(DISABLE);
        }
    }
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
        _usbhs_rcc_config(true);
        work_mode = WORK_MODE_USBD;
    }
}

void usb_dc_low_level_deinit(uint8_t busid)
{
    if (busid == 0)
    {
        _usbhs_rcc_config(false);
        work_mode = WORK_MODE_IDLE;
    }
}

void usb_hc_low_level_init(struct usbh_bus *bus)
{
    if (bus->busid == 0)
    {
        _usbhs_rcc_config(true);
        work_mode = WORK_MODE_USBH;
    }
}

void usb_hc_low_level_deinit(struct usbh_bus *bus)
{
    if (bus->busid == 0)
    {
        _usbhs_rcc_config(false);
        work_mode = WORK_MODE_IDLE;
    }
}

__attribute__((interrupt())) void SysTick0_Handler(void)
{
    GET_INT_SP();

    /* enter interrupt */
    rt_interrupt_enter();

    SysTick0->ISR &= ~(1 << 0);
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

    if (work_mode == WORK_MODE_USBD)
    {
        void USBD_IRQHandler(uint8_t busid);
        USBD_IRQHandler(0);
    }
    else if (work_mode == WORK_MODE_USBH)
    {
        void USBH_IRQHandler(uint8_t busid);
        USBH_IRQHandler(0);
    }

    /* leave interrupt */
    rt_interrupt_leave();

    FREE_INT_SP();
}
