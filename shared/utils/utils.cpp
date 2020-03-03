// thx https://github.com/jbro129/Unity-Substrate-Hook-Android

#include <jni.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "utils.h"
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <new>

using namespace std;

// https://developer.arm.com/docs/ddi0596/a/a64-shared-pseudocode-functions/aarch64-instrs-pseudocode#impl-aarch64.DecodeBitMasks
// Explanation at https://dinfuehr.github.io/blog/encoding-of-immediate-values-on-aarch64/
uint64_t DecodeBitMasks(unsigned N, unsigned imms, unsigned immr, unsigned regSize) {
    auto len = HighestSetBit((N << 6) | trunc(~imms, 6), 7);
    if (len < 1) return -1;

    unsigned size = 1 << len;
    unsigned levels = size - 1;  // a real bitmask of the rightmost "size" bits
    unsigned R = immr & levels;
    unsigned S = imms & levels;
    // For logical immediates an all-ones value of S is reserved since it would generate a useless all-ones result
    if (S == levels) return -1;

    uint64_t pattern = (1ULL << (S + 1)) - 1;
    pattern = ROR(pattern, size, R);
    // Replicate the pattern to fill regSize
    while (size < regSize) {
        pattern |= (pattern << size);
        size *= 2;
    }
    return pattern;
}

static const uint_fast8_t RLINK = 30;
static const uint_fast8_t RZR = 31;
static const uint_fast8_t SP = 31;
std::ostream& operator<<(std::ostream& os, const Register& regName) {
    auto reg = regName.num;
    if (reg == SP || reg == RZR) {
        if (regName.r31_is_sp) {
            os << "SP";
        } else {
            os << "XZR";
        }
    } else if (reg == RLINK) {
        os << "R30 (link register)";
    } else if (reg == 29) {
        os << "R29 (frame ptr)";
    } else {
        os << "R" << std::dec << +reg;
        if (reg < 0 || reg >= 32) os << " (invalid register!)";
    }
    return os;
}

std::string Register::toString() {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, const Instruction& inst) {
    os << std::showbase;
    if (inst.parseLevel <= 0) return os << "Unparsable";
    os << '"' << inst.kind[inst.parseLevel - 1] << '"';
    if (inst.valid) {
        if (inst.imm) os << ", imm: " << std::hex << inst.imm;
        if (inst.Rd >= 0) os << ", destination register: " << Register(inst.Rd, inst.RdCanBeSP);
        if (inst.Rd2 >= 0) os << ", destination register 2: " << Register(inst.Rd2, inst.RdCanBeSP);
        if (inst.numSourceRegisters == 0) {
            if (inst.Rd >= 0) os << ", result: " << std::hex << inst.result;
        } else if (inst.numSourceRegisters > 0) {
            os << ", source registers: ";
            for (int j = 0; j < inst.numSourceRegisters; j++) {
                if (j != 0) os << ", ";
                os << Register(inst.Rs[j], j ? false : inst.Rs0CanBeSP);
            }
        }
        if (inst.branchType != Instruction::NOBRANCH) {
            os << ", branch type: " << Instruction::branchTypeInfo[inst.branchType];
        }
    }
    return os << std::noshowbase;
}

std::string Instruction::toString() {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

static const char* unalloc = "UNALLOCATED";
Instruction::Instruction(const int32_t* inst) {
    addr = inst;
    auto pc = (unsigned long long)inst;
    parseLevel = 0;
    parsed = false;
    auto code = *inst;
    // https://developer.arm.com/docs/ddi0596/a/top-level-encodings-for-a64#top
    uint_fast8_t top0 = bits(code, 28, 25);  // op0 for top-level only
    log(DEBUG, "inst: ptr = 0x%llX (offset 0x%llX), bytes = 0x%X, top-level op0: %i",
        pc, pc - getRealOffset(0), code, top0);
    // Bit patterns like 1x0x where x is any bit and all other bits must match are implemented by:
    // 1. (a & [1's where pattern has non-x]) == [pattern with x's as 0]
    // 2. (a | [1's where pattern has x])     == [pattern with x's as 1]
    //   2. is usually the shorter option when x's are mostly on the right, otherwise 1.
    if (top0 <= 3) {
        for (int i = 0; i < sizeof(kind) / sizeof(kind[0]); i++) {
            kind[parseLevel++] = "Invalid instruction";
            valid = false;
        }
    } else if ((top0 & 0b111) == 0b101) {  // x101
        // https://developer.arm.com/docs/ddi0596/a/top-level-encodings-for-a64/data-processing-register
        kind[parseLevel++] = "Data Processing -- Register";
        bool op0 = bits(code, 30, 30);
        bool op1 = bits(code, 28, 28);
        uint_fast8_t op2 = bits(code, 24, 21);
        uint_fast8_t op3 = bits(code, 15, 10);
        // not listed at the top level but all subcategories have it
        bool sf = bits(code, 31, 31);
        if (op1 == 0) {
            numSourceRegisters = 2;
            Rd = bits(code, 4, 0);
            auto Rn = Rs[0] = bits(code, 9, 5);
            auto Rm = Rs[1] = bits(code, 20, 16);
            if ((op2 & 0b1000) == 0) {  // 0xxx
                // https://developer.arm.com/docs/ddi0596/a/top-level-encodings-for-a64/data-processing-register#log_shift
                kind[parseLevel++] = "Logical (shifted register)";
                RdCanBeSP = Rs0CanBeSP = false;
                imm = bits(code, 15, 10);  // "imm6"
                shiftType = static_cast<ShiftType>(bits(code, 23, 22));  // "shift"

                bool N = bits(code, 21, 21);
                uint_fast8_t opc = bits(code, 30, 29);
                if (opc == 1) {
                    if (N == 0) {
                        if ((shiftType == LSL) && (imm == 0) && Rn == RZR) {
                            kind[parseLevel++] = "MOV (register)";  // preferred alias
                            if (Rm == RZR) {
                                numSourceRegisters = 0;
                                result = 0;
                            } else {
                                Rs[0] = Rs[1];
                                numSourceRegisters = 1;
                            }
                        } else {
                            if (sf == 0) {
                                kind[parseLevel++] = "ORR (shifted register) — 32-bit";
                            } else {
                                kind[parseLevel++] = "ORR (shifted register) — 64-bit";
                            }
                        }
                    }
                }
            } else {  // op2 == 1xxx
                bool op = bits(code, 30, 30);
                bool S = bits(code, 29, 29);
                if ((op2 & 0b1) == 0) {  // 1xx0
                    // https://developer.arm.com/docs/ddi0596/a/top-level-encodings-for-a64/data-processing-register#addsub_shift
                    kind[parseLevel++] = "Add/subtract (shifted register)";
                    RdCanBeSP = Rs0CanBeSP = false;
                    imm = bits(code, 15, 10);  // "imm6"
                    shiftType = static_cast<ShiftType>(bits(code, 23, 22));  // "shift"

                    if ((shiftType == ROR) || ((sf == 0) && ((imm & 0b100000) != 0))) {
                        kind[parseLevel++] = unalloc;
                    } else {
                        // TODO: if sf == 0, all regs are 32-bit views
                        if (op == 0) {
                            if (S == 0) {
                                kind[parseLevel++] = sf ? "ADD (shifted register) — 64-bit" : "ADD (shifted register) — 32-bit";
                            } else {
                                if (Rd == RZR) {
                                    // https://developer.arm.com/docs/ddi0596/a/a64-base-instructions-alphabetic-order/cmn-shifted-register-compare-negative-shifted-register-an-alias-of-adds-shifted-register
                                    kind[parseLevel++] = "CMN (shifted register)";  // preferred alias
                                    Rd = -1;
                                } else {
                                    kind[parseLevel++] = sf ? "ADDS (shifted register) — 64-bit" : "ADDS (shifted register) — 32-bit";
                                }
                            }
                        } else {
                            if (S == 0) {
                                if (Rn == RZR) {
                                    // https://developer.arm.com/docs/ddi0596/a/a64-base-instructions-alphabetic-order/neg-shifted-register-negate-shifted-register-an-alias-of-sub-shifted-register
                                    kind[parseLevel++] = "NEG (shifted register)";  // preferred alias
                                    Rs[0] = Rs[1];
                                    numSourceRegisters = 1;
                                } else {
                                    kind[parseLevel++] = sf ? "SUB (shifted register) — 64-bit" : "SUB (shifted register) — 32-bit";
                                }
                            } else {
                                if (Rd == RZR) {
                                    // https://developer.arm.com/docs/ddi0596/a/a64-base-instructions-alphabetic-order/cmp-shifted-register-compare-shifted-register-an-alias-of-subs-shifted-register
                                    kind[parseLevel++] = "CMP (shifted register)";  // preferred alias
                                    Rd = -1;
                                } else if (Rn == RZR) {
                                    // https://developer.arm.com/docs/ddi0596/a/a64-base-instructions-alphabetic-order/negs-negate-setting-flags-an-alias-of-subs-shifted-register
                                    kind[parseLevel++] = "NEGS";  // preferred alias
                                    Rs[0] = Rs[1];
                                    numSourceRegisters = 1;
                                } else {
                                    kind[parseLevel++] = sf ? "SUBS (shifted register) — 64-bit" : "SUBS (shifted register) — 32-bit";
                                }
                            }
                        }
                    }
                    log(DEBUG, "op1 = 0, op0: %i, op2: %i (1xxx), op3: %i", op0, op2, op3);
                } else {
                    // https://developer.arm.com/docs/ddi0596/a/top-level-encodings-for-a64/data-processing-register#addsub_ext
                    kind[parseLevel++] = "Add/subtract (extended register)";
                    RdCanBeSP = (!S);
                    Rs0CanBeSP = true;
                    uint_fast8_t opt = bits(code, 23, 22);
                    extendType = static_cast<ExtendType>(bits(code, 15, 13));  // "option"
                    imm = bits(code, 12, 10);  // "imm3"

                    if (Rn == SP) {
                        if (sf == 0) {
                            if (extendType == UXTW) shiftType = LSL;
                        } else {
                            if (extendType == UXTX) shiftType = LSL;
                        }
                    }

                    if ((opt != 0) || (imm > 4)) {
                        kind[parseLevel++] = unalloc;
                    } else {
                        // TODO: if sf == 0, all regs are 32-bit views; else all are 64-bit except Rm is a 32-bit view iff option != x11
                        if (op == 0) {
                            if (S == 0) {
                                kind[parseLevel++] = sf ? "ADD (shifted register) — 64-bit" : "ADD (shifted register) — 32-bit";
                            } else {
                                if (Rd == RZR) {
                                    // https://developer.arm.com/docs/ddi0596/a/a64-base-instructions-alphabetic-order/cmn-shifted-register-compare-negative-shifted-register-an-alias-of-adds-shifted-register
                                    kind[parseLevel++] = "CMN (shifted register)";  // preferred alias
                                    Rd = -1;
                                } else {
                                    kind[parseLevel++] = sf ? "ADDS (shifted register) — 64-bit" : "ADDS (shifted register) — 32-bit";
                                }
                            }
                        } else {
                            if (S == 0) {
                                if (Rn == RZR) {
                                    // https://developer.arm.com/docs/ddi0596/a/a64-base-instructions-alphabetic-order/neg-shifted-register-negate-shifted-register-an-alias-of-sub-shifted-register
                                    kind[parseLevel++] = "NEG (shifted register)";  // preferred alias
                                    Rs[0] = Rs[1];
                                    numSourceRegisters = 1;
                                } else {
                                    kind[parseLevel++] = sf ? "SUB (shifted register) — 64-bit" : "SUB (shifted register) — 32-bit";
                                }
                            } else {
                                if (Rd == RZR) {
                                    // https://developer.arm.com/docs/ddi0596/a/a64-base-instructions-alphabetic-order/cmp-shifted-register-compare-shifted-register-an-alias-of-subs-shifted-register
                                    kind[parseLevel++] = "CMP (shifted register)";  // preferred alias
                                    Rd = -1;
                                } else if (Rn == RZR) {
                                    // https://developer.arm.com/docs/ddi0596/a/a64-base-instructions-alphabetic-order/negs-negate-setting-flags-an-alias-of-subs-shifted-register
                                    kind[parseLevel++] = "NEGS";  // preferred alias
                                    Rs[0] = Rs[1];
                                    numSourceRegisters = 1;
                                } else {
                                    kind[parseLevel++] = sf ? "SUBS (shifted register) — 64-bit" : "SUBS (shifted register) — 32-bit";
                                }
                            }
                        }
                    }
                }
            }
        } else {  // op1 == 1
            if (op2 == 0b100) {
                // https://developer.arm.com/docs/ddi0596/a/top-level-encodings-for-a64/data-processing-register#condsel
                kind[parseLevel++] = "Conditional select";
                RdCanBeSP = Rs0CanBeSP = false;
                numSourceRegisters = 2;
                Rd = bits(code, 4, 0);
                auto Rn = Rs[0] = bits(code, 9, 5);
                cond = bits(code, 15, 12);
                auto Rm = Rs[1] = bits(code, 20, 16);

                bool op = op0;
                bool S = bits(code, 29, 29);
                op2 = bits(code, 11, 10);

                if (S || ((op2 & 0b10) != 0)) {  // 1x
                    kind[parseLevel++] = unalloc;
                } else if (op == 0) {
                    if (op2 == 0) {
                        kind[parseLevel++] = sf ? "CSEL — 64-bit" : "CSEL — 32-bit";
                    } else {
                        kind[parseLevel++] = sf ? "CSINC — 64-bit" : "CSINC — 32-bit";
                    }
                } else {  // op == 1
                    if (op2 == 0) {
                        kind[parseLevel++] = sf ? "CSINV — 64-bit" : "CSINV — 32-bit";
                    } else {
                        kind[parseLevel++] = sf ? "CSNEG — 64-bit" : "CSNEG — 32-bit";
                    }
                }
            } else {
                log(DEBUG, "op1 = 1, op0: %i, op2: %i, op3: %i", op0, op2, op3);
            }
        }
    } else if ((top0 & 0b111) == 0b111) {  // x111
        // https://developer.arm.com/docs/ddi0596/a/top-level-encodings-for-a64/data-processing-scalar-floating-point-and-advanced-simd
        kind[parseLevel++] = "Data Processing -- Scalar Floating-Point and Advanced SIMD";
    } else if ((top0 | 0b1) == 0b1001) {  // 100x
        // https://developer.arm.com/docs/ddi0596/a/top-level-encodings-for-a64/data-processing-immediate
        kind[parseLevel++] = "Data Processing -- Immediate";
        Rd = bits(code, 4, 0);
        bool sf = bits(code, 31, 31);
        uint_fast8_t op0 = bits(code, 25, 24);
        uint_fast8_t op1 = bits(code, 23, 22);
        if (op0 == 0) {
            // https://developer.arm.com/docs/ddi0596/a/top-level-encodings-for-a64/data-processing-immediate#pcreladdr
            kind[parseLevel++] = "PC-rel. addressing";
            RdCanBeSP = false;
            numSourceRegisters = 0;
            bool op = sf;
            const uint_fast8_t ilh = 30, ill = 29, ihh = 23, ihl = 5;
            uint_fast8_t immlo = bits(code, ilh, ill);
            auto immhi = bits(code, ihh, ihl);
            log(DEBUG, "immhi: 0x%X (%i), immlo: 0x%X (%i)", immhi, immhi, immlo, immlo);
            auto immI = (immhi << (ilh - ill + 1)) + immlo;
            uint_fast8_t immINumBits = ihh - ihl + 1 + ilh - ill + 1;
            if (op == 0b1) {
                kind[parseLevel++] = "ADRP";
                const uint_fast8_t zeros = 12;
                immI <<= zeros;
                immINumBits += zeros;
                pc = (pc >> zeros) << zeros;  // zero out the last 12 bits
            } else {
                kind[parseLevel++] = "ADR";
            }
            log(DEBUG, "imm initial: 0x%X (%i); immNumBits: %i", immI, immI, immINumBits);
            imm = SignExtend<int64_t>(immI, immINumBits);
            result = pc + imm;
            log(DEBUG, "imm: 0x%lX; result: 0x%lX (offset 0x%llX)", imm, result, result - getRealOffset(0));
        } else if (op0 == 0b1) {
            numSourceRegisters = 1;
            Rs0CanBeSP = true;
            Rs[0] = bits(code, 9, 5);
            bool op = bits(code, 30, 30);
            bool S = bits(code, 29, 29);
            if ((op1 | 0b1) == 0b11) {  // 1x
                // https://developer.arm.com/docs/ddi0596/a/top-level-encodings-for-a64/data-processing-immediate#addsub_immtags
                kind[parseLevel++] = "Add/subtract (immediate, with tags)";
                RdCanBeSP = true;
                if ((sf == 0) || ((sf == 1) && (S == 1))) {
                    kind[parseLevel++] = unalloc;
                }
            } else {
                // https://developer.arm.com/docs/ddi0596/a/top-level-encodings-for-a64/data-processing-immediate#addsub_imm
                kind[parseLevel++] = "Add/subtract (immediate)";
                RdCanBeSP = !S;
                auto shift = op1;
                uint_fast16_t imm12 = bits(code, 21, 10);
                imm = ZeroExtend<int64_t>(imm12, 12) << 12 * shift;
                if (op == 0) {
                    if (S == 0) {
                        if ((imm == 0) && ((Rd == SP) || (Rs[0] == SP))) {
                            // https://developer.arm.com/docs/ddi0596/a/a64-base-instructions-alphabetic-order/mov-tofrom-sp-move-between-register-and-stack-pointer-an-alias-of-add-immediate
                            kind[parseLevel++] = "MOV (to/from SP)";  // preferred alias
                        } else {
                            kind[parseLevel++] = sf ? "ADD (immediate) — 64-bit" : "ADD (immediate) — 32-bit";
                        }
                    } else {
                        if (Rd == RZR) {
                            // https://developer.arm.com/docs/ddi0596/a/a64-base-instructions-alphabetic-order/cmn-immediate-compare-negative-immediate-an-alias-of-adds-immediate
                            kind[parseLevel++] = "CMN (immediate)";  // preferred alias
                            Rd = -1;
                        } else {
                            kind[parseLevel++] = sf ? "ADDS (immediate) — 64-bit" : "ADDS (immediate) — 32-bit";
                        }
                    }
                } else {
                    if (S == 0) {
                        kind[parseLevel++] = sf ? "SUB (immediate) — 64-bit" : "SUB (immediate) — 32-bit";
                    } else {
                        if (Rd == RZR) {
                            // https://developer.arm.com/docs/ddi0596/a/a64-base-instructions-alphabetic-order/cmp-immediate-compare-immediate-an-alias-of-subs-immediate
                            kind[parseLevel++] = "CMP (immediate)";  // preferred alias
                            Rd = -1;
                        } else {
                            kind[parseLevel++] = sf ? "SUBS (immediate) — 64-bit" : "SUBS (immediate) — 32-bit";
                        }
                    }
                }
            }
        } else if (op0 == 0b10) {
            uint_fast8_t opc = bits(code, 30, 29);
            if ((op1 & 0b10) == 0) {  // 0x
                // https://developer.arm.com/docs/ddi0596/a/top-level-encodings-for-a64/data-processing-immediate#log_imm
                kind[parseLevel++] = "Logical (immediate)";
                RdCanBeSP = (opc != 0b11);  // for all but ANDS
                Rs0CanBeSP = false;
                bool N = bits(code, 22, 22);
                uint_fast8_t immr = bits(code, 21, 16);
                uint_fast8_t imms = bits(code, 15, 10);
                uint_fast8_t Rn = bits(code, 9, 5);
                imm = DecodeBitMasks(N, imms, immr, sf ? 64 : 32);
                log(DEBUG, "N: %i, immr: 0x%X, imms: 0x%X, decoded imm: 0x%lX", N, immr, imms, imm);
                if (imm == -1) valid = false;

                if (Rn == RZR) {
                    numSourceRegisters = 0;
                } else {
                    numSourceRegisters = 1;
                    Rs[0] = Rn;
                }

                if ((sf == 0) && (N == 1)) {
                    kind[parseLevel++] = unalloc;
                } else if (opc == 0) {
                    if (Rn == RZR) result = 0;
                    if (sf == 0) {
                        kind[parseLevel++] = "AND (immediate) — 32-bit";
                    } else {
                        kind[parseLevel++] = "AND (immediate) — 64-bit";
                    }
                } else if (opc == 0b1) {
                    if (Rn == RZR) result = imm;
                    if (sf == 0) {
                        kind[parseLevel++] = "ORR (immediate) — 32-bit";
                    } else {
                        kind[parseLevel++] = "ORR (immediate) — 64-bit";
                    }
                } else if (opc == 0b10) {
                    if (Rn == RZR) result = imm;
                    if (sf == 0) {
                        kind[parseLevel++] = "EOR (immediate) — 32-bit";
                    } else {
                        kind[parseLevel++] = "EOR (immediate) — 64-bit";
                    }
                } else {  // 11
                    if (Rn == RZR) result = 0;
                    if (sf == 0) {
                        kind[parseLevel++] = "ANDS (immediate) — 32-bit";
                    } else {
                        kind[parseLevel++] = "ANDS (immediate) — 64-bit";
                    }
                }
            }
        }
    } else if ((top0 | 0b1) == 0b1011) {  // 101x
        // https://developer.arm.com/docs/ddi0596/a/top-level-encodings-for-a64/branches-exception-generating-and-system-instructions
        kind[parseLevel++] = "Branches, Exception Generating and System instructions";
        Rd = -1;
        uint_fast8_t op0 = bits(code, 31, 29);
        uint_fast16_t op1 = bits(code, 25, 12);
        uint_fast8_t op2 = bits(code, 4, 0);
        if (op0 == 0b10) {
            if ((op1 & 0b10000000000000) != 0) {  // 1xxxxxxxxxxxxx
                kind[parseLevel++] = unalloc;
            } else {
                // https://developer.arm.com/docs/ddi0596/a/top-level-encodings-for-a64/branches-exception-generating-and-system-instructions#condbranch
                kind[parseLevel++] = "Conditional branch (immediate)";
                numSourceRegisters = 0;
                bool o1 = bits(code, 24, 24);
                int_fast32_t imm19 = bits(code, 23, 5);
                bool o0 = bits(code, 4, 4);
                cond = bits(code, 3, 0);
                if (o0 || o1) {
                    kind[parseLevel++] = unalloc;
                } else {
                    kind[parseLevel++] = "B.cond";
                    imm = pc + (SignExtend<int64_t>(imm19, 19) << 2);
                    log(DEBUG, "imm's offset: %llX", imm - getRealOffset(0));
                    branchType = DIR;
                }
            }
        } else if (op0 == 0b110) {
            if ((op1 & 0b10000000000000) != 0) {  // 1xxxxxxxxxxxxx
                // https://developer.arm.com/docs/ddi0596/a/top-level-encodings-for-a64/branches-exception-generating-and-system-instructions#branch_reg
                kind[parseLevel++] = "Unconditional branch (register)";
                numSourceRegisters = 1;
                uint_fast8_t opc = bits(code, 24, 21);
                uint_fast8_t op2 = bits(code, 20, 16);
                uint_fast8_t op3 = bits(code, 15, 10);
                Rs[0] = bits(code, 9, 5);
                uint_fast8_t op4 = bits(code, 4, 0);
                if (op2 != 0b11111) {
                    kind[parseLevel++] = unalloc;
                } else if (opc == 0) {
                    branchType = INDIR;
                    if (op3 == 0) {
                        if (op4 != 0) {
                            kind[parseLevel++] = unalloc;
                        } else {
                            kind[parseLevel++] = "BR";
                        }
                    } else {
                        log(DEBUG, "TODO: BRA[A/AZ/B/BZ]! opc = 0, op3: %i, op4: %i", op3, op4);
                    }
                } else if (opc == 0b1) {
                    branchType = INDCALL;
                    Rd = RLINK;
                    result = pc + 4;
                    if (op3 == 0) {
                        if (op4 != 0) {
                            kind[parseLevel++] = unalloc;
                        } else {
                            kind[parseLevel++] = "BLR";
                        }
                    } else {
                        log(DEBUG, "TODO: BLRA[A/AZ/B/BZ]! opc = 0, op3: %i, op4: %i", op3, op4);
                    }
                } else if (opc == 0b10) {
                    branchType = RET;
                    if (op3 == 0) {
                        if (op4 != 0) {
                            kind[parseLevel++] = unalloc;
                        } else {
                            kind[parseLevel++] = "RET";
                        }
                    } else {
                        log(DEBUG, "TODO: RETAA/RETAB! opc = 0b10, op3: %i, op4: %i", op3, op4);
                    }
                } else {
                    log(DEBUG, "opc: %i, op3: %i, op4: %i", opc, op3, op4);
                }
            } else {
                log(DEBUG, "op0 = 0b110, op1: %lu", op1);
            }
        } else if ((op0 & 0b11) == 0) {  // x00
            // https://developer.arm.com/docs/ddi0596/a/top-level-encodings-for-a64/branches-exception-generating-and-system-instructions#branch_imm
            kind[parseLevel++] = "Unconditional branch (immediate)";
            numSourceRegisters = 0;
            bool op = bits(code, 31, 31);
            uint_fast32_t imm26 = bits(code, 25, 0);
            auto offset = SignExtend<int64_t>(imm26, 26) << 2;
            imm = pc + offset;

            auto off = imm - getRealOffset(0);
            log(DEBUG, "imm's offset: %llX", off);
            if (off >= 0x03000000) {
                log(ERROR, "0x%llX is probably not a valid offset! Please investigate!", off);
            }
            if (!op) {
                kind[parseLevel++] = "B";
                branchType = DIR;
            } else {
                kind[parseLevel++] = "BL";
                branchType = DIRCALL;
                Rd = RLINK;
                result = pc + 4;
            }
        } else {
            log(DEBUG, "op0: %i", op0);
        }
    } else if ((top0 & 0b101) == 0b100) {  // x1x0
        // https://developer.arm.com/docs/ddi0596/a/top-level-encodings-for-a64/loads-and-stores
        kind[parseLevel++] = "Loads and Stores";
        uint_fast8_t op0 = bits(code, 31, 28);
        bool op1 = bits(code, 26, 26);
        uint_fast8_t op2 = bits(code, 24, 23);
        uint_fast8_t op3 = bits(code, 21, 16);
        uint_fast8_t op4 = bits(code, 11, 10);
        if ((op0 & 0b11) == 0b11) {  // xx11
            uint_fast8_t size = bits(code, 31, 30);
            auto V = op1;
            uint_fast8_t opc = bits(code, 23, 22);

            uint_fast8_t Rt = bits(code, 4, 0);  // cannot be SP
            uint_fast8_t Rn = bits(code, 9, 5);  // can be SP

            bool isStandardLdrStr = false;
            if ((op2 | 0b1) == 0b11) {  // 1x
                // https://developer.arm.com/docs/ddi0596/a/top-level-encodings-for-a64/loads-and-stores#ldst_pos
                kind[parseLevel++] = "Load/store register (unsigned immediate)";
                uint_fast16_t imm12 = bits(code, 21, 10);
                log(DEBUG, "size: %i; imm12: 0x%lX", size, imm12);
                imm = ZeroExtend<int64_t>(imm12, 12) << size;
                wback = false;
                postindex = false;
                isStandardLdrStr = true;
            } else if ((op3 & 0b100000) == 0) {  // 0xxxxx
                uint_fast16_t imm9 = bits(code, 20, 12);
                log(DEBUG, "size: %i; imm9: 0x%lX", size, imm9);
                imm = SignExtend<int64_t>(imm9, 9);

                if (op4 == 0b11) {
                    // https://developer.arm.com/docs/ddi0596/a/top-level-encodings-for-a64/loads-and-stores#ldst_immpre
                    kind[parseLevel++] = "Load/store register (immediate pre-indexed)";
                    wback = true;
                    postindex = false;
                    isStandardLdrStr = true;
                } else if (op4 == 0b1) {
                    // https://developer.arm.com/docs/ddi0596/a/top-level-encodings-for-a64/loads-and-stores#ldst_immpost
                    kind[parseLevel++] = "Load/store register (immediate post-indexed)";
                    wback = true;
                    postindex = true;
                    isStandardLdrStr = true;
                } else {
                    log(DEBUG, "op0 = xx11, op2 = 0x, op3 = 0xxxxx, op4: %i", op4);
                }
            } else {
                if (op4 == 0b10) {
                    // https://developer.arm.com/docs/ddi0596/a/top-level-encodings-for-a64/loads-and-stores#ldst_regoff
                    kind[parseLevel++] = "Load/store register (register offset)";
                    numSourceRegisters = 2;
                    auto Rm = Rs[1] = bits(code, 20, 16);  // cannot be SP

                    extendType = static_cast<ExtendType>(bits(code, 15, 13));  // "option"
                    bool S = bits(code, 12, 12);
                    bool shifted = (extendType == UXTX);
                    if (shifted) shiftType = LSL;
                    imm = (S ? size : 0);

                    // TODO: when option & 0b1 == 0, Rm is a 32-bit view, else 64
                    // TODO: when opc & 0b10 == 0, Rt is a 32-bit view, else 32 iff opc & 0b1 == 1

                    if ((extendType & 0b10) == 0 || ( ((size & 0b1) == 0b1) && V == 1 && ((opc & 0b10) != 0) )) {
                        kind[parseLevel++] = unalloc;
                    } else if (V == 0) {
                        if (opc == 0) {
                            Rs[0] = Rt; Rs0CanBeSP = false;
                            Rd = Rn; RdCanBeSP = true;
                            if (size == 3) {
                                kind[parseLevel++] = "STR (register) — 64-bit";
                            } else if (size == 2) {
                                kind[parseLevel++] = "STR (register) — 32-bit";
                            } else if (size == 1) {
                                kind[parseLevel++] = "STRH (register)";
                            } else if (shifted) {
                                kind[parseLevel++] = "STRB (register) — shifted register";
                            } else {
                                kind[parseLevel++] = "STRB (register) — extended register";
                            }
                        } else {
                            Rs[0] = Rn; Rs0CanBeSP = true;
                            Rd = Rt; RdCanBeSP = false;
                            if (opc == 0b1) {
                                if (size == 3) {
                                    kind[parseLevel++] = "LDR (register) — 64-bit";
                                } else if (size == 2) {
                                    kind[parseLevel++] = "LDR (register) — 32-bit";
                                } else if (size == 1) {
                                    kind[parseLevel++] = "LDRH (register)";
                                } else if (shifted) {
                                    kind[parseLevel++] = "LDRB (register) — shifted register";
                                } else {
                                    kind[parseLevel++] = "LDRB (register) — extended register";
                                }
                            } else {  // opc == 10 or 11
                                bool opc64 = (opc == 0b10);
                                if (size == 3) {
                                    kind[parseLevel++] = opc64 ? "PRFM (register)" : unalloc;
                                } else if (size == 2) {
                                    kind[parseLevel++] = opc64 ? "LDRSW (register)" : unalloc;
                                } else if (size == 1) {
                                    kind[parseLevel++] = opc64 ? "LDRSH (register) — 64-bit" : "LDRSH (register) — 32-bit";
                                } else {
                                    // TODO: Rt is a 32-bit view iff opc == 0b11;
                                    if (opc64) {
                                        if (shifted) {
                                            kind[parseLevel++] = "LDRSB (register) — 64-bit with shifted register offset";
                                        } else {
                                            kind[parseLevel++] = "LDRSB (register) — 64-bit with extended register offset";
                                        }
                                    } else {
                                        if (shifted) {
                                            kind[parseLevel++] = "LDRSB (register) — 32-bit with shifted register offset";
                                        } else {
                                            kind[parseLevel++] = "LDRSB (register) — 32-bit with extended register offset";
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        log(DEBUG, "TODO: STR/LDR (register, SIMD&FP)");
                    }
                } else {
                    log(DEBUG, "op0 = xx11, op2 = 0x, op3 = 1xxxxx, op4: %i", op4);
                }
            }

            if (isStandardLdrStr) {
                numSourceRegisters = 1;

                if (V == 0) {
                    if (opc == 0) {
                        Rs[0] = Rt; Rs0CanBeSP = false;
                        Rd = Rn; RdCanBeSP = true;
                        if (size == 3) {
                            kind[parseLevel++] = "STR (immediate) — 64-bit";
                        } else if (size == 2) {
                            kind[parseLevel++] = "STR (immediate) — 32-bit";
                        } else if (size == 1) {
                            kind[parseLevel++] = "STRH (immediate)";
                        } else {
                            kind[parseLevel++] = "STRB (immediate)";
                        }
                    } else {
                        Rs[0] = Rn; Rs0CanBeSP = true;
                        Rd = Rt; RdCanBeSP = false;
                        if (opc == 0b1) {
                            if (size == 3) {
                                kind[parseLevel++] = "LDR (immediate) — 64-bit";
                            } else if (size == 2) {
                                kind[parseLevel++] = "LDR (immediate) — 32-bit";
                            } else if (size == 1) {
                                kind[parseLevel++] = "LDRH (immediate)";
                            } else {
                                kind[parseLevel++] = "LDRB (immediate)";
                            }
                        } else {  // opc == 10 or 11
                            bool opc64 = (opc == 0b10);
                            if (size == 3) {
                                kind[parseLevel++] = opc64 ? "PRFM (immediate)" : unalloc;
                            } else if (size == 2) {
                                kind[parseLevel++] = opc64 ? "LDRSW (immediate)" : unalloc;
                            } else if (size == 1) {
                                kind[parseLevel++] = opc64 ? "LDRSH (immediate) — 64-bit" : "LDRSH (immediate) — 32-bit";
                            } else {
                                kind[parseLevel++] = opc64 ? "LDRSB (immediate) — 64-bit" : "LDRSB (immediate) — 32-bit";
                            }
                        }
                    }
                } else {
                    log(DEBUG, "V: %i (TODO: SIMD&FP STR/LDR)", V);
                }
            }
        } else if ((op0 & 0b11) == 0b10) {  // xx10
            uint_fast8_t opc = bits(code, 31, 30);
            auto V = op1;
            bool L = bits(code, 22, 22);
            int_fast8_t imm7 = bits(code, 21, 15);
            uint_fast8_t Rt = bits(code, 4, 0);  // cannot be SP
            uint_fast8_t Rn = bits(code, 9, 5);  // can be SP
            uint_fast8_t Rt2 = bits(code, 14, 10);  // cannot be SP

            if (op2 == 0) {
                // https://developer.arm.com/docs/ddi0596/a/top-level-encodings-for-a64/loads-and-stores#ldstnapair_offs
                kind[parseLevel++] = "Load/store no-allocate pair (offset)";
                log(DEBUG, "opc: %i, V: %i, L: %i", opc, V, L);
            } else {
                if (op2 == 0b1) {
                    // https://developer.arm.com/docs/ddi0596/a/top-level-encodings-for-a64/loads-and-stores#ldstpair_post
                    kind[parseLevel++] = "Load/store register pair (post-indexed)";
                    wback = true;
                    postindex = true;
                } else if (op2 == 0b10) {
                    // https://developer.arm.com/docs/ddi0596/a/top-level-encodings-for-a64/loads-and-stores#ldstpair_off
                    kind[parseLevel++] = "Load/store register pair (offset)";
                    wback = false;
                    postindex = false;
                } else {  // 11
                    // https://developer.arm.com/docs/ddi0596/a/top-level-encodings-for-a64/loads-and-stores#ldstpair_pre
                    kind[parseLevel++] = "Load/store register pair (pre-indexed)";
                    wback = true;
                    postindex = false;
                }

                if (opc == 0b11) {
                    kind[parseLevel++] = unalloc;
                } else if (!V && !L) {
                    numSourceRegisters = 2;
                    Rs[0] = Rt; Rs0CanBeSP = true;
                    Rs[1] = Rt2;
                    Rd = Rn; RdCanBeSP = true;
                    if (opc == 0) {
                        kind[parseLevel++] = "STP — 32-bit";
                    } else if (opc == 0b1) {
                        kind[parseLevel++] = "STGP";
                    } else {  // 10
                        kind[parseLevel++] = "STP — 64-bit";
                    }
                } else if (!V && L) {
                    numSourceRegisters = 1;
                    Rs[0] = Rn; Rs0CanBeSP = true;
                    Rd = Rt; RdCanBeSP = false;
                    Rd2 = Rt2;
                    if (opc == 0) {
                        kind[parseLevel++] = "LDP — 32-bit";
                    } else if (opc == 0b1) {
                        kind[parseLevel++] = "LDPSW";
                    } else {  // 10
                        kind[parseLevel++] = "LDP — 64-bit";
                    }
                } else {
                    log(DEBUG, "TODO: SIMD&FP LDP/STP. opc: %i, V: %i, L: %i", opc, V, L);
                }
            }
        } else {
            log(DEBUG, "op0: %i, op2: %i, op3: %i, op4: %i", op0, op2, op3, op4);
        }
    } else {
        kind[parseLevel++] = "ERROR: Our top-level bit patterns have a gap!";
        valid = false;
    }
    if (parseLevel != sizeof(kind) / sizeof(kind[0])) {
        log(ERROR, "Could not complete parsing of 0x%X - need more handling for kind '%s'!", code, kind[parseLevel - 1]);
    } else {
        parsed = true;
        if (kind[parseLevel - 1] == unalloc) {
            valid = false;
        }
    }
}

/*
void InstructionTree::Eval(ProgramState* state) {
    // TODO: if instruction is indirect branch, use state to populate branch
}
*/

InstructionTree* FindOrCreateInstruction(const int32_t* pc, ParseState& parseState, const char* msg) {
    auto p = parseState.codeToInstTree.find(pc);
    if (p != parseState.codeToInstTree.end()) {
        log(DEBUG, "not recursing: InstructionTree for %p (offset %llX) already exists", pc, ((long long)pc) - getRealOffset(0));
        return p->second;
    } else {
        log(DEBUG, "%s (pc %p, offset %llX)", msg, pc, ((long long)pc) - getRealOffset(0));
        auto inst = new (std::nothrow) InstructionTree(pc);
        parseState.frontier.push(inst);
        parseState.codeToInstTree[pc] = inst;
        return inst;
    }
}

void InstructionTree::PopulateChildren(ParseState& parseState) {
    auto pc = this->addr;
    log(DEBUG, "InstructionTree: %p, %s", pc, this->toString().c_str());
    // If instruction was not fully parsed, stop.
    if (!parsed || !valid) return;

    if ((this->numSourceRegisters < 0) || (this->Rs[this->numSourceRegisters - 1] < 0)) {
        log(ERROR, "The Instruction constructor did not properly parse this instruction's source registers! Please fix!");
    }

    // if instruction is return, stop parsing.
    if (isReturn()) return;

    // TODO: populate parseState.dependencyMap using this' dest and source registers

    // if instruction is direct branch, branch's addr is imm
    if (isDirectBranch()) {
        auto immAsAddr = (const int32_t*)imm;
        branch = FindOrCreateInstruction(immAsAddr, parseState, "InstructionTree: recursing to branch location");
        if (isCall()) {
            parseState.functionCandidates.insert({immAsAddr, parseState.dependencyMap});
        }
    }
    // unless instruction is unconditional branch, populate noBranch with next instruction in memory
    if (!isUnconditionalBranch()) {
        noBranch = FindOrCreateInstruction(pc + 1, parseState, "InstructionTree: recursing to next instruction");
    }
}

InstructionTree::InstructionTree(const int32_t* pc): Instruction(pc) {
}

std::ostream& operator<<(std::ostream& os, const AssemblyFunction& func) {
    os << std::showbase;

    return os << std::noshowbase;
}

std::string AssemblyFunction::toString() {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

AssemblyFunction::AssemblyFunction(const int32_t* pc): parseState() {
    auto root = new InstructionTree(pc);
    parseState.frontier.push(root);
    while (!parseState.frontier.empty()) {
        auto next = parseState.frontier.top();
        parseState.frontier.pop();
        next->PopulateChildren(parseState);
    }
}

void resetSS(std::stringstream& ss) {
    ss.str("");
    ss.clear();  // Clear state flags.
}

void tabs(std::ostream& os, int tabs, int spacesPerTab) {
    for (int i = 0; i < tabs * spacesPerTab; i++) {
        os << " ";
    }
}

void print(std::stringstream& ss, LOG_VERBOSE_TYPE lvl) {
    ss << std::endl;
    log(lvl, "%s", ss.str().c_str());
    resetSS(ss);
}

static std::unordered_set<const void*> analyzed;
void analyzeBytes(std::stringstream& ss, const void* ptr, int indent) {
    if (!ptr || ((const unsigned long long)ptr) > 0x7fffffffffll) return;

    tabs(ss, indent);
    if (analyzed.count(ptr)) {
        ss << "! loop at 0x" << std::hex << ptr << "!";
        print(ss);
        return;
    }
    analyzed.insert(ptr);

    auto asUInts = reinterpret_cast<const unsigned long long*>(ptr);
    auto asInts = reinterpret_cast<const long long*>(ptr);
    auto asChars = reinterpret_cast<const char*>(ptr);
    if (asUInts[0] >= 0x1000000000000ll && isprint(asChars[0])) {
        ss << "chars: \"" << asChars << "\"";
        ss << " (first 8 bytes in hex = 0x" << std::hex << std::setw(16) << asUInts[0] << ")";
        print(ss);
        return;
    }
    for (int i = 0; i < 4; i++) {
        if (i != 0) tabs(ss, indent);

        ss << "pos " << std::dec << i << ": 0x" << std::hex << std::setw(16) << asUInts[i];
        if (asUInts[i] >= 0x8000000000ll) {
            // todo: read no more than 8 chars or move asInts to last aligned point in string
            ss << " (as chars = \"" << reinterpret_cast<const char*>(asUInts + i) << "\")";
            ss << " (as int = " << std::dec << asInts[i] << ")";  // signed int
        } else if (asUInts[i] <= 0x7f00000000ll) {
            ss << " (as int = " << std::dec << asUInts[i] << ")";
        }
        else {
            Dl_info inf;
            if (dladdr((void*)asUInts[i], &inf)) {
                ss << " (dli_fname: " << inf.dli_fname << ", dli_fbase: " << std::hex << std::setw(16) << (long long)inf.dli_fbase;
                ss << ", offset = 0x" << std::hex << std::setw(8) << (asUInts[i] - (unsigned long long)inf.dli_fbase);
                if (inf.dli_sname) {
                    ss << ", dli_sname: " << inf.dli_sname << ", dli_saddr: " << std::hex << std::setw(16) << (long long)inf.dli_saddr;
                }
                ss << ")";
            }
        }
        print(ss);
        if (asUInts[i] > 0x7f00000000ll) {
            analyzeBytes(ss, (void*)asUInts[i], indent + 1);
        }
    }
}

void analyzeBytes(const void* ptr) {
    analyzed.clear();
    std::stringstream ss;
    ss << std::setfill('0');
    ss << "ptr: " << std::hex << std::setw(16) << (unsigned long long) ptr;
    print(ss);
    analyzeBytes(ss, ptr, 0);
}

long long baseAddr(const char *soname)  // credits to https://github.com/ikoz/AndroidSubstrate_hookingC_examples/blob/master/nativeHook3/jni/nativeHook3.cy.cpp
{
    void *imagehandle = dlopen(soname, RTLD_LOCAL | RTLD_LAZY);
    if (soname == NULL)
        return (long long)NULL;
    if (imagehandle == NULL){
        return (long long)NULL;
    }
    uintptr_t * irc = NULL;
    FILE *f = NULL;
    char line[200] = {0};
    char *state = NULL;
    char *tok = NULL;
    char * baseAddr = NULL;
    if ((f = fopen("/proc/self/maps", "r")) == NULL)
        return (long long)NULL;
    while (fgets(line, 199, f) != NULL)
    {
        tok = strtok_r(line, "-", &state);
        baseAddr = tok;
        tok = strtok_r(NULL, "\t ", &state);
        tok = strtok_r(NULL, "\t ", &state); // "r-xp" field
        tok = strtok_r(NULL, "\t ", &state); // "0000000" field
        tok = strtok_r(NULL, "\t ", &state); // "01:02" field
        tok = strtok_r(NULL, "\t ", &state); // "133224" field
        tok = strtok_r(NULL, "\t ", &state); // path field

        if (tok != NULL) {
            int i;
            for (i = (int)strlen(tok)-1; i >= 0; --i) {
                if (!(tok[i] == ' ' || tok[i] == '\r' || tok[i] == '\n' || tok[i] == '\t'))
                    break;
                tok[i] = 0;
            }
            {
                size_t toklen = strlen(tok);
                size_t solen = strlen(soname);
                if (toklen > 0) {
                    if (toklen >= solen && strcmp(tok + (toklen - solen), soname) == 0) {
                        fclose(f);
                        return (long long)strtoll(baseAddr,NULL,16);
                    }
                }
            }
        }
    }
    fclose(f);
    return (long long)NULL;
}

long long location; // save lib.so base address so we do not have to recalculate every time causing lag.

long long getRealOffset(const void* offset) // calculate dump.cs address + lib.so base address.
{
    if (location == 0)
    {
        //arm
        location = baseAddr(IL2CPP_SO_PATH); // replace the com.package.name with the package name of the app you are modding.
    }
    return location + (long long)offset;
}

long long findPattern(long long dwAddress, const char* pattern, long long dwSearchRangeLen) {
    #define in_range(x, a, b) (x >= a && x <= b)
    #define get_bits(x) (in_range((x & (~0x20)), 'A', 'F') ? ((x & (~0x20)) - 'A' + 0xA): (in_range(x, '0', '9') ? x - '0': 0))
    #define get_byte(x) (get_bits(x[0]) << 4 | get_bits(x[1]))

    // To avoid a lot of bad match candidates, pre-process wildcards at the front of the pattern
    long long skippedStartBytes = 0;
    while(pattern[0] == '\?') {
        // see comments below for insight on these numbers
        pattern += (pattern[1] == '\?') ? 3 : 2;
        skippedStartBytes++;
    }
    long long match = 0;  // current match candidate
    const char* pat = pattern;  // current spot in the pattern

    // TODO: align dwAddress to word boundary first, then iterate by 4?
    for (long long pCur = dwAddress + skippedStartBytes; pCur < dwAddress + dwSearchRangeLen; pCur++) {
        if (!pat[0]) return match;  // end of pattern means match is complete!
        if (pat[0] == '\?' || *(char *)pCur == get_byte(pat)) {  // does this pCur match this pat?
            if (!match) match = pCur - skippedStartBytes;  // start match
            if (!pat[2]) return match;  // no more chars in pattern means match is complete!

            if (pat[0] != '\?' || pat[1] == '\?') {
                pat += 3;  // advance past "xy " or "?? "
            } else {
                pat += 2;  // advance past "? "
            }
        } else {
            // reset search position to beginning of the failed match; for loop will begin new search at match + 1
            if (match) pCur = match + skippedStartBytes;
            pat = pattern;
            match = 0;
        }
    }
    return 0;
}

long long findUniquePattern(bool& multiple, long long dwAddress, const char* pattern, const char* label, long long dwSearchRangeLen) {
    long long firstMatchAddr = 0, newMatchAddr, start = dwAddress, dwEnd = dwAddress + dwSearchRangeLen;
    int matches = 0;
    while (start > 0 && start < dwEnd && (newMatchAddr = findPattern(start, pattern, dwEnd - start))) {
        if (!firstMatchAddr) firstMatchAddr = newMatchAddr;
        matches++;
        if (label) log(DEBUG, "Sigscan found possible \"%s\": offset 0x%llX, pointer 0x%llX", label, newMatchAddr - dwAddress, newMatchAddr);
        start = newMatchAddr + 1;
        log(DEBUG, "start = 0x%llX, end = 0x%llX", start, dwEnd);
        usleep(1000);
    }
    if (matches > 1) {
        multiple = true;
        log(WARNING, "Multiple sig scan matches for \"%s\"!", label);
    }
    return firstMatchAddr;
}

// C# SPECIFIC

void setcsstr(Il2CppString* in, u16string_view str) {
    in->length = str.length();
    for(int i = 0; i < in->length; i++) {
        // Can assume that each char is only a single char (a single word --> double word)
        in->chars[i] = str[i];
    }
}

// Inspired by DaNike
string to_utf8(u16string_view view) {
    char dat[view.length() + 1];
    transform(view.data(), view.data() + view.size(), dat, [](auto utf16_char) {
        return static_cast<char>(utf16_char);
    });
    dat[view.length()] = '\0';
    return {dat};
}

u16string to_utf16(string_view view) {
    char16_t dat[view.length() + 1];
    transform(view.data(), view.data() + view.size(), dat, [](auto standardChar) {
        return static_cast<char16_t>(standardChar);
    });
    dat[view.length()] = '\0';
    return {dat};
}

u16string_view csstrtostr(Il2CppString* in)
{
    return {in->chars, static_cast<uint32_t>(in->length)};
}

// Thanks DaNike!
void dump(int before, int after, void* ptr) {
    log(DEBUG, "Dumping Immediate Pointer: %p: %08x", ptr, *reinterpret_cast<int*>(ptr));
    auto begin = static_cast<int*>(ptr) - before;
    auto end = static_cast<int*>(ptr) + after;
    for (auto cur = begin; cur != end; ++cur) {
        log(DEBUG, "%p: %08x", cur, *cur);
    }
}

// SETTINGS

bool fileexists(const char* filename) {
    return access(filename, W_OK | R_OK) != -1;
}

bool direxists(const char* dirname) {
    struct stat info;

    if (stat(dirname, &info) != 0) {
        return false;
    } if (info.st_mode & S_IFDIR) {
        return true;
    }
    return false;
}

char* readfile(const char* filename) {
    FILE* fp = fopen(filename, "r");
    char* content = NULL;
    long size = 0;
    if (fp) {
        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        rewind(fp);
        content = (char*)malloc(size * sizeof(char));
        fread(content, sizeof(char), size, fp);
        fclose(fp);
    }
    return content;
}

bool writefile(const char* filename, const char* text) {
    FILE* fp = fopen(filename, "w");
    if (fp) {
        fwrite(text, sizeof(char), strlen(text), fp);
        fclose(fp);
        return true;
    }
    return false;
}

bool deletefile(const char* filename) {
    if (fileexists(filename))
        return remove(filename) == 0;
    return false;
}

void* loadfromasset(const char* assetFilePath, const char* assetName) {
    // TODO IMPLEMENT
    // Create C# string
    auto str = il2cpp_utils::createcsstr(assetFilePath);
    void* fromFileParams[] = {str};
    // auto asyncBundle = il2cpp_functions::runtime_invoke()
    return nullptr;
}

bool parsejson(ConfigDocument& doc, string_view js) {
    char temp[js.length()];
    memcpy(temp, js.data(), js.length());

    if (doc.ParseInsitu(temp).HasParseError()) {
        return false;
    }
    return true;
}

string getconfigpath() {
    string filename;
    filename = filename.append(CONFIG_PATH);
    filename = filename.append(MOD_ID);
    filename = filename.append(".json");
    return filename;
}
