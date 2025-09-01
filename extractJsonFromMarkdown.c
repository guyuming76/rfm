//附件代码读取markdown文件，输入gkeyfile格式文本，请改成使用json-glib输出json文本,gkeyfile中的key对应json文件的key
///home/guyuming/rfm1/extractKeyValuePairFromMarkdown.c
//为了解决这个问题，我们需要将现有的C语言代码从输出GKeyFile格式改为使用JSON-GLib库输出JSON格式。主要改动包括添加JSON-GLib支持、重构数据处理逻辑以及调整输出格式。
//### 方法思路
//1. **添加JSON-GLib支持**：引入JSON-GLib库来处理JSON对象的创建和输出。
//2. **重构数据处理逻辑**：将原有的GKeyFile格式输出逻辑改为构建JSON对象。
//3. **处理重复键**：确保JSON对象中的键唯一，后出现的值会覆盖先前的值。
//4. **内存管理**：正确分配和释放JSON对象及相关字符串的内存。

#include <cmark.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <json-glib/json-glib.h>

int debug = 0;
char **HeadersToMatch = NULL;
int HeadersToMatch_Count = 0;
int heading_level_to_match = -1;
int current_heading_level = 0;
int currently_in_matched_header = 0;
int current_heading_finished_testing_HeadersToMatch = 0;
int currently_matched_header_finished_printing = 0;
int skip = 0;
char *value_str = NULL;

JsonObject *json_object = NULL;
char *current_key = NULL;

void print_event(cmark_event_type ev_type) {
    switch (ev_type) {
        case CMARK_EVENT_ENTER: printf("event:enter "); break;
        case CMARK_EVENT_EXIT: printf("event:exit "); break;
        case CMARK_EVENT_NONE: printf("event:none "); break;
    }
}

void print_node(cmark_node *node, cmark_event_type ev_type) {
    if (debug) print_event(ev_type);
    if (!node) return;

    const char* typestring = cmark_node_get_type_string(node);
    if (typestring) {
        if (debug) printf("typestring:%s\n", typestring);
        if (strcmp(typestring, "html_inline") == 0) { skip = 0; return; }
        if (strcmp(typestring, "html_block") == 0) { skip = 1; if (debug) printf("skip\n"); }
    }
    if (skip == 1) return;

    cmark_node_type nodetype = cmark_node_get_type(node);
    if (nodetype == CMARK_NODE_HEADING) {
        current_heading_level = cmark_node_get_heading_level(node);
        if (heading_level_to_match == -1) heading_level_to_match = current_heading_level;
        else if (heading_level_to_match > current_heading_level) {
            printf("Error: we suppose the first header node we get from file has the highest level, but we get level %d before %d here\n", heading_level_to_match, current_heading_level);
            exit(1);
        } else if (current_heading_level != heading_level_to_match) return;

        if (ev_type == CMARK_EVENT_ENTER) {
            currently_in_matched_header = 0;
            current_heading_finished_testing_HeadersToMatch = 0;
        }
        if (debug) printf("heading level:%d\n", current_heading_level);
    }

    const char* literal = cmark_node_get_literal(node);
    if (literal) {
        if (debug) printf("%d:literal:%s\n", current_heading_level, literal);

        if (nodetype == CMARK_NODE_TEXT && current_heading_level == heading_level_to_match && !current_heading_finished_testing_HeadersToMatch) {
            if (HeadersToMatch_Count > 0) {
                for (int i = 0; i < HeadersToMatch_Count; i++) {
                    if (strcmp(literal, HeadersToMatch[i]) == 0) {
                        currently_in_matched_header = 1;
                        currently_matched_header_finished_printing = 0;
                        if (current_key) g_free(current_key);
                        current_key = g_strdup(HeadersToMatch[i]);
                    }
                }
            } else {
                currently_in_matched_header = 1;
                currently_matched_header_finished_printing = 0;
                if (current_key) g_free(current_key);
                current_key = g_strdup(literal);
            }
	    if (value_str) g_free(value_str);
	    value_str = NULL;
            current_heading_finished_testing_HeadersToMatch = 1;
        } else if (currently_in_matched_header && !currently_matched_header_finished_printing) {
	    char *new_value_str;
            if (strcmp(typestring, "code_block") == 0) {
                new_value_str = value_str? g_strdup_printf("%s\n%s",value_str,"code_block") : g_strdup("code_block");
            } else {//append literal to value_str
		new_value_str = value_str? g_strdup_printf("%s\n%s",value_str,literal) : g_strdup(literal);
            }
	    g_free(value_str);
	    value_str = new_value_str;
	    if (current_key) json_object_set_string_member(json_object, current_key, value_str);
            //currently_matched_header_finished_printing = 1;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("We parse a file specified by the MarkdownFilename argument with cmark, and output KeyValue pair lines. The HeadersToMatch arguments specify the keys, which are the highest level header texts in the markdown file. Note that we suppose the first header node we get in a file has the highest level. The first line of literal text for the matched header will be the value for the key\n\n");
        printf("Usage: %s (0|1) MarkdownFilename {HeadersToMatch}\n", argv[0]);
        printf(" 0 or 1 for the debug option, 1 for debug\n");
        printf(" HeadersToMatch is the list of markdown headers text to match, seperated with space\n");
        printf(" All headers of the highest level will be in output if no HeadersToMatch is specified\n");
        printf("example: %s 0 your_markdown_file.md Header_level1_text1 Header_level1_text2\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[2], "rb");
    if (!fp) {
        fprintf(stderr, "Error opening file: %s\n", argv[2]);
        return 1;
    }
    cmark_node *root = cmark_parse_file(fp, CMARK_OPT_DEFAULT);
    fclose(fp);

    debug = atoi(argv[1]);
    HeadersToMatch = argv + 3;
    HeadersToMatch_Count = argc - 3;

    json_object = json_object_new();

    cmark_event_type ev_type;
    cmark_iter *iter = cmark_iter_new(root);
    while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
        cmark_node *cur = cmark_iter_get_node(iter);
        print_node(cur, ev_type);
    }

    JsonNode *json_node = json_node_new(JSON_NODE_OBJECT);
    json_node_set_object(json_node, json_object);

    JsonGenerator *generator = json_generator_new();
    json_generator_set_root(generator, json_node);
    json_generator_set_pretty(generator, TRUE);
    gchar *json_str = json_generator_to_data(generator, NULL);
    printf("%s\n", json_str);

    g_free(json_str);
    g_object_unref(generator);
    json_node_free(json_node);
    json_object_unref(json_object);
    if (current_key) g_free(current_key);
    cmark_iter_free(iter);
    cmark_node_free(root);

    return 0;
}

//1. **添加JSON-GLib支持**：通过包含`json-glib.h`头文件并初始化JSON对象来支持JSON处理。
//2. **重构数据处理逻辑**：在`print_node`函数中，将原有的GKeyFile输出逻辑改为构建JSON对象，使用`json_object_set_string_member`来设置键值对。
//3. **处理重复键**：通过JSON对象的特性自动处理重复键，后设置的值会覆盖先前的值。
//4. **内存管理**：使用`g_free`和`g_object_unref`正确释放分配的内存，避免内存泄漏。
//这些改动确保了代码能够正确读取Markdown文件并输出JSON格式的文本，其中键对应JSON对象的键，值对应Markdown中相应标题下的内容。

