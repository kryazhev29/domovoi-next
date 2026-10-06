// Доработка: Кряжев А., 2026
// USB CDC платы v5: usb_istr.
// Модуль сохраняет дескрипторы, порядок событий и двоичный протокол приложения.
// Диагностические счётчики отделены от обязательных таймаутов восстановления.
#ifndef BOOTLOADER
#include "../../User/domovoi_diag.h"
#endif
// Доработка: Кряжев А., 2026
/**
  ******************************************************************************
  * @file    usb_istr.c
  * @author  MCD Application Team
  * @version V4.0.0
  * @date    21-January-2013
  * @brief   ISTR events interrupt service routines
  ******************************************************************************
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

/*
Ищи сноску USERCOMMENTED
То что закоментировано приводило к нестабильной работе
*/


#ifndef BOOTLOADER
#include "usb_stm32f10x.h"
#endif
#include "usb_lib.h"
#include "usb_prop.h"
#include "usb_pwr.h"
#include "usb_istr.h"
#ifndef BOOTLOADER
#include "hw_config.h"
#endif

__IO uint16_t wIstr;
__IO uint8_t bIntPackSOF = 0;
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


// Обслужить USB-прерывания и ограниченное восстановление шины.
void USB_Istr(void)
{
    uint32_t i=0;
 __IO uint32_t EP[8];

  wIstr = _GetISTR();

#if (IMR_MSK & ISTR_SOF)
  if (wIstr & ISTR_SOF & wInterrupt_Mask)
  {
    _SetISTR((uint16_t)CLR_SOF);
    bIntPackSOF++;

#ifdef SOF_CALLBACK
    SOF_Callback();
#endif
  }
#endif
  /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

#if (IMR_MSK & ISTR_CTR)
  if (wIstr & ISTR_CTR & wInterrupt_Mask)
  {


    CTR_LP();
#ifdef CTR_CALLBACK
    CTR_Callback();
#endif
  }
#endif
  /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#if (IMR_MSK & ISTR_RESET)
  if (wIstr & ISTR_RESET & wInterrupt_Mask)
  {
    _SetISTR((uint16_t)CLR_RESET);
    Device_Property.Reset();
#ifdef RESET_CALLBACK
    RESET_Callback();
#endif
  }
#endif
  /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#if (IMR_MSK & ISTR_DOVR)
  if (wIstr & ISTR_DOVR & wInterrupt_Mask)
  {
    _SetISTR((uint16_t)CLR_DOVR);
#ifdef DOVR_CALLBACK
    DOVR_Callback();
#endif
  }
#endif
  /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#if (IMR_MSK & ISTR_ERR)
  if (wIstr & ISTR_ERR & wInterrupt_Mask)
  {
    _SetISTR((uint16_t)CLR_ERR);
#ifndef BOOTLOADER
#if DOMOVOI_DIAG
    usb_bus_error_seen();
#endif
#endif
#ifdef ERR_CALLBACK
    ERR_Callback();
#endif
  }
#endif
  /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#if (IMR_MSK & ISTR_WKUP)
  if (wIstr & ISTR_WKUP & wInterrupt_Mask)
  {
    _SetISTR((uint16_t)CLR_WKUP);
    Resume(RESUME_EXTERNAL);
#ifdef WKUP_CALLBACK
    WKUP_Callback();
#endif
  }
#endif
  /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#if (IMR_MSK & ISTR_SUSP)
  if (wIstr & ISTR_SUSP & wInterrupt_Mask)
  {

    if (fSuspendEnabled)
    {
      Suspend();
    }
    else
    {

      Resume(RESUME_LATER);
    }
    /* Флаг ISTR очищается после установки CNTR_FSUSP: порядок задан аппаратурой. */
    _SetISTR((uint16_t)CLR_SUSP);
#ifdef SUSP_CALLBACK
    SUSP_Callback();
#endif
  }
#endif
  /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

#if (IMR_MSK & ISTR_ESOF)
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
#ifndef BOOTLOADER
        {
          uint32_t budget = 32000u;
          while ((_GetISTR() & ISTR_RESET) == 0 && budget != 0) --budget;
          if (budget == 0) {
            /* Один FRES оставляет порт мёртвым навсегда. Назначается ограниченное отключение и
             * переподключение; главный цикл и модем продолжают работать. */
#if DOMOVOI_DIAG
            usb_esof_timeouts++;
#endif
#if DOMOVOI_DIAG
            usb_recovery_reason = 1u; /* таймаут сброса ESOF */
#endif
            usb_request_recovery();
            esof_counter = 0;
            return;
          }
        }
#else
        if (!USB_WaitReset()) { esof_counter = 0; return; }
#endif


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


#ifdef ESOF_CALLBACK
    ESOF_Callback();
#endif
  }
#endif
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
