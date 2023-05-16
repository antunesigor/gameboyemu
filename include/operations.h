#include "common.h"
#include "bus.h"

void ld_r1_r2(u8* reg_dest, u8* reg_src);

void ld_r_n(u8* reg_dest, u8 value);

void ld_r_addr(u8* reg_dest, u16 address);

void ld_addr_r(u16 address, u8* reg_src);

void ld_addr_n(u16 address, u8 value);

void ld_2r_nn(u8* reg_dest_start, u16 value);

void add_r_n(u8* reg_dest, u8 value);

void adc_r_n(u8* reg_dest, u8 value);

void sub_r_n(u8* reg_dest, u8 value);

void sbc_r_n(u8* reg_dest, u8 value);

void and_r_n(u8* reg_dest, u8 value);

void or_r_n(u8* reg_dest, u8 value);

void xor_r_n(u8* reg_dest, u8 value);

void cp_r_n(u8* reg_dest, u8 value);

void inc_r(u8* reg);

void dec_r(u8* reg);

void inc_addr(u16 address);

void dec_addr(u16 address);

void rrc_r(u8* reg);

void rr_r(u8* reg);

void rlc_r(u8* reg);

void rl_r(u8* reg);

void add_2r_nn(u8* reg_dest_start, u16 value);

void add_2r_sn(u8* reg_start, s8 value);