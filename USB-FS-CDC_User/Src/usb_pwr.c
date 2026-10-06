// Доработка: Кряжев А., 2026
// USB CDC платы v5: usb_pwr.
// Модуль сохраняет дескрипторы, порядок событий и двоичный протокол приложения.
// Диагностические счётчики отделены от обязательных таймаутов восстановления.
/**
  ******************************************************************************
  * @file    usb_pwr.c
  * @author  MCD Application Team
  * @version V4.0.0
  * @date    21-January-2013
  * @brief   Connection/disconnection & power management
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


#include "usb_lib.h"
#include "usb_conf.h"
#include "usb_pwr.h"
#include "hw_config.h"
#ifdef BOOTLOADER
extern void boot_usb_request_recovery(void);
#else
extern void usb_request_recovery(void);
#endif
// ожидание RESET ограничено и не оставляет главный цикл запертым в IRQ
// Обслужить USB_WaitReset по контракту USB CDC.
uint8_t USB_WaitReset(void)
{
  uint32_t budget = 32000u;
  while ((_GetISTR() & ISTR_RESET) == 0 && budget != 0) --budget;
  if (budget != 0) return 1;
#ifdef BOOTLOADER
  boot_usb_request_recovery();
#else
  usb_request_recovery();
#endif
  return 0;
}


__IO uint32_t bDeviceState = UNCONNECTED; /* Текущее состояние устройства USB. */
#ifdef BOOTLOADER
__IO bool fSuspendEnabled = TRUE;
#else
__IO usb_bool_t fSuspendEnabled = TRUE;
#endif  /* Разрешение аппаратной приостановки USB. */
__IO uint32_t EP[8];

struct
{
  __IO RESUME_STATE eState;
  __IO uint8_t bESOFcnt;
}
ResumeS;

__IO uint32_t remotewakeupon=0;

// Включить USB с прежним аппаратным порядком сброса.
RESULT PowerOn(void)
{
  uint16_t wRegVal;




  wRegVal = CNTR_FRES;
  _SetCNTR(wRegVal);


  wInterrupt_Mask = 0;
  _SetCNTR(wInterrupt_Mask);

  _SetISTR(0);

  wInterrupt_Mask = CNTR_RESETM | CNTR_SUSPM | CNTR_WKUPM;
  _SetCNTR(wInterrupt_Mask);

  return USB_SUCCESS;
}


// Остановить USB и очистить аппаратные события.
RESULT PowerOff()
{

  _SetCNTR(CNTR_FRES);

  _SetISTR(0);


  _SetCNTR(CNTR_FRES + CNTR_PDWN);

  /* ... */

  return USB_SUCCESS;
}


// Ввести USB в низкое потребление с сохранением прежнего состояния.
void Suspend(void)
{
	uint32_t i =0;
	uint16_t wCNTR;
	uint32_t tmpreg = 0;
  __IO uint32_t savePWR_CR=0;

	/* ... */

	/* Сохранить маску USB-прерываний до сброса. */
	wCNTR = _GetCNTR();

    /* Принудительный сброс восстанавливает известное зависание периферии USB. */

	/* Сохранить настройки конечных точек до сброса USB. */
    for (i=0;i<8;i++) EP[i] = _GetENDPOINT(i);


	wCNTR|=CNTR_RESETM;
	_SetCNTR(wCNTR);


	wCNTR|=CNTR_FRES;
	_SetCNTR(wCNTR);


	wCNTR&=~CNTR_FRES;
	_SetCNTR(wCNTR);

	/* Дождаться подтверждения аппаратного сброса с ограничением времени. */
	if (!USB_WaitReset()) return;


	_SetISTR((uint16_t)CLR_RESET);

	/* Восстановить настройки конечных точек после сброса. */
	for (i=0;i<8;i++)
	_SetENDPOINT(i, EP[i]);

	/* После сброса можно безопасно приостановить блок USB. */
	wCNTR |= CNTR_FSUSP;
	_SetCNTR(wCNTR);


	wCNTR = _GetCNTR();
	wCNTR |= CNTR_LPMODE;
	_SetCNTR(wCNTR);



	savePWR_CR = PWR->CR;
	tmpreg = PWR->CR;

	tmpreg &= ((uint32_t)0xFFFFFFFC);

	tmpreg |= PWR_Regulator_LowPower;

	PWR->CR = tmpreg;

        SCB->SCR |= SCB_SCR_SLEEPDEEP;

	/* Входить в STOP только без ожидающего флага пробуждения. */
	if((_GetISTR()&ISTR_WKUP)==0)
	{
		__WFI();

                SCB->SCR &= (uint32_t)~((uint32_t)SCB_SCR_SLEEPDEEP);
	}
	else
	{

		_SetISTR(CLR_WKUP);

        wCNTR = _GetCNTR();
        wCNTR&=~CNTR_FSUSP;
        _SetCNTR(wCNTR);



		PWR->CR = savePWR_CR;


                SCB->SCR &= (uint32_t)~((uint32_t)SCB_SCR_SLEEPDEEP);
    }
}


// Восстановить USB после приостановки.
void Resume_Init(void)
{
  uint16_t wCNTR;



  /* ...  */


  wCNTR = _GetCNTR();
  wCNTR &= (~CNTR_LPMODE);
  _SetCNTR(wCNTR);



  Leave_LowPowerMode();


  _SetCNTR(IMR_MSK);


  /* ... */

}


// Выполнить согласованную фазу восстановления USB.
void Resume(RESUME_STATE eResumeSetVal)
{
  uint16_t wCNTR;

  if (eResumeSetVal != RESUME_ESOF)
    ResumeS.eState = eResumeSetVal;
  switch (ResumeS.eState)
  {
    case RESUME_EXTERNAL:
      if (remotewakeupon ==0)
      {
        Resume_Init();
        ResumeS.eState = RESUME_OFF;
      }
      else /* При RESUME во время удалённого пробуждения продолжить текущую фазу. */
      {
        ResumeS.eState = RESUME_ON;
      }
      break;
    case RESUME_INTERNAL:
      Resume_Init();
      ResumeS.eState = RESUME_START;
      remotewakeupon = 1;
      break;
    case RESUME_LATER:
      ResumeS.bESOFcnt = 2;
      ResumeS.eState = RESUME_WAIT;
      break;
    case RESUME_WAIT:
      ResumeS.bESOFcnt--;
      if (ResumeS.bESOFcnt == 0)
        ResumeS.eState = RESUME_START;
      break;
    case RESUME_START:
      wCNTR = _GetCNTR();
      wCNTR |= CNTR_RESUME;
      _SetCNTR(wCNTR);
      ResumeS.eState = RESUME_ON;
      ResumeS.bESOFcnt = 10;
      break;
    case RESUME_ON:
      ResumeS.bESOFcnt--;
      if (ResumeS.bESOFcnt == 0)
      {
        wCNTR = _GetCNTR();
        wCNTR &= (~CNTR_RESUME);
        _SetCNTR(wCNTR);
        ResumeS.eState = RESUME_OFF;
        remotewakeupon = 0;
      }
      break;
    case RESUME_OFF:
    case RESUME_ESOF:
    default:
      ResumeS.eState = RESUME_OFF;
      break;
  }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
