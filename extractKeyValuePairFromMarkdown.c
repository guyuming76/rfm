#include <cmark.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int debug=0;
char **HeadersToMatch = NULL;
int HeadersToMatch_Count = 0;
int heading_level_to_match = 1;//TODO: parameterize this
int current_heading_level = 0;
int currently_in_matched_header = 0;
int current_heading_finished_testing_HeadersToMatch = 0; //the first cmark_node_type_text is for header text, do not try for later text nodes
int currently_matched_header_finished_printing = 0; //we only print the first line of content for matched header

void print_event(cmark_event_type ev_type){
    switch (ev_type){
        case CMARK_EVENT_ENTER: printf("event:enter ");break;
        case CMARK_EVENT_EXIT:  printf("event:exit  ");break;
        case CMARK_EVENT_NONE:  printf("event:none  ");break;
    }
}

void print_node(cmark_node *node, cmark_event_type ev_type) {
    if (debug) print_event(ev_type);
    if (!node) return;

    cmark_node_type nodetype = cmark_node_get_type(node);
    if ( nodetype == CMARK_NODE_HEADING) {
        current_heading_level = cmark_node_get_heading_level(node);
	if (ev_type == CMARK_EVENT_ENTER) {
	  currently_in_matched_header = 0;
	  current_heading_finished_testing_HeadersToMatch = 0;
	}
        if (debug) printf("heading level:%d\n",current_heading_level);
    }    
    
    const char* typestring = cmark_node_get_type_string(node);
    if (typestring){
        if (debug) printf("typestring:%s\n",typestring);
        //free(typestring);
    }
    
    const char* literal = cmark_node_get_literal(node);
    if (literal){
        if (debug) printf("%d:literal:%s\n", current_heading_level, literal);
        //free(literal);

	if ( nodetype == CMARK_NODE_TEXT && current_heading_level == heading_level_to_match && !current_heading_finished_testing_HeadersToMatch){
	  if (HeadersToMatch_Count>0){
	    for (int i=0;i<HeadersToMatch_Count;i++){
	      if (strcmp(literal, HeadersToMatch[i])==0){
		currently_in_matched_header = 1;
		currently_matched_header_finished_printing = 0;
		printf("\n%s=",HeadersToMatch[i]);// gkeyfile use = to seperate key and value
	      }
	    }
	  }else{
	    currently_in_matched_header = 1;
	    currently_matched_header_finished_printing = 0;
	    printf("\n%s=",literal);// gkeyfile use = to seperate key and value
	  }
	  current_heading_finished_testing_HeadersToMatch = 1;
	}else if (currently_in_matched_header && !currently_matched_header_finished_printing){
	      printf("%s",literal);
	      currently_matched_header_finished_printing = 1;
	}
    }
}


int main(int argc, char *argv[]){
    if (argc < 3) {
        printf("We parse a file specified by the MarkdownFilename argument with cmark, and output KeyValue pair lines. The HeadersToMatch arguments specify the keys, which are the level %d header text in the markdown file. The first line of literal text for the matched header will be the value for the key\n\n", heading_level_to_match);
        printf("Usage:   %s debug MarkdownFilename HeadersToMatch\n", argv[0]);
	printf("         0 or 1 for the debug option, 1 for debug\n");
	printf("         HeadersToMatch is the list of markdown headers text to match, seperated with space\n");
	printf("         All headers of level 1 will be in output if no HeadersToMatch is specified\n");
	printf("example: %s 0 your_markdown_file.md Header_level1_text1 Header_level1_text2\n",argv[0]);
	return 1;
    }
  
    FILE *fp = fopen(argv[2], "rb");
    cmark_node *root = cmark_parse_file(fp, CMARK_OPT_DEFAULT);

    debug = atoi(argv[1]);
    HeadersToMatch = argv + 3;
    HeadersToMatch_Count = argc - 3;

    printf("[https://discourse.gnome.org/t/gkeyfile-to-handle-conf-without-groupname/23080/3]\n");
    cmark_event_type ev_type;
    cmark_iter *iter = cmark_iter_new(root);
    while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
        cmark_node *cur = cmark_iter_get_node(iter);
        print_node(cur,ev_type);
    }
    printf("\n");
    cmark_iter_free(iter);

    return 0;
}
