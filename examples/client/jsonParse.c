//
// Created by impact on 18-7-5.
//

#include <liblwm2m.h>
#include <stdio.h>
#include <internals.h>
#include "sensorData.h"
#include "cJSON.h"
/*
example:

 *
 *
 *
 */

#define FAILURE   (0)
#define SUCCESS   (1)
#define INVALID_ID (-1)
#define IS_VALID_ID(x) (x != INVALID_ID)





ObjectData* convertJsonToObjectData(const char *const jsonData)
{
    if(jsonData == NULL) return NULL;

    static ObjectData objData;// static to avoid stackoverflow
    memset(&objData, 0, sizeof(objData));

    int status = SUCCESS;


    cJSON *root = cJSON_Parse(jsonData);
    if (root == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error parse json: %s\n", error_ptr);
        }
        return NULL;
    }

    cJSON* firstLevelChild;
    cJSON_ArrayForEach(firstLevelChild, root)
    {
        objData.objId = atoi(firstLevelChild->string);

        cJSON* secondLevelChild;
        cJSON_ArrayForEach(secondLevelChild, firstLevelChild)
        {
            if(objData.instNum >= MAX_INSTANCE_PER_OBJ)  goto end;

            InstanceData* instData = &objData.data[objData.instNum++];
            instData->instId = atoi(secondLevelChild->string);

            cJSON* thirdLevelChild;
            cJSON_ArrayForEach(thirdLevelChild, secondLevelChild)
            {
                appendResourceIdValue(thirdLevelChild->string, thirdLevelChild->valuestring, instData);
            }

        }

        break;//currenty support only 1 object in one msg
    }

end:
    cJSON_Delete(root);
    return (status == SUCCESS) ? &objData : NULL;
}




