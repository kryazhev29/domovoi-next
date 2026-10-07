

#include "stm32f10x_flash.h"

/**
  * @}
  */

#define ACR_LATENCY_Mask         ((uint32_t)0x00000038)
#define ACR_HLFCYA_Mask          ((uint32_t)0xFFFFFFF7)
#define ACR_PRFTBE_Mask          ((uint32_t)0xFFFFFFEF)

#define ACR_PRFTBS_Mask          ((uint32_t)0x00000020)

#define CR_PG_Set                ((uint32_t)0x00000001)
#define CR_PG_Reset              ((uint32_t)0x00001FFE)
#define CR_PER_Set               ((uint32_t)0x00000002)
#define CR_PER_Reset             ((uint32_t)0x00001FFD)
#define CR_MER_Set               ((uint32_t)0x00000004)
#define CR_MER_Reset             ((uint32_t)0x00001FFB)
#define CR_OPTPG_Set             ((uint32_t)0x00000010)
#define CR_OPTPG_Reset           ((uint32_t)0x00001FEF)
#define CR_OPTER_Set             ((uint32_t)0x00000020)
#define CR_OPTER_Reset           ((uint32_t)0x00001FDF)
#define CR_STRT_Set              ((uint32_t)0x00000040)
#define CR_LOCK_Set              ((uint32_t)0x00000080)

#define RDPRT_Mask               ((uint32_t)0x00000002)
#define WRP0_Mask                ((uint32_t)0x000000FF)
#define WRP1_Mask                ((uint32_t)0x0000FF00)
#define WRP2_Mask                ((uint32_t)0x00FF0000)
#define WRP3_Mask                ((uint32_t)0xFF000000)
#define OB_USER_BFB2             ((uint16_t)0x0008)

#define FLASH_BANK1_END_ADDRESS   ((uint32_t)0x807FFFF)

#define EraseTimeout          ((uint32_t)0x000B0000)
#define ProgramTimeout        ((uint32_t)0x00002000)
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

void FLASH_Unlock(void)
{

  FLASH->KEYR = FLASH_KEY1;
  FLASH->KEYR = FLASH_KEY2;

#ifdef STM32F10X_XL

  FLASH->KEYR2 = FLASH_KEY1;
  FLASH->KEYR2 = FLASH_KEY2;
#endif
}

#ifdef STM32F10X_XL

#endif

void FLASH_Lock(void)
{

  FLASH->CR |= CR_LOCK_Set;

#ifdef STM32F10X_XL

  FLASH->CR2 |= CR_LOCK_Set;
#endif
}

#ifdef STM32F10X_XL

#endif

FLASH_Status FLASH_ErasePage(uint32_t Page_Address)
{
  FLASH_Status status = FLASH_COMPLETE;

  assert_param(IS_FLASH_ADDRESS(Page_Address));

#ifdef STM32F10X_XL
  if(Page_Address < FLASH_BANK1_END_ADDRESS)
  {

    status = FLASH_WaitForLastBank1Operation(EraseTimeout);
    if(status == FLASH_COMPLETE)
    {

      FLASH->CR|= CR_PER_Set;
      FLASH->AR = Page_Address;
      FLASH->CR|= CR_STRT_Set;

      status = FLASH_WaitForLastBank1Operation(EraseTimeout);

      FLASH->CR &= CR_PER_Reset;
    }
  }
  else
  {

    status = FLASH_WaitForLastBank2Operation(EraseTimeout);
    if(status == FLASH_COMPLETE)
    {

      FLASH->CR2|= CR_PER_Set;
      FLASH->AR2 = Page_Address;
      FLASH->CR2|= CR_STRT_Set;

      status = FLASH_WaitForLastBank2Operation(EraseTimeout);

      FLASH->CR2 &= CR_PER_Reset;
    }
  }
#else

  status = FLASH_WaitForLastOperation(EraseTimeout);

  if(status == FLASH_COMPLETE)
  {

    FLASH->CR|= CR_PER_Set;
    FLASH->AR = Page_Address;
    FLASH->CR|= CR_STRT_Set;

    status = FLASH_WaitForLastOperation(EraseTimeout);

    FLASH->CR &= CR_PER_Reset;
  }
#endif

  return status;
}

#ifdef STM32F10X_XL

#endif

FLASH_Status FLASH_ProgramWord(uint32_t Address, uint32_t Data)
{
  FLASH_Status status = FLASH_COMPLETE;
  __IO uint32_t tmp = 0;

  assert_param(IS_FLASH_ADDRESS(Address));

#ifdef STM32F10X_XL
  if(Address < FLASH_BANK1_END_ADDRESS - 2)
  {

    status = FLASH_WaitForLastBank1Operation(ProgramTimeout);
    if(status == FLASH_COMPLETE)
    {

      FLASH->CR |= CR_PG_Set;

      *(__IO uint16_t*)Address = (uint16_t)Data;

      status = FLASH_WaitForLastOperation(ProgramTimeout);

      if(status == FLASH_COMPLETE)
      {

        tmp = Address + 2;

        *(__IO uint16_t*) tmp = Data >> 16;

        status = FLASH_WaitForLastOperation(ProgramTimeout);

        FLASH->CR &= CR_PG_Reset;
      }
      else
      {

        FLASH->CR &= CR_PG_Reset;
       }
    }
  }
  else if(Address == (FLASH_BANK1_END_ADDRESS - 1))
  {

    status = FLASH_WaitForLastBank1Operation(ProgramTimeout);

    if(status == FLASH_COMPLETE)
    {

      FLASH->CR |= CR_PG_Set;

      *(__IO uint16_t*)Address = (uint16_t)Data;

      status = FLASH_WaitForLastBank1Operation(ProgramTimeout);

      FLASH->CR &= CR_PG_Reset;
    }
    else
    {

      FLASH->CR &= CR_PG_Reset;
    }

    status = FLASH_WaitForLastBank2Operation(ProgramTimeout);

    if(status == FLASH_COMPLETE)
    {

      FLASH->CR2 |= CR_PG_Set;
      tmp = Address + 2;

      *(__IO uint16_t*) tmp = Data >> 16;

      status = FLASH_WaitForLastBank2Operation(ProgramTimeout);

      FLASH->CR2 &= CR_PG_Reset;
    }
    else
    {

      FLASH->CR2 &= CR_PG_Reset;
    }
  }
  else
  {

    status = FLASH_WaitForLastBank2Operation(ProgramTimeout);

    if(status == FLASH_COMPLETE)
    {

      FLASH->CR2 |= CR_PG_Set;

      *(__IO uint16_t*)Address = (uint16_t)Data;

      status = FLASH_WaitForLastBank2Operation(ProgramTimeout);

      if(status == FLASH_COMPLETE)
      {

        tmp = Address + 2;

        *(__IO uint16_t*) tmp = Data >> 16;

        status = FLASH_WaitForLastBank2Operation(ProgramTimeout);

        FLASH->CR2 &= CR_PG_Reset;
      }
      else
      {

        FLASH->CR2 &= CR_PG_Reset;
      }
    }
  }
#else

  status = FLASH_WaitForLastOperation(ProgramTimeout);

  if(status == FLASH_COMPLETE)
  {

    FLASH->CR |= CR_PG_Set;

    *(__IO uint16_t*)Address = (uint16_t)Data;

    status = FLASH_WaitForLastOperation(ProgramTimeout);

    if(status == FLASH_COMPLETE)
    {

      tmp = Address + 2;

      *(__IO uint16_t*) tmp = Data >> 16;

      status = FLASH_WaitForLastOperation(ProgramTimeout);

      FLASH->CR &= CR_PG_Reset;
    }
    else
    {

      FLASH->CR &= CR_PG_Reset;
    }
  }
#endif

  return status;
}

#ifdef STM32F10X_XL

#endif

void FLASH_ClearFlag(uint32_t FLASH_FLAG)
{
#ifdef STM32F10X_XL

  assert_param(IS_FLASH_CLEAR_FLAG(FLASH_FLAG)) ;

  if((FLASH_FLAG & 0x80000000) != 0x0)
  {

    FLASH->SR2 = FLASH_FLAG;
  }
  else
  {

    FLASH->SR = FLASH_FLAG;
  }

#else

  assert_param(IS_FLASH_CLEAR_FLAG(FLASH_FLAG)) ;

  FLASH->SR = FLASH_FLAG;
#endif
}

FLASH_Status FLASH_GetBank1Status(void)
{
  FLASH_Status flashstatus = FLASH_COMPLETE;

  if((FLASH->SR & FLASH_FLAG_BANK1_BSY) == FLASH_FLAG_BSY)
  {
    flashstatus = FLASH_BUSY;
  }
  else
  {
    if((FLASH->SR & FLASH_FLAG_BANK1_PGERR) != 0)
    {
      flashstatus = FLASH_ERROR_PG;
    }
    else
    {
      if((FLASH->SR & FLASH_FLAG_BANK1_WRPRTERR) != 0 )
      {
        flashstatus = FLASH_ERROR_WRP;
      }
      else
      {
        flashstatus = FLASH_COMPLETE;
      }
    }
  }

  return flashstatus;
}

#ifdef STM32F10X_XL

FLASH_Status FLASH_GetBank2Status(void)
{
  FLASH_Status flashstatus = FLASH_COMPLETE;

  if((FLASH->SR2 & (FLASH_FLAG_BANK2_BSY & 0x7FFFFFFF)) == (FLASH_FLAG_BANK2_BSY & 0x7FFFFFFF))
  {
    flashstatus = FLASH_BUSY;
  }
  else
  {
    if((FLASH->SR2 & (FLASH_FLAG_BANK2_PGERR & 0x7FFFFFFF)) != 0)
    {
      flashstatus = FLASH_ERROR_PG;
    }
    else
    {
      if((FLASH->SR2 & (FLASH_FLAG_BANK2_WRPRTERR & 0x7FFFFFFF)) != 0 )
      {
        flashstatus = FLASH_ERROR_WRP;
      }
      else
      {
        flashstatus = FLASH_COMPLETE;
      }
    }
  }

  return flashstatus;
}
#endif

FLASH_Status FLASH_WaitForLastOperation(uint32_t Timeout)
{
  FLASH_Status status = FLASH_COMPLETE;

  status = FLASH_GetBank1Status();

  while((status == FLASH_BUSY) && (Timeout != 0x00))
  {
    status = FLASH_GetBank1Status();
    Timeout--;
  }
  if(Timeout == 0x00 )
  {
    status = FLASH_TIMEOUT;
  }

  return status;
}

FLASH_Status FLASH_WaitForLastBank1Operation(uint32_t Timeout)
{
  FLASH_Status status = FLASH_COMPLETE;

  status = FLASH_GetBank1Status();

  while((status == FLASH_FLAG_BANK1_BSY) && (Timeout != 0x00))
  {
    status = FLASH_GetBank1Status();
    Timeout--;
  }
  if(Timeout == 0x00 )
  {
    status = FLASH_TIMEOUT;
  }

  return status;
}

#ifdef STM32F10X_XL

FLASH_Status FLASH_WaitForLastBank2Operation(uint32_t Timeout)
{
  FLASH_Status status = FLASH_COMPLETE;

  status = FLASH_GetBank2Status();

  while((status == (FLASH_FLAG_BANK2_BSY & 0x7FFFFFFF)) && (Timeout != 0x00))
  {
    status = FLASH_GetBank2Status();
    Timeout--;
  }
  if(Timeout == 0x00 )
  {
    status = FLASH_TIMEOUT;
  }

  return status;
}
#endif

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
