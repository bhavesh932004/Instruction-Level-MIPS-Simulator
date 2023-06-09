/*
 * sim.c
 * MIPS ISA simulator instruction implementation. 
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "sim.h"
#include "mips.h"
#include "shell.h"
#include "decode.h"

/* ----------------------------------------------------------------------------
	Instruction Handler Dipatch
*/
	
// function table, keyed by instruction function field
int (*FUNCTION_DISPATCH[DISPATCH_SIZE])();

// function table, keyed by instruction target field
int (*TARGET_DISPATCH[DISPATCH_SIZE])();

// function table, keyed by instruction opcode field
int (*OPCODE_DISPATCH[DISPATCH_SIZE])(); 

/* ----------------------------------------------------------------------------
	Local Prototypes 
*/

// by opcode 
int handle_j(uint32_t instr); 
int handle_jal(uint32_t instr); 
int handle_beq(uint32_t instr);
int handle_bne(uint32_t instr);
int handle_blez(uint32_t instr);
int handle_bgtz(uint32_t instr); 
int handle_addi(uint32_t instr);
int handle_addiu(uint32_t instr);
int handle_slti(uint32_t instr);
int handle_sltiu(uint32_t instr);
int handle_andi(uint32_t instr);
int handle_ori(uint32_t instr);
int handle_xori(uint32_t instr);
int handle_lui(uint32_t instr);
int handle_lb(uint32_t instr);
int handle_lh(uint32_t instr);
int handle_lw(uint32_t instr);
int handle_lbu(uint32_t instr);
int handle_lw(uint32_t instr); 
int handle_lhu(uint32_t instr);
int handle_sb(uint32_t instr);
int handle_sh(uint32_t instr);
int handle_sw(uint32_t instr);

// by function code 
int handle_bltz(uint32_t instr);
int handle_bgez(uint32_t instr);
int handle_sll(uint32_t instr); 
int handle_srl(uint32_t instr);
int handle_sra(uint32_t instr);
int handle_sllv(uint32_t instr); 
int handle_srlv(uint32_t instr);
int handle_srav(uint32_t instr);
int handle_jr(uint32_t instr);
int handle_jalr(uint32_t instr);
int handle_syscall(uint32_t instr);
int handle_mfhi(uint32_t instr);
int handle_mthi(uint32_t instr);
int handle_mflo(uint32_t instr);
int handle_mtlo(uint32_t instr); 
int handle_mult(uint32_t instr); 
int handle_multu(uint32_t instr);
int handle_div(uint32_t instr);
int handle_divu(uint32_t instr); 
int handle_add(uint32_t instr);
int handle_addu(uint32_t instr);
int handle_sub(uint32_t instr);
int handle_subu(uint32_t instr);
int handle_and(uint32_t instr);
int handle_or(uint32_t instr);
int handle_xor(uint32_t instr);
int handle_nor(uint32_t instr);
int handle_slt(uint32_t instr);
int handle_sltu(uint32_t instr);

// by target code 
int handle_bltz(uint32_t instr); 
int handle_bgez(uint32_t instr); 
int handle_bltzal(uint32_t instr); 
int handle_bgezal(uint32_t instr); 

// unrecognized codes 
int handle_unrecognized_opcode(uint32_t instr); 
int handle_unrecognized_function(uint32_t instr); 
int handle_unrecognized_target(uint32_t instr); 

/* ----------------------------------------------------------------------------
	Process Instruction (Entry Point)
*/

void process_instruction(void) {
	// read the instr from memory text segment
	uint32_t raw_instr = mem_read_32(CURRENT_STATE.PC); 
	printf("Instruction : %d\n", raw_instr);
	
	if(!raw_instr){
		RUN_BIT = 0;
		return;
	}
	
	// decode the opcode 
	int op = decode_opcode(raw_instr);
	printf("Opcode : %d\n", op);
	if (op == OPCODE_SPECIAL) {
		// for special instructions, decode the function value from instruction
		int func = decode_r_funct(raw_instr);
		// dispatch the appropriate instruction handler based on function
		(*FUNCTION_DISPATCH[func])(raw_instr);
	} else if (op == OPCODE_REGIMM) {
		// for regimm instructions, decode the target value from instruction
		int target = decode_i_rt(raw_instr);
		// dispatch the appropriate instruction handler based on function
		(*TARGET_DISPATCH[target])(raw_instr); 
	} else {
		// otherwise, dispatch the appropriate instruction handler based on opcode 
		(*OPCODE_DISPATCH[op])(raw_instr); 
	}
}

/* ----------------------------------------------------------------------------
	Instruction Handlers, by Opcode 
*/

/*
 * handle_j 
 * Jump 
 * Opcode: 2
 */
int handle_j(uint32_t instr) {
	// decode target address and shift left by 2 bits 
	uint32_t target = (decode_j_target(instr) << 2);

	// isolate high order bits of current address
	uint32_t current_addr = (CURRENT_STATE.PC & MASK_PC_HIGH); 

	// update the program counter unconditionally 
	NEXT_STATE.PC = current_addr + target; 

	return STATUS_OK; 
}

/*
 * handle_jal
 * Jump And Link
 * Opcode: 3
 */
int handle_jal(uint32_t instr) {
	// decode target address and shift left by 2 bits 
	uint32_t target = (decode_j_target(instr) << 2);

	// isolate high order bits of current address
	uint32_t current_addr = (CURRENT_STATE.PC & MASK_PC_HIGH);

	// update the program counter unconditionally 
	NEXT_STATE.PC = current_addr + target; 

	// place address of instruction after jump in link register 
	NEXT_STATE.REGS[REG_LINK] = CURRENT_STATE.PC + 4;

	return STATUS_OK;
}

/*
 * handle_beq
 * Branch On Equal
 * Opcode: 4
 */
int handle_beq(uint32_t instr) {
	// decode source and target registers 
	int rs = decode_i_rs(instr);
	int rt = decode_i_rt(instr);

	// decode offset, shift left 2 bits, and sign-extend 
	int32_t offset = (int32_t) (decode_i_immediate(instr) << 2); 

	if (CURRENT_STATE.REGS[rs] == CURRENT_STATE.REGS[rt]) {
		// if contents of source and target registers are equal, branch is taken
		NEXT_STATE.PC = CURRENT_STATE.PC + offset; 
	} else {
		// otherwise, not taken
		NEXT_STATE.PC = CURRENT_STATE.PC + 4;
	}

	return STATUS_OK;
}

/*
 * handle_bne
 * Branch On Not Equal Zero
 * Opcode: 5
 */
int handle_bne(uint32_t instr) {
	// decode source and target registers 
	int rs = decode_i_rs(instr);
	int rt = decode_i_rt(instr);

	// decode offset, shift left 2 bits, and sign-extend 
	int32_t offset = (int32_t) (decode_i_immediate(instr) << 2);

	if (CURRENT_STATE.REGS[rs] != CURRENT_STATE.REGS[rt]) {
		// if contents of source and taregt registers are not equal, branch is taken
		NEXT_STATE.PC = CURRENT_STATE.PC + offset;
	} else {
		// otherwise, not taken
		NEXT_STATE.PC = CURRENT_STATE.PC + 4;
	}

	return STATUS_OK; 
}

/*
 * handle_blez
 * Branch On Less Than Or Equal Zero
 * Opcode: 6
 */
int handle_blez(uint32_t instr) {
	// decode source register 
	int rs = decode_i_rs(instr);

	// decode offset, shift left 2 bits, and sign-extend 
	int32_t offset = (int32_t) (decode_i_immediate(instr) << 2); 

	if (CURRENT_STATE.REGS[rs] <= 0) {
		// if contents of source register less than or equal to zero, branch is taken
		NEXT_STATE.PC = CURRENT_STATE.PC + offset;
	} else {
		// otherwise, not taken
		NEXT_STATE.PC = CURRENT_STATE.PC + 4; 
	}

	return STATUS_OK; 
}

/*
 * handle_bgtz
 * Branch On Greater Than Zero
 * Opcode: 7
 */
int handle_bgtz(uint32_t instr) {
	// decode source register 
	int rs = decode_i_rs(instr);

	// decode offset, shift left 2 bits, and sign-extend 
	int32_t offset = (int32_t) (decode_i_immediate(instr) << 2);

	if (CURRENT_STATE.REGS[rs] > 0) {
		// if contents of source register greater than zero, branch is taken
		NEXT_STATE.PC = CURRENT_STATE.PC + offset;
	} else {
		// otherwise, not taken
		NEXT_STATE.PC = CURRENT_STATE.PC + 4;
	}

	return STATUS_OK;
}

/*
 * handle_addi
 * Add Immediate 
 * Opcode: 8
 */
int handle_addi(uint32_t instr) {
	// decode source and target register 
	int rs = decode_i_rs(instr);
	int rt = decode_i_rt(instr);

	// decode and sign-extend immediate value 
	int32_t immediate = (int32_t) decode_i_immediate(instr);

	// add contents of source register to immediate to form result
	// store result in target register 
	// NOTE: addi normally raises exception on overflow (and does not store in this case)
	// however, this functionality is not implemented here (per lab specs)
	NEXT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] + immediate;

	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK;
}

/*
 * handle_addiu
 * Add Immediate Unsigned 
 * Opcode: 9
 */
int handle_addiu(uint32_t instr) {
	printf("addiu called\n");
	// decode source and target registers 
	int rs = decode_i_rs(instr);
	int rt = decode_i_rt(instr);

	// decode and sign-extend immediate value 
	int32_t immediate = (int32_t) decode_i_immediate(instr);

	// add contents of source register to immediate to form result
	// store result in target register 
	// NOTE: addiu never causes overflow exception
	NEXT_STATE.REGS[rt] =  CURRENT_STATE.REGS[rs] + immediate; 

	// update the program counter to point to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK; 
}

/*
 * handle_slti
 * Set On Less Than Immediate 
 * Opcode: 10
 */
int handle_slti(uint32_t instr) {
	// decode source and target registers 
	int rs = decode_i_rs(instr);
	int rt = decode_i_rt(instr);

	// decode and sign-extend immediate value 
	int32_t immediate = (int32_t) decode_i_immediate(instr);

	if (((int32_t) CURRENT_STATE.REGS[rs]) < immediate) {
		// if, considering both quantities as signed integers, 
		// contents of source register are less than immediate, 
		// contents of target register set to 1
		NEXT_STATE.REGS[rt] = (uint32_t) 1;
	} else {
		// otherwise, set to 0 
		NEXT_STATE.REGS[rt] = (uint32_t) 0; 
	}

	// update the program counter to point to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK; 
}

/*
 * handle_sltiu
 * Set On Less Than Immediate Unsigned
 * Opcode: 11
 */
int handle_sltiu(uint32_t instr) {
	// decode source and target registers 
	int rs = decode_i_rs(instr);
	int rt = decode_i_rt(instr);

	// decode and sign-extend immediate value 
	int32_t immediate = (int32_t) decode_i_immediate(instr);

	if (CURRENT_STATE.REGS[rs] < ((uint32_t) immediate)) {
		// if, considering both quantities as unsigned integers, 
		// contents of source register are less than immediate, 
		// contents of target register set to 1
		NEXT_STATE.REGS[rt] = (uint32_t) 1;
	} else {
		// otherwise, set to 0 
		NEXT_STATE.REGS[rt] = (uint32_t) 0; 
	}

	// update the program counter to point to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK; 
}

/*
 * handle_andi
 * AND Immediate
 * Opcode: 12
 */
int handle_andi(uint32_t instr) {
	// decode source and target registers 
	int rs = decode_i_rs(instr);
	int rt = decode_i_rt(instr);

	// decode and zero-extend immediate value 
	uint32_t immediate = (uint32_t) decode_i_immediate(instr);

	// contents of source register and immediate combined in bitwise AND
	// store result in target register 
	NEXT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] & immediate;

	// update the program counter to point to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK; 
}

/*
 * handle_ori
 * OR Immediate  
 * Opcode: 13
 */
int handle_ori(uint32_t instr) {
	// decode source and target registers 
	int rs = decode_i_rs(instr);
	int rt = decode_i_rt(instr);

	// decode and zero-extend immediate value 
	uint32_t immediate = (uint32_t) decode_i_immediate(instr);

	// contents of source register and immediate combined in bitwise OR
	// store result in target register 
	NEXT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] | immediate;

	// update the program counter to point to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK; 
}

/*
 * handle_xori
 * Exclusive OR Immediate  
 * Opcode: 14
 */
int handle_xori(uint32_t instr) {
	// decode source and target registers 
	int rs = decode_i_rs(instr);
	int rt = decode_i_rt(instr);

	// decode and zero-extend immediate value 
	uint32_t immediate = (uint32_t) decode_i_immediate(instr);

	// contents of source register and immediate combined in bitwise XOR
	// store result in target register 
	NEXT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] ^ immediate;

	// update the program counter to point to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK; 
}

/*
 * handle_lui
 * Load Upper Immediate 
 * Opcode: 13
 */
int handle_lui(uint32_t instr) {
	// decode target register 
	int rt = decode_i_rt(instr);

	// decode immediate, left shift 16 bits 
	// likely don't need bitwise AND operation, but its insurance 
	int32_t immediate = (int32_t) ((decode_i_immediate(instr) << 16) & 0xFFFF0000); 

	// store immediate in target register 
	NEXT_STATE.REGS[rt] = immediate; 

	// update the program counter to point to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK; 
}

/*
 * handle_lb
 * Load Byte
 * Opcode: 32
 */
int handle_lb(uint32_t instr) {
	// decode base and target registers 
	int base = decode_i_rs(instr);
	int rt   = decode_i_rt(instr);

	// decode and sign extend offset  
	int32_t offset = (int32_t) decode_i_immediate(instr);

	// combine contents of base register and offset to form virtual address
	uint32_t address = CURRENT_STATE.REGS[base] + offset;

	// load 32 bit word at address, mask off all but low byte 
	int8_t byte = (int8_t) (mem_read_32(address) & 0x000000FF); 

	// store sign-extended result in target register 
	NEXT_STATE.REGS[rt] = (int32_t) byte; 

	// update the program counter to point to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4; 

	return STATUS_OK; 
}

/*
 * handle_lh
 * Load Halfword
 * Opcode: 33
 */
int handle_lh(uint32_t instr) {
	// decode base and target registers 
	int base = decode_i_rs(instr);
	int rt   = decode_i_rt(instr);

	// decode and sign extend offset  
	int32_t offset = (int32_t) decode_i_immediate(instr);

	// combine contents of base register and offset to form virtual address
	uint32_t address = CURRENT_STATE.REGS[base] + offset;

	// load 32 bit word at address, mask off all but low halfword
	int16_t halfword = (int16_t) (mem_read_32(address) & 0x0000FFFF);

	// store sign-extended result in target register 
	NEXT_STATE.REGS[rt] = (int32_t) halfword; 

	// update the program counter to point to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK; 
}

/*
 * handle_lw
 * Load Word
 * Opcode: 35
 */
int handle_lw(uint32_t instr) {
	// decode base and target registers 
	int base = decode_i_rs(instr);
	int rt   = decode_i_rt(instr);

	// decode and sign-extend offset 
	int32_t offset = (int32_t) decode_i_immediate(instr);

	// add offet to contents of base register to form address
	uint32_t address = CURRENT_STATE.REGS[base] + offset; 
		
	// load memory contents at effective address into target register 
	NEXT_STATE.REGS[rt] = mem_read_32(address); 

	// update the program counter to point to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4; 

	return STATUS_OK; 
}

/*
 * handle_lbu
 * Load Byte Unsigned
 * Opcode: 36
 */
int handle_lbu(uint32_t instr) {
	// decode base and target registers 
	int base = decode_i_rs(instr);
	int rt   = decode_i_rt(instr);

	// decode and sign extend offset  
	int32_t offset = (int32_t) decode_i_immediate(instr);

	// combine contents of base register and offset to form virtual address
	uint32_t address = CURRENT_STATE.REGS[base] + offset;

	// load 32 bit word at address, mask off all but low byte 
	uint8_t byte = (uint8_t) (mem_read_32(address) & 0x000000FF); 

	// store zero-extended result in target register 
	NEXT_STATE.REGS[rt] = (uint32_t) byte; 

	// update the program counter to point to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4; 

	return STATUS_OK; 
}

/*
 * handle_lhu
 * Load Halfword Unsigned
 * Opcode: 37
 */
int handle_lhu(uint32_t instr) {
	// decode base and target registers 
	int base = decode_i_rs(instr);
	int rt   = decode_i_rt(instr);

	// decode and sign extend offset  
	int32_t offset = (int32_t) decode_i_immediate(instr);

	// combine contents of base register and offset to form virtual address
	uint32_t address = CURRENT_STATE.REGS[base] + offset;

	// load 32 bit word at address, mask off all but low halfword
	uint16_t halfword = (uint16_t) (mem_read_32(address) & 0x0000FFFF);

	// store zero-extended result in target register 
	NEXT_STATE.REGS[rt] = (uint32_t) halfword; 

	// update the program counter to point to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK; 
}

/*
 * handle_sb
 * Store Byte 
 * Opcode: 40
 */
int handle_sb(uint32_t instr) {
	// decode base and target registers
	int base = decode_i_rs(instr);
	int rt   = decode_i_rt(instr);

	// decode and sign extend offset  
	int32_t offset = (int32_t) decode_i_immediate(instr);

	// combine contents of base register and offset to form virtual address
	uint32_t address = CURRENT_STATE.REGS[base] + offset;

	// isolate the low byte of target register 
	uint32_t byte = (CURRENT_STATE.REGS[rt] & 0x000000FF);

	// load current data at memory location and mask off low byte
	uint32_t mem = (mem_read_32(address) & 0xFFFFFF00);

	// store contents of target register at memory location specified by address
	// contents of low byte of target registered 'written' via bitwise AND
	mem_write_32(address, mem & byte);

	// update the program counter to point to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK; 
}

/*
 * handle_sh
 * Store Halfword
 * Opcode: 41
 */
int handle_sh(uint32_t instr) {
	// decode base and target registers
	int base = decode_i_rs(instr);
	int rt   = decode_i_rt(instr);

	// decode and sign extend offset  
	int32_t offset = (int32_t) decode_i_immediate(instr);

	// combine contents of base register and offset to form virtual address
	uint32_t address = CURRENT_STATE.REGS[base] + offset;

	// isolate the low halfword of target register 
	uint32_t byte = (CURRENT_STATE.REGS[rt] & 0x0000FFFF);

	// load current data at memory location and mask off low halfword
	uint32_t mem = (mem_read_32(address) & 0xFFFF0000);

	// store contents of target register at memory location specified by address
	// contents of low byte of target registered 'written' via bitwise AND
	mem_write_32(address, mem & byte);

	// update the program counter to point to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK; 
}

/*
 * handle_sw
 * Store Word
 * Opcode: 43
 */
int handle_sw(uint32_t instr) {
	// decode base and target registers
	int base = decode_i_rs(instr);
	int rt   = decode_i_rt(instr);

	// decode and sign extend offset  
	int32_t offset = (int32_t) decode_i_immediate(instr);

	// combine contents of base register and offset to form virtual address
	uint32_t address = CURRENT_STATE.REGS[base] + offset;

	// store contents of target register at memory location specified by address
	mem_write_32(address, CURRENT_STATE.REGS[rt]);

	// update the program counter to point to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK; 
}

/* ----------------------------------------------------------------------------
	Instruction Handlers, by Function (Special Instructions)
*/

/*
 * handle_sll
 * Shift Left Logical
 * Function: 0
 */
int handle_sll(uint32_t instr) {
	// decode target register, destination register, and shift amount
	int rt = decode_r_rt(instr);
	int rd = decode_r_rd(instr);
	int sa = decode_r_shamt(instr);

	// contents of target register shifted left by sa bits
	// store result in destination regiter
	NEXT_STATE.REGS[rd] = (CURRENT_STATE.REGS[rt] << sa); 

	// update the program counter to point to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK; 
} 

/*
 * handle_srl
 * Shift Right Logical
 * Function: 2
 */
int handle_srl(uint32_t instr) {
	// decode target register, destination register, and shift amount
	int rt = decode_r_rt(instr);
	int rd = decode_r_rd(instr);
	int sa = decode_r_shamt(instr);

	// contents of target register shifted left by sa bits
	// store result in destination regiter
	NEXT_STATE.REGS[rd] = (CURRENT_STATE.REGS[rt] >> sa);

	// update the program counter to point to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK; 
}

/*
 * handle_sra
 * Shift Right Arithmetic
 * Function: 3
 */
int handle_sra(uint32_t instr) {
	// decode target register, destination register, and shift amount
	int rt = decode_r_rt(instr);
	int rd = decode_r_rd(instr);
	int sa = decode_r_shamt(instr);

	uint32_t mask   = ( (~((int32_t) 0)) << (32 - sa) ); 
	uint32_t result = CURRENT_STATE.REGS[rt] >> sa; 

	// bitwise OR result with sign bit of original register content
	NEXT_STATE.REGS[rd] = result | mask;

	// update the program counter to point to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK; 
}

/*
 * handle_sllv
 * Shift Left Logical Variable
 * Function: 4
 */
int handle_sllv(uint32_t instr) {
	// decode source register, target register, and destination register
	int rs = decode_r_rs(instr);
	int rt = decode_r_rt(instr);
	int rd = decode_r_rd(instr);

	// shift amount determined by low order five bits of source register 
	int sa = (CURRENT_STATE.REGS[rs] & 0x000001F);

	// store result of left shift of target register content in destination register
	NEXT_STATE.REGS[rd] = (CURRENT_STATE.REGS[rt] << sa); 

	// update the program counter to point to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK; 
}

/*
 * handle_srlv
 * Shift Right Logical Variable
 * Function: 6
 */
int handle_srlv(uint32_t instr) {
	// decode source register, target register, and destination register
	int rs = decode_r_rs(instr);
	int rt = decode_r_rt(instr);
	int rd = decode_r_rd(instr);

	// shift amount determined by low order five bits of source register 
	int sa = (CURRENT_STATE.REGS[rs] & 0x000001F);

	// store result of left shift of target register content in destination register
	NEXT_STATE.REGS[rd] = (CURRENT_STATE.REGS[rt] >> sa); 

	// update the program counter to point to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK; 
}

/*
 * handle_srav
 * Shift Right Arithmetic Variable
 * Function: 7
 */
int handle_srav(uint32_t instr) {
	// decode source register, target register, and destination register
	int rs = decode_r_rs(instr);
	int rt = decode_r_rt(instr);
	int rd = decode_r_rd(instr);

	// shift amount determined by low order five bits of source register 
	int sa = (CURRENT_STATE.REGS[rs] & 0x000001F);

	uint32_t mask   = ( (~((int32_t) 0)) << (32 - sa) ); 
	uint32_t result = CURRENT_STATE.REGS[rt] >> sa; 

	// bitwise OR result with sign bit of original register content
	NEXT_STATE.REGS[rd] = result | mask;

	// update the program counter to point to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK; 
}

/*
 * handle_jr
 * Jump Register 
 * Function: 8
 */
int handle_jr(uint32_t instr) {
	// decode source register 
	int rs = decode_r_rs(instr);

	// unconditionally jump to address stored in source register 
	NEXT_STATE.PC = CURRENT_STATE.REGS[rs]; 

	return STATUS_OK;
}

/*
 * handle_jalr
 * Jump And Link Register 
 * Function: 9
 */
int handle_jalr(uint32_t instr) {
	// decode source register and destination register 
	int rs = decode_r_rs(instr);
	int rd = decode_r_rd(instr);

	// address of next sequential instruction stored in destination register 
	// NOTE: specs say the destination register may be ommitted by the assembler (why?)
	// and that, if this is the case, the link register (r31) is default 
	NEXT_STATE.REGS[rd] = CURRENT_STATE.PC + 4;

	// unconditionally jump to address stored in source register 
	NEXT_STATE.PC = CURRENT_STATE.REGS[rs];

	return STATUS_OK;  
}

/*
 * handle_syscall
 * System Call
 * Function: 12
 */
int handle_syscall(uint32_t instr) {
	if (CURRENT_STATE.REGS[REG_SYSCALL] == 0x0000000A) {
		// if syscall register has value 0x0A, halt 
		// otherwise, instruction has no effect
		RUN_BIT = 0; 
	} 
	
	// increment program counter to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK; 
}

/*
 * handle_mfhi
 * Move From Hi
 * Function: 16
 */
int handle_mfhi(uint32_t instr) {
	// decode destination register 
	int rd = decode_r_rd(instr);

	// contents of special register HI loaded into destination register 
	NEXT_STATE.REGS[rd] = CURRENT_STATE.HI;

	// update the program counter to point to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK;
}

/*
 * handle_mthi
 * Move To Hi
 * Function: 17
 */
int handle_mthi(uint32_t instr) {
	// decode source register
	int rs = decode_r_rs(instr);

	// contents of source register loaded into special register HI 
	NEXT_STATE.HI = CURRENT_STATE.REGS[rs];

	// update the program counter to point to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK;
}

/*
 * handle_mflo
 * Move From Lo
 * Function: 18
 */
int handle_mflo(uint32_t instr) {
	// decode destination register 
	int rd = decode_r_rd(instr);

	// contents of special register LO loaded into destination register 
	NEXT_STATE.REGS[rd] = CURRENT_STATE.LO;

	// update the program counter to point to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK;
}

/*
 * handle_mtlo
 * Move To Lo
 * Function: 19
 */
int handle_mtlo(uint32_t instr) {
	// decode source register
	int rs = decode_r_rs(instr);

	// contents of source register loaded into special register LO 
	NEXT_STATE.LO = CURRENT_STATE.REGS[rs];

	// update the program counter to point to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK;
}

/*
 * handle_mult
 * Multiply
 * Function: 24
 */
int handle_mult(uint32_t instr) {
	// decode source register and target register
	int rs = decode_r_rs(instr);
	int rt = decode_r_rt(instr);

	// treat both quantities as 32-bit signed values 
	int64_t source = (int64_t) CURRENT_STATE.REGS[rs];
	int64_t target = (int64_t) CURRENT_STATE.REGS[rt];

	// contents of source register multiplied by contents of target register 
	int64_t result = source * target;

	// high word of result stored in special register HI
	NEXT_STATE.HI = (result >> 32) & 0xFFFFFFFF;

	// low word of result stored in special register LO 
	NEXT_STATE.LO = (result >> 0)  & 0xFFFFFFFF; 

	// update the program counter to point to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK;
}

/*
 * handle_multu
 * Multiply Unsigned
 * Function: 25
 */
int handle_multu(uint32_t instr) {
	// decode source register and target register
	int rs = decode_r_rs(instr);
	int rt = decode_r_rt(instr);

	// treat both quantities as 32-bit unsigned values 
	uint64_t source = (uint64_t) CURRENT_STATE.REGS[rs];
	uint64_t target = (uint64_t) CURRENT_STATE.REGS[rt];

	// contents of source register multiplied by contents of target register 
	uint64_t result = source * target;

	// high word of result stored in special register HI
	NEXT_STATE.HI = (result >> 32) & 0xFFFFFFFF;

	// low word of result stored in special register LO 
	NEXT_STATE.LO = (result >> 0)  & 0xFFFFFFFF; 

	// update the program counter to point to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK;
}

/*
 * handle_div
 * Divide
 * Function: 26
 */
int handle_div(uint32_t instr) {
	// decode source register and target register
	int rs = decode_r_rs(instr);
	int rt = decode_r_rt(instr);

	// treat both quantities as 32-bit signed values 
	int64_t source = (int64_t) CURRENT_STATE.REGS[rs];
	int64_t target = (int64_t) CURRENT_STATE.REGS[rt];

	// contents of source register divided by contents of target register 
	int64_t result = source / target;

	// high word of result stored in special register HI
	NEXT_STATE.HI = (result >> 32) & 0xFFFFFFFF;

	// low word of result stored in special register LO 
	NEXT_STATE.LO = (result >> 0)  & 0xFFFFFFFF; 

	// update the program counter to point to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK;
}

/*
 * handle_divu
 * Divide Unsigned
 * Function: 27
 */
int handle_divu(uint32_t instr) {
	// decode source register and target register
	int rs = decode_r_rs(instr);
	int rt = decode_r_rt(instr);

	// treat both quantities as 32-bit unsigned values 
	uint64_t source = (uint64_t) CURRENT_STATE.REGS[rs];
	uint64_t target = (uint64_t) CURRENT_STATE.REGS[rt];

	// contents of source register divided by contents of target register 
	uint64_t result = source / target;

	// high word of result stored in special register HI
	NEXT_STATE.HI = (result >> 32) & 0xFFFFFFFF;

	// low word of result stored in special register LO 
	NEXT_STATE.LO = (result >> 0)  & 0xFFFFFFFF; 

	// update the program counter to point to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK;
}

/*
 * handle_add
 * Add 
 * Function: 32
 */
int handle_add(uint32_t instr) {
	// decode source, target, and destination registers
	int rs = decode_r_rs(instr);
	int rt = decode_r_rt(instr);
	int rd = decode_r_rd(instr);

	int32_t source = (int32_t) CURRENT_STATE.REGS[rs];
	int32_t target = (int32_t) CURRENT_STATE.REGS[rt];

	// contents of source and target registers added to form result 
	NEXT_STATE.REGS[rd] = source + target; 

	// update the program counter to point to next sequential instr
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK; 
}

/*
 * handle_addu
 * Add Unsigned 
 * Function: 33
 */
int handle_addu(uint32_t instr) {
	// decode source, target, and destination registers
	int rs = decode_r_rs(instr);
	int rt = decode_r_rt(instr);
	int rd = decode_r_rd(instr);

	int32_t source = (int32_t) CURRENT_STATE.REGS[rs];
	int32_t target = (int32_t) CURRENT_STATE.REGS[rt];

	// contents of source and target registers added to form result 
	// no overflow exception occurs under any circumtances 
	NEXT_STATE.REGS[rd] = source + target; 

	// update the program counter to point to next sequential instr 
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK; 
}

/*
 * handle_sub
 * Subtract
 * Function: 34
 */
int handle_sub(uint32_t instr) {
	// decode source, target, and destination registers
	int rs = decode_r_rs(instr);
	int rt = decode_r_rt(instr);
	int rd = decode_r_rd(instr);

	int32_t source = (int32_t) CURRENT_STATE.REGS[rs];
	int32_t target = (int32_t) CURRENT_STATE.REGS[rt];

	// contents of target subtracted from contents of soucre to form result  
	NEXT_STATE.REGS[rd] = source - target;

	// update the program counter to point to next sequential instr 
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK; 
}

/*
 * handle_subu
 * Subtract Unsigned 
 * Function: 35
 */
int handle_subu(uint32_t instr) {
	// decode source, target, and destination registers
	int rs = decode_r_rs(instr);
	int rt = decode_r_rt(instr);
	int rd = decode_r_rd(instr);

	int32_t source = (int32_t) CURRENT_STATE.REGS[rs];
	int32_t target = (int32_t) CURRENT_STATE.REGS[rt];

	// contents of target subtracted from contents of soucre to form result  
	// no overflow exception occurs under any circumtances 
	NEXT_STATE.REGS[rd] = source - target;

	// update the program counter to point to next sequential instr 
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK; 
}

/*
 * handle_and
 * And
 * Function: 36
 */
int handle_and(uint32_t instr) {
	// decode source, target, and destination registers
	int rs = decode_r_rs(instr);
	int rt = decode_r_rt(instr);
	int rd = decode_r_rd(instr);

	// contents of source register and target register combined in bitwise logical AND 
	// result stored in destination register 
	NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] & CURRENT_STATE.REGS[rt];

	// update the program counter to point to next sequential instr 
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK;
}

/*
 * handle_or
 * Or
 * Function: 37
 */
int handle_or(uint32_t instr) {
	// decode source, target, and destination registers
	int rs = decode_r_rs(instr);
	int rt = decode_r_rt(instr);
	int rd = decode_r_rd(instr);

	// contents of source register and target register combined in bitwise logical OR 
	// result stored in destination register 
	NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] | CURRENT_STATE.REGS[rt];

	// update the program counter to point to next sequential instr 
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK;
}

/*
 * handle_xor
 * Exclusive Or
 * Function: 38
 */
int handle_xor(uint32_t instr) {
	// decode source, target, and destination registers
	int rs = decode_r_rs(instr);
	int rt = decode_r_rt(instr);
	int rd = decode_r_rd(instr);

	// contents of source register and target register combined in bitwise logical XOR 
	// result stored in destination register 
	NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] ^ CURRENT_STATE.REGS[rt];

	// update the program counter to point to next sequential instr 
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK;
}

/*
 * handle_nor
 * Nor 
 * Function: 35
 */
int handle_nor(uint32_t instr) {
	// decode source, target, and destination registers
	int rs = decode_r_rs(instr);
	int rt = decode_r_rt(instr);
	int rd = decode_r_rd(instr);

	// contents of source register and target register combined in bitwise logical NOR 
	// result stored in destination register 
	NEXT_STATE.REGS[rd] = ~(CURRENT_STATE.REGS[rs] | CURRENT_STATE.REGS[rt]);

	// update the program counter to point to next sequential instr 
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK;
}
 
/*
 * handle_slt
 * Set On Less Than
 * Function: 42
 */
int handle_slt(uint32_t instr) {
	// decode source, target, and destination registers
	int rs = decode_r_rs(instr);
	int rt = decode_r_rt(instr);
	int rd = decode_r_rd(instr);

	// consider both register contents as signed integers
	int32_t source = (int32_t) CURRENT_STATE.REGS[rs];
	int32_t target = (int32_t) CURRENT_STATE.REGS[rt];

	if (source < target) {
		// if contents of source less than contents of target, result set to 1
		NEXT_STATE.REGS[rd] = 1;
	} else {
		// otherwise, result set to 0
		NEXT_STATE.REGS[rd] = 0; 
	}

	// update the program counter to point to next sequential instr 
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK;
}

/*
 * handle_sltu
 * Set On Less Than Unsigned 
 * Function: 43
 */
int handle_sltu(uint32_t instr) {
	// decode source, target, and destination registers
	int rs = decode_r_rs(instr);
	int rt = decode_r_rt(instr);
	int rd = decode_r_rd(instr);

	// consider both register contents as unsigned integers
	uint32_t source = (uint32_t) CURRENT_STATE.REGS[rs];
	uint32_t target = (uint32_t) CURRENT_STATE.REGS[rt];

	if (source < target) {
		// if contents of source less than contents of target, result set to 1
		NEXT_STATE.REGS[rd] = 1;
	} else {
		// otherwise, result set to 0
		NEXT_STATE.REGS[rd] = 0; 
	}

	// update the program counter to point to next sequential instr 
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;

	return STATUS_OK;
}

/* ----------------------------------------------------------------------------
	Instruction Handlers, by Target (Register-Immediate Instructions)
*/

/*
 * handle_bltz
 * Branch On Less Than Zero 
 * Target: 0
 */
int handle_bltz(uint32_t instr) {
	// decode source register 
	int rs = decode_i_rs(instr);

	// decode offset, shift left 2 bits, and sign extend
	int32_t offset = (int32_t) (decode_i_immediate(instr) << 2);

	if (CURRENT_STATE.REGS[rs] < 0) {
		// if contents of source register less than zero, branch is taken
		NEXT_STATE.PC = CURRENT_STATE.PC + offset;
	} else {
		// otherwise, not taken
		NEXT_STATE.PC = CURRENT_STATE.PC + 4;
	}

	return STATUS_OK; 
}

/*
 * handle_bgez
 * Branch On Greater Than Or Equal To Zero
 * Target: 1
 */
int handle_bgez(uint32_t instr) {
	// decode source register 
	int rs = decode_i_rs(instr);

	// decode offset, shift left 2 bits, and sign extend
	int32_t offset = (int32_t) (decode_i_immediate(instr) << 2);

	if (CURRENT_STATE.REGS[rs] >= 0) {
		// if contents of source register greater than or equal to zero, branch is taken
		NEXT_STATE.PC = CURRENT_STATE.PC + offset;
	} else {
		// otherwise, not taken
		NEXT_STATE.PC = CURRENT_STATE.PC + 4;
	}

	return STATUS_OK; 
}

/*
 * handle_bltzal
 * Branch On Less Than Zero And Link
 * Target: 16
 */
int handle_bltzal(uint32_t instr) {
	// decode source register 
	int rs = decode_i_rs(instr);

	// decode offset, shift left 2 bits, and sign extend
	int32_t offset = (int32_t) (decode_i_immediate(instr) << 2);

	// unconditionally, address of next instruction stored in link register 
	NEXT_STATE.REGS[REG_LINK] = CURRENT_STATE.PC + 4;

	if (CURRENT_STATE.REGS[rs] < 0) {
		// if contents of source register less than zero, branch is taken
		NEXT_STATE.PC = CURRENT_STATE.PC + offset;
	} else {
		// otherwise, not taken
		NEXT_STATE.PC = CURRENT_STATE.PC + 4;
	}

	return STATUS_OK; 
}

/*
 * handle_bgezal
 * Branch On Greater Than Or Equal To Zero And Link
 * Target: 17
 */
int handle_bgezal(uint32_t instr) {
	// decode source register 
	int rs = decode_i_rs(instr);

	// decode offset, shift left 2 bits, and sign extend
	int32_t offset = (int32_t) (decode_i_immediate(instr) << 2);

	// unconditionally, address of next instruction stored in link register 
	NEXT_STATE.REGS[REG_LINK] = CURRENT_STATE.PC + 4;

	if (CURRENT_STATE.REGS[rs] >= 0) {
		// if contents of source register greater than or equal to zero, branch is taken
		NEXT_STATE.PC = CURRENT_STATE.PC + offset;
	} else {
		// otherwise, not taken
		NEXT_STATE.PC = CURRENT_STATE.PC + 4;
	}

	return STATUS_OK; 
}

/* ----------------------------------------------------------------------------
	Unrecognized Instruction Handlers (Opcode and Function)
*/

/*
 * handle_unrecognized_opcode
 * Unrecognized Instruction Opcode
 * Opcode: any undefined opcode 
 */
int handle_unrecognized_opcode(uint32_t instr) {
	fprintf(stderr, "ERROR: unrecognzied opcode\n");
	return STATUS_ERR;
}

/*
 * handle_unrecognized_function
 * Unrecognized Instruction Function
 * Opcode: any undefined function 
 */
int handle_unrecognized_function(uint32_t instr) {
	fprintf(stderr, "ERROR: unrecognzied function\n");
	return STATUS_ERR;
}

/*
 * handle_unrecognized_target
 * Unrecognized Instruction Target
 * Opcode: any undefined target 
 */
int handle_unrecognized_target(uint32_t instr) {
	fprintf(stderr, "ERROR: unrecognzied target\n");
	return STATUS_ERR;
}

/* ----------------------------------------------------------------------------
	Instruction Handler Dispatch Setup 
*/

void init_opcode_dispatch(void) {
	memset(OPCODE_DISPATCH, 0, sizeof(void *)*DISPATCH_SIZE);

	// setup handler for unrecognized instructions
	// prevents segmentation faults on bad opcode decodes 
	for (int i = 0; i < DISPATCH_SIZE; i++) {
		OPCODE_DISPATCH[i] = handle_unrecognized_opcode;
	}

	// instruction handlers, in increasing order of opcode value
	OPCODE_DISPATCH[OPCODE_J]     = handle_j; 
	OPCODE_DISPATCH[OPCODE_JAL]   = handle_jal; 
	OPCODE_DISPATCH[OPCODE_BEQ]   = handle_beq; 
	OPCODE_DISPATCH[OPCODE_BNE]   = handle_bne; 
	OPCODE_DISPATCH[OPCODE_BLEZ]  = handle_blez; 
	OPCODE_DISPATCH[OPCODE_BGTZ]  = handle_bgtz;
	OPCODE_DISPATCH[OPCODE_ADDI]  = handle_addi;
	OPCODE_DISPATCH[OPCODE_ADDIU] = handle_addiu;
	OPCODE_DISPATCH[OPCODE_SLTI]  = handle_slti;
	OPCODE_DISPATCH[OPCODE_SLTIU] = handle_sltiu;
	OPCODE_DISPATCH[OPCODE_ANDI]  = handle_andi;
	OPCODE_DISPATCH[OPCODE_ORI]   = handle_ori; 
	OPCODE_DISPATCH[OPCODE_XORI]  = handle_xori;
	OPCODE_DISPATCH[OPCODE_LUI]   = handle_lui;
	OPCODE_DISPATCH[OPCODE_LB]    = handle_lb;
	OPCODE_DISPATCH[OPCODE_LH]    = handle_lh;
	OPCODE_DISPATCH[OPCODE_LW]    = handle_lw;
	OPCODE_DISPATCH[OPCODE_LBU]   = handle_lbu; 
	OPCODE_DISPATCH[OPCODE_LW]    = handle_lw; 
	OPCODE_DISPATCH[OPCODE_LHU]   = handle_lhu;
	OPCODE_DISPATCH[OPCODE_SB]    = handle_sb;
	OPCODE_DISPATCH[OPCODE_SH]    = handle_sh;
	OPCODE_DISPATCH[OPCODE_SW]    = handle_sw;
}

void init_function_dispatch(void) {
	memset(FUNCTION_DISPATCH, 0, sizeof(void *)*DISPATCH_SIZE);

	// setup handler for unrecognized instructions
	// prevents segmentation faults on bad function decodes 
	for (int i = 0; i < DISPATCH_SIZE; i++) {
		FUNCTION_DISPATCH[i] = handle_unrecognized_function;
	}

	// instruction handlers, in increasing order of function code value
	FUNCTION_DISPATCH[FUNC_SLL]     = handle_sll; 
	FUNCTION_DISPATCH[FUNC_SRL]     = handle_srl;
	FUNCTION_DISPATCH[FUNC_SRA]     = handle_sra;
	FUNCTION_DISPATCH[FUNC_SLLV]    = handle_sllv; 
	FUNCTION_DISPATCH[FUNC_SRLV]    = handle_srlv;
	FUNCTION_DISPATCH[FUNC_SRAV]    = handle_srav;
	FUNCTION_DISPATCH[FUNC_JR]      = handle_jr;
	FUNCTION_DISPATCH[FUNC_JALR]    = handle_jalr;
	FUNCTION_DISPATCH[FUNC_SYSCALL] = handle_syscall; 
	FUNCTION_DISPATCH[FUNC_MFHI]    = handle_mfhi;
	FUNCTION_DISPATCH[FUNC_MTHI]    = handle_mthi;
	FUNCTION_DISPATCH[FUNC_MFLO]    = handle_mflo;
	FUNCTION_DISPATCH[FUNC_MTLO]    = handle_mtlo;
	FUNCTION_DISPATCH[FUNC_MULT]    = handle_mult; 
	FUNCTION_DISPATCH[FUNC_MULTU]   = handle_multu; 
	FUNCTION_DISPATCH[FUNC_DIV]     = handle_div;
	FUNCTION_DISPATCH[FUNC_DIVU]    = handle_divu;
	FUNCTION_DISPATCH[FUNC_ADD]     = handle_add; 
	FUNCTION_DISPATCH[FUNC_ADDU]    = handle_addu;
	FUNCTION_DISPATCH[FUNC_SUB]     = handle_sub;
	FUNCTION_DISPATCH[FUNC_SUBU]    = handle_subu;
	FUNCTION_DISPATCH[FUNC_AND]     = handle_and;
	FUNCTION_DISPATCH[FUNC_OR]      = handle_or; 
	FUNCTION_DISPATCH[FUNC_XOR]     = handle_xor;
	FUNCTION_DISPATCH[FUNC_NOR]     = handle_nor; 
	FUNCTION_DISPATCH[FUNC_SLT]     = handle_slt;
	FUNCTION_DISPATCH[FUNC_SLTU]    = handle_sltu;
}

void init_target_dispatch(void) {
	memset(TARGET_DISPATCH, 0, sizeof(void *)*DISPATCH_SIZE);

	// setup handler for unrecognized instructions
	// prevents segmentation faults on bad function decodes 
	for (int i = 0; i < DISPATCH_SIZE; i++) {
		TARGET_DISPATCH[i] = handle_unrecognized_target;
	}

	// instruction handlers, in increasing order of target code value
	TARGET_DISPATCH[TARGET_BLTZ]   = handle_bltz; 
	TARGET_DISPATCH[TARGET_BGEZ]   = handle_bgez;
	TARGET_DISPATCH[TARGET_BLTZAL] = handle_bltzal;
	TARGET_DISPATCH[TARGET_BGEZAL] = handle_bgezal;
}




