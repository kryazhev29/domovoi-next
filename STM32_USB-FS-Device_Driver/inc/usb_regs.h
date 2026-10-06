// Доработка: Кряжев А., 2026
// Регистры USB, поля конечных точек и аппаратная таблица буферов.
// Дескрипторы и порядок событий сохраняют протокол CDC.
// Общие файлы также используются загрузчиком; память PMA требует особого доступа.

/**
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

#ifndef __USB_REGS_H
#define __USB_REGS_H

#define RegBase  (0x40005C00L)
#define PMAAddr  (0x40006000L)

#define CNTR    ((__IO unsigned *)(RegBase + 0x40))

#define ISTR    ((__IO unsigned *)(RegBase + 0x44))

#define FNR     ((__IO unsigned *)(RegBase + 0x48))

#define DADDR   ((__IO unsigned *)(RegBase + 0x4C))

#define BTABLE  ((__IO unsigned *)(RegBase + 0x50))

#define EP0REG  ((__IO unsigned *)(RegBase))

#define EP3_OUT     ((uint8_t)0x03)

#define ENDP0       ((uint8_t)0)
#define ENDP1       ((uint8_t)1)
#define ENDP2       ((uint8_t)2)
#define ENDP3       ((uint8_t)3)

#define ISTR_CTR    (0x8000)
#define ISTR_ERR    (0x2000)
#define ISTR_RESET  (0x0400)
#define ISTR_SOF    (0x0200)
#define ISTR_ESOF   (0x0100)

#define ISTR_DIR    (0x0010)
#define ISTR_EP_ID  (0x000F)

#define CLR_ERR    (~ISTR_ERR)
#define CLR_RESET  (~ISTR_RESET)
#define CLR_SOF    (~ISTR_SOF)
#define CLR_ESOF   (~ISTR_ESOF)

#define CNTR_CTRM   (0x8000)
#define CNTR_ERRM   (0x2000)
#define CNTR_WKUPM  (0x1000)
#define CNTR_SUSPM  (0x0800)
#define CNTR_RESETM (0x0400)
#define CNTR_SOFM   (0x0200)
#define CNTR_ESOFM  (0x0100)

#define CNTR_FSUSP  (0x0008)
#define CNTR_PDWN   (0x0002)
#define CNTR_FRES   (0x0001)

#define FNR_RXDP (0x8000)

#define DADDR_EF (0x80)

#define EP_CTR_RX      (0x8000)
#define EP_DTOG_RX     (0x4000)
#define EPRX_STAT      (0x3000)
#define EP_SETUP       (0x0800)
#define EP_T_FIELD     (0x0600)
#define EP_KIND        (0x0100)
#define EP_CTR_TX      (0x0080)
#define EP_DTOG_TX     (0x0040)
#define EPTX_STAT      (0x0030)
#define EPADDR_FIELD   (0x000F)

#define EPREG_MASK     (EP_CTR_RX|EP_SETUP|EP_T_FIELD|EP_KIND|EP_CTR_TX|EPADDR_FIELD)

#define EP_BULK        (0x0000)
#define EP_CONTROL     (0x0200)
#define EP_INTERRUPT   (0x0600)
#define EP_T_MASK      (~EP_T_FIELD & EPREG_MASK)

#define EPKIND_MASK    (~EP_KIND & EPREG_MASK)

#define EP_TX_DIS      (0x0000)
#define EP_TX_STALL    (0x0010)
#define EP_TX_NAK      (0x0020)
#define EP_TX_VALID    (0x0030)
#define EPTX_DTOGMASK  (EPTX_STAT|EPREG_MASK)

#define EP_RX_DIS      (0x0000)
#define EP_RX_STALL    (0x1000)
#define EP_RX_NAK      (0x2000)
#define EP_RX_VALID    (0x3000)
#define EPRX_DTOGMASK  (EPRX_STAT|EPREG_MASK)

// Задать управляющий регистр USB.
static inline void _SetCNTR(uint16_t value)
{
  *CNTR = value;
}

// Очистить события ISTR записью нулей в выбранные биты.
static inline void _SetISTR(uint16_t value)
{
  *ISTR = value;
}

// Задать адрес и разрешение устройства.
static inline void _SetDADDR(uint16_t value)
{
  *DADDR = value;
}

// Задать адрес таблицы с выравниванием на восемь байтов.
static inline void _SetBTABLE(uint16_t value)
{
  *BTABLE = value & 0xFFF8u;
}

// Прочитать управление USB.
static inline uint16_t _GetCNTR(void)
{
  return (uint16_t)*CNTR;
}

// Прочитать аппаратные события USB.
static inline uint16_t _GetISTR(void)
{
  return (uint16_t)*ISTR;
}

// Прочитать номер кадра и состояние линий USB.
static inline uint16_t _GetFNR(void)
{
  return (uint16_t)*FNR;
}

// Прочитать текущий адрес устройства.
static inline uint16_t _GetDADDR(void)
{
  return (uint16_t)*DADDR;
}

// Прочитать положение таблицы буферов.
static inline uint16_t _GetBTABLE(void)
{
  return (uint16_t)*BTABLE;
}

// Записать аппаратный регистр проверенной конечной точки.
static inline void _SetENDPOINT(uint8_t ep, uint16_t value)
{
  EP0REG[ep] = value;
}

// Прочитать аппаратный регистр проверенной конечной точки.
static inline uint16_t _GetENDPOINT(uint8_t ep)
{
  return (uint16_t)EP0REG[ep];
}

// Задать тип точки без переключения STAT/DTOG.
static inline void _SetEPType(uint8_t ep, uint16_t type)
{
  _SetENDPOINT(ep, (_GetENDPOINT(ep) & EP_T_MASK) | type);
}

// Переключить STAT_TX до нужного значения, сохраняя оба события CTR.
static inline void _SetEPTxStatus(uint8_t ep, uint16_t state)
{
  uint16_t value = (_GetENDPOINT(ep) & EPTX_DTOGMASK) ^ (state & EPTX_STAT);
  _SetENDPOINT(ep, value | EP_CTR_RX | EP_CTR_TX);
}

// Переключить STAT_RX до нужного значения, сохраняя оба события CTR.
static inline void _SetEPRxStatus(uint8_t ep, uint16_t state)
{
  uint16_t value = (_GetENDPOINT(ep) & EPRX_DTOGMASK) ^ (state & EPRX_STAT);
  _SetENDPOINT(ep, value | EP_CTR_RX | EP_CTR_TX);
}

// Атомарно задать оба статуса точки без потери события на другой стороне.
static inline void _SetEPRxTxStatus(uint8_t ep, uint16_t rx, uint16_t tx)
{
  uint16_t value = _GetENDPOINT(ep) & (EPRX_DTOGMASK | EPTX_STAT);
  value ^= (rx & EPRX_STAT) | (tx & EPTX_STAT);
  _SetENDPOINT(ep, value | EP_CTR_RX | EP_CTR_TX);
}

// Прочитать состояние передачи IN.
static inline uint16_t _GetEPTxStatus(uint8_t ep)
{
  return _GetENDPOINT(ep) & EPTX_STAT;
}

// Прочитать состояние приёма OUT.
static inline uint16_t _GetEPRxStatus(uint8_t ep)
{
  return _GetENDPOINT(ep) & EPRX_STAT;
}

// Проверить блокировку IN.
static inline uint16_t _GetTxStallStatus(uint8_t ep)
{
  return _GetEPTxStatus(ep) == EP_TX_STALL;
}

// Проверить блокировку OUT.
static inline uint16_t _GetRxStallStatus(uint8_t ep)
{
  return _GetEPRxStatus(ep) == EP_RX_STALL;
}

// Снять EP_KIND без изменения флагов передачи.
static inline void _ClearEP_KIND(uint8_t ep)
{
  _SetENDPOINT(ep, EP_CTR_RX | EP_CTR_TX | (_GetENDPOINT(ep) & EPKIND_MASK));
}

// Подтвердить обработанное событие приёма, сохранив событие передачи.
static inline void _ClearEP_CTR_RX(uint8_t ep)
{
  _SetENDPOINT(ep, _GetENDPOINT(ep) & 0x7FFFu & EPREG_MASK);
}

// Подтвердить обработанное событие передачи, сохранив событие приёма.
static inline void _ClearEP_CTR_TX(uint8_t ep)
{
  _SetENDPOINT(ep, _GetENDPOINT(ep) & 0xFF7Fu & EPREG_MASK);
}

// Переключить приём между DATA0 и DATA1.
static inline void _ToggleDTOG_RX(uint8_t ep)
{
  _SetENDPOINT(ep, EP_CTR_RX | EP_CTR_TX | EP_DTOG_RX | (_GetENDPOINT(ep) & EPREG_MASK));
}

// Переключить передачу между DATA0 и DATA1.
static inline void _ToggleDTOG_TX(uint8_t ep)
{
  _SetENDPOINT(ep, EP_CTR_RX | EP_CTR_TX | EP_DTOG_TX | (_GetENDPOINT(ep) & EPREG_MASK));
}

// Вернуть приём к DATA0, переключая бит только при необходимости.
static inline void _ClearDTOG_RX(uint8_t ep)
{
  if (_GetENDPOINT(ep) & EP_DTOG_RX) _ToggleDTOG_RX(ep);
}

// Вернуть передачу к DATA0, переключая бит только при необходимости.
static inline void _ClearDTOG_TX(uint8_t ep)
{
  if (_GetENDPOINT(ep) & EP_DTOG_TX) _ToggleDTOG_TX(ep);
}

// Задать адрес точки без переключения состояния передачи.
static inline void _SetEPAddress(uint8_t ep, uint8_t address)
{
  _SetENDPOINT(ep, EP_CTR_RX | EP_CTR_TX | (_GetENDPOINT(ep) & EPREG_MASK) | address);
}

// Найти поле таблицы: в CPU-памяти слова PMA разделены двумя байтами.
static inline __IO uint32_t * usb_pma_field(uint8_t ep, uint8_t offset)
{
  return (__IO uint32_t *)((_GetBTABLE() + ep * 8u + offset) * 2u + PMAAddr);
}

// Задать выровненный адрес буфера TX в PMA.
static inline void _SetEPTxAddr(uint8_t ep, uint16_t address)
{
  *usb_pma_field(ep, 0) = address & 0xFFFEu;
}

// Прочитать адрес буфера TX в PMA.
static inline uint16_t _GetEPTxAddr(uint8_t ep)
{
  return (uint16_t)*usb_pma_field(ep, 0);
}

// Задать выровненный адрес буфера RX в PMA.
static inline void _SetEPRxAddr(uint8_t ep, uint16_t address)
{
  *usb_pma_field(ep, 4) = address & 0xFFFEu;
}

// Прочитать адрес буфера RX в PMA.
static inline uint16_t _GetEPRxAddr(uint8_t ep)
{
  return (uint16_t)*usb_pma_field(ep, 4);
}

// Задать число байтов очередного пакета IN.
static inline void _SetEPTxCount(uint8_t ep, uint16_t count)
{
  *usb_pma_field(ep, 2) = count;
}

// Задать ёмкость OUT: до 62 байт блоками по два, дальше — по 32 байта.
static inline void _SetEPRxCount(uint8_t ep, uint16_t count)
{
  uint32_t blocks;
  if (count > 62u) {
    blocks = (count - 1u) / 32u;
    *usb_pma_field(ep, 6) = (blocks << 10) | 0x8000u;
  } else {
    blocks = (count + 1u) / 2u;
    *usb_pma_field(ep, 6) = blocks << 10;
  }
}

// Прочитать фактический размер пакета TX, отделив служебные биты.
static inline uint16_t _GetEPTxCount(uint8_t ep)
{
  return (uint16_t)*usb_pma_field(ep, 2) & 0x03FFu;
}

// Прочитать фактический размер пакета RX, отделив служебные биты.
static inline uint16_t _GetEPRxCount(uint8_t ep)
{
  return (uint16_t)*usb_pma_field(ep, 6) & 0x03FFu;
}

// Последнее чтение ISTR разделяют USB_Istr и обработчик завершённых передач.
extern __IO uint16_t wIstr;

// Задать выровненный адрес таблицы буферов в памяти USB.
void SetBTABLE(uint16_t );
// Последнее чтение ISTR разделяют USB_Istr и обработчик завершённых передач.
extern __IO uint16_t wIstr;

// Задать выровненный адрес таблицы буферов в памяти USB.
void SetBTABLE(uint16_t );
// Задать тип конечной точки, сохранив аппаратные флаги.
void SetEPType(uint8_t , uint16_t );
// Изменить статус IN через аппаратные переключающие биты.
void SetEPTxStatus(uint8_t , uint16_t );
// Изменить статус OUT через аппаратные переключающие биты.
void SetEPRxStatus(uint8_t , uint16_t );
// Разрешить передачу уже подготовленного пакета IN.
void SetEPTxValid(uint8_t );
// Разрешить приём следующего пакета OUT.
void SetEPRxValid(uint8_t );
// Снять признак ожидаемого статуса OUT управляющей точки.
void Clear_Status_Out(uint8_t );
// Вернуть приём конечной точки к последовательности DATA0.
void ClearDTOG_RX(uint8_t );
// Вернуть передачу конечной точки к последовательности DATA0.
void ClearDTOG_TX(uint8_t );
// Задать выровненное положение буфера IN в памяти USB.
void SetEPTxAddr(uint8_t , uint16_t );
// Задать выровненное положение буфера OUT в памяти USB.
void SetEPRxAddr(uint8_t , uint16_t );
// Прочитать положение буфера IN конечной точки.
uint16_t GetEPTxAddr(uint8_t );
// Прочитать положение буфера OUT конечной точки.
uint16_t GetEPRxAddr(uint8_t );
// Задать размер подготовленного пакета IN.
void SetEPTxCount(uint8_t , uint16_t );
// Закодировать ёмкость буфера OUT аппаратными блоками 2 или 32 байта.
void SetEPRxCount(uint8_t , uint16_t );
// Прочитать фактическую длину принятого пакета OUT.
uint16_t GetEPRxCount(uint8_t );
// Поменять местами байты 16-битного поля USB.
uint16_t ByteSwap(uint16_t);

#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
