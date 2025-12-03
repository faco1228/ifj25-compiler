/**
 * @file built_in_funs.h
 * @author Kristian Cilling (xcillik00)
 * @brief Header file for built_in_funs.h containg function prototypes
 * @version 0.1
 * @date 2025-12-01
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef _BUILT_IN_FUNS_
#define _BUILT_IN_FUNS_

// Functions declarations for built-in functions code generation

/**
 * @brief Generates IFJcode25 implementation of built-in function IFJ_read_str.
 *
 * Behavior:
 *  - Reads one string from stdin (`READ LF@retval string`)
 *  - Pushes result on data stack
 *  - Returns to caller
 *
 * Semantics:
 *  - On EOF -> returns nil@nil
 *  - Uses local frame for variable `retval`
 */
void gen_built_in_read_str();

/**
 * @brief Generates IFJcode25 implementation of built-in function IFJ_read_num.
 *
 * Behavior:
 *  - Reads a numeric literal (float)
 *  - If input is EOF/invalid -> returns nil
 *  - If input is whole float (3.0) -> converts to int
 *  - Else returns float unchanged
 *
 * Uses variables:
 *  - retval – raw input value
 *  - is_whole – bool indicating if float has zero decimal part
 */
void gen_built_in_read_num();

/**
 * @brief Generates IFJcode25 implementation of built-in function IFJ_write.
 *
 * Behavior:
 *  - Pops one argument from data stack
 *  - Prints it using WRITE instruction
 *  - Pushes nil@nil as return value
 */
void gen_built_in_write();

/**
 * @brief Generates IFJcode25 implementation of built-in function IFJ_floor.
 *
 * Behavior:
 *  - Accepts Num (int or float)
 *  - If int -> returns argument unchanged
 *  - If float -> truncates fractional part using FLOAT2INT
 *  - Otherwise -> runtime error !ERROR_ARG_L (exit 25)
 */
void gen_built_in_floor();

/**
 * @brief Generates IFJcode25 implementation of built-in function IFJ_str.
 *
 * Behavior:
 *  - Converts argument to string:
 *      STRING  -> unchanged
 *      NIL     -> "null"
 *      INT     -> decimal string
 *      FLOAT   -> "n" or "n.n"
 *      BOOL    -> "true" or "false"
 *  - Otherwise -> !ERROR_ARG_L (25)
 *
 * Includes logic for formatting whole floats as integers.
 */
void gen_built_in_str();

/**
 * @brief Generates IFJcode25 implementation of built-in function IFJ_length.
 *
 * Behavior:
 *  - Accepts string
 *  - Returns its length using STRLEN
 *  - For invalid type -> !ERROR_ARG_L (25)
 */
void gen_built_in_length();

/**
 * @brief Generates IFJcode25 implementation of built-in function IFJ_substring.
 *
 * Parameters:
 *  - s -> String
 *  - i -> Num (int or float, must be whole)
 *  - j -> Num (int or float, must be whole)
 *
 * Behavior:
 *  - Performs substring from i to j-1
 *  - Validates bounds (returns nil on invalid interval)
 *  - Validates numeric integer types (exit 26 on float with fraction)
 *  - On type mismatch -> !ERROR_ARG_L (25)
 */
void gen_built_in_substring();

/**
 * @brief Generates IFJcode25 implementation of built-in function IFJ_strcmp.
 *
 * Behavior:
 *  - Lexicographically compares two strings
 *  - Returns:
 *      -1 if s1 <  s2
 *       0 if s1 == s2
 *       1 if s1 >  s2
 *  - Type mismatch → !ERROR_ARG_L (25)
 */
void gen_built_in_strcmp();

/**
 * @brief Generates IFJcode25 built-in function IFJ_ord.
 *
 * Behavior:
 *  - Accepts (string s, Num i)
 *  - i must be integer (else exit 26)
 *  - If index valid -> return ASCII code of char at s[i]
 *  - Else return 0
 *  - Type mismatch -> exit 25
 */
void gen_built_in_ord();

/**
 * @brief Generates IFJcode25 built-in function IFJ_chr.
 *
 * Behavior:
 *  - Converts integer ASCII code to character
 *  - Accepts Num (int/float, must be whole)
 *  - Type mismatch -> exit 25
 *  - Noninteger -> exit 26
 */
void gen_built_in_chr();

#endif