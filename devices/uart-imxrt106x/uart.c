/*
 * Phoenix-RTOS
 *
 * plo - operating system loader
 *
 * i.MXRT1064 Serial driver
 *
 * Copyright 2020 Phoenix Systems
 * Author: Hubert Buczyński
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#include "../../errors.h"
#include "../../timer.h"
#include "../../hal.h"

#include "peripherals.h"
#include "imxrt.h"
#include "uart.h"

#define CONCATENATE(x, y) x##y
#define PIN2MUX(x) CONCATENATE(pctl_mux_gpio_, x)
#define PIN2PAD(x) CONCATENATE(pctl_pad_gpio_, x)

#define UART_MAX_CNT 7

#define UART1_POS 0
#define UART2_POS (UART1_POS + UART1)
#define UART3_POS (UART2_POS + UART2)
#define UART4_POS (UART3_POS + UART3)
#define UART5_POS (UART4_POS + UART4)
#define UART6_POS (UART5_POS + UART5)
#define UART7_POS (UART6_POS + UART6)
#define UART8_POS (UART7_POS + UART7)

#define UART_CNT (UART1 + UART2 + UART3 + UART4 + UART5 + UART6 + UART7 + UART8)


#define BUFFER_SIZE 0x200


typedef struct {
	volatile u32 *base;
	u16 irq;

	u16 rxFifoSz;
	u16 txFifoSz;

	u8 rxBuff[BUFFER_SIZE];
	u16 rxHead;
	u16 rxTail;

	u8 txBuff[BUFFER_SIZE];
	u16 txHead;
	u16 txTail;
	u8 tFull;
} uart_t;


struct {
	uart_t uarts[UART_CNT];
} uart_common;


const int uartConfig[] = { UART1, UART2, UART3, UART4, UART5, UART6, UART7, UART8 };


const int uartPos[] = { UART1_POS, UART2_POS, UART3_POS, UART4_POS, UART5_POS, UART6_POS,
	UART7_POS, UART8_POS };


enum { veridr = 0, paramr, globalr, pincfgr, baudr, statr, ctrlr, datar, matchr, modirr, fifor, waterr };


static inline int uart_getRXcount(uart_t *uart)
{
	return (*(uart->base + waterr) >> 24) & 0xff;
}


static inline int uart_getTXcount(uart_t *uart)
{
	return (*(uart->base + waterr) >> 8) & 0xff;
}


int uart_rxEmpty(unsigned int pn)
{
	uart_t *uart;
	--pn;

	if (pn > UART_MAX_CNT || !uartConfig[pn])
		return ERR_ARG;

	uart = &uart_common.uarts[uartPos[pn]];

	return uart->rxHead == uart->rxTail;
}


int uart_handleIntr(u16 irq, void *buff)
{
	u32 flags;
	uart_t *uart = (uart_t *)buff;

	if (uart == NULL)
		return 0;

	/* Error flags: parity, framing, noise, overrun */
	flags = *(uart->base + statr) & (0xf << 16);

	/* RX overrun: invalidate fifo */
	if (flags & (1 << 19))
		*(uart->base + fifor) |= 1 << 14;

	*(uart->base + statr) |= flags;

	/* Receive */
	while (uart_getRXcount(uart)) {
		uart->rxBuff[uart->rxHead] = *(uart->base + datar);
		uart->rxHead = (uart->rxHead + 1) % BUFFER_SIZE;
		if (uart->rxHead == uart->rxTail) {
			uart->rxTail = (uart->rxTail + 1) % BUFFER_SIZE;
		}
	}

	/* Transmit */
	while (uart_getTXcount(uart) < uart->txFifoSz && !((*(uart->base + statr) >> 22) & 0x1))
	{
		uart->txHead = (uart->txHead + 1) % BUFFER_SIZE;
		if (uart->txHead != uart->txTail) {
			*(uart->base + datar) = uart->txBuff[uart->txHead];
			uart->tFull = 0;
		}
		else {
			*(uart->base + ctrlr) &= ~(1 << 23);
			break;
		}
	}

	return 0;
}


int uart_read(unsigned int pn, u8 *buff, u16 len, u16 timeout)
{
	uart_t *uart;
	u16 l, cnt;

	--pn;

	if (pn > UART_MAX_CNT || !uartConfig[pn])
		return ERR_ARG;

	uart = &uart_common.uarts[uartPos[pn]];

	if (!timer_wait(timeout, TIMER_VALCHG, &uart->rxHead, uart->rxTail))
		return ERR_UART_TIMEOUT;

	hal_cli();

	if (uart->rxHead > uart->rxTail)
		l = min(uart->rxHead - uart->rxTail, len);
	else
		l = min(BUFFER_SIZE - uart->rxTail, len);

	hal_memcpy(buff, &uart->rxBuff[uart->rxTail], l);
	cnt = l;
	if ((len > l) && (uart->rxHead < uart->rxTail)) {
		hal_memcpy(buff + l, &uart->rxBuff[0], min(len - l, uart->rxHead));
		cnt += min(len - l, uart->rxHead);
	}
	uart->rxTail = ((uart->rxTail + cnt) % BUFFER_SIZE);

	hal_sti();

	return cnt;
}


int uart_write(unsigned int pn, const u8 *buff, u16 len)
{
	uart_t *uart;
	u16 l, cnt = 0;

	--pn;

	if (pn > UART_MAX_CNT || !uartConfig[pn])
		return ERR_ARG;

	uart = &uart_common.uarts[uartPos[pn]];

	while (uart->txHead == uart->txTail && uart->tFull)
		;

	hal_cli();
	if (uart->txHead > uart->txTail)
		l = min(uart->txHead - uart->txTail, len);
	else
		l = min(BUFFER_SIZE - uart->txTail, len);

	hal_memcpy(&uart->txBuff[uart->txTail], buff, l);
	cnt = l;
	if ((len > l) && (uart->txTail >= uart->txHead)) {
		hal_memcpy(uart->txBuff, buff + l, min(len - l, uart->txHead));
		cnt += min(len - l, uart->txHead);
	}

	/* Initialize sending */
	if (uart->txTail == uart->txHead)
		*(uart->base + datar) = uart->txBuff[uart->txHead];

	uart->txTail = ((uart->txTail + cnt) % BUFFER_SIZE);

	if (uart->txTail == uart->txHead)
		uart->tFull = 1;

	*(uart->base + ctrlr) |= 1 << 23;

	hal_sti();

	return cnt;
}


int uart_safewrite(unsigned int pn, const u8 *buff, u16 len)
{
	int l;

	for (l = 0; len;) {
		if ((l = uart_write(pn, buff, len)) < 0)
			return ERR_MSG_IO;
		buff += l;
		len -= l;
	}
	return 0;
}



static int uart_muxVal(int mux)
{
	switch (mux) {
		case pctl_mux_gpio_b1_12:
		case pctl_mux_gpio_b1_13:
			return 1;

		case pctl_mux_gpio_b0_08:
		case pctl_mux_gpio_b0_09:
			return 3;

		case pctl_mux_gpio_sd_b1_00:
		case pctl_mux_gpio_sd_b1_01:
			return 4;

	}

	return 2;
}


static u32 calculate_baudrate(int baud)
{
	u32 osr, sbr, t, tDiff;
	u32 bestOsr = 0, bestSbr = 0, bestDiff = (u32)baud;

	if (baud <= 0) {
		return 0;
	}

	for (osr = 4; osr <= 32; ++osr) {
		/* find sbr value in range between 1 and 8191 */
		sbr = (UART_CLK / ((u32)baud * osr)) & 0x1fff;
		sbr = (sbr == 0) ? 1 : sbr;

		/* baud rate difference based on temporary osr and sbr */
		tDiff = UART_CLK / (osr * sbr) - (u32)baud;
		t = UART_CLK / (osr * (sbr + 1));

		/* select best values between sbr and sbr+1 */
		if (tDiff > (u32)baud - t) {
			tDiff = (u32)baud - t;
			sbr += (sbr < 0x1fff);
		}

		if (tDiff <= bestDiff) {
			bestDiff = tDiff;
			bestOsr = osr - 1;
			bestSbr = sbr;
		}
	}

	return (bestOsr << 24) | ((bestOsr <= 6) << 17) | (bestSbr & 0x1fff);
}


static int uart_getIsel(int mux, int *isel, int *val)
{
	switch (mux) {
		case pctl_mux_gpio_ad_b1_02: *isel = pctl_isel_lpuart2_tx; *val = 1; break;
		case pctl_mux_gpio_sd_b1_11: *isel = pctl_isel_lpuart2_tx; *val = 0; break;
		case pctl_mux_gpio_ad_b1_03: *isel = pctl_isel_lpuart2_rx; *val = 1; break;
		case pctl_mux_gpio_sd_b1_10: *isel = pctl_isel_lpuart2_rx; *val = 0; break;
		case pctl_mux_gpio_emc_13:   *isel = pctl_isel_lpuart3_tx; *val = 1; break;
		case pctl_mux_gpio_ad_b1_06: *isel = pctl_isel_lpuart3_tx; *val = 0; break;
		case pctl_mux_gpio_b0_08:    *isel = pctl_isel_lpuart3_tx; *val = 2; break;
		case pctl_mux_gpio_emc_14:   *isel = pctl_isel_lpuart3_rx; *val = 1; break;
		case pctl_mux_gpio_ad_b1_07: *isel = pctl_isel_lpuart3_rx; *val = 0; break;
		case pctl_mux_gpio_b0_09:    *isel = pctl_isel_lpuart3_rx; *val = 2; break;
		case pctl_mux_gpio_emc_15:   *isel = pctl_isel_lpuart3_cts_b; *val = 0; break;
		case pctl_mux_gpio_ad_b1_04: *isel = pctl_isel_lpuart3_cts_b; *val = 1; break;
		case pctl_mux_gpio_emc_19:   *isel = pctl_isel_lpuart4_tx; *val = 1; break;
		case pctl_mux_gpio_b1_00:    *isel = pctl_isel_lpuart4_tx; *val = 2; break;
		case pctl_mux_gpio_sd_b1_00: *isel = pctl_isel_lpuart4_tx; *val = 0; break;
		case pctl_mux_gpio_emc_20:   *isel = pctl_isel_lpuart4_rx; *val = 1; break;
		case pctl_mux_gpio_b1_01:    *isel = pctl_isel_lpuart4_rx; *val = 2; break;
		case pctl_mux_gpio_sd_b1_01: *isel = pctl_isel_lpuart4_rx; *val = 0; break;
		case pctl_mux_gpio_emc_23:   *isel = pctl_isel_lpuart5_tx; *val = 0; break;
		case pctl_mux_gpio_b1_12:    *isel = pctl_isel_lpuart5_tx; *val = 1; break;
		case pctl_mux_gpio_emc_24:   *isel = pctl_isel_lpuart5_rx; *val = 0; break;
		case pctl_mux_gpio_b1_13:    *isel = pctl_isel_lpuart5_rx; *val = 1; break;
		case pctl_mux_gpio_emc_25:   *isel = pctl_isel_lpuart6_tx; *val = 0; break;
		case pctl_mux_gpio_ad_b0_12: *isel = pctl_isel_lpuart6_tx; *val = 1; break;
		case pctl_mux_gpio_emc_26:   *isel = pctl_isel_lpuart6_rx; *val = 0; break;
		case pctl_mux_gpio_ad_b0_03: *isel = pctl_isel_lpuart6_rx; *val = 1; break;
		case pctl_mux_gpio_emc_31:   *isel = pctl_isel_lpuart7_tx; *val = 1; break;
		case pctl_mux_gpio_sd_b1_08: *isel = pctl_isel_lpuart7_tx; *val = 0; break;
		case pctl_mux_gpio_emc_32:   *isel = pctl_isel_lpuart7_rx; *val = 1; break;
		case pctl_mux_gpio_sd_b1_09: *isel = pctl_isel_lpuart7_rx; *val = 0; break;
		case pctl_mux_gpio_emc_38:   *isel = pctl_isel_lpuart8_tx; *val = 2; break;
		case pctl_mux_gpio_ad_b1_10: *isel = pctl_isel_lpuart8_tx; *val = 1; break;
		case pctl_mux_gpio_sd_b0_04: *isel = pctl_isel_lpuart8_tx; *val = 0; break;
		case pctl_mux_gpio_emc_39:   *isel = pctl_isel_lpuart8_rx; *val = 2; break;
		case pctl_mux_gpio_ad_b1_11: *isel = pctl_isel_lpuart8_rx; *val = 1; break;
		case pctl_mux_gpio_sd_b0_05: *isel = pctl_isel_lpuart8_rx; *val = 0; break;
		default: return -1;
	}

	return 0;
}


static void uart_initPins(void)
{
	int i, isel, val;
	static const int muxes[] = {
#if UART1
		PIN2MUX(UART1_TX_PIN), PIN2MUX(UART1_RX_PIN),
#endif
#if UART1_HW_FLOWCTRL
		PIN2MUX(UART1_RTS_PIN), PIN2MUX(UART1_CTS_PIN),
#endif
#if UART2
		PIN2MUX(UART2_TX_PIN), PIN2MUX(UART2_RX_PIN),
#endif
#if UART2_HW_FLOWCTRL
		PIN2MUX(UART2_RTS_PIN), PIN2MUX(UART2_CTS_PIN),
#endif
#if UART3
		PIN2MUX(UART3_TX_PIN), PIN2MUX(UART3_RX_PIN),
#endif
#if UART3_HW_FLOWCTRL
		PIN2MUX(UART3_RTS_PIN), PIN2MUX(UART3_CTS_PIN),
#endif
#if UART4
		PIN2MUX(UART4_TX_PIN), PIN2MUX(UART4_RX_PIN),
#endif
#if UART4_HW_FLOWCTRL
		PIN2MUX(UART4_RTS_PIN), PIN2MUX(UART4_CTS_PIN),
#endif
#if UART5
		PIN2MUX(UART5_TX_PIN), PIN2MUX(UART5_RX_PIN),
#endif
#if UART5_HW_FLOWCTRL
		PIN2MUX(UART5_RTS_PIN), PIN2MUX(UART5_CTS_PIN),
#endif
#if UART6
		PIN2MUX(UART6_TX_PIN), PIN2MUX(UART6_RX_PIN),
#endif
#if UART6_HW_FLOWCTRL
		PIN2MUX(UART6_RTS_PIN), PIN2MUX(UART6_CTS_PIN),
#endif
#if UART7
		PIN2MUX(UART7_TX_PIN), PIN2MUX(UART7_RX_PIN),
#endif
#if UART7_HW_FLOWCTRL
		PIN2MUX(UART7_RTS_PIN), PIN2MUX(UART7_CTS_PIN),
#endif
#if UART8
		PIN2MUX(UART8_TX_PIN), PIN2MUX(UART8_RX_PIN),
#endif
#if UART8_HW_FLOWCTRL
		PIN2MUX(UART8_RTS_PIN), PIN2MUX(UART8_CTS_PIN),
#endif
	};

	for (i = 0; i < sizeof(muxes) / sizeof(muxes[0]); ++i) {
		_imxrt_setIOmux(muxes[i], 0, uart_muxVal(muxes[i]));



		if (uart_getIsel(muxes[i], &isel, &val) < 0)
			continue;

		_imxrt_setIOisel(isel, val);
	}
}

u32 uart_getBaudrate(void)
{
	return UART_BAUDRATE;
}


void uart_init(void)
{
	u32 t;
	int i, dev;
	uart_t *uart;

	static const u32 fifoSzLut[] = { 1, 4, 8, 16, 32, 64, 128, 256 };
	static const struct {
		volatile u32 *base;
		int dev;
		unsigned irq;
	} info[] = {
		{ UART1_BASE, UART1_CLK, UART1_IRQ },
		{ UART2_BASE, UART2_CLK, UART2_IRQ },
		{ UART3_BASE, UART3_CLK, UART3_IRQ },
		{ UART4_BASE, UART4_CLK, UART4_IRQ },
		{ UART5_BASE, UART5_CLK, UART5_IRQ },
		{ UART6_BASE, UART6_CLK, UART6_IRQ },
		{ UART7_BASE, UART7_CLK, UART7_IRQ },
		{ UART8_BASE, UART8_CLK, UART8_IRQ }
	};

	uart_initPins();

	_imxrt_ccmSetMux(clk_mux_uart, 0);
	_imxrt_ccmSetDiv(clk_div_uart, 0);

	for (i = 0, dev = 0; dev < sizeof(uartConfig) / sizeof(uartConfig[0]); ++dev) {
		if (!uartConfig[dev])
			continue;

		uart = &uart_common.uarts[i++];
		uart->base = info[dev].base;
		uart->rxHead = 0;
		uart->txHead = 0;
		uart->rxTail = 0;

		uart->txTail = 0;
		uart->tFull = 0;

		_imxrt_ccmControlGate(info[dev].dev, clk_state_run_wait);
		/* Disable TX and RX */
		*(uart->base + ctrlr) &= ~((1 << 19) | (1 << 18));

		/* Reset all internal logic and registers, except the Global Register */
		*(uart->base + globalr) |= 1 << 1;
		imxrt_dataBarrier();
		*(uart->base + globalr) &= ~(1 << 1);
		imxrt_dataBarrier();

		/* Disable input trigger */
		*(uart->base + pincfgr) &= ~3;

		/* Set 115200 default baudrate */
		t = *(uart->base + baudr) & ~((0x1f << 24) | (1 << 17) | 0x1fff);
		*(uart->base + baudr) = t | calculate_baudrate(UART_BAUDRATE);

		/* Set 8 bit and no parity mode */
		*(uart->base + ctrlr) &= ~0x117;

		/* One stop bit */
		*(uart->base + baudr) &= ~(1 << 13);

		*(uart->base + waterr) = 0;

		/* Enable FIFO */
		*(uart->base + fifor) |= (1 << 7) | (1 << 3);
		*(uart->base + fifor) |= 0x3 << 14;

		/* Clear all status flags */
		*(uart->base + statr) |= 0xc01fc000;

		uart->rxFifoSz = fifoSzLut[*(uart->base + fifor) & 0x7];
		uart->txFifoSz = fifoSzLut[(*(uart->base + fifor) >> 4) & 0x7];

		/* Enable overrun, noise, framing error and receiver interrupts */
		*(uart->base + ctrlr) |= (1 << 27) | (1 << 26) | (1 << 25) | (1 << 21);

		/* Enable TX and RX */
		*(uart->base + ctrlr) |= (1 << 19) | (1 << 18);

		_imxrt_setDevClock(info[dev].dev, clk_state_run);

		hal_irqinst(info[dev].irq, uart_handleIntr, (void *)uart);
	}

	return;
}


void uart_done(void)
{
	int i, dev;
	uart_t *uart;

	for (i = 0, dev = 0; dev < sizeof(uartConfig) / sizeof(uartConfig[0]); ++dev) {
		if (!uartConfig[dev])
			continue;

		uart = &uart_common.uarts[i++];

		/* Disable TX and RX */
		*(uart->base + ctrlr) &= ~((1 << 19) | (1 << 18));
		imxrt_dataBarrier();

		/* Disable overrun, noise, framing error, TX and RX interrupts */
		*(uart->base + ctrlr) &= ~((1 << 27) | (1 << 26) | (1 << 25) | (1 << 23) | (1 << 21));

		/* Flush TX and RX fifo */
		*(uart->base + fifor) |= (1 << 15) | (1 << 14);

		/* Safely perform LPUART software reset procedure */
		*(uart->base + globalr) |= (1 << 1);
		imxrt_dataBarrier();
		*(uart->base + globalr) &= ~(1 << 1);
		imxrt_dataBarrier();

		hal_irquninst(uart->irq);
	}
}