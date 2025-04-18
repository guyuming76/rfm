#include <cmark.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int debug;

// Function to match a header title and extract its content
char* matchTitleAndExtractContent(const char* headerTitle, const char* markdownText) {
    cmark_node *root = cmark_parse_document(markdownText, strlen(markdownText), CMARK_OPT_DEFAULT);
    cmark_iter *iter = cmark_iter_new(root);
    
    int in_matched_section = 0;
    int target_level = -1;
    size_t start_pos = 0;
    size_t end_pos = SIZE_MAX;
    
    cmark_event_type ev_type;
    while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
        cmark_node *node = cmark_iter_get_node(iter);
        
        if (node && cmark_node_get_type(node) == CMARK_NODE_HEADING) {
            int current_level = cmark_node_get_heading_level(node);
            
            if (ev_type == CMARK_EVENT_ENTER) {
                if (!in_matched_section) {
                    // Check if this is our target header
                    cmark_node *child = cmark_node_first_child(node);
                    if (child && cmark_node_get_type(child) == CMARK_NODE_TEXT) {
                        const char* text = cmark_node_get_literal(child);
                        if (text && strcmp(text, headerTitle) == 0) {
                            in_matched_section = 1;
                            target_level = current_level;
                            start_pos = cmark_node_get_start_line(node);
			    if (debug) fprintf(stderr, "%s start at line %ld\n", text, start_pos);
                        }
                    }
                } else if (current_level <= target_level) {
                    // Found same or higher level heading - end of our section
                    in_matched_section = 0;
		    end_pos = cmark_node_get_start_line(node);
		    if (debug) fprintf(stderr, "%s end at line %ld\n",headerTitle,end_pos);
                }
            }
        }
    }
    
    cmark_iter_free(iter);
    cmark_node_free(root);

    if (start_pos == 0 || end_pos <= start_pos) {
        return NULL;
    }

    // Allocate and copy the substring
    char* result = calloc(strlen(markdownText),sizeof(char));

    char * oneline = strtok(markdownText, "\n");
    size_t lineNumber = 0;
    while (oneline && lineNumber < end_pos){
      if (lineNumber>start_pos) {
	result = strcat(result, oneline);
	result = strcat(result, "\n");
	if (debug) fprintf(stderr,"copy line %ld to return\n",lineNumber);
      }
      lineNumber ++;
      oneline = strtok(NULL, "\n");
    }
    return result;
}


int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Usage: %s (0|1) MarkdownFilename HeaderTitle1 [HeaderTitle2 ...]\n", argv[0]);
        printf("  0 or 1 for debug output\n");
        return 1;
    }

    debug = atoi(argv[1]);
    const char* filename = argv[2];
    
    // Read the entire file
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Error opening file: %s\n", filename);
        return 1;
    }
    
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char *content = malloc(fsize + 1);
    fread(content, 1, fsize, fp);
    content[fsize] = 0;
    fclose(fp);
    
    // Process each header title in sequence
    char *current_content = content;
    for (int i = 3; i < argc; i++) {
        if (debug) fprintf(stderr,"Matching header: %s\n", argv[i]);
        
        char *new_content = matchTitleAndExtractContent(argv[i], current_content);
        if (i > 3) free(current_content); // Free previous content if not the original
        current_content = new_content;
        
        if (!current_content || strlen(current_content) == 0) {
            fprintf(stderr, "Header not found: %s\n", argv[i]);
            free(current_content);
            free(content);
            return 1;
        }
    }
    
    // Output the final result
    printf("%s", current_content);
    
    // Cleanup
    if (argc > 3) free(current_content);
    free(content);
    
    return 0;
}
