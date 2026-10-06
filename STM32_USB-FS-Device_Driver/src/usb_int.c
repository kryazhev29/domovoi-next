// Доработка: Кряжев А., 2026
// Разбор завершённых передач и вызовы обработчиков точек.
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

#include "usb_lib.h"

__IO uint16_t SaveRState;
__IO uint16_t SaveTState;

extern void (*const pEpInt_IN[7])(void);
extern void (*const pEpInt_OUT[7])(void);

// Разобрать завершённые передачи EP0 и вызвать обработчики точек CDC.
void CTR_LP(void)
{
  uint16_t wEPVal;
  uint8_t EPindex;

  while (((wIstr = _GetISTR()) & ISTR_CTR) != 0)
  {

    EPindex = (uint8_t)(wIstr & ISTR_EP_ID);
    if (EPindex == 0)
    {

	    SaveRState = _GetENDPOINT(ENDP0);
	    SaveTState = SaveRState & EPTX_STAT;
	    SaveRState &=  EPRX_STAT;

	    _SetEPRxTxStatus(ENDP0,EP_RX_NAK,EP_TX_NAK);

      if ((wIstr & ISTR_DIR) == 0)
      {

        _ClearEP_CTR_TX(ENDP0);
        In0_Process();

            _SetEPRxTxStatus(ENDP0,SaveRState,SaveTState);
		  return;
      }
      else
      {

        wEPVal = _GetENDPOINT(ENDP0);

        if ((wEPVal &EP_SETUP) != 0)
        {
          _ClearEP_CTR_RX(ENDP0);
          Setup0_Process();

		      _SetEPRxTxStatus(ENDP0,SaveRState,SaveTState);
          return;
        }

        else if ((wEPVal & EP_CTR_RX) != 0)
        {
          _ClearEP_CTR_RX(ENDP0);
          Out0_Process();

		     _SetEPRxTxStatus(ENDP0,SaveRState,SaveTState);
          return;
        }
      }
    }
    else
    {

      wEPVal = _GetENDPOINT(EPindex);
      if ((wEPVal & EP_CTR_RX) != 0)
      {

        _ClearEP_CTR_RX(EPindex);

        (*pEpInt_OUT[EPindex-1])();

      }

      if ((wEPVal & EP_CTR_TX) != 0)
      {

        _ClearEP_CTR_TX(EPindex);

        (*pEpInt_IN[EPindex-1])();
      }

    }

  }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
