#include <gtk/gtk.h>

GList *rfmFileChooser(GList** fileSelectionStringList);

void Test_rfmFileChooser(){
  GList* selectionList=NULL;
  rfmFileChooser(&selectionList);
  while(selectionList!=NULL){
    printf("%s\n",selectionList->data);
    selectionList=g_list_next(selectionList);
  }
}
