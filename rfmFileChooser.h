
char** rfmFileChooser(char* fileSelectionStringArray[], gboolean startWithVirtualTerminal,char* search_cmd);
GList* rfmFileChooser_glist(GList** fileSelectionStringList, gboolean startWithVirtualTerminal,char* search_cmd);

static GList* selectionList=NULL;
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

      	      //selectionList = rfmFileChooser_glist(&selectionList,FALSE, NULL);
	      selectionList = rfmFileChooser_glist(&selectionList,FALSE, "locate rfm.c");
}
