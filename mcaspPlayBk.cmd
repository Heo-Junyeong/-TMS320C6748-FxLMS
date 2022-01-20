// ============================================================================
// Linker Command File for Linking c674 DSP Programs
//
// These linker options are for command line linking only. For IDE linking,
// you should set your linker options in Project Properties.
//         -c                    Link Using C Conventions
//        -stack     0x1000        Software Stack Size
//        -heap    0x1000        Heap Area Size
// ===========================================================================
-stack 0x1000
-heap 0x1000

// ============================================================================
//                         Specify the System Memory Map
// ============================================================================
MEMORY
{
    L1P:    o = 0x11E00000        l = 0x00008000
    L1D:    o = 0x11F00000        l = 0x00008000
    L2:     o = 0x11800000        l = 0x00040000
    DDR2:   o = 0xC0000000        l = 0x08000000
}

// ============================================================================
//                 Specify the Sections Allocation into Memory
// ============================================================================
SECTIONS
{
    .cinit        >        DDR2               // Initialization Tables
    .pinit        >        DDR2               // Constructor Tables
    .init_array   >        DDR2               // 
    .binit        >        DDR2               // Boot Tables
    .const        >        DDR2               // Constant Data
    .switch       >        DDR2               // Jump Tables
    .text         >        DDR2               // Executable Code
    .text:_c_int00: align=1024 > DDR2         // Entrypoint
    
    GROUP (NEARDP_DATA)                       // group near data
    {
       .neardata
       .rodata
       .bss                                   // note: removed fill = 0
    }             >        DDR2
    .far: fill = 0x0, load > DDR2             // Far Global & Static Variables
    .fardata      >        DDR2               // Far RW Data
    .stack        >        DDR2               // Software System Stack
    .sysmem       >        DDR2               // Dynamic Memory Allocation Area
    
    .cio          >        DDR2               // C I/O Buffer
    .vecs         >        DDR2               // Interrupt Vectors
}
