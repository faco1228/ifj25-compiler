/**
 * @file built_in_funs.c
 * @author Kristian Cilling (xcillik00)
 * @brief Implementation of functions generating built-in function in IFJcode25 code
 * @version 0.1
 * @date 2025-12-01
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <stdio.h>

/******************** Ifj.read_str ********************/

void gen_built_in_read_str() {
    // Prints the label where this function begins
    // The interpreter jumps here when calling Ifj.read_str
    printf("\nLABEL Ifj.read_str\n");
    
    // CREATEFRAME – creates a new TEMPORARY frame (TF)
    // Needed for the function’s local variables
    printf("CREATEFRAME\n");
    
    // PUSHFRAME – moves TF onto the frame stack
    // TF becomes the LOCAL frame (LF) for this function
    printf("PUSHFRAME\n");

    // DEFVAR – defines a new variable "retval" in LF
    // It is uninitialized at this point (has no value)
    printf("DEFVAR LF@retval\n");

    // READ – reads a value of type STRING from stdin
    // If EOF or error occurs -> stores nil@nil
    // Otherwise -> stores the read string
    printf("READ LF@retval string\n");

    // PUSHS – pushes the value of LF@retval onto the DATA stack
    // This is the function’s return value
    printf("PUSHS LF@retval\n");

    // POPFRAME – removes the current LF from the frame stack
    // Frees memory for local variables
    printf("POPFRAME\n");
    
    // RETURN – returns to the call site (uses the call stack)
    printf("RETURN\n");
}
/******************** Ifj.read_num ********************/

void gen_built_in_read_num() {
    printf("\nLABEL Ifj.read_num\n");
    printf("CREATEFRAME\n");
    printf("PUSHFRAME\n");

    printf("DEFVAR LF@retval\n");
    
    // Read number as float 
    // Returns nil@nil if input is not a valid number or EOF
    printf("READ LF@retval float\n");

    printf("PUSHS LF@retval\n");
    printf("POPFRAME\n");
    printf("RETURN\n");
}

/******************** Ifj.write ********************/

void gen_built_in_write() {
    printf("\nLABEL Ifj.write\n");
    printf("CREATEFRAME\n");
    printf("PUSHFRAME\n");

    printf("DEFVAR LF@arg\n");

    // Pop parameter from data stack (pushed by caller)
    printf("POPS LF@arg\n");

    // Write value to stdout with automatic formatting
    printf("WRITE LF@arg\n");

    // write() always returns null
    printf("PUSHS nil@nil\n");

    printf("POPFRAME\n");
    printf("RETURN\n");
}

/******************** Ifj.floor(term : Num) ********************/

void gen_built_in_floor() {
    printf("\nLABEL Ifj.floor\n");
    printf("CREATEFRAME\n");
    printf("PUSHFRAME\n");

    printf("DEFVAR LF@arg\n");
    printf("DEFVAR LF@type\n");
    printf("DEFVAR LF@result\n");

    printf("POPS LF@arg\n");

    // Type check: parameter must be Num (int or float)
    printf("TYPE LF@type LF@arg\n");
    printf("JUMPIFEQ _Ifj.floor_is_int LF@type string@int\n");
    printf("JUMPIFEQ _Ifj.floor_is_float LF@type string@float\n");
    printf("JUMP _Ifj.floor_type_err\n");

    // If already int, floor(x) = x
    printf("\nLABEL _Ifj.floor_is_int\n");
    printf("MOVE LF@result LF@arg\n");
    printf("JUMP _Ifj.floor_end_compute\n");

    // If float, truncate decimal part
    printf("\nLABEL _Ifj.floor_is_float\n");
    printf("FLOAT2INT LF@result LF@arg\n");

    printf("\nLABEL _Ifj.floor_end_compute\n");
    printf("PUSHS LF@result\n");
    printf("POPFRAME\n");
    printf("RETURN\n");

    // Runtime type error (exit code 25)
    printf("\nLABEL _Ifj.floor_type_err\n");
    printf("JUMP !ERROR_ARG_L\n");
}

/******************** Ifj.str(term) ********************/

void gen_built_in_str() {
    printf("\nLABEL Ifj.str\n");
    printf("CREATEFRAME\n");
    printf("PUSHFRAME\n");

    printf("DEFVAR LF@arg\n");
    printf("DEFVAR LF@type\n");
    printf("DEFVAR LF@result\n");
    printf("DEFVAR LF@is_int\n");
    printf("DEFVAR LF@tmp_int\n");

    printf("POPS LF@arg\n");
    printf("TYPE LF@type LF@arg\n");

    // STRING -> return unchanged
    printf("JUMPIFNEQ _Ifj.str_check_nil LF@type string@string\n");
    printf("MOVE LF@result LF@arg\n");
    printf("JUMP _Ifj.str_return\n");

    // NULL -> convert to string "null"
    printf("\nLABEL _Ifj.str_check_nil\n");
    printf("JUMPIFNEQ _Ifj.str_check_int LF@type string@nil\n");
    printf("MOVE LF@result string@null\n");
    printf("JUMP _Ifj.str_return\n");

    // INT -> convert using INT2STR
    printf("\nLABEL _Ifj.str_check_int\n");
    printf("JUMPIFNEQ _Ifj.str_check_float LF@type string@int\n");
    printf("INT2STR LF@result LF@arg\n");
    printf("JUMP _Ifj.str_return\n");

    // FLOAT -> check if whole number
    printf("\nLABEL _Ifj.str_check_float\n");
    printf("JUMPIFNEQ _Ifj.str_check_bool LF@type string@float\n");

    // Check if float has zero fractional part
    printf("ISINT LF@is_int LF@arg\n");
    printf("JUMPIFNEQ _Ifj.str_float_nonint LF@is_int bool@true\n");

    // Whole number float -> print as int (3.0 -> "3")
    printf("FLOAT2INT LF@tmp_int LF@arg\n");
    printf("INT2STR LF@result LF@tmp_int\n");
    printf("JUMP _Ifj.str_return\n");

    // Non-whole float -> use FLOAT2STR (%.2f format)
    printf("\nLABEL _Ifj.str_float_nonint\n");
    printf("FLOAT2STR LF@result LF@arg\n");
    printf("JUMP _Ifj.str_return\n");

    // BOOL -> convert to "true" or "false" (for extensions)
    printf("\nLABEL _Ifj.str_check_bool\n");
    printf("JUMPIFNEQ _Ifj.str_type_err LF@type string@bool\n");
    printf("JUMPIFEQ _Ifj.str_bool_true LF@arg bool@true\n");
    printf("MOVE LF@result string@false\n");
    printf("JUMP _Ifj.str_return\n");

    printf("\nLABEL _Ifj.str_bool_true\n");
    printf("MOVE LF@result string@true\n");
    printf("JUMP _Ifj.str_return\n");

    // Unknown type (should not happen)
    printf("\nLABEL _Ifj.str_type_err\n");
    printf("JUMP !ERROR_ARG_L\n");

    printf("\nLABEL _Ifj.str_return\n");
    printf("PUSHS LF@result\n");
    printf("POPFRAME\n");
    printf("RETURN\n");
}

/******************** Ifj.length(s : String) ********************/

void gen_built_in_length() {
    printf("\nLABEL Ifj.length\n");
    printf("CREATEFRAME\n");
    printf("PUSHFRAME\n");

    printf("DEFVAR LF@s\n");
    printf("DEFVAR LF@type\n");
    printf("DEFVAR LF@len\n");

    printf("POPS LF@s\n");

    // Type check: must be string
    printf("TYPE LF@type LF@s\n");
    printf("JUMPIFNEQ _Ifj.length_type_err LF@type string@string\n");

    // Get string length
    printf("STRLEN LF@len LF@s\n");
    
    printf("PUSHS LF@len\n");
    printf("POPFRAME\n");
    printf("RETURN\n");

    printf("\nLABEL _Ifj.length_type_err\n");
    printf("JUMP !ERROR_ARG_L\n");
}

/******************** Ifj.substring(s : String, i : Num, j : Num) ********************/

void gen_built_in_substring() {
    printf("\nLABEL Ifj.substring\n");
    printf("CREATEFRAME\n");
    printf("PUSHFRAME\n");

    // Variable definitions
    printf("DEFVAR LF@s\n");
    printf("DEFVAR LF@i\n");
    printf("DEFVAR LF@j\n");
    printf("DEFVAR LF@type\n");
    printf("DEFVAR LF@is_int\n");
    printf("DEFVAR LF@i_int\n");
    printf("DEFVAR LF@j_int\n");
    printf("DEFVAR LF@len\n");
    printf("DEFVAR LF@index\n");
    printf("DEFVAR LF@char\n");
    printf("DEFVAR LF@result\n");
    printf("DEFVAR LF@cmp\n");

    // Pop parameters in reverse order (LIFO stack - Pascal convention)
    // Caller pushes: s, i, j -> we pop: j, i, s
    printf("POPS LF@j\n");
    printf("POPS LF@i\n");
    printf("POPS LF@s\n");

    // Type check: s must be String
    printf("TYPE LF@type LF@s\n");
    printf("JUMPIFNEQ _Ifj.substring_type_err LF@type string@string\n");

    // Type check: i must be Num (int or float)
    printf("TYPE LF@type LF@i\n");
    printf("JUMPIFEQ _Ifj.substring_i_type_ok LF@type string@int\n");
    printf("JUMPIFEQ _Ifj.substring_i_type_ok LF@type string@float\n");
    printf("JUMP _Ifj.substring_type_err\n");
    printf("\nLABEL _Ifj.substring_i_type_ok\n");

    // Type check: j must be Num
    printf("TYPE LF@type LF@j\n");
    printf("JUMPIFEQ _Ifj.substring_j_type_ok LF@type string@int\n");
    printf("JUMPIFEQ _Ifj.substring_j_type_ok LF@type string@float\n");
    printf("JUMP _Ifj.substring_type_err\n");
    printf("\nLABEL _Ifj.substring_j_type_ok\n");

    // Check if i and j are integers (exit 26 if not)
    printf("ISINT LF@is_int LF@i\n");
    printf("JUMPIFNEQ _Ifj.substring_not_int LF@is_int bool@true\n");
    printf("ISINT LF@is_int LF@j\n");
    printf("JUMPIFNEQ _Ifj.substring_not_int LF@is_int bool@true\n");

    // Convert i to int (if float with zero fractional part)
    printf("TYPE LF@type LF@i\n");
    printf("JUMPIFEQ _Ifj.substring_i_already_int LF@type string@int\n");
    printf("FLOAT2INT LF@i_int LF@i\n");
    printf("JUMP _Ifj.substring_i_int_done\n");
    printf("\nLABEL _Ifj.substring_i_already_int\n");
    printf("MOVE LF@i_int LF@i\n");
    printf("\nLABEL _Ifj.substring_i_int_done\n");

    // Convert j to int
    printf("TYPE LF@type LF@j\n");
    printf("JUMPIFEQ _Ifj.substring_j_already_int LF@type string@int\n");
    printf("FLOAT2INT LF@j_int LF@j\n");
    printf("JUMP _Ifj.substring_j_int_done\n");
    printf("\nLABEL _Ifj.substring_j_already_int\n");
    printf("MOVE LF@j_int LF@j\n");
    printf("\nLABEL _Ifj.substring_j_int_done\n");

    // Get string length
    printf("STRLEN LF@len LF@s\n");

    // Validations (return null if any fails):
    // 1. i < 0
    printf("LT LF@cmp LF@i_int int@0\n");
    printf("JUMPIFEQ _Ifj.substring_return_null LF@cmp bool@true\n");
    
    // 2. j < 0
    printf("LT LF@cmp LF@j_int int@0\n");
    printf("JUMPIFEQ _Ifj.substring_return_null LF@cmp bool@true\n");
    
    // 3. i > j
    printf("GT LF@cmp LF@i_int LF@j_int\n");
    printf("JUMPIFEQ _Ifj.substring_return_null LF@cmp bool@true\n");
    
    // 4. i >= length(s)
    printf("LT LF@cmp LF@i_int LF@len\n");
    printf("JUMPIFEQ _Ifj.substring_return_null LF@cmp bool@false\n");
    
    // 5. j > length(s)
    printf("GT LF@cmp LF@j_int LF@len\n");
    printf("JUMPIFEQ _Ifj.substring_return_null LF@cmp bool@true\n");

    // Build substring by iterating from i to j-1
    printf("MOVE LF@index LF@i_int\n");
    printf("MOVE LF@result string@\n");

    printf("\nLABEL _Ifj.substring_loop\n");
    // Loop condition: index < j
    printf("LT LF@cmp LF@index LF@j_int\n");
    printf("JUMPIFEQ _Ifj.substring_loop_end LF@cmp bool@false\n");

    // Get character at current index and append to result
    printf("GETCHAR LF@char LF@s LF@index\n");
    printf("CONCAT LF@result LF@result LF@char\n");
    
    // Increment index
    printf("ADD LF@index LF@index int@1\n");
    printf("JUMP _Ifj.substring_loop\n");

    printf("\nLABEL _Ifj.substring_loop_end\n");
    printf("PUSHS LF@result\n");
    printf("POPFRAME\n");
    printf("RETURN\n");

    // Return null on validation failure
    printf("\nLABEL _Ifj.substring_return_null\n");
    printf("PUSHS nil@nil\n");
    printf("POPFRAME\n");
    printf("RETURN\n");

    // Type error (exit 25)
    printf("\nLABEL _Ifj.substring_type_err\n");
    printf("JUMP !ERROR_ARG_L\n");

    // Integer check failed (exit 26)
    printf("\nLABEL _Ifj.substring_not_int\n");
    printf("JUMP !ERROR_EXP_L\n");
}

/******************** Ifj.strcmp(s1 : String, s2 : String) ********************/

void gen_built_in_strcmp() {
    printf("\nLABEL Ifj.strcmp\n");
    printf("CREATEFRAME\n");
    printf("PUSHFRAME\n");

    printf("DEFVAR LF@s1\n");
    printf("DEFVAR LF@s2\n");
    printf("DEFVAR LF@type\n");
    printf("DEFVAR LF@len1\n");
    printf("DEFVAR LF@len2\n");
    printf("DEFVAR LF@index\n");
    printf("DEFVAR LF@c1\n");
    printf("DEFVAR LF@c2\n");
    printf("DEFVAR LF@cmp\n");
    printf("DEFVAR LF@result\n");

    // Pop parameters (Pascal convention: s1, s2 -> pop s2, s1)
    printf("POPS LF@s2\n");
    printf("POPS LF@s1\n");

    // Type check: both parameters must be strings
    printf("TYPE LF@type LF@s1\n");
    printf("JUMPIFNEQ _Ifj.strcmp_type_err LF@type string@string\n");
    printf("TYPE LF@type LF@s2\n");
    printf("JUMPIFNEQ _Ifj.strcmp_type_err LF@type string@string\n");

    // Get lengths of both strings
    printf("STRLEN LF@len1 LF@s1\n");
    printf("STRLEN LF@len2 LF@s2\n");

    printf("MOVE LF@index int@0\n");

    // Lexicographic comparison: compare character by character
    printf("\nLABEL _Ifj.strcmp_loop\n");
    // Exit loop if we've reached end of either string
    printf("LT LF@cmp LF@index LF@len1\n");
    printf("JUMPIFEQ _Ifj.strcmp_after_loop LF@cmp bool@false\n");
    printf("LT LF@cmp LF@index LF@len2\n");
    printf("JUMPIFEQ _Ifj.strcmp_after_loop LF@cmp bool@false\n");

    // Get ASCII values of characters at current index
    printf("STRI2INT LF@c1 LF@s1 LF@index\n");
    printf("STRI2INT LF@c2 LF@s2 LF@index\n");

    // Compare characters
    printf("LT LF@cmp LF@c1 LF@c2\n");
    printf("JUMPIFEQ _Ifj.strcmp_s1_lt_s2 LF@cmp bool@true\n");

    printf("LT LF@cmp LF@c2 LF@c1\n");
    printf("JUMPIFEQ _Ifj.strcmp_s1_gt_s2 LF@cmp bool@true\n");

    // Characters equal, continue to next
    printf("ADD LF@index LF@index int@1\n");
    printf("JUMP _Ifj.strcmp_loop\n");

    // All compared characters equal, decide by length
    printf("\nLABEL _Ifj.strcmp_after_loop\n");
    printf("LT LF@cmp LF@len1 LF@len2\n");
    printf("JUMPIFEQ _Ifj.strcmp_s1_lt_s2 LF@cmp bool@true\n");
    printf("LT LF@cmp LF@len2 LF@len1\n");
    printf("JUMPIFEQ _Ifj.strcmp_s1_gt_s2 LF@cmp bool@true\n");

    // Strings are equal
    printf("MOVE LF@result int@0\n");
    printf("JUMP _Ifj.strcmp_return\n");

    printf("\nLABEL _Ifj.strcmp_s1_lt_s2\n");
    printf("MOVE LF@result int@-1\n");
    printf("JUMP _Ifj.strcmp_return\n");

    printf("\nLABEL _Ifj.strcmp_s1_gt_s2\n");
    printf("MOVE LF@result int@1\n");
    printf("JUMP _Ifj.strcmp_return\n");

    printf("\nLABEL _Ifj.strcmp_return\n");
    printf("PUSHS LF@result\n");
    printf("POPFRAME\n");
    printf("RETURN\n");

    printf("\nLABEL _Ifj.strcmp_type_err\n");
    printf("JUMP !ERROR_ARG_L\n");
}

/******************** Ifj.ord(s : String, i : Num) ********************/

void gen_built_in_ord() {
    printf("\nLABEL Ifj.ord\n");
    printf("CREATEFRAME\n");
    printf("PUSHFRAME\n");

    printf("DEFVAR LF@s\n");
    printf("DEFVAR LF@i\n");
    printf("DEFVAR LF@type\n");
    printf("DEFVAR LF@is_int\n");
    printf("DEFVAR LF@i_int\n");
    printf("DEFVAR LF@len\n");
    printf("DEFVAR LF@cmp\n");
    printf("DEFVAR LF@result\n");

    // Pop parameters (Pascal: s, i -> pop i, s)
    printf("POPS LF@i\n");
    printf("POPS LF@s\n");

    // Type check: s must be String
    printf("TYPE LF@type LF@s\n");
    printf("JUMPIFNEQ _Ifj.ord_type_err LF@type string@string\n");

    // Type check: i must be Num
    printf("TYPE LF@type LF@i\n");
    printf("JUMPIFEQ _Ifj.ord_i_type_ok LF@type string@int\n");
    printf("JUMPIFEQ _Ifj.ord_i_type_ok LF@type string@float\n");
    printf("JUMP _Ifj.ord_type_err\n");
    printf("\nLABEL _Ifj.ord_i_type_ok\n");

    // Check if i is integer (exit 26 if not)
    printf("ISINT LF@is_int LF@i\n");
    printf("JUMPIFNEQ _Ifj.ord_not_int LF@is_int bool@true\n");

    // Convert i to int
    printf("TYPE LF@type LF@i\n");
    printf("JUMPIFEQ _Ifj.ord_i_already_int LF@type string@int\n");
    printf("FLOAT2INT LF@i_int LF@i\n");
    printf("JUMP _Ifj.ord_i_int_done\n");
    printf("\nLABEL _Ifj.ord_i_already_int\n");
    printf("MOVE LF@i_int LF@i\n");
    printf("\nLABEL _Ifj.ord_i_int_done\n");

    printf("STRLEN LF@len LF@s\n");

    // Return 0 if: empty string, i < 0, or i >= length
    printf("JUMPIFEQ _Ifj.ord_return_zero LF@len int@0\n");

    printf("LT LF@cmp LF@i_int int@0\n");
    printf("JUMPIFEQ _Ifj.ord_return_zero LF@cmp bool@true\n");

    printf("LT LF@cmp LF@i_int LF@len\n");
    printf("JUMPIFEQ _Ifj.ord_return_zero LF@cmp bool@false\n");

    // Valid index: get ASCII value of character
    printf("STRI2INT LF@result LF@s LF@i_int\n");
    printf("PUSHS LF@result\n");
    printf("POPFRAME\n");
    printf("RETURN\n");

    printf("\nLABEL _Ifj.ord_return_zero\n");
    printf("PUSHS int@0\n");
    printf("POPFRAME\n");
    printf("RETURN\n");

    printf("\nLABEL _Ifj.ord_type_err\n");
    printf("JUMP !ERROR_ARG_L\n");

    printf("\nLABEL _Ifj.ord_not_int\n");
    printf("JUMP !ERROR_EXP_L\n");
}

/******************** Ifj.chr(i : Num) ********************/

void gen_built_in_chr() {
    printf("\nLABEL Ifj.chr\n");
    printf("CREATEFRAME\n");
    printf("PUSHFRAME\n");

    printf("DEFVAR LF@i\n");
    printf("DEFVAR LF@type\n");
    printf("DEFVAR LF@is_int\n");
    printf("DEFVAR LF@i_int\n");
    printf("DEFVAR LF@result\n");

    printf("POPS LF@i\n");

    // Type check: i must be Num
    printf("TYPE LF@type LF@i\n");
    printf("JUMPIFEQ _Ifj.chr_i_type_ok LF@type string@int\n");
    printf("JUMPIFEQ _Ifj.chr_i_type_ok LF@type string@float\n");
    printf("JUMP _Ifj.chr_type_err\n");
    printf("\nLABEL _Ifj.chr_i_type_ok\n");

    // Check if i is integer (exit 26 if not)
    printf("ISINT LF@is_int LF@i\n");
    printf("JUMPIFNEQ _Ifj.chr_not_int LF@is_int bool@true\n");

    // Convert i to int
    printf("TYPE LF@type LF@i\n");
    printf("JUMPIFEQ _Ifj.chr_i_already_int LF@type string@int\n");
    printf("FLOAT2INT LF@i_int LF@i\n");
    printf("JUMP _Ifj.chr_i_int_done\n");
    printf("\nLABEL _Ifj.chr_i_already_int\n");
    printf("MOVE LF@i_int LF@i\n");
    printf("\nLABEL _Ifj.chr_i_int_done\n");

    // Convert ASCII value to character 
    printf("INT2CHAR LF@result LF@i_int\n");
    printf("PUSHS LF@result\n");
    printf("POPFRAME\n");
    printf("RETURN\n");

    printf("\nLABEL _Ifj.chr_type_err\n");
    printf("JUMP !ERROR_ARG_L\n");

    printf("\nLABEL _Ifj.chr_not_int\n");
    printf("JUMP !ERROR_EXP_L\n");
}
