/**
 * @file
 * @brief BACnet Network Port Objects
 * @date 2022
 * @author Legrand North America, LLC.
 * @copyright SPDX-License-Identifier: Apache-2.0
 */
#include "object.h"
#include "bacnet/readrange.h"
#include "bacnet/basic/object/netport.h"

OBJECT_FUNCTIONS(Network_Port, struct netport_object_data);

bool Network_Port_Object_Instance_Number_Set(
    unsigned index, uint32_t object_instance)
{
    struct netport_object_data *descr;

    uint32_t old_inst = Network_Port_Index_To_Instance(index);
    descr = Keylist_Data_Delete(Object_List, old_inst);
    if (descr == NULL) {
        return false;
    }

    descr->Instance_Number = object_instance;
    return Keylist_Data_Add(Object_List, object_instance, descr) >= 0;
}
