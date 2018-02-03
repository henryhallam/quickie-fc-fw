#include "can.h"
#include "clock.h"
#include "font.h"
#include "lcd.h"
#include "mc_telem.h"
#include "usb.h"
#include <libopencm3/stm32/can.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <string.h>

volatile can_stats_t can_stats;

int can_rx_max_interval = 0;

#define N_RX_SW_MAILBOX 10

#define CAN_ID_SIC_L 0x12
#define CAN_ID_SIC_R 0x11

#define ID_EMERG_L (0x80 | CAN_ID_SIC_L)
#define ID_EMERG_R (0x80 | CAN_ID_SIC_R)
// PDO addresses taken from Scott's SiCtroller code - though they seem off-by-one compared to what I read on the web
#define ID_PDO1_L_RX ((5<<7)  | CAN_ID_SIC_L)
#define ID_PDO2_L_RX ((7<<7)  | CAN_ID_SIC_L)
#define ID_PDO3_L_RX ((9<<7)  | CAN_ID_SIC_L)
#define ID_PDO4_L_RX ((11<<7) | CAN_ID_SIC_L)

#define ID_PDO1_R_RX ((5<<7)  | CAN_ID_SIC_R)
#define ID_PDO2_R_RX ((7<<7)  | CAN_ID_SIC_R)
#define ID_PDO3_R_RX ((9<<7)  | CAN_ID_SIC_R)
#define ID_PDO4_R_RX ((11<<7) | CAN_ID_SIC_R)

enum {
  SW_MBX_EMERG_L,
  SW_MBX_EMERG_R,
  SW_MBX_PDO1_L,
  SW_MBX_PDO2_L,
  SW_MBX_PDO1_R,
  SW_MBX_PDO2_R,
  SW_MBX_PDO3_L,
  SW_MBX_PDO4_L,
  SW_MBX_PDO3_R,
  SW_MBX_PDO4_R,
  N_RX_SW_MBX};

static can_sw_mbx_t can_sw_mbx[N_RX_SW_MAILBOX];

int can_setup(void) {
  memset(can_sw_mbx, 0, sizeof(can_sw_mbx));
  rcc_periph_clock_enable(RCC_CAN1);
  // Pins and alt functions are set up in board.c:init_pins()
  nvic_enable_irq(NVIC_CAN1_RX0_IRQ);
  nvic_enable_irq(NVIC_CAN1_RX1_IRQ);
  // TODO: Ensure RX0 IRQ has priority (emergency msgs)
  can_reset(CAN1);
  /* CAN cell init.
   * Setting the bitrate to 1 MBit. APB1 = 42 MHz, 
   * prescaler = 3 -> 14 MHz time quanta frequency.
   * 1tq sync + 8tq bit segment1 (TS1) + 5tq bit segment2 (TS2) = 
   * 14 time quanto per bit period, therefor 14 MHz / 14 = 1 MHz
   */
  if (can_init(CAN1,
               false,           /* TTCM: Time triggered comm mode? */
               true,            /* ABOM: Automatic bus-off management? */
               false,           /* AWUM: Automatic wakeup mode? */
               false,           /* NART: No automatic retransmission? */
               false,           /* RFLM: Receive FIFO locked mode? */
               false,           /* TXFP: Transmit FIFO priority? */
               CAN_BTR_SJW_3TQ,
               CAN_BTR_TS1_8TQ,
               CAN_BTR_TS2_5TQ,
               3,             /* BRP+1: Baud rate prescaler */
               false,
               false))
    return -1;
  /* CAN filter 0 init. */
  /* can_filter_id_mask_32bit_init(CAN1, */
  /*                               0,     /\* Filter ID *\/ */
  /*                               ID_EMERG_L,     /\* CAN ID *\/ */
  /*                               0xFFF,     /\* CAN ID mask *\/ */
  /*                               0,     /\* FIFO assignment (here: FIFO0) *\/ */
  /*                               true); /\* Enable the filter. *\/ */
  /* can_filter_id_mask_32bit_init(CAN1, 1, ID_EMERG_R, 0xFFF, 0, true); */

  can_filter_id_list_32bit_init(CAN1, 0, ID_EMERG_L << (5 + 16), ID_EMERG_R << (5 + 16), 0, true);
  can_filter_id_list_16bit_init(CAN1, 1,
                                ID_PDO2_L_RX << 5, ID_PDO1_L_RX << 5,
                                ID_PDO2_R_RX << 5, ID_PDO1_R_RX << 5,
                                1, true);
  
  can_filter_id_list_16bit_init(CAN1, 2,
                                ID_PDO4_L_RX << 5, ID_PDO3_L_RX << 5,
                                ID_PDO4_R_RX << 5, ID_PDO3_R_RX << 5,
                                1, true);

  can_enable_irq(CAN1, CAN_IER_FMPIE0);
  can_enable_irq(CAN1, CAN_IER_FMPIE1);
  return 0;
}


void can1_rx0_isr(void) {
  // Emergency!
  can_stats.emerg_count++;
  
  can_sw_mbx_t rx;
  rx.rx_time = mtime();
  // TODO: re-implement can_receive to avoid double copy
  can_receive(CAN1, 1, false, &rx.id, &rx.ext, &rx.rtr, &rx.fmi, &rx.length, rx.data);
  
  uint8_t sw_mbx_id = rx.fmi; // The filter match index is a little tricky
  if (sw_mbx_id <= SW_MBX_EMERG_R && !can_sw_mbx[sw_mbx_id].full) {
    rx.full = 1;
    can_sw_mbx[sw_mbx_id] = rx;
  }
  // TODO: Dispatch time-critical emergency response
  can_fifo_release(CAN1, 0);
}

void can1_rx1_isr(void) {
  can_sw_mbx_t rx;
  rx.rx_time = mtime();
  // TODO: re-implement can_receive to avoid double copy
  can_receive(CAN1, 1, false, &rx.id, &rx.ext, &rx.rtr, &rx.fmi, &rx.length, rx.data);

  uint8_t sw_mbx_id = SW_MBX_PDO1_L + rx.fmi; // The filter match index is a little tricky
  if (sw_mbx_id < N_RX_SW_MAILBOX && !can_sw_mbx[sw_mbx_id].full) {
    rx.full = 1;
    can_sw_mbx[sw_mbx_id] = rx;
  }
  can_stats.rx_count++;
  can_fifo_release(CAN1, 1);
}

void can_show_debug(void) {
  lcd_textbox_prep(0, 0, 480, 2*18, LCD_BLACK);
  lcd_printf(LCD_GREEN, &FreeMono9pt7b, "rx_count = %d  emerg_count = %d\n"
             "MBX ID  ms_ago  Data", can_stats.rx_count, can_stats.emerg_count);
  lcd_textbox_show();

  for (int i = 0; i < N_RX_SW_MAILBOX; i++) {
    lcd_textbox_prep(0, 18 * (2 + i), 480, 18, LCD_BLACK);
    lcd_printf(LCD_GREEN, &FreeMono9pt7b,
               "(%d) %03X %6d",
               i, (unsigned int)can_sw_mbx[i].id, (int)(mtime() - can_sw_mbx[i].rx_time));
    for (int j = 0; j < can_sw_mbx[i].length; j++)
      lcd_printf(LCD_GREEN, &FreeMono9pt7b, " %02X", can_sw_mbx[i].data[j]);
    can_sw_mbx[i].full = 0;
    lcd_textbox_show();
  }
}


static void crmi(void) {
    static int lastrx = 0;
    if (lastrx == 0) {
        lastrx = mtime();
    }

    int delta = mtime() - lastrx;
    lastrx = mtime();
    if(delta > can_rx_max_interval) {
        can_rx_max_interval = delta;
    }
}



void can_process_rx(void) {

  // Called from the main loop to dispatch the various handlers
  for (int i = 0; i < N_RX_SW_MAILBOX; i++) {
    if (can_sw_mbx[i].full) {
        if (usb_on == 1)
            usb_printf("CAN: %02x %02x %02x %02x %02x %02x %02x %02x\n",
                    can_sw_mbx[i].data[0],
                    can_sw_mbx[i].data[1],
                    can_sw_mbx[i].data[2],
                    can_sw_mbx[i].data[3],
                    can_sw_mbx[i].data[4],
                    can_sw_mbx[i].data[5],
                    can_sw_mbx[i].data[6],
                    can_sw_mbx[i].data[7]);
      switch(i) {
      case SW_MBX_PDO1_L:
          crmi();
        mc_telem_parse_pdo1(can_sw_mbx[i].data, MC_LEFT);
        break;
      case SW_MBX_PDO2_L:
          crmi();
        mc_telem_parse_pdo2(can_sw_mbx[i].data, MC_LEFT);
        break;
      case SW_MBX_PDO1_R:
          crmi();
        mc_telem_parse_pdo1(can_sw_mbx[i].data, MC_RIGHT);
        break;
      case SW_MBX_PDO2_R:
          crmi();
        mc_telem_parse_pdo2(can_sw_mbx[i].data, MC_RIGHT);
        break;
      }
      can_sw_mbx[i].full = 0;
    }
  }
}
