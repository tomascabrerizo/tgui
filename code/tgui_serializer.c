#include "tgui_serializer.h"
#include "common.h"
#include "os.h"
#include "tgui_docker.h"
#include "tgui.h"

#include <stdio.h>
#include <stdarg.h>

extern TGui state;
extern TGuiDocker docker;

static FILE *file = NULL;

static inline void indent(FILE *stream, u32 n) {
    for(u32 i = 0; i < n; ++i) {
        fprintf(stream, "  ");
    }
}

static inline void write_to_file(u32 depth, char *str, ...) {
    va_list argp;
    va_start(argp, str);
    indent(file, depth); vfprintf(file, str, argp);
    va_end(argp);
}

void tgui_serializer_write_window(TGuiWindow *window, u32 depth) {
    write_to_file(depth, "window: {\n");
    write_to_file(depth+1, "name: \"%s\"\n", window->name);
    write_to_file(depth+1, "is_scrolling: %d\n", window->is_scrolling);
    write_to_file(depth, "}\n");
}

void tgui_serializer_write_node(TGuiDockerNode *node, u32 depth) {

    switch(node->type) {
    
        case TGUI_DOCKER_NODE_WINDOW: {
            write_to_file(depth, "window_node: {\n");
            write_to_file(depth+1, "active_window: \"%s\"\n", node->active_window->name);
            write_to_file(depth+1, "window_count: %d\n", node->windows_count);
            TGuiWindow *window = node->windows->next;
            while (!clink_list_end(window, node->windows)) {
                tgui_serializer_write_window(window, depth+1);
                window = window->next;
            }
            write_to_file(depth, "}\n");

        } break;

        case TGUI_DOCKER_NODE_SPLIT: {
            write_to_file(depth, "split_node: {\n");
            write_to_file(depth+1, "split_position: %d\n", (u32)(node->split_position * 1000));
            write_to_file(depth, "}\n");
        } break;

        case TGUI_DOCKER_NODE_ROOT: {
            write_to_file(depth, "root_node: {\n");
            write_to_file(depth+1, "dir: %d\n", node->dir);
            TGuiDockerNode *child = node->childs->next;
            while(!clink_list_end(child, node->childs)) {
                tgui_serializer_write_node(child, depth+1);
                child = child->next;
            }
            write_to_file(depth, "}\n");

        } break;
    }
}

void tgui_serializer_write_docker_tree(void) {
    ASSERT(docker.root);
    file = fopen("./tgui.dat", "w");
    fseek(file, 0, SEEK_SET);
    tgui_serializer_write_node(docker.root, 0);
    fclose(file);

}

b32 tgui_is_digit(char character) {
    return (character >= '0' && character <= '9');
}

b32 tgui_is_alpha(char character) {
 return ((character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z'));
}

b32 tgui_is_space(char character) {
    return (character == ' ' || character == '\n' || character == '\r' || character == '\t');
}

b32 tgui_token_contain(TGuiToken *token, char *str) {
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

void tgui_tokenizer_skip_space_or_new_line(TGuiTokenizer *tokenizer, b32 *error) {
    UNUSED(error);
    while (tgui_is_space(*tokenizer->current) && (tokenizer->current != tokenizer->end)) {

        ++tokenizer->current_col;
        if(*tokenizer->current == '\n') {
            ++tokenizer->current_line;
            tokenizer->current_col = TGUI_START_LINE_AND_COL;
        }
        ++tokenizer->current;
    }
}

void tokenizer_token_number(TGuiTokenizer *tokenizer, TGuiToken *token, b32 *error) {
    
    char *start = tokenizer->current;
    u32 col = tokenizer->current_col;

    while(tgui_is_digit(*tokenizer->current)) {
        ++tokenizer->current;
        ++tokenizer->current_col;
    }

    token->start = start;
    token->col = col;
    token->end = tokenizer->current - 1;
    token->line = tokenizer->current_line;
    token->type = TGUI_TOKEN_NUMBER;
    
    if(!tgui_is_space(*tokenizer->current)) {
        *error = true;
    }
}

void tokenizer_token_identifier(TGuiTokenizer *tokenizer, TGuiToken *token, b32 *error) {

    char *start = tokenizer->current;
    u32 col = tokenizer->current_col;

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

void tokenizer_token_string(TGuiTokenizer *tokenizer, TGuiToken *token, b32 *error) {
    
    char *start = tokenizer->current;
    u32 col = tokenizer->current_col;
    
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

void tokenizer_token_single_char(TGuiTokenizer *tokenizer, TGuiToken *token, TGuiTokenType type) {

    char *start = tokenizer->current;
    u32 col = tokenizer->current_col;

    ++tokenizer->current;
    ++tokenizer->current_col;

    token->start = start;
    token->col = col;
    token->end = tokenizer->current - 1;
    token->line = tokenizer->current_line;
    token->type = type;

}

void tgui_tokenizer_start(TGuiTokenizer *tokenizer, struct OsFile *file) {
    tokenizer->current = (char *)file->data;
    tokenizer->end = (char *)((u8 *)file->data + file->size);
    tokenizer->current_line = TGUI_START_LINE_AND_COL;
    tokenizer->current_col = TGUI_START_LINE_AND_COL;
}

b32 tgui_tokenizer_next_token(TGuiTokenizer *tokenizer, TGuiToken *token, b32 *error) {
    b32 tokenizer_error = false;

    tgui_tokenizer_skip_space_or_new_line(tokenizer, &tokenizer_error);
    
    if(tokenizer->current ==  tokenizer->end) {
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

    printf("content: %.*s\n", (u32)(token->end - token->start + 1), token->start);
    printf("line: %d\n", token->line);
    printf("col: %d\n", token->col);

}
