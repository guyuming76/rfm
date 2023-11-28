#include <dlfcn.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

void* handle = NULL;

static void fileChooserCallback(char** fileSelectionArray){
  for(int i=0; fileSelectionArray[i]!=NULL; i++)
    printf("%s\n",fileSelectionArray[i]);
  free(fileSelectionArray);
  dlclose(handle);
}

int main(int argc, char *argv[]){
	char** selectionList;
	void (*fileChooser)(char**, bool, char*, void(*)(char**));

    	handle = dlopen("librfm.so", RTLD_LAZY);
	fileChooser = (void (*)(char**, bool, char* ,void(*)(char**)))dlsym(handle,"rfmFileChooser");

	fileChooser(argv, 0, "locate rfm.c", fileChooserCallback);
}
