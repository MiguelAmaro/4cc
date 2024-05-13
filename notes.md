# Custom Layer

## Basics

`win32_4ed.cpp` runs a sequence of code before the  **main application loop** to load the custom layer `DLL` . That dll has a function to get a **VTable** defined by the custom layer code and pass to the main application. The main application will make use of those functions referenced in the **VTable** for their altered behavior.

```c
// located in: 4ed.h
struct App_Functions{
    App_Load_VTables *load_vtables;
    App_Get_Logger *get_logger;
    App_Read_Command_Line *read_command_line;
    App_Init *init;
    App_Step *step;
};
```

If  the custom layer dll successfully loaded and the custom layer init proc is found then the function pointer to init the custom layer is passed into function called `App_Init`.

is passed in the 

```c
#define App_Init_Sig(name) \
    void name(Thread_Context *tctx,     \
    Render_Target *target,    \
    void *base_ptr,           \
    String_Const_u8 current_directory,\
    Custom_API api)
```

The `custom_api.h` file is a generated header file that defines `App_Init` so that it can used by the custom layer code and is included in the custom layer **dll** compilation. In the main application the actual name of the function that fills out the `VTable` and calls the custome layer init file is called `app_init`.

```c
// found in: 4ed.cpp
App_Init_Sig(app_init){
  Models *models = (Models*)base_ptr;
  models->keep_playing = true;
  models->hard_exit = false;
  // ... omitted section 
  // ...
  API_VTable_custom custom_vtable = {};
  custom_api_fill_vtable(&custom_vtable);
  API_VTable_system system_vtable = {};
  system_api_fill_vtable(&system_vtable);
  Custom_Layer_Init_Type *custom_init = api.init_apis(&custom_vtable, &system_vtable);
  Assert(custom_init != 0);
  //... omitted section
  //...
```

The `custom_api_fill_vtable(API_VTable_custom *vtable)` call assigns the funtion pointers with the addresses of functions that are defined by the main application(4coder core).  This is essentially defines 4coder default behavior.

Then the VTable is passed to `api.init_apis(&custom_vtable, &system_vtable)` which has the following implementation.

```c
// found in: 4coder_custom.cpp
init_apis(API_VTable_custom *custom_vtable, API_VTable_system *system_vtable){
    custom_api_read_vtable(custom_vtable);
    system_api_read_vtable(system_vtable);
    return(custom_layer_init);
}
```

The `custom_api_read_vtable(custom_vtable);` call fills out a **set** of global function pointers that are referenced by the 4coder custom layer. It uses the values of the VTable that was passed in by the main application(4coder core) to make sure that the custom code can make use of the 4coder's default behavior. The **set** of function pointers that exist in the VTable and **globally** are essentially what make up the interface of the  **4coder API**.

The `system_api_read_vtable(system_vtable);` does the reverse of filling out the VTable with the **custom layer global function pointers**. This essentially doesn't do anything except give the **custom layer** a point in the code to **override the function pointer** so that it is pointing to a **custom layer defined function** rather than the function originally defined by the main application(4coder core).  

## Hooks

## Custom Commands

Custom commands are definded using the signiture
`void name(struct Application_Links *app)`

where   `Application_Links` just has a **thread context** and a generic pointer for a **command context**. 

```c
// found in: 4coder_types.h
api(custom)
struct Application_Links{
    Thread_Context *tctx;
    void *cmd_context;
};
```

The **command context** is just a void pointer that gets casted to a `Models` type in the `set_custom_hook` implementation.

```c
// found in: 4ed_api_implementations.cpp
api(custom) function void
set_custom_hook(Application_Links *app, Hook_ID hook_id, Void_Func *func_ptr){
    Models *models = (Models*)app->cmd_context;
    switch (hook_id){
        case HookID_BufferViewerUpdate:
        {
            models->buffer_viewer_update = (Hook_Function*)func_ptr;
        }break;
        case HookID_DeltaRule:
        {
            models->delta_rule = (Delta_Rule_Function*)func_ptr;
        }break;
```

The struct `API_VTable_custom` contains the **set** of function pointers of the **4coder API**.  

Examples of setting up hooks can be found in `4coder_default_hooks.cpp`

```c
// found in: 4coder_default_hooks.cpp
internal void
set_all_default_hooks(Application_Links *app){
  set_custom_hook(app, HookID_BufferViewerUpdate, default_view_adjust);

  set_custom_hook(app, HookID_ViewEventHandler, default_view_input_handler);
  set_custom_hook(app, HookID_Tick, default_tick);
```

In the Vtable are function for handling custom commands such as

```c
// found in: custom_api_master_list.h
// a function prototype for the set_custom_hook function
api(custom) function void set_custom_hook(Application_Links* app, Hook_ID hook_id, Void_Func* func_ptr);


// found in: custom_api.h
// type definition of a pointer to a function like custom_set_custom_hook
typedef void custom_set_custom_hook_type(Application_Links* app, Hook_ID hook_id, Void_Func* func_ptr);

// function pointer member in API_VTable_custom
custom_set_custom_hook_type *set_custom_hook;

// a forward decleration to the function set_custom_hook
internal void set_custom_hook(Application_Links* app, Hook_ID hook_id, Void_Func* func_ptr);

// a global fucntion pointer to a 'custom_set_custom_hook' function
global custom_set_custom_hook_type *set_custom_hook = 0;
```

This function takes in a void function pointer.

```c
#define HOOK_SIG(name) i32 name(Application_Links *app)
#define BUFFER_HOOK_SIG(name) i32 name(Application_Links *app, Buffer_ID buffer_id)
```

```c
// found in: 4coder_types.h
typedef i32 Hook_ID;
enum{
    HookID_Tick,
    HookID_RenderCaller,
    HookID_WholeScreenRenderCaller,
    HookID_DeltaRule,
    HookID_BufferViewerUpdate,
    HookID_ViewEventHandler,
    HookID_BufferNameResolver,
    HookID_BeginBuffer,
    HookID_EndBuffer,
    HookID_NewFile,
    HookID_SaveFile,
    HookID_BufferEditRange,
    HookID_BufferRegion,
    HookID_Layout,
    HookID_ViewChangeBuffer,
};
```

## Languages

The `default_begin_buffer` function defines how a buffer will be process while it is open and visable in the editor. This function is passed a `buffer id`  and that is used to get the extension which will be used to determined how the texted should be layed out and if lexer for a programming language should be used.
The `buffer_set_layout(app, buffer_id, layout_virt_indent_literal_generic);` function is called when the **.cpp, .h, .c, ect...** extensions are called however I believe this function is tailored to `c` and `c++` syntax.

```c
// found in: 4coder_default_hooks.cpp
BUFFER_HOOK_SIG(default_begin_buffer){
  ProfileScope(app, "begin buffer");

  Scratch_Block scratch(app);

  b32 treat_as_code = false;
  String_Const_u8 file_name = push_buffer_file_name(app, scratch, buffer_id);
  if (file_name.size > 0){
    String_Const_u8 treat_as_code_string = def_get_config_string(scratch, vars_save_string_lit("treat_as_code"));
    String_Const_u8_Array extensions = parse_extension_line_to_extension_list(app, scratch, treat_as_code_string);
    String_Const_u8 ext = string_file_extension(file_name);
    for (i32 i = 0; i < extensions.count; ++i){
      if (string_match(ext, extensions.strings[i])){

        if (string_match(ext, string_u8_litexpr("cpp")) ||
            string_match(ext, string_u8_litexpr("h")) ||
            string_match(ext, string_u8_litexpr("c")) ||
            string_match(ext, string_u8_litexpr("hpp")) ||
            string_match(ext, string_u8_litexpr("cc"))){
          treat_as_code = true;
        //... ommitted section
        //...
#if 0 // not compiled in
        if (string_match(ext, string_u8_litexpr("cs"))){
          if (parse_context_language_cs == 0){
            init_language_cs(app);
          }
          parse_context_id = parse_context_language_cs;
        }

        if (string_match(ext, string_u8_litexpr("java"))){
          if (parse_context_language_java == 0){
            init_language_java(app);
          }
          parse_context_id = parse_context_language_java;
        }
        //...
#endif
    }
    //...
    if (treat_as_code){
      buffer_set_layout(app, buffer_id, layout_virt_indent_literal_generic);
    }
    else{
      buffer_set_layout(app, buffer_id, layout_generic);
    }
    //...
    return
}
```

The `Model` struct contains references to most of the essential pieces of infomation and functionality used by 4coder. This include references to some of the **hook**able functions.

```c

struct Models{
    Arena arena_;
    Arena *arena;
    Heap heap;
    
    App_Settings settings;
    App_State state;
    
    Face_ID global_face_id;
    
    Coroutine_Group coroutines;
    Model_Wind_Down_Co *wind_down_stack;
    Model_Wind_Down_Co *free_wind_downs;
    
    Child_Process_Container child_processes;
    Custom_API config_api;
    
    Tick_Function *tick;
    Render_Caller_Function *render_caller;
    Whole_Screen_Render_Caller_Function *whole_screen_render_caller;
    Delta_Rule_Function *delta_rule;
    u64 delta_rule_memory_size;
    
    Hook_Function *buffer_viewer_update;
    Custom_Command_Function *view_event_handler;
    Buffer_Name_Resolver_Function *buffer_name_resolver;
    Buffer_Hook_Function *begin_buffer;
    Buffer_Hook_Function *end_buffer;
    Buffer_Hook_Function *new_file;
    Buffer_Hook_Function *save_file;
    Buffer_Edit_Range_Function *buffer_edit_range;
    Buffer_Region_Function *buffer_region;
    Layout_Function *layout_func;
    View_Change_Buffer_Function *view_change_buffer;
    
```
