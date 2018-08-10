#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "sensorData.h"

#define FAIL       0
#define SUCCESS    1

int writeFile(const char* filename, const char* data) {

	FILE* dstfile = fopen(filename, "w+");//override
	if (dstfile) {
		int result = fputs(data, dstfile);
		if (result != EOF) {

			fflush(dstfile);
			fclose(dstfile);
			fprintf(stdout, "write file %s ok, data=%s \n", filename, data);

			return SUCCESS;
		}
		else {
			fprintf(stdout, "write file %s error=%d, reason=%s \n", filename, errno,strerror(errno));
		fclose(dstfile);
		}

	}
	else{
		fprintf(stdout, "open file %s error=%d, reason=%s \n", filename, errno,strerror(errno));
	}
	return FAIL;
}


char* encode_json(const int objId, const int instId, const ResourceValue* rv, const char* mode)//mode=R(read), W(write)
{
	//eg: {"10255":{"0":{"5524":"600","M":"W"}}}
	static char buff[200];
	memset(buff, 0, sizeof(buff));

	fprintf(stdout,"before encode:%d/%d/%d : %s \n",objId, instId, rv->resId,rv->value);

	int len = 0;
	len += sprintf(buff+len,"{\"%d\":{\"%d\":{\"%d\":\"%s\",\"M\":\"%s\"}}}", objId, instId, rv->resId, rv->value,mode);
	buff[len] = 0;
	fprintf(stdout,"after encode:%s \n",buff);
	return buff;
}
