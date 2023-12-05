/* this is for client which does not reference glib, my test is that async==TRUE don't work here, the callback never called, i guess this is because the client does not have a gmainloop*/
char**
rfmFileChooser(enum rfmTerminal startWithVirtualTerminal, char *search_cmd, gboolean async,
	       char *fileSelectionStringArray[],
               void (*fileChooserClientCallback)(char **));

/* this is for client which reference glib and hence glist*/
GList*
rfmFileChooser_glist(enum rfmTerminal startWithVirtualTerminal, char *search_cmd, gboolean async,
	       GList **fileSelectionStringList, 
               void (*fileChooserClientCallback)(char **));

/* hold default file selection before, and hold returned user selection in fileChooser after, so this will be freed during rfmFileChooser calling*/
static GList *fileChooserSelectionList = NULL;

void fileChooserCallback(char** fileSelectionArray){
  if (fileSelectionArray==NULL) {
    printf("fileChooser returned NULL.");
  }else {
    for(int i=0; fileSelectionArray[i]!=NULL; i++)
      printf("%s\n",fileSelectionArray[i]);
    free(fileSelectionArray); //content of fileSelectionArray, the char*, is also referenced by selectionList, so we just free the array, don't free full.
  }
}

static void Test_rfmFileChooser(){
	      GtkTreeIter iter;
	      GList *listElement;
	      GList *view_selection_list = get_view_selection_list(icon_or_tree_view,treeview,&treemodel);

	      if (fileChooserSelectionList!=NULL){
		g_list_free_full(fileChooserSelectionList, (GDestroyNotify)g_free);
		fileChooserSelectionList = NULL;
	      };

	      RFM_FileAttributes *fileAttributes;
	      if (view_selection_list!=NULL) {
		listElement=g_list_first(view_selection_list);
		while(listElement!=NULL) {
		  gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, listElement->data);
		  gtk_tree_model_get (GTK_TREE_MODEL(store), &iter, COL_ATTR, &fileAttributes, -1);
		  fileChooserSelectionList = g_list_prepend(fileChooserSelectionList, strdup(fileAttributes->path));
		  listElement=g_list_next(listElement);
		}
	      }
	      g_list_free_full(view_selection_list, (GDestroyNotify)gtk_tree_path_free);

	      rfmFileChooser_glist(FALSE, "locate rfm.c", TRUE, &fileChooserSelectionList, fileChooserCallback);
}
