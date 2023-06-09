/*
 * decode.c
 * Instruction decode helpers.
 */

#include <stdint.h>
#include <stdlib.h>

#include "mips.h"

/* ----------------------------------------------------------------------------
	Opcode (All Instruction Types)
	See module header file (decode.h) for detailed function comments. 
*/

int decode_opcode(uint32_t instr) {
	// opcode is always top 6 bits of instr 
	return (int) ((instr & MASK_OPCODE) >> SHIFT_OPCODE); 
}

/* ----------------------------------------------------------------------------
	I-Type Instructions 
	See module header file (decode.h) for detailed function comments. 
*/

int decode_i_rs(uint32_t instr) {
	return (int) ((instr & MASK_I_RS) >> SHIFT_I_RS);
}


int decode_i_rt(uint32_t instr) {
	return (int) ((instr & MASK_I_RT) >> SHIFT_I_RT);
}

int16_t decode_i_immediate(uint32_t instr) {
	return (int16_t) ((instr & MASK_I_IMMEDIATE) >> SHIFT_I_IMMEDIATE); 
}

/* ----------------------------------------------------------------------------
	J-Type Instructions 
	See module header file (decode.h) for detailed function comments. 
*/

uint32_t decode_j_target(uint32_t instr) {
	return (uint32_t) ((instr & MASK_J_TARGET) >> SHIFT_J_TARGET); 
}

int decode_j_funct(uint32_t instr) {
	return (int) ((instr & MASK_J_FUNCT) >> SHIFT_J_FUNCT);
}

/* ----------------------------------------------------------------------------
	R-Type Instructions 
	See module header file (decode.h) for detailed function comments. 
*/

int decode_r_rs(uint32_t instr) {
	return (int) ((instr & MASK_R_RS) >> SHIFT_R_RS);
}

int decode_r_rt(uint32_t instr) {
	return (int) ((instr & MASK_R_RT) >> SHIFT_R_RT);
}

int decode_r_rd(uint32_t instr) {
	return (int) ((instr & MASK_R_RD) >> SHIFT_R_RD);
}

int decode_r_shamt(uint32_t instr) {
	return (int) ((instr & MASK_R_SHAMT) >> SHIFT_R_SHAMT); 
}

int decode_r_funct(uint32_t instr) {
	return (int) ((instr & MASK_R_FUNCT) >> SHIFT_R_FUNCT); 
}
