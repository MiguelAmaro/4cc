/*
4coder_variables.cpp - Variables system
*/

// TOP

////////////////////////////////
// NOTE(allen): String hashing

global Arena vars_arena = {};
global Table_Data_u64 vars_string_to_id = {};
global Table_u64_Data vars_id_to_string = {};
global String_ID vars_string_id_counter = 0;

function void
_vars_init(void){
    local_persist b32 did_init = false;
    if (!did_init)
    {
        did_init = true;
        Base_Allocator *base = get_base_allocator_system();
        vars_arena = make_arena(base);
        vars_string_to_id = make_table_Data_u64(base, 100);
        vars_id_to_string = make_table_u64_Data(base, 100);
    }
}

function String_ID
vars_save_string(String_Const_u8 string){
    _vars_init();
    
    String_ID result = 0;
    Data string_data = make_data(string.str, string.size);
    Table_Lookup location = table_lookup(&vars_string_to_id, string_data);
    if (location.found_match){
        table_read(&vars_string_to_id, location, &result);
    }
    else{
        vars_string_id_counter += 1;
        result = vars_string_id_counter;
        string_data = push_data_copy(&vars_arena, string_data);
        table_insert(&vars_string_to_id, string_data, result);
        table_insert(&vars_id_to_string, result, string_data);
    }
    return(result);
}

function String_Const_u8
vars_read_string(Arena *arena, String_ID id){
    _vars_init();
    
    String_Const_u8 result = {};
    Table_Lookup location = table_lookup(&vars_id_to_string, id);
    if (location.found_match){
        Data data = {};
        table_read(&vars_id_to_string, location, &data);
        result.str = push_array(arena, u8 , data.size);
        block_copy(result.str, data.data, data.size);
        result.size = data.size;
    }
    return(result);
}

////////////////////////////////
// NOTE(allen): Variable structure

global Variable vars_global_root = {};
global Variable vars_nil = {};
global Variable *vars_free_variables = 0;

function Variable_Handle
vars_get_root(void){
    Variable_Handle handle = {&vars_global_root};
    return(handle);
}

function Variable_Handle
vars_get_nil(void){
    Variable_Handle handle = {&vars_nil};
    return(handle);
}

function b32
vars_is_nil(Variable_Handle var){
    return(var.ptr == 0 || var.ptr == &vars_nil);
}

function b32
vars_match(Variable_Handle a, Variable_Handle b){
    return(a.ptr == b.ptr);
}

function Variable_Handle
vars_first_child(Variable_Handle var){
    Variable_Handle result = {};
    if (var.ptr != 0){
        result.ptr = var.ptr->first;
    }
    else{
        result.ptr = &vars_nil;
    }
    return(result);
}

function Variable_Handle
vars_next_sibling(Variable_Handle var){
    Variable_Handle result = {};
    if (var.ptr != 0){
        result.ptr = var.ptr->next;
    }
    else{
        result.ptr = &vars_nil;
    }
    return(result);
}

function String_ID
vars_key_id_from_var(Variable_Handle var){
    return(var.ptr->key);
}

function String_Const_u8
vars_key_from_var(Arena *arena, Variable_Handle var){
    String_ID id = vars_key_id_from_var(var);
    String_Const_u8 result = vars_read_string(arena, id);
    return(result);
}

function String_ID
vars_string_id_from_var(Variable_Handle var){
    return(var.ptr->string);
}

function String_Const_u8
vars_string_from_var(Arena *arena, Variable_Handle var){
    String_ID id = vars_string_id_from_var(var);
    String_Const_u8 result = vars_read_string(arena, id);
    return(result);
}

function Variable_Handle
vars_read_key(Variable_Handle var, String_ID key){
    Variable_Handle result = vars_get_nil();
    for (Variable *node = var.ptr->first;
         node != 0;
         node = node->next){
        if (node->key == key){
            result.ptr = node;
            break;
        }
    }
    return(result);
}

function Variable_Handle
vars_read_key(Variable_Handle var, String_Const_u8 key){
    String_ID id = vars_save_string(key);
    Variable_Handle result = vars_read_key(var, id);
    return(result);
}

function void
vars_set_string(Variable_Handle var, String_ID string){
    if (var.ptr != &vars_nil){
        var.ptr->string = string;
    }
}

function void
vars_set_string(Variable_Handle var, String_Const_u8 string){
    String_ID id = vars_save_string(string);
    vars_set_string(var, id);
}

function void
_vars_free_variable_children(Variable *var){
    for (Variable *node = var->first;
         node != 0;
         node = node->next){
        _vars_free_variable_children(node);
    }
    
    if (var->last != 0){
        var->last->next = vars_free_variables;
    }
    if (var->first != 0){
        vars_free_variables = var->first;
    }
}

function void
vars_erase(Variable_Handle var, String_ID key){
    if (var.ptr != &vars_nil){
        Variable *prev = 0;
        Variable *node = var.ptr->first;
        for (; node != 0;
             node = node->next){
            if (node->key == key){
                break;
            }
            prev = node;
        }
        
        if (node != 0){
            _vars_free_variable_children(node);
            if (prev != 0){
                prev->next = node->next;
            }
            if (var.ptr->first == node){
                var.ptr->first = node->next;
            }
            if (var.ptr->last == node){
                var.ptr->last = prev;
            }
            sll_stack_push(vars_free_variables, node);
        }
    }
}

function Variable_Handle
vars_new_variable(Variable_Handle var, String_ID key){
    Variable_Handle handle = vars_get_nil();
    if (var.ptr != &vars_nil){
        Variable *prev = 0;
        Variable *node = var.ptr->first;
        for (; node != 0;
             node = node->next){
            if (node->key == key){
                break;
            }
            prev = node;
        }
        
        if (node != 0){
            handle.ptr = node;
            _vars_free_variable_children(node);
        }
        else{
            handle.ptr = vars_free_variables;
            if (handle.ptr != 0){
                sll_stack_pop(vars_free_variables);
            }
            else{
                handle.ptr = push_array(&vars_arena, Variable, 1);
            }
            sll_queue_push(var.ptr->first, var.ptr->last, handle.ptr);
            handle.ptr->key = key;
        }
        
        handle.ptr->string = 0;
        handle.ptr->first = 0;
        handle.ptr->last = 0;
    }
    return(handle);
}

function Variable_Handle
vars_new_variable(Variable_Handle var, String_ID key, String_ID string){
    Variable_Handle result = vars_new_variable(var, key);
    vars_set_string(result, string);
    return(result);
}

function void
vars_clear_keys(Variable_Handle var){
    if (var.ptr != &vars_nil){
        _vars_free_variable_children(var.ptr);
    }
}

function void
vars_print_indented(Application_Links *app, Variable_Handle var, i32 indent){
    Scratch_Block scratch(app);
    local_persist char spaces[] =
        "                                                                "
        "                                                                "
        "                                                                "
        "                                                                ";
    
    String_Const_u8 var_key = vars_key_from_var(scratch, var);
    String_Const_u8 var_val = vars_string_from_var(scratch, var);
    
    String_Const_u8 line = push_stringf(scratch, "%.*s%.*s: \"%.*s\"\n",
                                        clamp_top(indent, sizeof(spaces)), spaces,
                                        string_expand(var_key),
                                        string_expand(var_val));
    print_message(app, line);
    
    i32 sub_indent = indent + 1;
    for (Variable_Handle sub = vars_first_child(var);
         !vars_is_nil(sub);
         sub = vars_next_sibling(sub)){
        vars_print_indented(app, sub, sub_indent);
    }
}

function void
vars_print(Application_Links *app, Variable_Handle var){
    vars_print_indented(app, var, 0);
}

// BOTTOM
