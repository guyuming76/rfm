#include <dlfcn.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

void* handle = NULL;

int main(int argc, char *argv[]){
	char** selectionList = NULL;
	char** (*fileChooser)(char**, bool, char*, bool, void(*)(char**));
	void (*fileChooserResultPrint)(char**);
    	handle = dlopen("librfm.so", RTLD_LAZY);
	fileChooser = (char** (*)(char**, bool, char*, bool, void(*)(char**)))dlsym(handle,"rfmFileChooser");
	fileChooserResultPrint = (void (*)(char**))dlsym(handle,"fileChooserCallback");
	fileChooserResultPrint(fileChooser(selectionList, 0, "locate rfm.c", 0, NULL));
        dlclose(handle);
}
