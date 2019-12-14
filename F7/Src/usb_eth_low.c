/* code derive de la demo :
 \Cube7\Projects\STM32746G-Discovery\Examples\PWR\PWR_CurrentConsumption

*/
#include "stm32746g_discovery.h"

/* ULPI PHY */
#define USBULPI_PHYCR     ((uint32_t)(0x40040000 + 0x034))
#define USBULPI_D07       ((uint32_t)0x000000FF)
#define USBULPI_New       ((uint32_t)0x02000000)
#define USBULPI_RW        ((uint32_t)0x00400000) 
#define USBULPI_S_BUSY    ((uint32_t)0x04000000) 
#define USBULPI_S_DONE    ((uint32_t)0x08000000)

#define Pattern_55        ((uint32_t)0x00000055)
#define Pattern_AA        ((uint32_t)0x000000AA)

#define PHY_PWR_DOWN       (1<<11)
#define PHY_ADDRESS        0x00 /* default ADDR for PHY: LAN8742 */

#define USB_OTG_READ_REG32(reg)  (*(__IO uint32_t *)(reg))
#define USB_OTG_WRITE_REG32(reg,value) (*(__IO uint32_t *)(reg) = (value))

void Error_Handler(void)
{ while (1){} }

/**
  * @brief  Read CR value
  * @param  Addr the Address of the ULPI Register
  * @retval Returns value of PHY CR register
  */
static uint32_t USB_ULPI_Read(uint32_t Addr)
{ 
   __IO uint32_t val = 0;
   __IO uint32_t timeout = 100; /* Can be tuned based on the Clock or etc... */
   
   USB_OTG_WRITE_REG32(USBULPI_PHYCR, USBULPI_New | (Addr << 16));
   val = USB_OTG_READ_REG32(USBULPI_PHYCR);
   while (((val & USBULPI_S_DONE) == 0) && (timeout--)) 
   { 
       val = USB_OTG_READ_REG32(USBULPI_PHYCR);
   } 
   val = USB_OTG_READ_REG32(USBULPI_PHYCR);  
   return  (val & 0x000000ff);
}

/**
  * @brief  Write CR value
  * @param  Addr the Address of the ULPI Register
  * @param  Data Data to write
  * @retval Returns value of PHY CR register
  */
static uint32_t USB_ULPI_Write(uint32_t Addr, uint32_t Data)   /* Parameter is the Address of the ULPI Register & Date to write */
{  
  __IO uint32_t val;
  __IO uint32_t timeout = 10;   /* Can be tuned based on the Clock or etc... */
  
  USB_OTG_WRITE_REG32(USBULPI_PHYCR, USBULPI_New | USBULPI_RW | (Addr << 16) | (Data & 0x000000ff));
  val = USB_OTG_READ_REG32(USBULPI_PHYCR);
  while (((val & USBULPI_S_DONE) == 0) && (timeout--)) 
  {
           val = USB_OTG_READ_REG32(USBULPI_PHYCR);
  }
  
  val = USB_OTG_READ_REG32(USBULPI_PHYCR);
  return 0; 
}

/**
  * @brief  Configures GPIO for USB ULPI
  * @param  None
  * @retval 0
  */
static void USB_ULPI_MspInit(void)
{
  GPIO_InitTypeDef  GPIO_InitStruct;
  
  /* Enable GPIOs clock */
   __HAL_RCC_GPIOA_CLK_ENABLE();
   __HAL_RCC_GPIOB_CLK_ENABLE();
   __HAL_RCC_GPIOC_CLK_ENABLE();
   __HAL_RCC_GPIOH_CLK_ENABLE();
  
    /* Common for all IOs */
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OTG_HS;
   
    /* D0 PA3*/
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    /* CLK PA5*/
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);    

    /* D1 D2 D3 D4 D5 D6 D7 :PB0/1/5/10/11/12/13 */
    GPIO_InitStruct.Pin = GPIO_PIN_0  | GPIO_PIN_1  | GPIO_PIN_5 |\
                          GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 |\
                          GPIO_PIN_13;
    GPIO_InitStruct.Alternate = GPIO_AF10_OTG_HS;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* NXT  PH4*/
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);
    
    /* STP PC0*/
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* DIR PC2 */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    
  __HAL_RCC_USB_OTG_HS_CLK_ENABLE();
  __HAL_RCC_USB_OTG_HS_ULPI_CLK_ENABLE();
}

/**
  * @brief  This function configures the USB PHY to enter the low power mode
  * @param  None
  * @retval None
  */
void USB_PhyEnterLowPowerMode(void)
{
  static __IO uint32_t regval = 0;
   
  /* USB ULPI MspInit */
  USB_ULPI_MspInit();
  
  /* disable ULPI_CLK by accessing ULPI_PHY */
  /* read Vendor ID : (Low, High) 0x24,0x04 for USB3300 */
  regval = USB_ULPI_Read(0x00);
  
  if(regval != 0x24)
  {
    while(regval != 0x24)
    {
      USB_ULPI_MspInit();
      regval = USB_ULPI_Read(0x00);
    }
  }
  
  regval = USB_ULPI_Read(0x01);
  if(regval != 0x04)
  {
    Error_Handler();
  }
  /* read Product ID */
  regval = USB_ULPI_Read(0x02);
  if(regval != 0x07)
  {
    Error_Handler();
  }
  regval = USB_ULPI_Read(0x03);
  if(regval != 0x00)
  {
    Error_Handler();
  }
  /* Write to scratch Register  the Pattern_55 */
  USB_ULPI_Write(0x16, 0x55);
  /* Read to scratch Register  and check-it again the written Pattern */
  regval = USB_ULPI_Read(0x16);
  if(regval != 0x55)
  {
    Error_Handler();
  }
  /* Write to scratch Register  the Pattern_AA */
  USB_ULPI_Write(0x16, 0xAA);
  /* Read to scratch Register  and check-it again the written Pattern */
  regval = USB_ULPI_Read(0x16);
  if(regval != 0xAA)
  {
    Error_Handler();
  }
  /* read InterfaceControl reg */
  regval = USB_ULPI_Read(0x07);

  /* write InterfaceControl reg,to disable PullUp on stp,
  to avoid USB_PHY wake up when MCU entering standby */
  USB_ULPI_Write(0x07, regval | 0x80) ;
  /* read InterfaceControl reg */
  regval = USB_ULPI_Read(0x07);
  if(regval != 0x80)
  {
    Error_Handler();
  }
  /* read FunctionControl reg */
  regval = USB_ULPI_Read(0x04);
  
  if(regval != 0x49)
  {
    Error_Handler();
  }
  /* write FunctionControl reg,to put PHY into LowPower mode */
  USB_ULPI_Write(0x04, regval & (~0x40));
  /* read FunctionControl reg again */ 
  regval = USB_ULPI_Read(0x04); 
  
  if(regval != 0x00)
  {
    Error_Handler();
  }
}


/**
  * @brief  This function configures the ETH PHY to enter the power down mode
  *         This function should be called before entering the low power mode.
  * @param  None
  * @retval None
  */
void ETH_PhyEnterPowerDownMode(void)
{
  ETH_HandleTypeDef heth;
  GPIO_InitTypeDef GPIO_InitStruct;
  uint32_t phyregval = 0; 
   
  /* This part of code is used only when the ETH peripheral is disabled 
	   when the ETH is used in the application this initialization code 
	   is called in HAL_ETH_MspInit() function  ***********************/
	
	/* Enable GPIO clocks*/
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  
  /* Configure PA2: ETH_MDIO */
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL; 
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  
  /* Configure PC1: ETH_MDC */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  
  /* Enable the ETH peripheral clocks */
  __HAL_RCC_ETH_CLK_ENABLE();
  
  /* Set ETH Handle parameters */
  heth.Instance = ETH;
  heth.Init.PhyAddress = LAN8742A_PHY_ADDRESS;

  /* Configure MDC clock: the MDC Clock Range configuration
     depend on the system clock: 216Mhz/102 = 2.1MHz  */
  /* MDC: a periodic clock that provides the timing reference for 
     the MDIO data transfer at the maximum frequency of 2.5 MHz.*/
  heth.Instance->MACMIIAR = (uint32_t)ETH_MACMIIAR_CR_Div102;

  /*****************************************************************/
  
  /* ETH PHY configuration in Power Down mode **********************/ 
  
  /* Read ETH PHY control register */
  HAL_ETH_ReadPHYRegister(&heth, PHY_BCR, &phyregval);
  
  /* Set Power down mode bit */
  phyregval |= PHY_POWERDOWN;
  
  /* Write new value to ETH PHY control register */
  HAL_ETH_WritePHYRegister(&heth, PHY_BCR, phyregval);
  
  /*****************************************************************/
  
  /* Disable periph CLKs */
  __HAL_RCC_GPIOA_CLK_DISABLE();
  __HAL_RCC_GPIOC_CLK_DISABLE();
  __HAL_RCC_ETH_CLK_DISABLE();
}
