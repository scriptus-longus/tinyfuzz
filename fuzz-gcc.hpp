#ifndef INSTRUMENTATION_H
#define INSTRUMENTATION_H

#include "fuzz.hpp"
#include <sstream>
#include <string>

std::string get_shmem_init_code() {
  std::stringstream shmem_code_string;

  shmem_code_string << 
  "\n\t/* INIT SHARED MEMORY */\n"
  "\t.comm	_shmem_trace_buffer,8,8\n"
  "\t.globl	__init_shared_mem\n"
  "\t.type	__init_shared_mem, @function\n"
  "__init_shared_mem:\n"
  "\t.cfi_startproc\n"
  "\tpush %rax\n"
  "\tpush %rdi\n"
  "\tpush %rsi\n"
  "\tpush %rdx\n"
  "\tmov $29, %rax\n"
  "\tmov $0x1001, %rdi\n"
  "\tmov $" << (SHARED_MEM_SIZE*sizeof(int)) << ", %rsi\n"
  "\tmov $1408, %rdx\n"
  "\tsyscall\n"
  "\tmov %rax, %rdi\n"
  "\tmov $30, %rax\n"
  "\tmov $0x0, %rsi\n"
  "\tmov $0x0, %rdx\n"
  "\tsyscall\n"
  "\tmovq	%rax, _shmem_trace_buffer(%rip)\n"
  "\tnop\n"
  "\tpop %rdx\n"
  "\tpop %rsi\n"
  "\tpop %rdi\n"
  "\tpop %rax\n"
  "\tret\n"
  "\t.cfi_endproc\n"
  "\t.size	__init_shared_mem, .-__init_shared_mem\n"
  "\t/*   END   */\n\n";
  return shmem_code_string.str();
}

std::string get_jump_code(int n) {
  std::stringstream jump_instr_code;
  jump_instr_code <<
  "\n\t/* INSTRUMENTATION NR. " << n << " */\n"
  "\tmovq  _shmem_trace_buffer(%rip), %rax\n"
  "\taddq  $" << n*4 << ", %rax\n"
  "\tadd	$1, (%rax)\n"
  "\t/*   END   */\n\n";
  
  return jump_instr_code.str();
}

const char CALL_SHMEM_INIT_CODE[] = 
  "\n\t/*   CALL SHMEM INIT  */\n"
  "\tcall	__init_shared_mem\n"
  "\t/*   END   */\n\n";



#endif
