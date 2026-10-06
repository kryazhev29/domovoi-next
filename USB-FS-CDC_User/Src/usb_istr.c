// Доработка: Кряжев А., 2026
// События USB и ограниченное восстановление по ESOF.
// Порядок событий и двоичные дескрипторы сохраняют протокол CDC.
// Общие файлы также используются загрузчиком.

#ifndef BOOTLOADER
#include "../../User/domovoi_diag.h"
#endif
// Доработка: Кряжев А., 2026
/**
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
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

#ifndef BOOTLOADER
#include "usb_stm32f10x.h"
#endif
#include "usb_lib.h"
#include "usb_prop.h"
#include "usb_pwr.h"
#include "usb_istr.h"
#ifdef BOOTLOADER
extern void boot_usb_request_recovery(void);
#endif
#ifndef BOOTLOADER
#include "hw_config.h"
#endif

__IO uint16_t wIstr;
__IO uint32_t esof_counter =0;
__IO uint32_t wCNTR=0;

/* Таблицы обратных вызовов конечных точек, кроме управляющей EP0. */
void (*const pEpInt_IN[7])(void) =
  {
    EP1_IN_Callback,
    EP2_IN_Callback,
    EP3_IN_Callback,
    EP4_IN_Callback,
    EP5_IN_Callback,
    EP6_IN_Callback,
    EP7_IN_Callback,
  };

void (*const pEpInt_OUT[7])(void) =
  {
    EP1_OUT_Callback,
    EP2_OUT_Callback,
    EP3_OUT_Callback,
    EP4_OUT_Callback,
    EP5_OUT_Callback,
    EP6_OUT_Callback,
    EP7_OUT_Callback,
  };

// Обслужить события USB и ограниченное восстановление после потери кадра.
void USB_Istr(void)
{
    uint32_t i=0;
 __IO uint32_t EP[8];

  wIstr = _GetISTR();

  if (wIstr & ISTR_SOF & wInterrupt_Mask)
  {
    _SetISTR((uint16_t)CLR_SOF);

    SOF_Callback();
  }

  if (wIstr & ISTR_CTR & wInterrupt_Mask)
  {

    CTR_LP();
  }

  if (wIstr & ISTR_RESET & wInterrupt_Mask)
  {
    _SetISTR((uint16_t)CLR_RESET);
    Device_Property.Reset();
  }

  if (wIstr & ISTR_ERR & wInterrupt_Mask)
  {
    _SetISTR((uint16_t)CLR_ERR);
#ifndef BOOTLOADER
#if DOMOVOI_DIAG
    usb_bus_error_seen();
#endif
#endif
  }

  if (wIstr & ISTR_ESOF & wInterrupt_Mask)
  {

    _SetISTR((uint16_t)CLR_ESOF);

    if ((_GetFNR()&FNR_RXDP)!=0)
    {

      esof_counter ++;

      /* Более трёх ESOF при FSUSP=0 и RXDP=1 означает возможную потерю SUSP. */
      if ((esof_counter >3)&&((_GetCNTR()&CNTR_FSUSP)==0))
      {
        /* Восстановление принудительным сбросом после потери события шины. */

        /* Сохранить маску USB-прерываний до сброса. */
        wCNTR = _GetCNTR();

        /* Сохранить настройки конечных точек до сброса USB. */
        for (i=0;i<8;i++) EP[i] = _GetENDPOINT(i);

#ifndef BOOTLOADER
#if DOMOVOI_DIAG
        usb_esof_forced++;
#endif
#endif

        wCNTR|=CNTR_FRES;
        _SetCNTR(wCNTR);

        wCNTR&=~CNTR_FRES;
        _SetCNTR(wCNTR);

        /* Дождаться подтверждения аппаратного сброса с ограничением времени. */
        if (!USB_WaitReset()) {
#ifndef BOOTLOADER
#if DOMOVOI_DIAG
          usb_esof_timeouts++;
          usb_recovery_reason = 1u;
#endif
          usb_request_recovery();
#else
          boot_usb_request_recovery();
#endif
          esof_counter = 0;
          return;
        }
        _SetISTR((uint16_t)CLR_RESET);

       /* Восстановить настройки конечных точек после сброса. */
        for (i=0;i<8;i++)
        _SetENDPOINT(i, EP[i]);

        esof_counter = 0;
      }
    }
    else
    {
        esof_counter = 0;
    }

    /* Временные фазы восстановления отсчитываются по событиям ESOF. */

  }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
