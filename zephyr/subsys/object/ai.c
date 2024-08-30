/**
 * @file
 * @brief BACnet Analog Input Objects
 * @date 2022
 * @author Legrand North America, LLC.
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include "object.h"
#include "bacnet/basic/object/ai.h"

OBJECT_FUNCTIONS_WITHOUT_INIT(Analog_Input, ANALOG_INPUT_DESCR);

void Analog_Input_Init(void)
{
    if (!Object_List) {
        Object_List = Keylist_Create();

#if defined(INTRINSIC_REPORTING)
        /* Set handler for GetEventInformation function */
        handler_get_event_information_set(
            OBJECT_ANALOG_INPUT, Analog_Input_Event_Information);
        /* Set handler for AcknowledgeAlarm function */
        handler_alarm_ack_set(OBJECT_ANALOG_INPUT, Analog_Input_Alarm_Ack);
        /* Set handler for GetAlarmSummary Service */
        handler_get_alarm_summary_set(
            OBJECT_ANALOG_INPUT, Analog_Input_Alarm_Summary);
#endif
    }
}
