//
// Created by lianzeng on 18-6-23.
//

#ifndef WAKAAMA_SHAREMEMDATA_H
#define WAKAAMA_SHAREMEMDATA_H

typedef  struct{
    float rpm;//engine speed
    float speed; //vehicle speed
    unsigned long timestamp;
    bool  updated;
    char  padding[7];
}ObdData;

#endif //WAKAAMA_SHAREMEMDATA_H
