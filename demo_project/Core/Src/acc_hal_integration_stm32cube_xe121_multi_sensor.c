// Copyright (c) Acconeer AB, 2022-2023
// All rights reserved
// This file is subject to the terms and conditions defined in the file
// 'LICENSES/license_acconeer.txt', (BSD 3-Clause License) which is part
// of this source code package.

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "acc_definitions_common.h"
#include "acc_hal_definitions_a121.h"
#include "acc_hal_integration_a121.h"
#include "acc_integration.h"
#include "acc_integration_log.h"

#include "spi.h"
#include "main.h"


# define A121_SPI_HANDLE hspi1
/* spi handle */
extern SPI_HandleTypeDef A121_SPI_HANDLE;

#define SENSOR_COUNT                (5)        /**< @brief The number of sensors available on the board */
#define STM32_SPI_MAX_TRANSFER_SIZE (65535)    /**< @brief The maximum SPI transfer size for the STM32 */


typedef struct
{
	GPIO_TypeDef *en_port;
	uint32_t     en_pin_mask;
	GPIO_TypeDef *int_port;
	uint32_t     int_pin_mask;
} acc_sensor_info_t;

const acc_sensor_info_t sensor_infos[SENSOR_COUNT] = {
	{SEN_EN1_GPIO_Port, SEN_EN1_Pin, SEN_INT1_GPIO_Port, SEN_INT1_Pin},
	{SEN_EN2_GPIO_Port, SEN_EN2_Pin, SEN_INT2_GPIO_Port, SEN_INT2_Pin},
//	{SEN_EN3_GPIO_Port, SEN_EN3_Pin, SEN_INT3_GPIO_Port, SEN_INT3_Pin},
//	{SEN_EN4_GPIO_Port, SEN_EN4_Pin, SEN_INT4_GPIO_Port, SEN_INT4_Pin},
//	{SEN_EN5_GPIO_Port, SEN_EN5_Pin, SEN_INT5_GPIO_Port, SEN_INT5_Pin},
};


static inline void disable_interrupts(void)
{
	__disable_irq();
}


static inline void enable_interrupts(void)
{
	__enable_irq();
	__ISB();
}


#ifdef STM32_USE_SPI_DMA
static volatile bool spi_transfer_complete;


void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *h_spi)
{
	(void)h_spi;
	spi_transfer_complete = true;
}


#endif

//----------------------------------------
// Implementation of RSS HAL handlers
//----------------------------------------


static void spi_select(acc_sensor_id_t sensor_id)
{
	GPIO_PinState sel0 = GPIO_PIN_RESET;
	GPIO_PinState sel1 = GPIO_PIN_RESET;
	GPIO_PinState sel2 = GPIO_PIN_RESET;

	switch (sensor_id)
	{
		case 1:
			break;
		case 2:
			sel0 = GPIO_PIN_SET;
			break;
		case 3:
			sel1 = GPIO_PIN_SET;
			break;
		case 4:
			sel0 = GPIO_PIN_SET;
			sel1 = GPIO_PIN_SET;
			break;
		case 5:
			sel2 = GPIO_PIN_SET;
			break;
		default:
			break;
	}

	HAL_GPIO_WritePin(SPI_SEL0_GPIO_Port, SPI_SEL0_Pin, sel0);
	HAL_GPIO_WritePin(SPI_SEL1_GPIO_Port, SPI_SEL1_Pin, sel1);
	HAL_GPIO_WritePin(SPI_SEL2_GPIO_Port, SPI_SEL2_Pin, sel2);
}


static void acc_hal_integration_sensor_transfer(acc_sensor_id_t sensor_id, uint8_t *buffer, size_t buffer_size)
{
	if ((sensor_id == 0) || (sensor_id > SENSOR_COUNT))
	{
		Error_Handler();
	}

	spi_select(sensor_id);

	const uint32_t SPI_TRANSMIT_RECEIVE_TIMEOUT = 5000;

	HAL_GPIO_WritePin(A121_SPI_SS_GPIO_Port, A121_SPI_SS_Pin, GPIO_PIN_RESET);

#ifdef STM32_USE_SPI_DMA
	spi_transfer_complete = false;
	HAL_StatusTypeDef status = HAL_SPI_TransmitReceive_DMA(&hspi1, spi_buffer, spi_buffer, buffer_size);

	if (status != HAL_OK)
	{
		Error_Handler();
	}

	uint32_t start = HAL_GetTick();

	while (!spi_transfer_complete && (HAL_GetTick() - start) < SPI_TRANSMIT_RECEIVE_TIMEOUT)
	{
		// Turn off interrupts
		disable_interrupts();
		// Check once more so that the interrupt have not occurred
		if (!spi_transfer_complete)
		{
			__WFI();
		}

		// Enable interrupt again, the ISR will execute directly after this
		enable_interrupts();
	}

#else
	HAL_SPI_TransmitReceive(&A121_SPI_HANDLE, buffer, buffer, buffer_size, SPI_TRANSMIT_RECEIVE_TIMEOUT);
#endif

	HAL_GPIO_WritePin(A121_SPI_SS_GPIO_Port, A121_SPI_SS_Pin, GPIO_PIN_SET);
}


void acc_hal_integration_sensor_supply_on(acc_sensor_id_t sensor_id)
{
	if ((sensor_id == 0) || (sensor_id > SENSOR_COUNT))
	{
		Error_Handler();
	}

	// There is no power supply control on the XE121
}


void acc_hal_integration_sensor_supply_off(acc_sensor_id_t sensor_id)
{
	if ((sensor_id == 0) || (sensor_id > SENSOR_COUNT))
	{
		Error_Handler();
	}

	// There is no power supply control on the XE121
}


void acc_hal_integration_sensor_enable(acc_sensor_id_t sensor_id)
{
	if ((sensor_id == 0) || (sensor_id > SENSOR_COUNT))
	{
		Error_Handler();
	}

	const acc_sensor_info_t *sensor_info = &sensor_infos[sensor_id - 1];

	HAL_GPIO_WritePin(sensor_info->en_port, sensor_info->en_pin_mask, GPIO_PIN_SET);

	// Wait 2 ms to make sure that the sensor crystal has time to stabilize
	acc_integration_sleep_us(2000);
}


void acc_hal_integration_sensor_disable(acc_sensor_id_t sensor_id)
{
	if ((sensor_id == 0) || (sensor_id > SENSOR_COUNT))
	{
		Error_Handler();
	}

	const acc_sensor_info_t *sensor_info = &sensor_infos[sensor_id - 1];

	HAL_GPIO_WritePin(sensor_info->en_port, sensor_info->en_pin_mask, GPIO_PIN_RESET);

	// Wait after disable to leave the sensor in a known state
	// in case the application intends to enable the sensor directly
	acc_integration_sleep_us(2000);
}


bool acc_hal_integration_wait_for_sensor_interrupt(acc_sensor_id_t sensor_id, uint32_t timeout_ms)
{
	bool status = false;

	if ((sensor_id == 0) || (sensor_id > SENSOR_COUNT))
	{
		Error_Handler();
	}

	const acc_sensor_info_t *sensor_info  = &sensor_infos[sensor_id - 1];
	GPIO_TypeDef            *int_port     = sensor_info->int_port;
	uint32_t                int_pin_mask  = sensor_info->int_pin_mask;
	const uint32_t          wait_begin_ms = HAL_GetTick();

	while ((HAL_GPIO_ReadPin(int_port, int_pin_mask) != GPIO_PIN_SET) &&
	       (HAL_GetTick() - wait_begin_ms < timeout_ms))
	{
		// Wait for the GPIO interrupt
		disable_interrupts();
		// Check again so that IRQ did not occur
		if (HAL_GPIO_ReadPin(int_port, int_pin_mask) != GPIO_PIN_SET)
		{
			__WFI();
		}

		// Enable interrupts again to allow pending interrupt to be handled
		enable_interrupts();
	}

	status = HAL_GPIO_ReadPin(int_port, int_pin_mask) == GPIO_PIN_SET;

	return status;
}


uint16_t acc_hal_integration_sensor_count(void)
{
	return SENSOR_COUNT;
}


const acc_hal_a121_t *acc_hal_rss_integration_get_implementation(void)
{
	static const acc_hal_a121_t val =
	{
		.max_spi_transfer_size = STM32_SPI_MAX_TRANSFER_SIZE,

		.mem_alloc = malloc,
		.mem_free  = free,

		.transfer = acc_hal_integration_sensor_transfer,
		.log      = acc_integration_log,

		.optimization.transfer16 = NULL,
	};

	return &val;
}
