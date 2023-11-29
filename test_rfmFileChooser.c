#include <dlfcn.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

void* handle = NULL;

void fileChooserCallback(char** fileSelectionArray){
  if (fileSelectionArray==NULL){
    printf("fileChooser return NULL");
  } else {
    for(int i=0; fileSelectionArray[i]!=NULL; i++)
      printf("%s\n",fileSelectionArray[i]);
    free(fileSelectionArray);
  }
  dlclose(handle);
}

int main(int argc, char *argv[]){
	char** selectionList = NULL;
	char** (*fileChooser)(char**, bool, char*, bool, void(*)(char**));

    	handle = dlopen("librfm.so", RTLD_LAZY);
	fileChooser = (char** (*)(char**, bool, char*, bool, void(*)(char**)))dlsym(handle,"rfmFileChooser");

	fileChooserCallback(fileChooser(selectionList, 0, "locate rfm.c", 0, fileChooserCallback));
}
