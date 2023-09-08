#ifndef _TGUI_SERIALIZER_ 
#define _TGUI_SERIALIZER_ 

#include "tgui_os.h"
#include "tgui_docker.h"

struct TGuiOsFile;
struct TGuiDockerNode;

void tgui_serializer_write_docker_tree(TGuiDockerNode *node, char *filename);

void tgui_serializer_read_docker_tree(TGuiOsFile *file, struct TGuiDockerNode **node, struct TGuiAllocatedWindow *list);

typedef enum TGuiTokenType {
    TGUI_TOKEN_NODE_ROOT,
    TGUI_TOKEN_NODE_WINDOW,
    TGUI_TOKEN_NODE_SPLIT,

    TGUI_TOKEN_WINDOW,

    TGUI_TOKEN_NUMBER,
    TGUI_TOKEN_STRING,
    TGUI_TOKEN_IDENTIFIER,

    TGUI_TOKEN_OPEN_BRACE,
    TGUI_TOKEN_CLOSE_BRACE,
    TGUI_TOKEN_DOUBLE_DOT,
} TGuiTokenType;

typedef struct TGuiToken {
    TGuiTokenType type;

    char *start;
    char *end;
    
    tgui_s32 value;

    tgui_u32 line;
    tgui_u32 col;

} TGuiToken;

typedef struct TGuiTokenizer {
    char *current;
    char *end;

    tgui_u32 current_line;
    tgui_u32 current_col;

} TGuiTokenizer;

#define TGUI_START_LINE_AND_COL 1

void tgui_tokenizer_start(TGuiTokenizer *tokenizer, struct TGuiOsFile *file);

tgui_b32 tgui_tokenizer_next_token(TGuiTokenizer *tokenizer, TGuiToken *token, tgui_b32 *error);

void tgui_token_print(TGuiToken *token);

#endif /* _TGUI_SERIALIZER_ */
