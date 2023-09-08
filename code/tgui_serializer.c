#include "tgui_serializer.h"

#include "tgui_os.h"
#include "tgui_docker.h"
#include "tgui.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

extern TGui state;
extern TGuiDocker docker;

static FILE *file = NULL;

static inline void indent(FILE *stream, tgui_u32 n) {
    for(tgui_u32 i = 0; i < n; ++i) {
        fprintf(stream, "  ");
    }
}

static inline void write_to_file(tgui_u32 depth, char *str, ...) {
    va_list argp;
    va_start(argp, str);
    indent(file, depth); vfprintf(file, str, argp);
    va_end(argp);
}

void tgui_serializer_write_window(TGuiWindow *window, tgui_u32 depth) {
    write_to_file(depth, "window: {\n");
    write_to_file(depth+1, "id: %d\n", window->id);
    write_to_file(depth+1, "name: \"%s\"\n", window->name);
    write_to_file(depth+1, "flags: %d\n", window->flags);
    write_to_file(depth, "}\n");
}

void tgui_serializer_write_node(TGuiDockerNode *node, tgui_u32 depth) {

    switch(node->type) {
    
        case TGUI_DOCKER_NODE_WINDOW: {
            write_to_file(depth, "window_node: {\n");
            write_to_file(depth+1, "active_window: %d\n", node->active_window);
            write_to_file(depth+1, "window_count: %d\n", node->windows_count);
            TGuiWindow *window = node->windows->next;
            while (!tgui_clink_list_end(window, node->windows)) {
                tgui_serializer_write_window(window, depth+1);
                window = window->next;
            }
            write_to_file(depth, "}\n");

        } break;

        case TGUI_DOCKER_NODE_SPLIT: {
            write_to_file(depth, "split_node: {\n");
            write_to_file(depth+1, "split_position: %d\n", (tgui_u32)(node->split_position * 1000));
            write_to_file(depth, "}\n");
        } break;

        case TGUI_DOCKER_NODE_ROOT: {
            write_to_file(depth, "root_node: {\n");
            write_to_file(depth+1, "dir: %d\n", node->dir);
            TGuiDockerNode *child = node->childs->next;
            while(!tgui_clink_list_end(child, node->childs)) {
                tgui_serializer_write_node(child, depth+1);
                child = child->next;
            }
            write_to_file(depth, "}\n");

        } break;
    }
}


void tgui_serializer_write_docker_tree(TGuiDockerNode *node, char *filename) {
    file = fopen(filename, "w");
    fseek(file, 0, SEEK_SET);
    tgui_serializer_write_node(node, 0);
    fclose(file);
}

void tgui_serializer_error(tgui_b32 *error, TGuiToken *token, char *str) {
    if(*error == false) {
        printf("line:%d:col:%d: error: %s\n", token->line, token->col, str);
        *error = true;
    }
}

void tgui_serializer_next_token(TGuiTokenizer *tokenizer, TGuiToken *token, tgui_b32 *error) {
    if(*error == false) {
        tgui_tokenizer_next_token(tokenizer, token, error); 
    }
}

void tgui_serializer_expect(TGuiTokenizer *tokenizer, TGuiToken *token, TGuiTokenType type, tgui_b32 *error, char *str) {
    tgui_serializer_next_token(tokenizer, token, error);
    if(token->type != type) {
        tgui_serializer_error(error, token, str);
    }
}

void tgui_serializer_peek_next(TGuiTokenizer *tokenizer, TGuiToken *token) {
    tgui_b32 error;
    TGuiTokenizer temp = *tokenizer;
    tgui_tokenizer_next_token(&temp, token, &error);
}

tgui_b32 tgui_token_contain(TGuiToken *token, char *str);

void tgui_serializer_expect_identifier(TGuiTokenizer *tokenizer, TGuiToken *token, char *identifier, tgui_b32 *error, char *error_str) {
    tgui_serializer_expect(tokenizer, token, TGUI_TOKEN_IDENTIFIER, error, "Node body can only contain identifiers");
    if(!tgui_token_contain(token, identifier)) {
        tgui_serializer_error(error, token, error_str);
    }
    tgui_serializer_expect(tokenizer, token, TGUI_TOKEN_DOUBLE_DOT, error, "Identifier must be follow by ':'");

}

char *tgui_token_to_c_string(TGuiToken *token) {
    /* NOTE: Removes "" from the string */
    char *start = token->start + 1;
    char *end = token->end - 1;

    tgui_u32 str_size = end - start + 1;
    char *c_str = tgui_arena_alloc(&state.arena, str_size+1, 8);
    memcpy(c_str, start, str_size);
    c_str[str_size] = '\0';
    return c_str;
}

TGuiWindow *tgui_serializer_read_window(TGuiTokenizer *tokenizer, TGuiToken *token, tgui_b32 *error, TGuiDockerNode *parent, TGuiAllocatedWindow *list) {
    if(*error) return NULL;

    tgui_serializer_expect(tokenizer, token, TGUI_TOKEN_DOUBLE_DOT, error, "Identifier must be follow by ':'");
    tgui_serializer_expect(tokenizer, token, TGUI_TOKEN_OPEN_BRACE, error, "Node body must start with '{'");

    tgui_serializer_expect_identifier(tokenizer, token, "id", error, "First member of window must be 'id'");
    tgui_serializer_expect(tokenizer, token, TGUI_TOKEN_NUMBER, error, "'id' value must be a number");
    tgui_u32 id = token->value;

    tgui_serializer_expect_identifier(tokenizer, token, "name", error, "Second member of window must be 'name'");
    tgui_serializer_expect(tokenizer, token, TGUI_TOKEN_STRING, error, "'name' value must be a string");
    char *name = tgui_token_to_c_string(token);

    tgui_serializer_expect_identifier(tokenizer, token, "flags", error, "Third member of window must be 'flags'");
    tgui_serializer_expect(tokenizer, token, TGUI_TOKEN_NUMBER, error, "'flags' value must be a number");
    TGuiWindowFlags flags = token->value;

    tgui_serializer_expect(tokenizer, token, TGUI_TOKEN_CLOSE_BRACE, error, "split_node must end with '}'");
    
    TGuiWindow *window = tgui_window_alloc(parent, name, flags, list);
    window->id = id;

    return window;
}

TGuiDockerNode *tgui_serializer_read_node_window(TGuiTokenizer *tokenizer, TGuiToken *token, tgui_b32 *error, TGuiDockerNode *parent, TGuiAllocatedWindow *list) {
    if(*error) return NULL;
    
    TGuiDockerNode *node = window_node_alloc(parent);

    tgui_serializer_expect(tokenizer, token, TGUI_TOKEN_DOUBLE_DOT, error, "Identifier must be follow by ':'");
    tgui_serializer_expect(tokenizer, token, TGUI_TOKEN_OPEN_BRACE, error, "Node body must start with '{'");

    tgui_serializer_expect_identifier(tokenizer, token, "active_window", error, "First member of window_node must be 'active_window'");
    tgui_serializer_expect(tokenizer, token, TGUI_TOKEN_NUMBER, error, "active_window value must be a number");
    tgui_u32 active_window = token->value;
    
    tgui_serializer_expect_identifier(tokenizer, token, "window_count", error, "Seconds member of window_node must be 'window_count'");
    tgui_serializer_expect(tokenizer, token, TGUI_TOKEN_NUMBER, error, "window_count value must be a number");
    tgui_u32 window_count_test = token->value;
    
    tgui_serializer_next_token(tokenizer, token, error);

    while(token->type == TGUI_TOKEN_WINDOW && *error == false) {  
        tgui_serializer_read_window(tokenizer, token, error, node, list);
        tgui_serializer_next_token(tokenizer, token, error);
    }

    if(window_count_test != node->windows_count) {
        tgui_serializer_error(error, token, "We area specting more windows");
    }
    
    node->active_window = active_window;

    if(token->type != TGUI_TOKEN_CLOSE_BRACE) {
        tgui_serializer_error(error, token, "window must end with '}'");
    }

    if(*error == true) {
        node_free(node);
        return NULL;
    }
    
    if(parent) {
        TGUI_ASSERT(parent->type == TGUI_DOCKER_NODE_ROOT);
        tgui_clink_list_insert_back(parent->childs, node);
    }
    
    return node;
}

TGuiDockerNode *tgui_serializer_read_node_split(TGuiTokenizer *tokenizer, TGuiToken *token, tgui_b32 *error, TGuiDockerNode *parent) {
    if(*error) return NULL;
    
    TGuiDockerNode *node = split_node_alloc(parent, 0);

    tgui_serializer_expect(tokenizer, token, TGUI_TOKEN_DOUBLE_DOT, error, "Identifier must be follow by ':'");
    tgui_serializer_expect(tokenizer, token, TGUI_TOKEN_OPEN_BRACE, error, "Node body must start with '{'");

    tgui_serializer_expect_identifier(tokenizer, token, "split_position", error, "First member of window_node must be 'split_position'");
    tgui_serializer_expect(tokenizer, token, TGUI_TOKEN_NUMBER, error, "split_position value must be a number");
    node->split_position = (token->value / 1000.0f);

    tgui_serializer_expect(tokenizer, token, TGUI_TOKEN_CLOSE_BRACE, error, "split_node must end with '}'");

    if(*error == true) {
        node_free(node);
        return NULL;
    }
    
    if(parent) {
        TGUI_ASSERT(parent->type == TGUI_DOCKER_NODE_ROOT);
        tgui_clink_list_insert_back(parent->childs, node);
    }

    return node;
}

TGuiDockerNode *tgui_serializer_read_node_root(TGuiTokenizer *tokenizer, TGuiToken *token, tgui_b32 *error, TGuiDockerNode *parent, TGuiAllocatedWindow *list) {
    if(*error) return NULL;

    TGuiDockerNode *node = root_node_alloc(parent);
    
    tgui_serializer_expect(tokenizer, token, TGUI_TOKEN_DOUBLE_DOT, error, "Identifier must be follow by ':'");
    tgui_serializer_expect(tokenizer, token, TGUI_TOKEN_OPEN_BRACE, error, "Node body must start with '{'");
    
    tgui_serializer_expect_identifier(tokenizer, token, "dir", error, "First member of root_node must be 'dir'");
    tgui_serializer_expect(tokenizer, token, TGUI_TOKEN_NUMBER, error, "dir value must be a number");
    node->dir = (TGuiSplitDirection)token->value;
    
    tgui_serializer_next_token(tokenizer, token, error);

    while((token->type == TGUI_TOKEN_NODE_WINDOW || token->type == TGUI_TOKEN_NODE_ROOT) && *error == false) {
        
        if(token->type == TGUI_TOKEN_NODE_WINDOW) {
            tgui_serializer_read_node_window(tokenizer, token, error, node, list);
        } else if(token->type == TGUI_TOKEN_NODE_ROOT) {
            tgui_serializer_read_node_root(tokenizer, token, error, node, list);
        }
        
        tgui_serializer_next_token(tokenizer, token, error);

        if(token->type == TGUI_TOKEN_NODE_SPLIT) {
            tgui_serializer_read_node_split(tokenizer, token, error, node);
            tgui_serializer_next_token(tokenizer, token, error);
        }
    }

    if(token->type != TGUI_TOKEN_CLOSE_BRACE) {
        tgui_serializer_error(error, token, "root_node must end with '}'");
    }
    
    if(*error == true) {
        node_free(node);
        return NULL;
    }

    if(parent) {
        TGUI_ASSERT(parent->type == TGUI_DOCKER_NODE_ROOT);
        tgui_clink_list_insert_back(parent->childs, node);
    }

    return node;
}

TGuiDockerNode *tgui_serializer_read_node(TGuiTokenizer *tokenizer, tgui_b32 *error, TGuiDockerNode *parent, TGuiAllocatedWindow *list) {
    TGuiDockerNode *node = NULL;

    TGuiToken token;
    while(tgui_tokenizer_next_token(tokenizer, &token, error) && *error == false) {
        if(token.type == TGUI_TOKEN_NODE_ROOT) {
            node = tgui_serializer_read_node_root(tokenizer, &token, error, parent, list);
        } else if(token.type == TGUI_TOKEN_NODE_WINDOW) {
            node = tgui_serializer_read_node_window(tokenizer, &token, error, parent, list);
        } else {
            *error = true;
            return NULL;
        }
    }

    return node;

}

void tgui_serializer_read_docker_tree(TGuiOsFile *file, struct TGuiDockerNode **node, TGuiAllocatedWindow *list) {
    tgui_b32 serializer_error = false;
    
    TGUI_ASSERT(list);
    tgui_clink_list_init(list);

    TGuiToken token;
    TGuiTokenizer tokenizer;
    tgui_tokenizer_start(&tokenizer, file);
    tgui_serializer_peek_next(&tokenizer, &token);
    
    if(token.type == TGUI_TOKEN_NODE_ROOT || token.type == TGUI_TOKEN_NODE_WINDOW) {
        *node = tgui_serializer_read_node(&tokenizer, &serializer_error, NULL, list);
    } else {
        *node = NULL;
    } 

}

tgui_b32 tgui_is_digit(char character) {
    return (character >= '0' && character <= '9');
}

tgui_b32 tgui_is_alpha(char character) {
 return ((character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z'));
}

tgui_b32 tgui_is_space(char character) {
    return (character == ' ' || character == '\n' || character == '\r' || character == '\t');
}

tgui_b32 tgui_token_contain(TGuiToken *token, char *str) {

    if((tgui_u32)(token->end - token->start + 1) != strlen(str)) {
        return false;
    }

    char *current = token->start;
    while (current <= token->end) {
        if(*current++ != *str++) {
            return false;
        }
    }
    return true;
}

void tgui_check_special_identifier(TGuiToken *token) {
    if(tgui_token_contain(token, "window_node")) {
        token->type = TGUI_TOKEN_NODE_WINDOW;
    } else if(tgui_token_contain(token, "root_node")) {
        token->type = TGUI_TOKEN_NODE_ROOT;
    } else if(tgui_token_contain(token, "split_node")) {
        token->type = TGUI_TOKEN_NODE_SPLIT;
    } else if(tgui_token_contain(token, "window")) {
        token->type = TGUI_TOKEN_WINDOW;
    }
}

void tgui_tokenizer_skip_space_or_new_line(TGuiTokenizer *tokenizer, tgui_b32 *error) {
    TGUI_UNUSED(error);
    while (tgui_is_space(*tokenizer->current) && (tokenizer->current != tokenizer->end)) {

        ++tokenizer->current_col;
        if(*tokenizer->current == '\n') {
            ++tokenizer->current_line;
            tokenizer->current_col = TGUI_START_LINE_AND_COL;
        }
        ++tokenizer->current;
    }
}

static void tokenizer_token_number(TGuiTokenizer *tokenizer, TGuiToken *token, tgui_b32 *error) {
    
    char *start = tokenizer->current;
    tgui_u32 col = tokenizer->current_col;
    
    tgui_u32 value = 0;
    while(tgui_is_digit(*tokenizer->current)) {
        value *= 10;
        value += ((*tokenizer->current) - '0');

        ++tokenizer->current;
        ++tokenizer->current_col;
    }

    token->start = start;
    token->col = col;
    token->end = tokenizer->current - 1;
    token->line = tokenizer->current_line;
    token->type = TGUI_TOKEN_NUMBER;
    token->value = value;
    
    if(!tgui_is_space(*tokenizer->current)) {
        *error = true;
    }
}

static void tokenizer_token_identifier(TGuiTokenizer *tokenizer, TGuiToken *token, tgui_b32 *error) {

    char *start = tokenizer->current;
    tgui_u32 col = tokenizer->current_col;

    while(tgui_is_alpha(*tokenizer->current) || tgui_is_digit(*tokenizer->current) || *tokenizer->current == '_') {
        ++tokenizer->current;
        ++tokenizer->current_col;
    }

    token->start = start;
    token->col = col;
    token->end = tokenizer->current - 1;
    token->line = tokenizer->current_line;
    token->type = TGUI_TOKEN_IDENTIFIER;
    
    tgui_check_special_identifier(token);

    if(!tgui_is_space(*tokenizer->current) && *tokenizer->current != ':') {
        *error = true;
    }
}

static void tokenizer_token_string(TGuiTokenizer *tokenizer, TGuiToken *token, tgui_b32 *error) {
    
    char *start = tokenizer->current;
    tgui_u32 col = tokenizer->current_col;
    
    ++tokenizer->current;

    while(*tokenizer->current != '"') {
        if(*tokenizer->current == '\n' || *tokenizer->current == '\r') {
            *error = true;
            break;
        }
        ++tokenizer->current;
        ++tokenizer->current_col;
    }

    ++tokenizer->current;

    token->start = start;
    token->col = col;
    token->end = tokenizer->current - 1;
    token->line = tokenizer->current_line;
    token->type = TGUI_TOKEN_STRING;

}

static void tokenizer_token_single_char(TGuiTokenizer *tokenizer, TGuiToken *token, TGuiTokenType type) {

    char *start = tokenizer->current;
    tgui_u32 col = tokenizer->current_col;

    ++tokenizer->current;
    ++tokenizer->current_col;

    token->start = start;
    token->col = col;
    token->end = tokenizer->current - 1;
    token->line = tokenizer->current_line;
    token->type = type;

}

void tgui_tokenizer_start(TGuiTokenizer *tokenizer, struct TGuiOsFile *file) {
    tokenizer->current = (char *)file->data;
    tokenizer->end = (char *)((tgui_u8 *)file->data + file->size);
    tokenizer->current_line = TGUI_START_LINE_AND_COL;
    tokenizer->current_col = TGUI_START_LINE_AND_COL;
}

tgui_b32 tgui_tokenizer_next_token(TGuiTokenizer *tokenizer, TGuiToken *token, tgui_b32 *error) {
    tgui_b32 tokenizer_error = false;

    tgui_tokenizer_skip_space_or_new_line(tokenizer, &tokenizer_error);
    
    if(tokenizer->current == tokenizer->end) {
        *error = false;
        return false;
    }

    switch(*tokenizer->current) {
        
    case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case '0': {
      tokenizer_token_number(tokenizer, token, &tokenizer_error);
    } break;

    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
    case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I':
    case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '_': {
        tokenizer_token_identifier(tokenizer, token, &tokenizer_error);
    } break;
    
    case '"': {
        tokenizer_token_string(tokenizer, token, &tokenizer_error);
    } break;

    case '{': {
        tokenizer_token_single_char(tokenizer, token, TGUI_TOKEN_OPEN_BRACE);
    } break;
    
    case '}': {
        tokenizer_token_single_char(tokenizer, token, TGUI_TOKEN_CLOSE_BRACE);
    } break;
    
    case ':': {
        tokenizer_token_single_char(tokenizer, token, TGUI_TOKEN_DOUBLE_DOT);
    } break;

    default: {
        tokenizer_error = true;
    } break;

    }

    if(error) *error = tokenizer_error;

    return !tokenizer_error;
}

void tgui_token_print(TGuiToken *token) {

    printf("type: ");
    switch(token->type) {
        case TGUI_TOKEN_NODE_ROOT:   { printf("TGUI_TOKEN_NODE_ROOT\n"); } break;
        case TGUI_TOKEN_NODE_WINDOW: { printf("TGUI_TOKEN_NODE_WINDOW\n"); } break;
        case TGUI_TOKEN_NODE_SPLIT:  { printf("TGUI_TOKEN_NODE_SPLIT\n"); } break;
        case TGUI_TOKEN_WINDOW:      { printf("TGUI_TONEN_WINDOW\n"); } break;
        case TGUI_TOKEN_NUMBER:      { printf("TGUI_TOKEN_NUMBER\n"); } break;
        case TGUI_TOKEN_STRING:      { printf("TGUI_TOKEN_STRING\n"); } break;
        case TGUI_TOKEN_IDENTIFIER:  { printf("TGUI_TOKEN_IDENTIFIER\n"); } break;
        case TGUI_TOKEN_OPEN_BRACE:  { printf("TGUI_TOKEN_OPEN_BRACE\n"); } break;
        case TGUI_TOKEN_CLOSE_BRACE: { printf("TGUI_TOKEN_CLOSE_BRACE\n"); } break;
        case TGUI_TOKEN_DOUBLE_DOT:  { printf("TGUI_TOKEN_DOUBLE_DOT\n"); } break;
    }

    printf("content: %.*s\n", (tgui_u32)(token->end - token->start + 1), token->start);
    printf("line: %d\n", token->line);
    printf("col: %d\n", token->col);

}
