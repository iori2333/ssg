///
/// ECL.h - Constants for the enemy control language
///

#pragma once

// [Revision history]

// 2000/11/27 : Added STG4EFC and related constants
// 2000/10/16 : Added JEQ
// 2000/09/05 : Added HLASER
// 2000/04/26 : Added LASER2
// 2000/03/22 : Added LLaser instruction
// 2000/03/15 : Added many instructions (interrupts, bullet clearing, register comparison)
// 2000/02/18 : Started system update

// [Constants]

// 0x0? : Control commands
inline constexpr auto ECL_CMD0 = 14;
inline constexpr auto ECL_SETUP = 0x00; // Initialize enemy data
inline constexpr auto ECL_END = 0x01;   // Force enemy elimination
inline constexpr auto ECL_JMP = 0x02;   // Force jump
inline constexpr auto ECL_LOOP =
    0x03; // Loop (cannot nest, CX is not used...)
inline constexpr auto ECL_CALL = 0x04; // Call subroutine
inline constexpr auto ECL_RET = 0x05;  // Return from subroutine
inline constexpr auto ECL_JHPL = 0x06; // Jump if HP is greater than specified value
inline constexpr auto ECL_JHPS = 0x07; // Jump if HP is less than specified value
inline constexpr auto ECL_JDIF = 0x08; // Switch by difficulty
inline constexpr auto ECL_JDSB =
    0x09; // Jump if direction matches player (within ±4 error)
inline constexpr auto ECL_JFCL = 0x0A; // Jump if frame counter is greater
inline constexpr auto ECL_JFCS = 0x0B; // Jump if frame counter is smaller
inline constexpr auto ECL_STI =
    0x0C; // Set interrupt vector
inline constexpr auto ECL_CLI =
    0x0D; // Clear interrupt vector

// 0x1? : Movement commands
inline constexpr auto ECL_CMD1 = 16;
inline constexpr auto ECL_NOP = 0x10;    // No operation
inline constexpr auto ECL_NOPSC = 0x11;  // Carried by scroll
inline constexpr auto ECL_MOV = 0x12;    // Move
inline constexpr auto ECL_ROL = 0x13;    // Rotate while moving
inline constexpr auto ECL_LROL = 0x14;   // Linear + rotate movement
inline constexpr auto ECL_WAVX = 0x15;   // Wave movement X
inline constexpr auto ECL_WAVY = 0x16;   // Wave movement Y
inline constexpr auto ECL_MXA = 0x17;    // X absolute movement
inline constexpr auto ECL_MYA = 0x18;    // Y absolute movement
inline constexpr auto ECL_MXYA = 0x19;   // XY absolute movement
inline constexpr auto ECL_MXS = 0x1A;    // X player-set movement
inline constexpr auto ECL_MYS = 0x1B;    // Y player-set movement
inline constexpr auto ECL_MXYS = 0x1C;   // XY player-set movement
inline constexpr auto ECL_ACC = 0x1D;    // Acceleration/deceleration movement
inline constexpr auto ECL_ACCXYA = 0x1E; // Deceleration XY absolute set
inline constexpr auto ECL_GRAX =
    0x1F; // Gravity X reflection movement (auto-delete if Y >= GY_MAX)

// 0x2? : Value set commands
inline constexpr auto ECL_CMD2 = 15;
inline constexpr auto ECL_DEGA = 0x20;  // Absolute angle set
inline constexpr auto ECL_DEGR = 0x21;  // Relative angle set
inline constexpr auto ECL_DEGX = 0x22;  // Random angle set
inline constexpr auto ECL_DEGS = 0x23;  // Angle player-set
inline constexpr auto ECL_SPDA = 0x24;  // Absolute speed set
inline constexpr auto ECL_SPDR = 0x25;  // Relative speed set
inline constexpr auto ECL_XYA = 0x26;   // Absolute coordinate set
inline constexpr auto ECL_XYR = 0x27;   // Relative coordinate set
inline constexpr auto ECL_DEGXU = 0x28; // Random angle set (upper 128 degrees)
inline constexpr auto ECL_DEGXD = 0x29; // Random angle set (lower 128 degrees)
inline constexpr auto ECL_DEGEX = 0x2A; // Special angle set (use with EXDEGD)
inline constexpr auto ECL_XYS = 0x2B;   // Player coordinate set
inline constexpr auto ECL_DEGX2 = 0x2C; // Bounded random angle
inline constexpr auto ECL_XYRND = 0x2D; // Bounded random coordinates
inline constexpr auto ECL_XYL = 0x2E;   // Length-based relative coordinate (polar coordinates)

// 0x4? : Bullet fire commands
inline constexpr auto ECL_CMD45 = 22;
inline constexpr auto ECL_TAMA = 0x40; // Fire bullet
inline constexpr auto ECL_TAUTO =
    0x41; // Set bullet fire interval (0: no auto-fire)
inline constexpr auto ECL_TXYR = 0x42;   // Bullet fire position relative offset
inline constexpr auto ECL_TCMD = 0x43;   // Bullet command
inline constexpr auto ECL_TDEGA = 0x44;  // Bullet fire angle absolute set
inline constexpr auto ECL_TDEGR = 0x45;  // Bullet fire angle relative set
inline constexpr auto ECL_TNUMA = 0x46;  // Bullet count absolute set
inline constexpr auto ECL_TNUMR = 0x47;  // Bullet count relative set
inline constexpr auto ECL_TSPDA = 0x48;  // Bullet initial speed absolute set
inline constexpr auto ECL_TSPDR = 0x49;  // Bullet initial speed relative set
inline constexpr auto ECL_TOPT = 0x4a;   // Bullet option set
inline constexpr auto ECL_TTYPE = 0x4b;  // Bullet type set
inline constexpr auto ECL_TCOL = 0x4c;   // Bullet color/shape set
inline constexpr auto ECL_TVDEG = 0x4d;  // Bullet angular velocity set
inline constexpr auto ECL_TREP = 0x4e;   // Bullet repeat set
inline constexpr auto ECL_TDEGS = 0x4f;  // Bullet fire angle player-set
inline constexpr auto ECL_TDEGE = 0x50;  // Set bullet fire angle to own direction
inline constexpr auto ECL_TAMA2 = 0x51;  // Fire bullet (no difficulty scaling)
inline constexpr auto ECL_TCLR = 0x52;   // Clear all bullets
inline constexpr auto ECL_TAMAL = 0x53;  // Fire bullets in a line
inline constexpr auto ECL_T2ITEM = 0x54; // Convert percentage of bullets to items
inline constexpr auto ECL_TAMAEX = 0x55; // Extra boss bullet hell command

// 0x6? : Laser fire commands
inline constexpr auto ECL_CMD67 = 18;
inline constexpr auto ECL_LASER = 0x60; // Fire laser
inline constexpr auto ECL_LCMD = 0x61;  // Laser command
inline constexpr auto ECL_LLA = 0x62;   // Laser length absolute set
inline constexpr auto ECL_LLR = 0x63;   // Laser length relative set
inline constexpr auto ECL_LL2 = 0x64;   // Laser fire position
inline constexpr auto ECL_LDEGA = 0x65; // Laser fire angle absolute set
inline constexpr auto ECL_LDEGR = 0x66; // Laser fire angle relative set
inline constexpr auto ECL_LNUMA = 0x67; // Laser count absolute set
inline constexpr auto ECL_LNUMR = 0x68; // Laser count relative set
inline constexpr auto ECL_LSPDA = 0x69; // Laser speed absolute set
inline constexpr auto ECL_LSPDR = 0x6a; // Laser speed relative set
inline constexpr auto ECL_LCOL = 0x6b;  // Laser color
inline constexpr auto ECL_LTYPE = 0x6c; // Laser type
inline constexpr auto ECL_LWA = 0x6d;   // Laser thickness absolute set
inline constexpr auto ECL_LDEGS = 0x6e; // Laser fire angle player-set
inline constexpr auto ECL_LDEGE = 0x6f; // Set laser fire angle to own direction
inline constexpr auto ECL_LXY =
    0x70; // Laser fire coordinate set (for thick laser?)
inline constexpr auto ECL_LASER2 = 0x71; // Fire laser

// 0x8? : Thick laser & homing laser commands (use above commands for struct set)
inline constexpr auto ECL_CMD8 = 6;
inline constexpr auto ECL_LLSET = 0x80;  // Thick laser set
inline constexpr auto ECL_LLOPEN = 0x81; // Thick laser open
inline constexpr auto ECL_LLCLOSE =
    0x82; // Thick laser close (delete & decrease reference count)
inline constexpr auto ECL_LLCLOSEL = 0x83; // Thick laser line state
inline constexpr auto ECL_LLDEGR = 0x84;   // Thick laser relative angle change
inline constexpr auto ECL_HLASER = 0x85;   // Homing laser activate!!

// 0x9? : Flag set commands
inline constexpr auto ECL_CMD9 = 10;
inline constexpr auto ECL_DRAW_ON = 0x90;    // Enable drawing
inline constexpr auto ECL_DRAW_OFF = 0x91;   // Disable drawing
inline constexpr auto ECL_CLIP_ON = 0x92;    // Do not delete when off-screen
inline constexpr auto ECL_CLIP_OFF = 0x93;   // Delete when off-screen
inline constexpr auto ECL_DAMAGE_ON = 0x94;  // Make invincible
inline constexpr auto ECL_DAMAGE_OFF = 0x95; // Make vulnerable
inline constexpr auto ECL_HITSB_ON = 0x96;   // Hit player
inline constexpr auto ECL_HITSB_OFF = 0x97;  // Do not hit player
inline constexpr auto ECL_RLCHG_ON = 0x98;   // Enable horizontal flip
inline constexpr auto ECL_RLCHG_OFF = 0x99;  // Disable horizontal flip

// 0xA? : Special commands
inline constexpr auto ECL_CMDA = 16;
inline constexpr auto ECL_ANM = 0xA0;       // Change animation
inline constexpr auto ECL_PSE = 0xA1;       // Play sound effect
inline constexpr auto ECL_INT = 0xA2;       // Generate boss interrupt...
inline constexpr auto ECL_EXDEGD = 0xA3;    // Special angle set initialization
inline constexpr auto ECL_ENEMYSET = 0xA4;  // Set enemy as minion
inline constexpr auto ECL_ENEMYSETD = 0xA5; // Set enemy (with angle specification)
inline constexpr auto ECL_HITXY = 0xA6;     // Change enemy hitbox
inline constexpr auto ECL_ITEM = 0xA7;      // Set item type
inline constexpr auto ECL_STG4EFC = 0xA8;   // Stage 4 boss sync effect management
inline constexpr auto ECL_ANMEX = 0xA9;     // Set animation during damage
inline constexpr auto ECL_BITLASER = 0xAA;  // Bit laser command set
inline constexpr auto ECL_BITATTACK = 0xAB; // Bit attack set
inline constexpr auto ECL_BITCMD = 0xAC;    // Bit command send
inline constexpr auto ECL_BOSSSET = 0xAD;   // Spawn boss
inline constexpr auto ECL_CEFC = 0xAE;      // Spawn circle effect
inline constexpr auto ECL_STG3EFC = 0xAF;   // Stage 3 star effect activation

// 0xB? : Register commands (x86 instruction-like)
inline constexpr auto ECL_CMDB = 15;
inline constexpr auto ECL_MOVR = 0xB0; // Register <-> struct variable assignment
inline constexpr auto ECL_MOVC = 0xB1; // Register <- constant (immediate) assignment
inline constexpr auto ECL_ADD = 0xB2;  // Add instruction
inline constexpr auto ECL_SUB = 0xB3;  // Subtract instruction
inline constexpr auto ECL_SINL = 0xB4; // sinl(Gr0,Gr1)
inline constexpr auto ECL_COSL = 0xB5; // cosl(Gr0,Gr1)
inline constexpr auto ECL_MOD = 0xB6;  // Gr0 = Gr0 % Const
inline constexpr auto ECL_RND = 0xB7;  // Gr0 = rnd()
inline constexpr auto ECL_CMPR = 0xB8; // Compare Gr0,Gr1
inline constexpr auto ECL_CMPC = 0xB9; // Compare Gr0,Const
inline constexpr auto ECL_JL = 0xBA;   // Jump if comparison result is >
inline constexpr auto ECL_JS = 0xBB;   // Jump if comparison result is <
inline constexpr auto ECL_INC = 0xBC;  // Register +1
inline constexpr auto ECL_DEC = 0xBD;  // Register -1
inline constexpr auto ECL_JEQ = 0xBE;  // Jump if comparison result is =

// ECL constants

// Lower interrupt vector numbers have higher priority
inline constexpr auto ECLVECT_MAX = 4;         // Maximum interrupt vectors
inline constexpr auto ECLVECT_BOSSLEFT = 0x00; // Boss remaining count interrupt
inline constexpr auto ECLVECT_HP = 0x01; // Interrupt when HP is less than specified value
inline constexpr auto ECLVECT_TIMER = 0x02;   // Timer interrupt
inline constexpr auto ECLVECT_BITLEFT = 0x03; // Remaining bit count interrupt

inline constexpr auto ECLREG_MAX = 8; // Number of registers
inline constexpr auto ECLCST_GR0 = 0; // Register 0
inline constexpr auto ECLCST_GR1 = 1; // Register 1
inline constexpr auto ECLCST_GR2 = 2; // Register 2
inline constexpr auto ECLCST_GR3 = 3; // Register 3
inline constexpr auto ECLCST_GR4 = 4; // Register 4
inline constexpr auto ECLCST_GR5 = 5; // Register 5
inline constexpr auto ECLCST_GR6 = 6; // Register 6
inline constexpr auto ECLCST_GR7 = 7; // Register 7

inline constexpr auto ECLCST_LCMD_D = (128 + 0);  // Laser command (angle)
inline constexpr auto ECLCST_LCMD_DW = (128 + 1); // Laser command (angle difference)
inline constexpr auto ECLCST_LCMD_N = (128 + 2);  // Laser command (count)
inline constexpr auto ECLCST_LCMD_C = (128 + 3);  // Laser command (color)
inline constexpr auto ECLCST_LCMD_L = (128 + 4);  // Laser command (length)
inline constexpr auto ECLCST_LCMD_V = (128 + 5);  // Laser command (speed)

inline constexpr auto ECLCST_TCMD_D = (128 + 6);    // Bullet command (angle)
inline constexpr auto ECLCST_TCMD_DW = (128 + 7);   // Bullet command (angle difference)
inline constexpr auto ECLCST_TCMD_N = (128 + 8);    // Bullet command (count)
inline constexpr auto ECLCST_TCMD_NS = (128 + 9);   // Bullet command (rapid fire count)
inline constexpr auto ECLCST_TCMD_V = (128 + 10);   // Bullet command (speed)
inline constexpr auto ECLCST_TCMD_C = (128 + 11);   // Bullet command (color)
inline constexpr auto ECLCST_TCMD_A = (128 + 12);   // Bullet command (acceleration)
inline constexpr auto ECLCST_TCMD_REP = (128 + 13); // Bullet command (repeat)
inline constexpr auto ECLCST_TCMD_VD = (128 + 14);  // Bullet command (angular velocity)

inline constexpr auto ECLCST_ENEMY_X = (128 + 15); // Enemy X coordinate
inline constexpr auto ECLCST_ENEMY_Y = (128 + 16); // Enemy Y coordinate
inline constexpr auto ECLCST_ENEMY_D = (128 + 17); // Enemy angle

inline constexpr auto ECLCST_LLASERALL =
    0xff; // Value to apply to all lasers

inline constexpr auto ECLINT_SNAKEON = 0x00;  // Snake type set
inline constexpr auto ECLINT_LBWING01 = 0x01; // Final boss butterfly wing mode
inline constexpr auto ECLINT_LBWING02 = 0x02; // Final boss bird wing mode
inline constexpr auto ECLINT_BITON5 = 0x03;   // Bit equip (5)
inline constexpr auto ECLINT_BITON6 = 0x04;   // Bit equip (6)
inline constexpr auto ECLINT_SHILD1 = 0x05;   // Bomb evade 1
inline constexpr auto ECLINT_SHILD2 = 0x06;   // Bomb evade 1

// ECL command maximum count (must be after all ECL_CMD* group sizes)
inline constexpr auto ECL_CMDMAX =
    (ECL_CMD0 + ECL_CMD1 + ECL_CMD2 + ECL_CMD45 + ECL_CMD67 + ECL_CMD8 +
     ECL_CMD9 + ECL_CMDA + ECL_CMDB);
