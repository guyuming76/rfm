#include <cmark.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int debug = 0;
char **HeadersToMatch = NULL;
int HeadersToMatch_Count = 0;
int heading_level_to_match = -1;
int current_heading_level = 0;
int current_matching_level = 0; // Which header in the path we're currently matching
int inside_matching_path = 0; // Are we inside a potential matching path
int currently_in_matched_header = 0;
int current_heading_finished_testing_HeadersToMatch = 0;
int currently_matched_header_finished_printing = 0;

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
    if (nodetype == CMARK_NODE_HEADING) {
        current_heading_level = cmark_node_get_heading_level(node);
	if (heading_level_to_match == -1) heading_level_to_match = current_heading_level;
	else if (heading_level_to_match > current_heading_level) { // 如果遇到的标题级别比第一次遇到的级别还高， 报错退出
	  printf("Error: we suppose the first header node we get from file has the highest level, but we get level %d before %d here\n", heading_level_to_match, current_heading_level);
	  exit(1);
	}

	if (ev_type == CMARK_EVENT_ENTER) {
	    // If we encounter same or higher level heading while in matched content, stop printing
	    if (currently_in_matched_header && current_heading_level <= heading_level_to_match) {
		currently_in_matched_header = 0;
		currently_matched_header_finished_printing = 1;
	    }
	    // Reset matching state for new heading
	    current_heading_finished_testing_HeadersToMatch = 0;
	    if (current_heading_level <= heading_level_to_match) {
		// Reset path matching if we're back to top level or higher
		current_matching_level = 0;
		inside_matching_path = 0;
	    }
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

	if (nodetype == CMARK_NODE_TEXT && !current_heading_finished_testing_HeadersToMatch) {
	    if (current_matching_level == 0) {
		// First header in path - set the heading level to match
		if (strcmp(literal, HeadersToMatch[0]) == 0) {
		    heading_level_to_match = current_heading_level;
		    current_matching_level = 1;
		    inside_matching_path = 1;
		}
	    } else if (inside_matching_path) {
		// Subsequent headers in path
		if (current_heading_level > heading_level_to_match) {
		    // Child header - check if matches next in path
		    if (current_matching_level < HeadersToMatch_Count && 
			strcmp(literal, HeadersToMatch[current_matching_level]) == 0) {
			current_matching_level++;
		    }
		} else if (current_heading_level <= heading_level_to_match) {
		    // Same or higher level - reset matching
		    inside_matching_path = 0;
		    current_matching_level = 0;
		}
	    }

	    // If we've matched all headers in path
	    if (current_matching_level == HeadersToMatch_Count) {
		currently_in_matched_header = 1;
		currently_matched_header_finished_printing = 0;
		printf("\n");
		for (int i = 0; i < HeadersToMatch_Count; i++) {
		    printf("%s%s", (i > 0) ? " > " : "", HeadersToMatch[i]);
		}
		printf("=");
	    }
	    current_heading_finished_testing_HeadersToMatch = 1;
	}else if (currently_in_matched_header) {
	    if (nodetype == CMARK_NODE_TEXT || nodetype == CMARK_NODE_CODE || nodetype == CMARK_NODE_PARAGRAPH) {
	        printf("%s", literal);
	    } else if (nodetype == CMARK_NODE_LINEBREAK || nodetype == CMARK_NODE_SOFTBREAK) {
	        printf("\n");
	    } else if (nodetype == CMARK_NODE_CODE_BLOCK) {
	        const char* info = cmark_node_get_fence_info(node);
	        printf("\n```%s\n%s\n```\n", info ? info : "", literal);
	    }
	}
    }
}


int main(int argc, char *argv[]){
    if (argc < 4) {
        printf("We parse a file specified by the MarkdownFilename argument with cmark, and output KeyValue pair lines. The HeaderToMatch arguments specify a path of headers to match, where each subsequent header must be a child of the previous one.\n\n");
        printf("Usage:   %s (0|1) MarkdownFilename HeaderToMatch1 [HeaderToMatch2 ...]\n", argv[0]);
	printf("         0 or 1 for the debug option, 1 for debug\n");
	printf("         HeaderToMatch arguments form a path of headers to match\n");
	printf("example: %s 0 your_markdown_file.md \"Section\" \"Subsection\" \"Details\"\n",argv[0]);
	return 1;
    }

    FILE *fp = fopen(argv[2], "rb");
    if (!fp) {
        fprintf(stderr, "Error opening file: %s\n", argv[2]);
        return 1;
    }
    cmark_node *root = cmark_parse_file(fp, CMARK_OPT_DEFAULT);
    fclose(fp); // Close the file after parsing

    debug = atoi(argv[1]);
    HeadersToMatch = argv + 3;
    HeadersToMatch_Count = argc - 3;

    // Remove the debug URL output
    cmark_event_type ev_type;
    cmark_iter *iter = cmark_iter_new(root);
    while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
        cmark_node *cur = cmark_iter_get_node(iter);
        print_node(cur,ev_type);
    }
    printf("\n");
    cmark_iter_free(iter);
    cmark_node_free(root); // Free the cmark node tree

    return 0;
}
