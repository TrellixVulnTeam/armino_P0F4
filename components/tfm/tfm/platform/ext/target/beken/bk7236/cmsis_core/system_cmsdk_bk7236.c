// Copyright 2022-2023 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "cmsis.h"

/*
 * BK7236 has different frequencies for system core clock (20MHz) and
 * peripherals clock (25MHz).
 */
#define  XTAL             (40000000UL)
#define  PERIPHERAL_XTAL  (50000000UL)

#define  SYSTEM_CLOCK     (XTAL/2)
#define  PERIPHERAL_CLOCK (PERIPHERAL_XTAL/2)

#if defined (__VTOR_PRESENT) && (__VTOR_PRESENT == 1U)
  extern uint32_t __Vectors;
#endif

uint32_t SystemCoreClock = SYSTEM_CLOCK;
uint32_t PeripheralClock = PERIPHERAL_CLOCK;

/* System Core Clock update function */
void SystemCoreClockUpdate (void)
{
  SystemCoreClock = SYSTEM_CLOCK;
}

/* System initialization function */
void SystemInit (void)
{
#if __DOMAIN_NS != 1U
#if defined (__VTOR_PRESENT) && (__VTOR_PRESENT == 1U)
  SCB->VTOR = (uint32_t) &__Vectors;
#endif
#ifdef UNALIGNED_SUPPORT_DISABLE
  SCB->CCR |= SCB_CCR_UNALIGN_TRP_Msk;
#endif
#endif /* __DOMAIN_NS != 1U */

  SystemCoreClock = SYSTEM_CLOCK;
  PeripheralClock = PERIPHERAL_CLOCK;
}
