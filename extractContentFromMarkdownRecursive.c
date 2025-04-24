#define _POSIX_C_SOURCE 200809L
#include <cmark.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int debug;

// Function to match a header title and extract its content
char* matchTitleAndExtractContent(const char* headerTitle, const char* markdownText) {
    // Make a copy of the original text to avoid modification
    char* textCopy = strdup(markdownText);
    cmark_node *root = cmark_parse_document(textCopy, strlen(textCopy), CMARK_OPT_DEFAULT);
    
    size_t start_pos = 0;
    size_t end_pos = 0;
    int target_level = 0;
    int in_section = 0;

    // First pass to find the section boundaries
    cmark_iter *iter = cmark_iter_new(root);
    cmark_event_type ev_type;
    while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
        cmark_node *node = cmark_iter_get_node(iter);
        
        if (node && cmark_node_get_type(node) == CMARK_NODE_HEADING) {
            int level = cmark_node_get_heading_level(node);
            
            if (ev_type == CMARK_EVENT_ENTER) {
                if (!in_section) {
                    // Check if this is our target header
                    cmark_node *child = cmark_node_first_child(node);
                    if (child && cmark_node_get_type(child) == CMARK_NODE_TEXT) {
                        const char* text = cmark_node_get_literal(child);
                        if (text && strcmp(text, headerTitle) == 0) {
                            in_section = 1;
                            target_level = level;
                            start_pos = cmark_node_get_start_line(node);
                            if (debug) fprintf(stderr, "%s start at line %ld\n", text, start_pos);
                        }
                    }
                } else if (level <= target_level) {
                    // Found same or higher level heading - end of our section
                    end_pos = cmark_node_get_start_line(node);
                    if (debug) fprintf(stderr, "%s end at line %ld\n", headerTitle, end_pos);
                    break;
                }
            }
        }
    }
    cmark_iter_free(iter);
    cmark_node_free(root);
    free(textCopy);

    if (!start_pos) return NULL;
    //TODO: 我让AI写代码的时候，AI曾使用了 cmark_node_get_start_offset 这么一个不存在的函数，搜索会找到 这个帖子：https://github.com/commonmark/cmark/issues/253 ， 如果类似函数存在，就不用向下面这样麻烦计算了
    // Second pass to extract the content without using strtok
    const char *start = markdownText;
    const char *end = markdownText + strlen(markdownText);
    size_t current_line = 1;
    const char *line_start = start;
    const char *section_start = NULL;
    const char *section_end = NULL;

    while (line_start < end) {
        const char *line_end = strchr(line_start, '\n');
        if (!line_end) line_end = end;

        if (current_line == start_pos + 1) {
            section_start = line_start;
        } else if (end_pos && current_line == end_pos) {
            section_end = line_start;
            break;
        }

        if (line_end < end) line_start = line_end + 1;
        else line_start = end;
        current_line++;
    }

    if (!section_start) return NULL;
    if (!section_end) section_end = end;

    // Allocate and copy the section
    size_t length = section_end - section_start;
    char *result = malloc(length + 1);
    memcpy(result, section_start, length);
    result[length] = '\0';

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
            fprintf(stderr, "Header %s not found in file: %s\n", argv[i], filename);
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
