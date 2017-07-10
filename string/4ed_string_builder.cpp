/*
 * Mr. 4th Dimention - Allen Webster
 *
 * 21.01.2017
 *
 * Builder for the 4coder_string.h header.
 *
 */

// TOP

// TODO(allen): Make sure to only publish the 4coder_string.h if it builds and passes a series of tests.

#define BUILD_NUMBER_FILE "4coder_string_build_num.txt"

#define GENERATED_FILE "4coder_string.h"
#define INTERNAL_STRING "internal_4coder_string.cpp"

#define BACKUP_FOLDER ".." SLASH ".." SLASH "string_backup"
#define PUBLISH_FOLDER ".." SLASH "4coder_helper"

#include "../4cpp/4cpp_lexer.h"
#define FSTRING_IMPLEMENTATION
#include "../4coder_lib/4coder_string.h"

#include "../4ed_defines.h"
#include "../meta/4ed_meta_defines.h"

#define FTECH_FILE_MOVING_IMPLEMENTATION
#include "../meta/4ed_file_moving.h"
#include "../meta/4ed_meta_parser.cpp"
#include "../meta/4ed_meta_keywords.h"
#include "../meta/4ed_out_context.cpp"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define V_MAJ_NUM 1
#define V_MIN_NUM 0

#define V_MAJ STR_(V_MAJ_NUM)
#define V_MIN STR_(V_MIN_NUM)

internal char*
parse_next_line(char *str, char *str_end){
    char *ptr = str;
    for (; ptr < str_end && *ptr != '\n'; ++ptr);
    ++ptr;
    return(ptr);
}

internal b32
parse_build_number(char *file_name, i32 *major_out, i32 *minor_out, i32 *build_out){
    b32 result = false;
    String file = file_dump(file_name);
    
    if (file.str != 0){
        *major_out = 0;
        *minor_out = 0;
        *build_out = 0;
        
        char *end_str = file.str + file.size;
        char *major_str = file.str;
        char *minor_str = parse_next_line(major_str, end_str);
        char *build_str = parse_next_line(minor_str, end_str);
        char *ender = parse_next_line(build_str, end_str);
        
        if (major_str < end_str && build_str < end_str && ender < end_str){
            minor_str[-1] = 0;
            build_str[-1] = 0;
            ender[-1] = 0;
            
            *major_out = str_to_int_c(major_str);
            *minor_out = str_to_int_c(minor_str);
            *build_out = str_to_int_c(build_str);
            
            result = true;
        }
        
        free(file.str);
    }
    
    return(result);
}

internal void
save_build_number(char *file_name, i32 major, i32 minor, i32 build){
    FILE *out = fopen(file_name, "wb");
    fprintf(out, "%d\n%d\n%d\n\n\n", major, minor, build);
    fclose(out);
}

///////////////////////////////

//
// Meta Parse Rules
//

internal void
print_function_body_code(String *out, Parse_Context *context, i32 start){
    String pstr = {0}, lexeme = {0};
    Cpp_Token *token = 0;
    
    i32 do_print = 0;
    i32 nest_level = 0;
    i32 finish = false;
    i32 do_whitespace_print = false;
    i32 is_first = true;
    
    for (; (token = get_token(context)) != 0; get_next_token(context)){
        if (do_whitespace_print){
            pstr = str_start_end(context->data, start, token->start);
            append(out, pstr);
        }
        else{
            do_whitespace_print = true;
        }
        
        do_print = true;
        if (token->type == CPP_TOKEN_COMMENT){
            lexeme = get_lexeme(*token, context->data);
            if (check_and_fix_docs(&lexeme)){
                do_print = false;
            }
        }
        else if (token->type == CPP_TOKEN_BRACE_OPEN){
            ++nest_level;
        }
        else if (token->type == CPP_TOKEN_BRACE_CLOSE){
            --nest_level;
            if (nest_level == 0){
                finish = true;
            }
        }
        if (is_first){
            do_print = false;
            is_first = false;
        }
        
        if (do_print){
            pstr = get_lexeme(*token, context->data);
            append(out, pstr);
        }
        
        start = token->start + token->size;
        
        if (finish){
            break;
        }
    }
}

internal void
file_move(char *path, char *file_name){
    fm_copy_file(fm_str(file_name), fm_str(path, "/", file_name));
}

int main(){
    META_BEGIN();
    fm_init_system();
    
#if 0
    i32 size = (512 << 20);
    void *mem = malloc(size);
    memset(mem, 0, size);
#endif
    
    // NOTE(allen): Parse the internal string file.
    char *string_files[] = { INTERNAL_STRING, 0 };
    Meta_Unit string_unit = compile_meta_unit(".", string_files, ExpandArray(meta_keywords));
    
    if (string_unit.parse == 0){
        Assert(!"Missing one or more input files!");
    }
    
    // NOTE(allen): Parse the version counter file
    i32 major_number = 0;
    i32 minor_number = 0;
    i32 build_number = 0;
    b32 parsed_version_counter = parse_build_number(BUILD_NUMBER_FILE, &major_number, &minor_number, &build_number);
    Assert(parsed_version_counter);
    
    if (V_MAJ_NUM < major_number){
        Assert(!"major version mismatch");
    }
    else if (V_MAJ_NUM > major_number){
        major_number = V_MAJ_NUM;
        minor_number = V_MIN_NUM;
        build_number = 0;
    }
    else{
        if (V_MIN_NUM < minor_number){
            Assert(!"minor version mismatch");
        }
        else if (V_MIN_NUM > minor_number){
            minor_number = V_MIN_NUM;
            build_number = 0;
        }
    }
    
    // NOTE(allen): Output
    String out = str_alloc(10 << 20);
    Out_Context context = {0};
    
    // NOTE(allen): String Library
    if (begin_file_out(&context, GENERATED_FILE, &out)){
        Cpp_Token *token = 0;
        i32 start = 0;
        
        Parse parse = string_unit.parse[0];
        Parse_Context pcontext = setup_parse_context(parse);
        
        for (; (token = get_token(&pcontext)) != 0; get_next_token(&pcontext)){
            if (!(token->flags & CPP_TFLAG_PP_BODY) &&
                token->type == CPP_TOKEN_IDENTIFIER){
                String lexeme = get_lexeme(*token, pcontext.data);
                if (match(lexeme, "FSTRING_BEGIN")){
                    start = token->start + token->size;
                    break;
                }
            }
        }
        
        append(&out, "/*\n");
        
        append(&out, GENERATED_FILE " - Version "V_MAJ"."V_MIN".");
        append_int_to_str(&out, build_number);
        append(&out, "\n");
        
        append(&out, STANDARD_DISCLAIMER);
        append(&out,
               "To include implementation: #define FSTRING_IMPLEMENTATION\n"
               "To use in C mode: #define FSTRING_C\n");
        
        append(&out, "*/\n");
        
        String pstr = {0};
        i32 do_whitespace_print = true;
        
        for(;(token = get_next_token(&pcontext)) != 0;){
            if (do_whitespace_print){
                pstr = str_start_end(pcontext.data, start, token->start);
                append(&out, pstr);
            }
            else{
                do_whitespace_print = true;
            }
            
            String lexeme = get_lexeme(*token, pcontext.data);
            
            i32 do_print = true;
            if (match(lexeme, "FSTRING_DECLS")){
                append(&out, "#if !defined(FCODER_STRING_H)\n#define FCODER_STRING_H\n\n");
                do_print = false;
                
                local_persist i32 RETURN_PADDING = 16;
                local_persist i32 SIG_PADDING = 35;
                
                for (i32 j = 0; j < string_unit.set.count; ++j){
                    char line_[2048];
                    String line = make_fixed_width_string(line_);
                    Item_Node *item = string_unit.set.items + j;
                    
                    if (item->t == Item_Function){
                        append          (&line, item->ret);
                        append_padding  (&line, ' ', SIG_PADDING);
                        append          (&line, item->name);
                        append          (&line, item->args);
                        append          (&line, ";\n");
                    }
                    else if (item->t == Item_Macro){
                        append          (&line, "#ifndef ");
                        append_padding  (&line, ' ', 10);
                        append          (&line, item->name);
                        append_s_char   (&line, '\n');
                        
                        append          (&line, "# define ");
                        append_padding  (&line, ' ', 10);
                        append          (&line, item->name);
                        append          (&line, item->args);
                        append_s_char   (&line, ' ');
                        append          (&line, item->body);
                        append_s_char   (&line, '\n');
                        
                        append          (&line, "#endif");
                        append_s_char   (&line, '\n');
                    }
                    else{
                        InvalidCodePath;
                    }
                    
                    append(&out, line);
                }
                
                append(&out, "\n#endif\n");
                
                // NOTE(allen): C++ overload definitions
                append(&out, "\n#if !defined(FSTRING_C) && !defined(FSTRING_GUARD)\n\n");
                
                for (i32 j = 0; j < string_unit.set.count; ++j){
                    char line_space[2048];
                    String line = make_fixed_width_string(line_space);
                    
                    Item_Node *item = &string_unit.set.items[j];
                    
                    if (item->t == Item_Function){
                        String cpp_name = item->cpp_name;
                        if (cpp_name.str != 0){
                            Argument_Breakdown breakdown = item->breakdown;
                            
                            append     (&line, item->ret);
                            append_padding(&line, ' ', SIG_PADDING);
                            append     (&line, cpp_name);
                            append     (&line, item->args);
                            if (match(item->ret, "void")){
                                append(&line, "{(");
                            }
                            else{
                                append(&line, "{return(");
                            }
                            append    (&line, item->name);
                            append_s_char(&line, '(');
                            
                            if (breakdown.count > 0){
                                for (i32 i = 0; i < breakdown.count; ++i){
                                    if (i != 0){
                                        append_s_char(&line, ',');
                                    }
                                    append(&line, breakdown.args[i].param_name);
                                }
                            }
                            else{
                                append(&line, "void");
                            }
                            
                            append(&line, "));}\n");
                            
                            append(&out, line);
                        }
                    }
                }
                
                append(&out, "\n#endif\n");
            }
            
            else if (match(lexeme, "API_EXPORT_MACRO")){
                token = get_next_token(&pcontext);
                if (token && token->type == CPP_TOKEN_COMMENT){
                    token = get_next_token(&pcontext);
                    if (token && token->type == CPP_PP_DEFINE){
                        for (;(token = get_next_token(&pcontext)) != 0;){
                            if (!(token->flags & CPP_TFLAG_PP_BODY)){
                                break;
                            }
                        }
                        if (token != 0){
                            get_prev_token(&pcontext);
                        }
                        do_print = false;
                        do_whitespace_print = false;
                    }
                }
            }
            
            else if (match(lexeme, "API_EXPORT") || match(lexeme, "API_EXPORT_INLINE")){
                if (!(token->flags & CPP_TFLAG_PP_BODY)){
                    if (match(lexeme, "API_EXPORT_INLINE")){
                        append(&out, "#if !defined(FSTRING_GUARD)\n");
                    }
                    else{
                        append(&out, "#if defined(FSTRING_IMPLEMENTATION)\n");
                    }
                    print_function_body_code(&out, &pcontext, start);
                    append(&out, "\n#endif");
                    do_print = false;
                }
            }
            
            else if (match(lexeme, "CPP_NAME")){
                Cpp_Token *token_start = token;
                i32 has_cpp_name = false;
                
                token = get_next_token(&pcontext);
                if (token && token->type == CPP_TOKEN_PARENTHESE_OPEN){
                    token = get_next_token(&pcontext);
                    if (token && token->type == CPP_TOKEN_IDENTIFIER){
                        token = get_next_token(&pcontext);
                        if (token && token->type == CPP_TOKEN_PARENTHESE_CLOSE){
                            has_cpp_name = true;
                            do_print = false;
                        }
                    }
                }
                
                if (!has_cpp_name){
                    token = set_token(&pcontext, token_start);
                }
            }
            
            else if (token->type == CPP_TOKEN_COMMENT){
                if (check_and_fix_docs(&lexeme)){
                    do_print = false;
                }
            }
            
            else if (token->type == CPP_PP_INCLUDE){
                token = get_next_token(&pcontext);
                if (token && token->type == CPP_PP_INCLUDE_FILE){
                    lexeme = get_lexeme(*token, pcontext.data);
                    lexeme.size -= 2;
                    lexeme.str += 1;
                    
                    char space[512];
                    String str = make_fixed_width_string(space);
                    append(&str, lexeme);
                    terminate_with_null(&str);
                    String dump = file_dump(str.str);
                    if (dump.str){
                        append(&out, dump);
                    }
                    else{
                        lexeme.size += 2;
                        lexeme.str -= 1;
                        append(&out, "#error Could not find ");
                        append(&out, lexeme);
                        append(&out, "\n");
                    }
                    free(dump.str);
                }
                
                do_print = false;
            }
            
            if ((token = get_token(&pcontext)) != 0){
                if (do_print){
                    pstr = get_lexeme(*token, pcontext.data);
                    append(&out, pstr);
                }
                start = token->start + token->size;
            }
        }
        pstr = str_start_end(pcontext.data, start, parse.code.size);
        append(&out, pstr);
        
        end_file_out(context);
    }
    
    // NOTE(allen): Publish the new file.  (Would like to be able to automatically test the result before publishing).
    {
        fm_make_folder_if_missing(BACKUP_FOLDER SLASH V_MAJ SLASH V_MIN);
        file_move(BACKUP_FOLDER SLASH V_MAJ SLASH V_MIN, INTERNAL_STRING);
        file_move(BACKUP_FOLDER SLASH V_MAJ SLASH V_MIN, GENERATED_FILE);
        //file_move(PUBLISH_FOLDER, GENERATED_FILE);
        fm_delete_file(GENERATED_FILE);
        printf("published "GENERATED_FILE": v%d.%d.%d\n", major_number, minor_number, build_number);
        save_build_number(BUILD_NUMBER_FILE, major_number, minor_number, build_number + 1);
    }
    
    META_FINISH();
}

// BOTTOM
