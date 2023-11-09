#include <gtk/gtk.h>

GList *rfmFileChooser(GList** fileSelectionStringList);

static GList* selectionList=NULL;
void Test_rfmFileChooser(){
  rfmFileChooser(&selectionList);
  while(selectionList!=NULL){
    printf("%s\n",selectionList->data);
    selectionList=g_list_next(selectionList);
  }
}
