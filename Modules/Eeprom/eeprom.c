/**
  ******************************************************************************
  * @file           : eeprom.c
  * @brief          : Eeprom emulation driver
  * @author         : MicroTechnics (microtechnics.ru)
  ******************************************************************************
  */



/* Includes ------------------------------------------------------------------*/

#include "eeprom.h"



/* Declarations and definitions ----------------------------------------------*/

static uint32_t pageAddress[PAGES_NUM] = {PAGE_0_ADDRESS, PAGE_1_ADDRESS};
static uint32_t varIdList[VAR_NUM] = {PARAM_1, PARAM_2};

uint32_t FLASH_Read(uint32_t address);
PageState EEPROM_ReadPageState(PageIdx idx);
EepromResult EEPROM_SetPageState(PageIdx idx, PageState state);
EepromResult EEPROM_ClearPage(PageIdx idx);
EepromResult EEPROM_Format();
EepromResult EEPROM_GetActivePageIdx(PageIdx *idx);
EepromResult EEPROM_Init();



/* Functions -----------------------------------------------------------------*/
EepromResult EEPROM_WriteData(uint32_t address, uint32_t varId, uint32_t varValue)
{
  EepromResult res = EEPROM_OK;
  HAL_StatusTypeDef flashRes = HAL_OK;
  uint64_t fullData = ((uint64_t)varValue << 32) | (uint64_t)varId;
  
  HAL_FLASH_Unlock();
  flashRes = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, fullData);
  HAL_FLASH_Lock();
  
  if (flashRes != HAL_OK)
  {
    res = EEPROM_ERROR;
  }
  
  return res;
}



/******************************************************************************/
EepromResult EEPROM_Read(uint32_t varId, uint32_t *varValue)
{
  EepromResult res = EEPROM_OK;
  
  PageIdx activePage = PAGE_0;
  res = EEPROM_GetActivePageIdx(&activePage);
  
  if (res != EEPROM_OK)
  {
    return res;
  }
  
  uint32_t startAddr = pageAddress[activePage] + PAGE_DATA_OFFSET;
  uint32_t endAddr = pageAddress[activePage] + PAGE_SIZE - PAGE_DATA_SIZE;
  uint32_t addr = endAddr;
  
  uint8_t dataFound = 0;
    
  while (addr >= startAddr)
  {
    uint32_t idData = FLASH_Read(addr);
    if (idData == varId)
    {
      dataFound = 1;
      *varValue = FLASH_Read(addr + 4);
      break;
    }
    else
    {
      addr -= PAGE_DATA_SIZE;
    }
  }
  
  if (dataFound == 0)
  {
    res = EEPROM_ERROR;
  }
  
  return res;
}



/******************************************************************************/
EepromResult EEPROM_PageTransfer(PageIdx activePage, uint32_t varId, uint32_t varValue)
{
  EepromResult res = EEPROM_OK;
  PageIdx oldPage, newPage;
  
  if (activePage == PAGE_0)
  {
    oldPage = PAGE_0;
    newPage = PAGE_1;
  }
  else
  {
    if (activePage == PAGE_1)
    {
      oldPage = PAGE_1;
      newPage = PAGE_0;
    }
  }
  
  res = EEPROM_SetPageState(newPage, PAGE_RECEIVING_DATA);
  
  if (res != EEPROM_OK)
  {
    return res;
  }
  
  uint32_t curAddr = pageAddress[newPage] + PAGE_DATA_OFFSET;
  res = EEPROM_WriteData(curAddr, varId, varValue);
  
  if (res != EEPROM_OK)
  {
    return res;
  }
  
  curAddr += PAGE_DATA_SIZE;
  
  for (uint32_t curVar = 0; curVar < VAR_NUM; curVar++)
  {
    if (varIdList[curVar] != varId)
    {
      uint32_t curVarValue = 0;
      res = EEPROM_Read(varIdList[curVar], &curVarValue);
      
      if (res == EEPROM_OK)
      {
        res = EEPROM_WriteData(curAddr, varIdList[curVar], curVarValue);
  
        if (res != EEPROM_OK)
        {
          return res;
        }

        curAddr += PAGE_DATA_SIZE;
      }
    }
  }
  
  res = EEPROM_ClearPage(oldPage);
  
  if (res != EEPROM_OK)
  {
    return res;
  }
  
  res = EEPROM_SetPageState(newPage, PAGE_ACTIVE);
  
  return res;
}



/******************************************************************************/
EepromResult EEPROM_Write(uint32_t varId, uint32_t varValue)
{
  EepromResult res = EEPROM_OK;
  
  uint8_t validId = 0;
  for (uint32_t curVar = 0; curVar < VAR_NUM; curVar++)
  {
    if (varIdList[curVar] == varId)
    {
      validId = 1;
      break;
    }
  }
  
  if (validId == 0)
  {
    res = EEPROM_ERROR;
    return res;
  }
  
  PageIdx activePage = PAGE_0;
  res = EEPROM_GetActivePageIdx(&activePage);
  
  if (res != EEPROM_OK)
  {
    return res;
  }
  
  uint32_t startAddr = pageAddress[activePage] + PAGE_DATA_OFFSET;
  uint32_t endAddr = pageAddress[activePage] + PAGE_SIZE - PAGE_DATA_SIZE;
  uint32_t addr = startAddr;
  
  uint8_t freeSpaceFound = 0;
  
  while (addr <= endAddr)
  {
    uint32_t idData = FLASH_Read(addr);
    if (idData == 0xFFFFFFFF)
    {
      freeSpaceFound = 1;
      break;
    }
    else
    {
      addr += PAGE_DATA_SIZE;
    }
  }
  
  if (freeSpaceFound == 1)
  {
    res = EEPROM_WriteData(addr, varId, varValue);
  }
  else
  {
    res = EEPROM_PageTransfer(activePage, varId, varValue);
  }
  
  return res;
}



/******************************************************************************/
EepromResult EEPROM_CopyPageData(PageIdx oldPage, PageIdx newPage)
{
  EepromResult res = EEPROM_OK;
  
  uint32_t curAddr = pageAddress[newPage] + PAGE_DATA_OFFSET;
    
  for (uint32_t curVar = 0; curVar < VAR_NUM; curVar++)
  {
    uint32_t curVarValue = 0;
    res = EEPROM_Read(varIdList[curVar], &curVarValue);
    
    if (res == EEPROM_OK)
    {
      res = EEPROM_WriteData(curAddr, varIdList[curVar], curVarValue);

      if (res != EEPROM_OK)
      {
        return res;
      }

      curAddr += PAGE_DATA_SIZE;
    }
  }
  
  res = EEPROM_SetPageState(newPage, PAGE_ACTIVE);
  
  if (res != EEPROM_OK)
  {
    return res;
  }
  
  res = EEPROM_ClearPage(oldPage);
  
  return res;
}



/******************************************************************************/
uint32_t FLASH_Read(uint32_t address)
{
  return (*(__IO uint32_t*)address);
}



/******************************************************************************/
PageState EEPROM_ReadPageState(PageIdx idx)
{
  PageState pageState;
  pageState = (PageState)FLASH_Read(pageAddress[idx]);
  return pageState;
}



/******************************************************************************/
EepromResult EEPROM_SetPageState(PageIdx idx, PageState state)
{
  EepromResult res = EEPROM_OK;
  HAL_StatusTypeDef flashRes = HAL_OK;
  
  HAL_FLASH_Unlock();
  flashRes = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, pageAddress[idx], state);                   
  HAL_FLASH_Lock();
  
  if (flashRes != HAL_OK)
  {
    res = EEPROM_ERROR;
  }
  
  return res;
}



/******************************************************************************/
EepromResult EEPROM_ClearPage(PageIdx idx)
{
  EepromResult res = EEPROM_OK;
  
  FLASH_EraseInitTypeDef erase;
  
  erase.TypeErase = FLASH_TYPEERASE_PAGES;
  erase.Banks = FLASH_BANK_1;
  erase.PageAddress = pageAddress[idx];  
  erase.NbPages = 1;
  
  HAL_StatusTypeDef flashRes = HAL_OK;
  uint32_t pageError = 0;
  
  HAL_FLASH_Unlock();
  flashRes = HAL_FLASHEx_Erase(&erase, &pageError);
  HAL_FLASH_Lock();
  
  if (flashRes != HAL_OK)
  {
    res = EEPROM_ERROR;
    return res;
  }
  
  res = EEPROM_SetPageState(idx, PAGE_CLEARED);
  
  return res;
}



/******************************************************************************/
EepromResult EEPROM_Format()
{
  EepromResult res = EEPROM_OK;
  
  for (uint8_t i = 0; i < PAGES_NUM; i++)
  {
    res = EEPROM_ClearPage((PageIdx)i);
    
    if (res != EEPROM_OK)
    {
      return res;
    }
  }
    
  return res;
}



/******************************************************************************/
EepromResult EEPROM_GetActivePageIdx(PageIdx *idx)
{
  EepromResult res = EEPROM_OK;
  PageState pageStates[PAGES_NUM];

  for (uint8_t i = 0; i < PAGES_NUM; i++)
  {
    pageStates[i] = EEPROM_ReadPageState((PageIdx)i);
  }
  
  if ((pageStates[PAGE_0] == PAGE_ACTIVE) && (pageStates[PAGE_1] != PAGE_ACTIVE))
  {
    *idx = PAGE_0;
  }
  else
  {
    if ((pageStates[PAGE_1] == PAGE_ACTIVE) && (pageStates[PAGE_0] != PAGE_ACTIVE))
    {
      *idx = PAGE_1;
    }
    else
    {
      res = EEPROM_ERROR;
    }
  }
  
  return res;
}



/******************************************************************************/
EepromResult EEPROM_Init()
{
  EepromResult res = EEPROM_OK;
  PageState pageStates[PAGES_NUM];
  
  for (uint8_t i = 0; i < PAGES_NUM; i++)
  {
    pageStates[i] = EEPROM_ReadPageState((PageIdx)i);
  }
  
  if (((pageStates[PAGE_0] == PAGE_CLEARED) && (pageStates[PAGE_1] == PAGE_CLEARED)) ||
      ((pageStates[PAGE_0] == PAGE_ACTIVE) && (pageStates[PAGE_1] == PAGE_ACTIVE)) || 
      ((pageStates[PAGE_0] == PAGE_RECEIVING_DATA) && (pageStates[PAGE_1] == PAGE_RECEIVING_DATA)))
  {
    res = EEPROM_Format();
    
    if (res != EEPROM_OK)
    {
      return res;
    }
  
    res = EEPROM_SetPageState(PAGE_0, PAGE_ACTIVE);
  }
  
  if ((pageStates[PAGE_0] == PAGE_RECEIVING_DATA) && (pageStates[PAGE_1] == PAGE_CLEARED))
  {
    res = EEPROM_SetPageState(PAGE_0, PAGE_ACTIVE);
  }
  
  if ((pageStates[PAGE_0] == PAGE_CLEARED) && (pageStates[PAGE_1] == PAGE_RECEIVING_DATA))
  {
    res = EEPROM_SetPageState(PAGE_1, PAGE_ACTIVE);
  }
  
  if ((pageStates[PAGE_0] == PAGE_RECEIVING_DATA) && (pageStates[PAGE_1] == PAGE_ACTIVE))
  {
    res = EEPROM_CopyPageData(PAGE_1, PAGE_0);
  }
  
  if ((pageStates[PAGE_0] == PAGE_ACTIVE) && (pageStates[PAGE_1] == PAGE_RECEIVING_DATA))
  {
    res = EEPROM_CopyPageData(PAGE_0, PAGE_1);
  }
  
  return res;
}



/******************************************************************************/
