#include "main.h"
#include <stdio.h>

// ===================== Function Prototypes =====================
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
void DHT11_Start(void);
uint8_t DHT11_Check_Response(void);
uint8_t DHT11_Read(void);

// ===================== Handle Declarations =====================
UART_HandleTypeDef huart2;

// ===================== DHT11 Pin =====================
#define DHT11_PORT GPIOA
#define DHT11_PIN  GPIO_PIN_1

// ===================== Delay Function =====================
void delay_us(uint16_t time)
{
    uint32_t i = 0;
    for (i = 0; i < time * 10; i++)   // Adjust for 50 MHz clock
        __NOP();
}

// ===================== UART printf Redirection =====================
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

// ===================== DHT11 Functions =====================
void Set_Pin_Output(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

void Set_Pin_Input(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

void DHT11_Start(void)
{
    Set_Pin_Output(DHT11_PORT, DHT11_PIN);
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET);
    HAL_Delay(18); // min 18ms
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);
    delay_us(30);
    Set_Pin_Input(DHT11_PORT, DHT11_PIN);
}

uint8_t DHT11_Check_Response(void)
{
    uint8_t response = 0;
    delay_us(40);
    if (!(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)))
    {
        delay_us(80);
        if ((HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN))) response = 1;
        else response = 0;
    }
    while ((HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN))); // wait for pin to go low
    return response;
}

uint8_t DHT11_Read(void)
{
    uint8_t i, j;
    for (j = 0; j < 8; j++)
    {
        while (!(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN))); // wait for pin high
        delay_us(40);
        if (!(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN))) // if low after 40us => bit '0'
            i &= ~(1 << (7 - j));
        else
            i |= (1 << (7 - j));
        while ((HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN))); // wait for pin low
    }
    return i;
}

// ===================== Main Function =====================
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();

    uint8_t Rh_byte1, Rh_byte2, Temp_byte1, Temp_byte2;
    uint16_t SUM, RH, TEMP;
    float Temperature = 0, Humidity = 0;

    printf("DHT11 + UART test started...\r\n");

    while (1)
    {
        DHT11_Start();
        if (DHT11_Check_Response())
        {
            Rh_byte1 = DHT11_Read();
            Rh_byte2 = DHT11_Read();
            Temp_byte1 = DHT11_Read();
            Temp_byte2 = DHT11_Read();
            SUM = DHT11_Read();

            TEMP = Temp_byte1;
            RH = Rh_byte1;

            if (SUM == (Rh_byte1 + Rh_byte2 + Temp_byte1 + Temp_byte2))
            {
                Temperature = (float)TEMP;
                Humidity = (float)RH;
                printf("Temperature = %.1f °C, Humidity = %.1f %%\r\n", Temperature, Humidity);
            }
            else
            {
                printf("Checksum Error!\r\n");
            }
        }
        else
        {
            printf("No response from DHT11\r\n");
        }
        HAL_Delay(2000); // Read every 2 seconds
    }
}

// ===================== USART2 Init =====================
static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 9600;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
}

// ===================== GPIO Init =====================
static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
}

// ===================== Clock Config =====================
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 100;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

// ===================== Error Handler =====================
void Error_Handler(void)
{
    while (1)
    {
    }
}
