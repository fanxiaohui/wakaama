#include <errno.h>
#include <stdio.h>
#include <string.h>

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
