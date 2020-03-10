#include <array>
#include <bitset>
#include <stack>
#include <unordered_map>

constexpr size_t minBitSet = sizeof(std::bitset<1>)*8;

template<size_t N>
using fastBitSet = std::bitset<minBitSet * ((N  + minBitSet - 1) / minBitSet)>;

class Register {
public:
    inline static const uint_fast8_t TOTAL_NUMBER = 32;
    inline static const uint_fast8_t RLINK = 30;
    inline static const uint_fast8_t RZR = 31;
    inline static const uint_fast8_t SP = 31;
    static_assert(TOTAL_NUMBER > SP);

    uint_fast8_t num;
    bool r31_is_sp;
    Register(uint_fast8_t reg, bool r31) : num(reg), r31_is_sp(r31) {};
    std::string toString() const;
    friend std::ostream& operator<<(std::ostream& os, const Register& n);
};

class Instruction {
public:
    // TODO: puzzle out and support 32-bit views of registers
    const int32_t* addr;  // the pointer to the instruction
    // Rd and Rs are capitalized in accordance with typical Register notation
    int_fast8_t Rd = -2;  // the destination register's number, or -1 if none
    int_fast8_t Rd2 = -2;  // the 2nd destination register's number, or -1 if none
    int_fast8_t numSourceRegisters = -1;  // the number of source registers this instruction has
    union {
        int_fast8_t Rs[2] = { -1, -1 };  // the number(s) of the source register(s), e.g. {12, 31} for X12 & SP
        int64_t result;  // iff numSourceRegisters is 0, the value that will be stored to Rd by this instruction
    };
    int64_t imm = 0;  // the immediate, if applicable

    // see https://developer.arm.com/docs/ddi0596/a/a64-shared-pseudocode-functions/aarch64-instrs-pseudocode#impl-aarch64.ShiftReg.3
    enum ShiftType {  // in most cases (exceptiing noShift) the amount is stored in imm and the shift is applied to Rm (Rs[1])
        LSL, LSR, ASR, ROR, noShift
    } shiftType = noShift;

    // see https://developer.arm.com/docs/ddi0596/a/a64-shared-pseudocode-functions/aarch64-instrs-pseudocode#impl-aarch64.ExtendReg.3
    enum ExtendType {  // in most cases (exceptiing noExtend) the amount is stored in imm and the extend is applied to Rm (Rs[1])
        UXTB, UXTH, UXTW, UXTX, SXTB, SXTH, SXTW, SXTX, noExtend
    } extendType = noExtend;
    static bool extendIsUnsigned(ExtendType extend) {
        return extend <= UXTX;
    }
    static uint_fast8_t extendGetLen(ExtendType extend) {
        if (extend == noExtend) return 0;
        return 8 << (extend % 4);
    }

    #define BRANCH_ENUM(DO) \
        DO(NOBRANCH,  "") \
        DO(DIRCALL,   "Direct Branch with link") \
        DO(INDCALL,   "Indirect Branch with link") \
        DO(ERET,      "Exception return (indirect)") \
        DO(DBGEXIT,   "Exit from Debug state") \
        DO(RET,       "Indirect branch with function return hint") \
        DO(DIR,       "Direct branch") \
        DO(INDIR,     "Indirect branch") \
        DO(EXCEPTION, "Exception entry") \
        DO(RESET,     "Reset") \
        DO(UNKNOWN,   "Other")

    enum BranchType {
        #define MAKE_ENUM(VAR, STR) VAR,  // if only we could add STR as a comment (for IDE purposes only)
        BRANCH_ENUM(MAKE_ENUM)
        #undef MAKE_ENUM
    } branchType = NOBRANCH;

    inline static const char* const branchTypeNames[] = {
        #define MAKE_NAMES(VAR, STR) #VAR,
        BRANCH_ENUM(MAKE_NAMES)
        #undef MAKE_NAMES
    };

    inline static const char* const branchTypeInfo[] = {
        #define MAKE_STRINGS(VAR, STR) STR,
        BRANCH_ENUM(MAKE_STRINGS)
        #undef MAKE_STRINGS
    };
    #undef BRANCH_ENUM

    bool parsed;  // whether the instruction was fully and successfully parsed
    bool valid = true;  // iff parsed, whether the instruction is a valid one

    inline bool isBranch() const {
        return branchType != NOBRANCH;
    }
    inline bool isUnconditionalBranch() const {
        return isBranch() && cond == -1;
    }
    inline bool isReturn() const {
        return branchType == RET;
    }
    // i.e. location to jump to is not read from a register, but encoded directly into the instruction
    inline bool isDirectBranch() const {
        return (branchType == DIRCALL) || (branchType == DIR);
    }
    inline bool isIndirectBranch() const {
        return (branchType != NOBRANCH) && (!isDirectBranch());
    }
    inline bool isCall() const {
        return (branchType == DIRCALL) || (branchType == INDCALL);
    }

    Instruction(const int32_t* inst);
    std::string toString() const;
    friend std::ostream& operator<<(std::ostream& os, const Instruction& inst);

    // pred should be a https://en.cppreference.com/w/cpp/named_req/FunctionObject that accepts an Instruction* and returns bool.
    // Returns the nth match of the predicate starting at this->addr, or nullptr if the nth match does not exist.
    // Use n=1 for the 1st match, not 0.
    // Stops search when a "ret" instruction (return) is encountered (unless the nth match is found first or "ret" matches pred).
        // TODO: Instead, continue search until nth match is found, but return the number of ret instructions passed?
            // or accept a number of ret instructions before the function is considered "over"?
    // The search does not follow jumps.
    template<class UnaryPredicate>
    Instruction* FindNth(int n, UnaryPredicate pred) {
        auto inst = this;
        decltype(n) matches = 0;
        while (true) {
            if (pred(inst)) {
                matches++;
                if (matches == n) return inst;
            } else if (inst->isReturn()) {
                break;
            }
            auto pc = inst->addr;
            if (inst != this) delete inst;
            inst = new Instruction(pc + 1);
        }
        log(ERROR, "Only found %i instructions matching this predicate!", matches);
        return nullptr;
    }
    // e.g. BL, BLR. Unless the jump in indirect, the address the instruction jumps to will be at ->imm.
    Instruction* FindNthCall(int n);
    // e.g. B
    Instruction* FindNthDirectBranchWithoutLink(int n);
private:
    const char* kind[3];  // strings describing the kind of instruction, from least to most specific
    char parseLevel;  // The lowest level we were able to parse at, 1-3 (subtract 1 for index of most specific string in 'kind')
    bool RdCanBeSP = false;
    bool Rs0CanBeSP = false;
    // For LDR/STR:
    bool wback;
    bool postindex;
    int_fast8_t cond = -1;  // if set, a 4-bit int. See https://developer.arm.com/docs/ddi0596/a/a64-shared-pseudocode-functions/shared-functions-pseudocode#impl-shared.ConditionHolds.1
};

struct RegisterSet {
    int_fast64_t regs[Register::TOTAL_NUMBER] = {0};
};

struct PSTATE {
    bool N, Z, C, V;
    bool D, A, I, F;
    bool SS, IL, nRW, SP;
    int_fast8_t EL;  // 0, 1, 2, or 3
};

struct ProgramState {
    RegisterSet regSet;
    PSTATE pstate;
};

struct ParseState;

class InstructionTree : Instruction {
public:
    InstructionTree *noBranch;
    InstructionTree *branch;

    InstructionTree(const int32_t* inst);
    // Note: this must be done in a stack frontier fashion to prevent StackOverflow
    void PopulateChildren(ParseState& parseState);
    ~InstructionTree() {
        if (noBranch) delete noBranch;
        if (branch) delete branch;
    }
};

struct ParseState {
    std::array<std::bitset<Register::TOTAL_NUMBER>, Register::TOTAL_NUMBER> dependencyMap;
    std::unordered_map<const int32_t*, InstructionTree*> codeToInstTree;  // the set of all instructions and their corresponding trees
    std::unordered_map<const int32_t*, decltype(dependencyMap)> functionCandidates;  // the set of instructions that were jumped to
    std::stack<std::pair<InstructionTree*, decltype(dependencyMap)>> frontier;

    ParseState() {
        for (uint_fast8_t i = 0; i < dependencyMap.size(); i++) {
            dependencyMap[i] = std::move(decltype(dependencyMap)::value_type(1 << i));
        }
    }
};

class AssemblyFunction {
public:
    InstructionTree* instructions;

    AssemblyFunction(const int32_t* inst);
    std::string toString() const;
    friend std::ostream& operator<<(std::ostream& os, const AssemblyFunction& func);
private:
    ParseState parseState;
};

// Extends the given bitset from length M to N, padding with 0's.
template<size_t N, size_t M>
std::bitset<N> ZeroExtend(std::bitset<M> x) {
    static_assert(M <= sizeof(decltype(x.to_ullong())) * CHAR_BIT);
    return std::bitset<N>(x.to_ullong());
}

template<size_t M, size_t N>
std::bitset<M+N> concat(const std::bitset<M>& a, const std::bitset<N>& b) noexcept {
    return (ZeroExtend<M+N>(a) << N) | ZeroExtend<M+N>(b);
}

// Truncates the given integer to its least significant N bits.
template<class T>
T trunc(T bits, uint8_t N) {
    return bits & ((1ull << N) - 1ull);
}

// Version that accepts high and low at runtime 
template<class T>
unsigned int bitRangeAsInt(T x, int high, int low) {
    return trunc(x >> low, high - low + 1);
}

// Returns the value of the bits in x at index high through low inclusive, where the LSB is index 0 and the MSB's index >= high.
template<size_t high, size_t low, class T>
auto bitRange(T x) {
    return std::bitset<high - low + 1>(x >> low);
}

template<size_t high, size_t low, class T>
unsigned int bitRangeAsInt(T x) {
    static_assert(high - low + 1 <= sizeof(int) * CHAR_BIT);
    return bitRange<high, low>(x).to_ulong();
}

template<size_t high, size_t low, size_t N>
auto range(std::bitset<N> x) {
    return ZeroExtend<high - low + 1>(x >> low);
}

template<size_t N>
std::bitset<N> Replicate(bool x) {
    auto result = std::bitset<N>(0);
    if (x) return result.flip();
    return result;
}

template<size_t N, size_t M>
std::bitset<N> Replicate(std::bitset<M> x) {
    static_assert(N % M == 0);

    auto result = ZeroExtend<N>(x);
    auto size = M;
    while (size < N) {
        result |= (result << size);
        size *= 2;
    }
    return result;
}

// Extends the given bitset from length M to N, padding with x's sign bit.
template<size_t N, size_t M>
std::bitset<N> SignExtend(std::bitset<M> x) {
    static_assert(N >= M);
    return concat(Replicate<N-M>(x[M-1]), x);
}

// Returns the index of the most significant ON bit in x.
template<size_t N>
int HighestSetBit(std::bitset<N> x) {
    for (int i = N - 1; i >= 0; i--) {
        if (x[i]) return i;
    }
    return -1;
}

// For all shifts (LSL, LSR, ASR, ROR): N is the true number of significant bits in x.
// Left shift
template<size_t N>
std::bitset<N> LSL(std::bitset<N> x, unsigned shift) {
    return x << shift;
}

template<class T>
T LSL(T x, int N, unsigned shift) {
    return trunc(x << shift, N);
}

// Right shift, taking x as unsigned.
template<size_t N>
std::bitset<N> LSR(std::bitset<N> x, unsigned shift) {
    return x >> shift;
}

template<class T>
T LSR(T x, int N, unsigned shift) {
    return x >> shift;
}

// Right shift, taking x as signed.
template<size_t N>
std::bitset<N> ASR(std::bitset<N> x, unsigned shift) {
    return SignExtend<N>(ZeroExtend<N-shift>(x >> shift));
}

template<class T>
T ASR(T x, uint8_t N, unsigned shift) {
    typedef typename std::make_signed<T>::type signedT;
    return trunc(SignExtend<signedT>(x, N) >> shift, N);
}

// Right shift, but bits that "fall off" move to the front instead.
template<size_t N>
std::bitset<N> ROR(std::bitset<N> x, unsigned shift) {
    shift %= N;
    return LSR(x, shift) | LSL(x, N - shift);
}
// N is the true number of significant bits in x.
template<class T>
T ROR(T x, uint8_t N, unsigned shift) {
    shift %= N;
    return LSR(x, N, shift) | LSL(x, N, N - shift);
}
