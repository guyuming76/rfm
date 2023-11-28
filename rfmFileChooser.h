/* this is for client which does not reference glib*/
void
rfmFileChooser(char *fileSelectionStringArray[],
               gboolean startWithVirtualTerminal, char *search_cmd,
               void (*fileChooserClientCallback)(char **));

/* this is for client which reference glib and hence glist*/
void
rfmFileChooser_glist(
    GList **fileSelectionStringList, gboolean startWithVirtualTerminal,
    char *search_cmd,
    void (*fileChooserClientCallback)(char **));


static GList *selectionList = NULL;

static void fileChooserCallback(char** fileSelectionArray){
  for(int i=0; fileSelectionArray[i]!=NULL; i++)
    printf("%s\n",fileSelectionArray[i]);
  free(fileSelectionArray); //content of fileSelectionArray, the char*, is also referenced by selectionList, so we just free the array, don't free full.
}

static void Test_rfmFileChooser(){
	      GtkTreeIter iter;
	      GList *listElement;
	      GList *view_selection_list = get_view_selection_list(icon_or_tree_view,treeview,&treemodel);

	      if (selectionList!=NULL){
		g_list_free_full(selectionList, (GDestroyNotify)g_free);
		selectionList = NULL;
	      };

	      RFM_FileAttributes *fileAttributes;
	      if (view_selection_list!=NULL) {
		listElement=g_list_first(view_selection_list);
		while(listElement!=NULL) {
		  gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, listElement->data);
		  gtk_tree_model_get (GTK_TREE_MODEL(store), &iter, COL_ATTR, &fileAttributes, -1);
		  selectionList = g_list_prepend(selectionList, strdup(fileAttributes->path));
		  listElement=g_list_next(listElement);
		}
	      }
	      g_list_free_full(view_selection_list, (GDestroyNotify)gtk_tree_path_free);

      	      //rfmFileChooser_glist(&selectionList,FALSE, NULL,fileChooserCallback);
	      rfmFileChooser_glist(&selectionList,FALSE, "locate rfm.c",fileChooserCallback);
}
