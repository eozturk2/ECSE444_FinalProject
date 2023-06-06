/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#define ARM_MATH_CM4
#define WINDOW_SIZE 1
#include <stdio.h>
#include "arm_math.h"
#include "ai_datatypes_defines.h"
#include "network_data_params.h"
#include "network_data.h"
#include "network.h"
#include "stm32l4s5i_iot01_accelero.h"
#include "stm32l4s5i_iot01_qspi.h"
#define FLASH_WRITE

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CRC_HandleTypeDef hcrc;

I2C_HandleTypeDef hi2c2;

OSPI_HandleTypeDef hospi1;

TIM_HandleTypeDef htim17;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

// Pointer to the network (very unfortunate macro name!)
static ai_handle network = AI_HANDLE_NULL;

// Scratchpad buffer
AI_ALIGNED(32) static ai_u8 activations[AI_NETWORK_DATA_ACTIVATIONS_SIZE];

// Point network metadata structs towards our buffers
static ai_buffer *ai_input;
static ai_buffer *ai_output;

// Conversion from network prediction to human-readable output
enum Mode {INFER, SAMPLE};
volatile enum Mode mode = INFER;
static char output_labels[14][30] = {"Using telephone", "Standing up from chair", "Lying down on bed", "Sitting on chair", "Pouring water",
									 "Walking", "Drinking from a glass", "Descending stairs", "Climbing stairs", "Eating with fork and knife",
								     "Getting up from bed", "Drinking soup", "Combing hair", "Brushing teeth"};


int8_t accel[3];
float x_in, x_out, x_state[WINDOW_SIZE];
float y_in, y_out, y_state[WINDOW_SIZE];
float z_in, z_out, z_state[WINDOW_SIZE];
volatile int array_idx = 0;
volatile uint8_t array_ready = 0;
volatile uint8_t xRounded, yRounded, zRounded;

uint8_t current_Data[288];
uint8_t copied_Data[288];
int i = 0;
float multiplier = 1/47.619048;
uint8_t x_Current;
uint8_t y_Current;
uint8_t z_Current;

// Have a simple moving average for FIR coefficients now,
// maybe elaborate later if needed
float w = 1 / (float)WINDOW_SIZE;
float coeff[WINDOW_SIZE] = {1};

arm_fir_instance_f32 x_instance, y_instance, z_instance;


uint32_t current_movement = -1;
uint32_t previous_movement = -1;
float ratio = (63.0f/3000.0f);

uint32_t movement_started=0;
uint32_t current_movement_duration=0;
uint8_t movement_changed=0;

float xCurrent, yCurrent, zCurrent;
float xRaw, yRaw, zRaw;

// The first byte on QSPI flash is reserved to store
// the value of this pointer
uint32_t qspi_flash_ptr = 0x01;
uint8_t sector_buffer[4096] = "0";
uint16_t sector_buffer_idx = 0;
uint32_t current_sector_addr = 0;
uint32_t current_sector_idx = 0;

uint8_t current_block = 1;
uint8_t num_samples;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CRC_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM17_Init(void);
static void MX_I2C2_Init(void);
static void MX_OCTOSPI1_Init(void);
/* USER CODE BEGIN PFP */

/* QSPI Flash functions ------------------------------------------------------*/

/*
 * @brief This function writes an array to flash memory and reads it back to verify. The data is
 * preceded by its user-given label.
 * @param write_array: Array of accelerometer data to write to memory
 * @param write_array_size: Size of array to write to memory
 * @param label: User-defined classification label of write_array
 * @retval None
 */
void flashWrite(uint8_t* write_array, int write_array_size, uint8_t label)
{
	// For this demo, only use the start addresses of blocks. In an actual application,
	// this wasteful method would be replaced by a struct that buffers multiple sample
	// arrays and autosaves them at certain intervals.
	uint32_t current_addr = (num_samples+1) * 64 * 1024;

	// Erase the current block first
	if(BSP_QSPI_Erase_Block(current_addr) != QSPI_OK) {
			Error_Handler();
	}

	// Write the label byte to the current address
	if(BSP_QSPI_Write(&label, current_addr, 1) != QSPI_OK) {
			Error_Handler();
	}

	// Read back the label byte for verification
	uint8_t label_read;
	if(BSP_QSPI_Read(&label_read, current_addr, 1)!= QSPI_OK) {
			Error_Handler();
	}

	// Increment the current write address by 1 to account for the label byte.
	current_addr += 1;

	// Write the data array to current write address
	if(BSP_QSPI_Write(write_array, current_addr, 288) != QSPI_OK) {
			Error_Handler();
	}

	// Read the data array from the current address and check for errors.
	uint8_t arr_cp[288];
	if(BSP_QSPI_Read(arr_cp, current_addr, 288)!= QSPI_OK) {
			Error_Handler();
	}

	// Increment the current block number.
	++current_block;

	// Erase the first block and check for errors.
	if(BSP_QSPI_Erase_Block(0x00) != QSPI_OK) {
			Error_Handler();
	}

	// Increment the number of samples.
	++num_samples;

	// Write the updated number of samples to the beginning of the flash memory and check for errors.
	if(BSP_QSPI_Write(&num_samples, 0x00, sizeof(num_samples)) != QSPI_OK) {
			Error_Handler();
	}
}

/*
 * @brief This function reads the contents of flash memory and outputs them over UART.
 * @param None
 * @retval None
 */
void flashRead()
{
	// Loop through each saved sample
	for (int i=1; i<=num_samples; ++i)
	{
		// Calculate the address of the current sample
		uint32_t addr = i * 64 * 1024;

		// Read the label of the current sample
		uint8_t current_label;
		if(BSP_QSPI_Read(&current_label, addr, sizeof(current_label))!= QSPI_OK) Error_Handler();

		// Increment the address to read the sample array
		addr += sizeof(current_label);

		// Read the sample array
		uint8_t arr[288];
		if(BSP_QSPI_Read(arr, addr, sizeof(arr))!= QSPI_OK) Error_Handler();

		// Print the label of the current sample
		char task_dsc[40] = "";
		sprintf(task_dsc, "\r\n%s\r\n", output_labels[current_label-1]);
		HAL_UART_Transmit(&huart1, task_dsc, sizeof(task_dsc), HAL_MAX_DELAY);

		// Print each line of the sample array
		for (int j=0; j<288/3; ++j)
		{
			char current_line[20] = "";
			sprintf(current_line, "\r\n%d, %d, %d", arr[j*3], arr[j*3+1],arr[j*3+2]);
			HAL_UART_Transmit(&huart1, current_line, sizeof(current_line), HAL_MAX_DELAY);
		}

		// Print a newline character to separate samples
		char newline[2] = "\r\n";
		HAL_UART_Transmit(&huart1, newline, sizeof(newline), HAL_MAX_DELAY);
	}
}

/* X-CUBE-AI functions ------------------------------------------------------*/

/**
 * Initializes the AI by creating and initializing the model, and receiving pointers
 * to the model's input/output tensors.
 *
 * @return 0 if initialization is successful, 1 if an error occurs.
 */
int aiInit(void)
{
  ai_error err;

  // Create and initialize the model
  const ai_handle acts[] = { activations };
  err = ai_network_create_and_init(&network, acts, NULL);
  if (err.type != AI_ERROR_NONE) {return 1;}

  // Receive pointers to the model's input/output tensors
  ai_input = ai_network_inputs_get(network, NULL);
  ai_output = ai_network_outputs_get(network, NULL);

  return 0;
}

/**
 * Runs the AI by updating IO handlers with the data payload and performing the inference.
 *
 * @param in_data Pointer to the input data.
 * @param out_data Pointer to the output data.
 * @return 0 if the inference is successful, -1 if an error occurs.
 */
int aiRun(const void *in_data, void *out_data)
{
  ai_i32 n_batch;

  /* 1 - Update IO handlers with the data payload */
  ai_input[0].data = AI_HANDLE_PTR(in_data);
  ai_output[0].data = AI_HANDLE_PTR(out_data);

  /* 2 - Perform the inference */
  n_batch = ai_network_run(network, &ai_input[0], &ai_output[0]);
  if (n_batch != 1) {return -1;}

  return 0;
}

/* UART UI functions ------------------------------------------------------*/

/**
 * Formats the output based on the result of the classification, time taken, and whether
 * the movement has changed or not.
 *
 * @param classification_result The result of the classification.
 * @param time_ms The time taken in milliseconds.
 * @param movement_changed Flag indicating whether the movement has changed or not.
 * @return 0 if the output is successfully formatted.
 */
int formatOutputFromResult(uint32_t classification_result, uint32_t time_ms, uint8_t movement_changed)
{
	char output_string[70] = "";
	char interbuf[70] = "";

	if(movement_changed)
	{
		// Output the classification result
		sprintf(output_string, "%s", output_labels[classification_result]);
	}
	else
	{
		// Output the classification result and the time taken
		sprintf(output_string, "%s for %d seconds", output_labels[classification_result], time_ms/1000);

		// Clear the line before outputting the result
		char clear_line[] = "\x1B[2K\r"; // escape code to clear line
		HAL_UART_Transmit(&huart1, (uint8_t *)clear_line, strlen(clear_line), HAL_MAX_DELAY);

		// Copy the classification result to a buffer
		strcpy(interbuf, output_labels[classification_result]);
	}

	// Transmit the output string via UART
	HAL_UART_Transmit(&huart1, output_string, sizeof(output_string), HAL_MAX_DELAY);

	return 0;
}


/**
 * Prompts the user to input a task number, displays a list of available tasks,
 * and waits for the user's input via UART.
 *
 * @return The task number entered by the user.
 */
uint8_t getUserInput(void)
{
	// Initialize variables
	char prompt[40] = "", first_task[40] = "", end_prompt[40]="";
	char response[2] = "";
	int i;

	// Output prompt and first task option
	sprintf(prompt, "\r\nEnter the task number as follows:\r\n");
	HAL_UART_Transmit(&huart1, prompt, sizeof(prompt), HAL_MAX_DELAY);
	sprintf(first_task, "%02d - %s\r\n", 0, "TO TRANSMIT THROUGH UART");
	HAL_UART_Transmit(&huart1, first_task, sizeof(first_task), HAL_MAX_DELAY);

	// Output the rest of the task options
	for (i = 0; i < 14; ++i)
	{
		char task[40] = "";
		sprintf(task, "%02d - %s\r\n", i + 1, output_labels[i]);
		HAL_UART_Transmit(&huart1, task, sizeof(task), HAL_MAX_DELAY);
	}

	//Output final option
	sprintf(end_prompt, "%02d - %s\r\n", 15, "TO ERASE FLASH");
	HAL_UART_Transmit(&huart1, end_prompt, sizeof(end_prompt), HAL_MAX_DELAY);

	// Wait for the user's input via UART
	HAL_UART_Receive(&huart1, response, sizeof(response), HAL_MAX_DELAY);

	// Transmit the user's input via UART
	HAL_UART_Transmit(&huart1, response, sizeof(response), HAL_MAX_DELAY);

	// Convert the user's input to an integer and return it
	return atoi(response);
}


/*
 * @brief This function sets the number of samples to zero, so we can overwrite old samples
 * @param None
 * @retval None
 */
void resetFlash()
{
	char info[20], ack[20];

	// Inform erase procedure
	sprintf(info, "\r\nERASING");
	HAL_UART_Transmit(&huart1, info, sizeof(info), HAL_MAX_DELAY);

	// Reset number of samples to zero
	num_samples = 0;

	// Write 0 to address of sample count
	if(BSP_QSPI_Write(&num_samples, 0x00, sizeof(num_samples)) != QSPI_OK) {
			Error_Handler();
	}

	// Acknowledge erase
	sprintf(ack, "\r\nERASED\r\n");
	HAL_UART_Transmit(&huart1, ack, sizeof(ack), HAL_MAX_DELAY);
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  arm_fir_init_f32(&x_instance, WINDOW_SIZE, coeff, x_state, 1);
  arm_fir_init_f32(&y_instance, WINDOW_SIZE, coeff, y_state, 1);
  arm_fir_init_f32(&z_instance, WINDOW_SIZE, coeff, z_state, 1);

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CRC_Init();
  MX_USART1_UART_Init();
  MX_TIM17_Init();
  MX_I2C2_Init();
  MX_OCTOSPI1_Init();
  /* USER CODE BEGIN 2 */

  // Initialize different drivers
  aiInit();
  BSP_ACCELERO_Init();
  BSP_QSPI_Init();

  // On startup, read from the first block to know where the processor left off
  if(BSP_QSPI_Read(&num_samples, 0x00, sizeof(num_samples))!= QSPI_OK) Error_Handler();

  // Start accelerometer sampling timer
  HAL_TIM_Base_Start_IT(&htim17);

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	// If the timer interrupt signals that the array is full, stop the sampling timer. Copy the
	// current samples into a buffer, give permission to the interrupt to restart sampling, and
	// start the sampling timer.
    if (array_ready)
    {
    	HAL_TIM_Base_Stop_IT(&htim17);
    	memcpy(copied_Data, current_Data, 288);
    	array_idx = 0;
    	array_ready = 0;
    	HAL_TIM_Base_Start_IT(&htim17);
    }

    if (mode == SAMPLE)
    {
    	// Prompt the user to enter what kind of motion they were doing. If they enter 0, then
    	// print the contents of flash memory to get extra training data
    	uint8_t userVal = getUserInput();
    	if (userVal == 0)
    	{
    		flashRead();
    	}
    	else if (userVal == 15)
    	{
    		resetFlash();
    	}
    	else
    	{
    		// If a nonzero value is entered in the prompt, save the current buffer along with
    		// the given label.
    	char ack[70] = "";
    	sprintf(ack, "\r\nSaving last 3 seconds with label: %02d\r\n", userVal);
			HAL_UART_Transmit(&huart1, ack, sizeof(ack), HAL_MAX_DELAY);
			flashWrite(copied_Data, 288, userVal);
			char fin[40] = "";
			sprintf(fin, "\r\nDone!\n");
			HAL_UART_Transmit(&huart1, fin, sizeof(fin), HAL_MAX_DELAY);
			mode = INFER;
      }
    }
    else
    {
    	// Do inference
    	float out_data[14];
    	aiRun(copied_Data, out_data);

    	// Get the index of the maximum of the final activation layer
    	arm_max_f32(out_data, 14, NULL, &current_movement);

    	// Keep track of how long the user has made the last motion. If the motion has changed,
    	// reset it.
    	uint8_t movement_changed = 0;
    	if (current_movement == previous_movement)
    	{
    		current_movement_duration = (HAL_GetTick() - movement_started);
    		movement_changed = 0;
    	}
    	else
    	{
    		previous_movement = current_movement;
    		movement_started = HAL_GetTick();
    		movement_changed = 1;
    	}

    	// Pass the classification result and movement timer to the UART UI function
    	formatOutputFromResult(current_movement, current_movement_duration, movement_changed);
    }
  }
  /* USER CODE END 3 */
}


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 60;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
  hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
  hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
  hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
  hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = 0x307075B1;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief OCTOSPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_OCTOSPI1_Init(void)
{

  /* USER CODE BEGIN OCTOSPI1_Init 0 */

  /* USER CODE END OCTOSPI1_Init 0 */

  OSPIM_CfgTypeDef OSPIM_Cfg_Struct = {0};

  /* USER CODE BEGIN OCTOSPI1_Init 1 */

  /* USER CODE END OCTOSPI1_Init 1 */
  /* OCTOSPI1 parameter configuration*/
  hospi1.Instance = OCTOSPI1;
  hospi1.Init.FifoThreshold = 1;
  hospi1.Init.DualQuad = HAL_OSPI_DUALQUAD_DISABLE;
  hospi1.Init.MemoryType = HAL_OSPI_MEMTYPE_MACRONIX;
  hospi1.Init.DeviceSize = 32;
  hospi1.Init.ChipSelectHighTime = 1;
  hospi1.Init.FreeRunningClock = HAL_OSPI_FREERUNCLK_DISABLE;
  hospi1.Init.ClockMode = HAL_OSPI_CLOCK_MODE_0;
  hospi1.Init.ClockPrescaler = 1;
  hospi1.Init.SampleShifting = HAL_OSPI_SAMPLE_SHIFTING_NONE;
  hospi1.Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_DISABLE;
  hospi1.Init.ChipSelectBoundary = 0;
  hospi1.Init.DelayBlockBypass = HAL_OSPI_DELAY_BLOCK_BYPASSED;
  if (HAL_OSPI_Init(&hospi1) != HAL_OK)
  {
    Error_Handler();
  }
  OSPIM_Cfg_Struct.ClkPort = 1;
  OSPIM_Cfg_Struct.NCSPort = 1;
  OSPIM_Cfg_Struct.IOLowPort = HAL_OSPIM_IOPORT_1_LOW;
  if (HAL_OSPIM_Config(&hospi1, &OSPIM_Cfg_Struct, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN OCTOSPI1_Init 2 */

  /* USER CODE END OCTOSPI1_Init 2 */

}

/**
  * @brief TIM17 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM17_Init(void)
{

  /* USER CODE BEGIN TIM17_Init 0 */

  /* USER CODE END TIM17_Init 0 */

  /* USER CODE BEGIN TIM17_Init 1 */

  /* USER CODE END TIM17_Init 1 */
  htim17.Instance = TIM17;
  htim17.Init.Prescaler = 120;
  htim17.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim17.Init.Period = 3124;
  htim17.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim17.Init.RepetitionCounter = 0;
  htim17.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim17) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM17_Init 2 */

  /* USER CODE END TIM17_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/*Configure GPIO pin : PBUTTON_Pin */
/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */
	if (htim->Instance == TIM17) {
		int16_t temp_buffer[3];

		// Get raw accelerometer data and scale it according
		// to the formula given in the ADL dataset
		BSP_ACCELERO_AccGetXYZ(temp_buffer);
		xRaw = ratio * (temp_buffer[0] + 1500.0f);
		yRaw = ratio * (temp_buffer[1] + 1500.0f);
		zRaw = ratio * (temp_buffer[2] + 1500.0f);

		// Apply FIR filter (maybe customize the filter coeffs later?)
		arm_fir_f32(&x_instance, &xRaw, &xCurrent, 1);
		arm_fir_f32(&y_instance, &yRaw, &yCurrent, 1);
		arm_fir_f32(&z_instance, &zRaw, &zCurrent, 1);

		// Clip, round and cast the filtered accelerometer data to fit the
		// network's specified input range
		if(xCurrent > 63) xCurrent = 63;
		if(xCurrent < 0) xCurrent = 0;
		if(yCurrent > 63) yCurrent = 63;
		if(yCurrent < 0) yCurrent = 0;
		if(zCurrent > 63) zCurrent = 63;
		if(zCurrent < 0) zCurrent = 0;
		uint8_t xRounded = round(xCurrent);
		uint8_t yRounded = round(yCurrent);
		uint8_t zRounded = round(zCurrent);

		// If the array is not yet full, write top it and increment the array
		// pointer by 3. If it is full, flag that the array is ready and wait
		// for write permission from the main loop (i.e. wait until array_idx
		// is cleared.
		if(array_idx <= 285)
		{
			array_ready = 0;
			current_Data[array_idx] = xRounded;
			current_Data[array_idx+1] = yRounded;
			current_Data[array_idx+2] = zRounded;
			array_idx += 3;
		}else{
			array_ready = 1;
		}
	  }

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin == GPIO_PIN_13)
	{
		mode = !mode;
	}
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
