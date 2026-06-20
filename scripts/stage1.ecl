.header 10
.offset 0 0x002C
.offset 1 0x00FC
.offset 2 0x014A
.offset 3 0x016C
.offset 4 0x018F
.offset 5 0x01F6
.offset 6 0x0226
.offset 7 0x0277
.offset 8 0x049A
.offset 9 0x063B

.org 0x002C
@script_0:
    SETUP hp=1500 score=222220
    ANM pattern=0 speed=16
    ANMEX pattern=6
    ITEM type=3
    SPDA speed=64
    STI jmp=@label_00DC vector=HP val=150
    DAMAGE_OFF
    MOV count=150
    NOP count=50
    DAMAGE_ON
.org 0x0053
@label_0053:
    CALL jmp=@script_1
    NOP count=50
    SPDA speed=256
    DEGA angle=112
    CALL jmp=@label_0136
.org 0x0067
@label_0067:
    MOV count=2
    SPDR speed=-7
    LOOP jmp=@label_0067 count=30
    TAUTO interval=0
    CALL jmp=@label_0114
    DEGR angle=-128
    CALL jmp=@label_0136
.org 0x0084
@label_0084:
    MOV count=2
    SPDR speed=7
    LOOP jmp=@label_0084 count=30
    TAUTO interval=0
    CALL jmp=@script_1
    NOP count=50
    SPDA speed=256
    DEGA angle=16
    CALL jmp=@label_0136
.org 0x00A9
@label_00A9:
    MOV count=2
    SPDR speed=-7
    LOOP jmp=@label_00A9 count=30
    TAUTO interval=0
    CALL jmp=@label_0114
    DEGR angle=-128
    CALL jmp=@label_0136
.org 0x00C6
@label_00C6:
    MOV count=2
    SPDR speed=7
    LOOP jmp=@label_00C6 count=30
    TAUTO interval=0
    JMP jmp=@label_0053
.org 0x00DC
@label_00DC:
    CLI vector=HP
    TCLR
    ANM pattern=0 speed=5
    ANMEX pattern=6
    CALL jmp=@label_012B
    TAUTO interval=0
.org 0x00EB
@label_00EB:
    NOP count=5
    TCOL color=0
    TAMA
    NOP count=5
    TCOL color=16
    TAMA
    JMP jmp=@label_00EB
.org 0x00FC
@script_1:
    TCMD cmd=0
    TDEGA angle=64 dw=20
    TNUMA n=5 ns=1
    TSPDA v=10 a=0
    TTYPE type=0
    TCOL color=21
    TAMA
    NOP count=10
    TNUMA n=6 ns=0
    TAMA
    RET
.org 0x0114
@label_0114:
    TCMD cmd=12
    TDEGA angle=0 dw=20
    TNUMA n=2 ns=10
    TSPDA v=6 a=0
    TTYPE type=0
    TCOL color=1
    NOP count=20
    TAMA
    NOP count=40
    RET
.org 0x012B
@label_012B:
    TDEGA angle=64 dw=255
    JMP jmp=@label_013C
    TNUMA n=10 ns=1
.org 0x0136
@label_0136:
    TDEGA angle=64 dw=120
    TNUMA n=4 ns=1
.org 0x013C
@label_013C:
    TCOL color=16
    TCMD cmd=2
    TSPDA v=30 a=-12
    TTYPE type=1
    TREP rep=30
    TAUTO interval=16
    RET
.org 0x014A
@script_2:
    SETUP hp=8 score=500
    RLCHG_ON
    ANM pattern=3 speed=0
    SPDA speed=224
.org 0x015C
@label_015C:
    ROL deg=1 count=64
    ROL deg=-1 count=32
    MOV count=200
    JMP jmp=@label_015C
.org 0x016C
@script_3:
    SETUP hp=8 score=500
    RLCHG_ON
    ANM pattern=3 speed=0
    SPDA speed=224
    DEGA angle=32
.org 0x0180
@label_0180:
    MOV count=30
    ROL deg=1 count=50
    MOV count=100
    JMP jmp=@label_0180
.org 0x018F
@script_4:
    SETUP hp=150 score=2000
    RLCHG_ON
    ANM pattern=2 speed=0
    TCMD cmd=8
    TDEGA angle=0 dw=18
    TNUMA n=3 ns=1
    TSPDA v=6 a=0
    TTYPE type=0
    TCOL color=16
    TAUTO interval=255
.org 0x01AD
@label_01AD:
    DEGA angle=112
    SPDA speed=64
    MOV count=60
    SPDA speed=64
    LROL vx=-24 vy=24 deg=-1 count=256
    LOOP jmp=@label_01AD count=2
.org 0x01CF
@label_01CF:
    DEGA angle=16
    SPDA speed=64
    MOV count=60
    SPDA speed=64
    LROL vx=24 vy=24 deg=-1 count=256
    LOOP jmp=@label_01CF count=2
    JMP jmp=@label_01AD
.org 0x01F6
@script_5:
    SETUP hp=9 score=600
    RLCHG_ON
    ANM pattern=4 speed=0
    SPDA speed=384
    DEGS
.org 0x0209
@label_0209:
    MOV count=2
    SPDR speed=-16
    LOOP jmp=@label_0209 count=30
    DEGS
.org 0x0219
@label_0219:
    MOV count=2
    SPDR speed=8
    JMP jmp=@label_0219
.org 0x0226
@script_6:
    SETUP hp=53 score=1000
    RLCHG_ON
    ANM pattern=1 speed=0
    SPDA speed=192
    TSPDA v=10 a=0
    TTYPE type=0
    TCOL color=17
    TDEGA angle=0 dw=0
    TNUMA n=16 ns=1
    TCMD cmd=9
    MOV count=35
    TAMA
    MOV count=35
.org 0x024E
@label_024E:
    DEGR angle=8
    NOP count=1
    LOOP jmp=@label_024E count=15
    TCOL color=0
    TSPDA v=8 a=0
    TDEGA angle=64 dw=30
    TCMD cmd=0
    TNUMA n=1 ns=1
    TAUTO interval=5
    MOV count=100
    TAUTO interval=0
    DEGXU
.org 0x026F
@label_026F:
    MOV count=100
    JMP jmp=@label_026F
.org 0x0277
@script_7:
    SETUP hp=9150 score=1000000
    STI jmp=@label_0323 vector=HP val=7350
    ANM pattern=5 speed=0
    ANMEX pattern=7
    SPDA speed=64
    DAMAGE_OFF
    MXYA x=319 y=110 count=150
    NOP count=100
    DAMAGE_ON
    MOVC dst=7 val=4
    SPDA speed=160
.org 0x02AB
@label_02AB:
    CALL jmp=@label_04A9
    NOP count=10
    MXYA x=471 y=160 count=60
    CALL jmp=@label_0591
    NOP count=40
    DEGA angle=96
    TDEGA angle=48 dw=16
    CALL jmp=@script_8
.org 0x02CC
@label_02CC:
    TDEGR angle=4 dw=0
    ROL deg=2 count=32
    LOOP jmp=@label_02CC count=1
    TAUTO interval=0
    CALL jmp=@label_04C6
    NOP count=10
    MXYA x=168 y=160 count=60
    CALL jmp=@label_0591
    NOP count=40
    DEGA angle=32
    TDEGA angle=80 dw=16
    CALL jmp=@script_8
.org 0x02FD
@label_02FD:
    TDEGR angle=-4 dw=0
    ROL deg=-2 count=32
    LOOP jmp=@label_02FD count=1
    TAUTO interval=0
    DEC reg=7
    CMPC reg=7 val=0
    JL jmp=@label_02AB
    SETUP hp=7350 score=1000000
.org 0x0323
@label_0323:
    STI jmp=@label_0429 vector=HP val=1275
    TAUTO interval=0
    TCLR
    MOVC dst=7 val=2
    MXYA x=319 y=110 count=60
.org 0x033D
@label_033D:
    MXYA x=319 y=110 count=10
    CALL jmp=@label_04E3
    CALL jmp=@label_04E3
    NOP count=40
    CALL jmp=@label_0571
    DEGA angle=0
    WAVX vx=128 amp=40 vd=3 count=42
    TAUTO interval=0
    NOP count=25
    CALL jmp=@label_0591
    DEGA angle=0
    WAVX vx=-128 amp=40 vd=3 count=42
    CALL jmp=@label_052A
    CALL jmp=@label_052A
    NOP count=40
    CALL jmp=@label_0571
    DEGA angle=0
    WAVX vx=-128 amp=40 vd=3 count=42
    TAUTO interval=0
    NOP count=25
    CALL jmp=@label_05E3
    DEGA angle=0
    WAVX vx=128 amp=40 vd=3 count=42
    MXYA x=319 y=110 count=10
    CALL jmp=@label_04E3
    CALL jmp=@label_04E3
    NOP count=40
    CALL jmp=@label_0571
    DEGA angle=0
    WAVX vx=128 amp=40 vd=3 count=42
    TAUTO interval=0
    NOP count=25
    CALL jmp=@label_060F
    DEGA angle=0
    WAVX vx=-128 amp=40 vd=3 count=42
    CALL jmp=@label_052A
    CALL jmp=@label_052A
    NOP count=40
    CALL jmp=@label_0571
    DEGA angle=0
    WAVX vx=-128 amp=40 vd=3 count=42
    TAUTO interval=0
    NOP count=25
    CALL jmp=@label_0591
    DEGA angle=0
    WAVX vx=128 amp=40 vd=3 count=42
    DEC reg=7
    CMPC reg=7 val=0
    JL jmp=@label_033D
    SETUP hp=1275 score=1000000
.org 0x0429
@label_0429:
    CLI vector=HP
    TAUTO interval=0
    TCLR
    MXYA x=319 y=80 count=60
    CALL jmp=@label_0583
    MOVC dst=1 val=0
    MOVC dst=2 val=16
    MOVC dst=4 val=50
    MOVC dst=7 val=5
.org 0x0452
@label_0452:
    TDEGR angle=2 dw=0
    MOVR dst=0 src=4
    SINL len=0 deg=1
    MOVR dst=134 src=0
    ADD dst=1 src=2
    NOP count=3
    TCOL color=17
    TAMA
    PSE id=4
    TCOL color=1
    NOP count=3
    TAMA
    PSE id=4
    LOOP jmp=@label_0452 count=30
    TNUMR n=1 ns=0
    DEC reg=7
    CMPC reg=7 val=0
    JL jmp=@label_0452
    SETUP hp=0 score=0  ; death marker
    TCLR
.org 0x0492
@label_0492:
    NOP count=100
    JMP jmp=@label_0492
.org 0x049A
@script_8:
    TCMD cmd=0
    TNUMA n=8 ns=1
    TSPDA v=10 a=0
    TTYPE type=0
    TCOL color=3
    TAUTO interval=10
    RET
.org 0x04A9
@label_04A9:
    TCMD cmd=1
    TNUMA n=12 ns=1
    TSPDA v=12 a=0
    TTYPE type=0
    TCOL color=17
.org 0x04B5
@label_04B5:
    PSE id=4
    TAMA
    TDEGR angle=1 dw=0
    NOP count=3
    LOOP jmp=@label_04B5 count=14
    RET
.org 0x04C6
@label_04C6:
    TCMD cmd=1
    TNUMA n=12 ns=1
    TSPDA v=12 a=0
    TTYPE type=0
    TCOL color=17
.org 0x04D2
@label_04D2:
    PSE id=4
    TAMA
    TDEGR angle=-1 dw=0
    NOP count=3
    LOOP jmp=@label_04D2 count=14
    RET
.org 0x04E3
@label_04E3:
    TCMD cmd=1
    TNUMA n=14 ns=1
    TSPDA v=20 a=-4
    TTYPE type=1
    TREP rep=55
.org 0x04EF
@label_04EF:
    TCOL color=17
    TAMA
    PSE id=12
    TDEGR angle=-1 dw=0
    NOP count=5
    TCOL color=1
    TAMA
    PSE id=12
    TDEGR angle=-1 dw=0
    NOP count=5
    LOOP jmp=@label_04EF count=4
.org 0x050C
@label_050C:
    TCOL color=17
    TAMA
    PSE id=12
    TDEGR angle=1 dw=0
    NOP count=5
    TCOL color=1
    TAMA
    PSE id=12
    TDEGR angle=1 dw=0
    NOP count=5
    LOOP jmp=@label_050C count=4
    RET
.org 0x052A
@label_052A:
    TCMD cmd=1
    TNUMA n=14 ns=1
    TSPDA v=20 a=-4
    TTYPE type=1
    TREP rep=55
.org 0x0536
@label_0536:
    TCOL color=17
    TAMA
    PSE id=12
    TDEGR angle=1 dw=0
    NOP count=5
    TCOL color=1
    TAMA
    PSE id=12
    TDEGR angle=1 dw=0
    NOP count=5
    LOOP jmp=@label_0536 count=4
.org 0x0553
@label_0553:
    TCOL color=17
    TAMA
    PSE id=12
    TDEGR angle=-1 dw=0
    NOP count=5
    TCOL color=1
    TAMA
    PSE id=12
    TDEGR angle=-1 dw=0
    NOP count=5
    LOOP jmp=@label_0553 count=4
    RET
.org 0x0571
@label_0571:
    TCMD cmd=2
    TNUMA n=10 ns=1
    TDEGA angle=64 dw=255
    TSPDA v=9 a=0
    TTYPE type=0
    TCOL color=19
    TAUTO interval=6
    RET
.org 0x0583
@label_0583:
    TCMD cmd=10
    TDEGA angle=64 dw=80
    TNUMA n=2 ns=1
    TSPDA v=206 a=0
    TTYPE type=0
    RET
.org 0x0591
@label_0591:
    DEGS
    LCMD cmd=0
    LLA len=5760
    LDEGA angle=0 dw=1
    LDEGE
    LNUMA n=1
    LSPDA v=320
    LCOL color=0
    LTYPE type=0
    LWA w=192
    MOVC dst=0 val=90
    MOVC dst=1 val=4294967206
    MOVR dst=2 src=128
    ADD dst=0 src=2
    ADD dst=1 src=2
    MOVC dst=2 val=5
.org 0x05C8
@label_05C8:
    MOVR dst=128 src=0
    LASER
    MOVR dst=128 src=1
    LASER
    PSE id=3
    SUB dst=0 src=2
    ADD dst=1 src=2
    NOP count=1
    LOOP jmp=@label_05C8 count=16
    RET
.org 0x05E3
@label_05E3:
    DEGS
    LCMD cmd=0
    LLA len=5760
    LDEGA angle=160 dw=1
    LNUMA n=1
    LSPDA v=320
    LCOL color=0
    LTYPE type=0
    LWA w=192
.org 0x05FE
@label_05FE:
    LASER
    PSE id=3
    LDEGR angle=-15 dw=0
    NOP count=2
    LOOP jmp=@label_05FE count=20
    RET
.org 0x060F
@label_060F:
    DEGS
    LCMD cmd=0
    LLA len=5760
    LDEGA angle=224 dw=1
    LNUMA n=1
    LSPDA v=320
    LCOL color=0
    LTYPE type=0
    LWA w=192
.org 0x062A
@label_062A:
    LASER
    PSE id=3
    LDEGR angle=15 dw=0
    NOP count=2
    LOOP jmp=@label_062A count=20
    RET
.org 0x063B
@script_9:
    MOVC dst=0 val=98
    MOVC dst=2 val=4294967294
    CALL jmp=@label_0583
    SPDA speed=128
.org 0x0651
@label_0651:
    TCOL color=1
    MOVC dst=1 val=64
    ADD dst=0 src=2
    SINL len=1 deg=0
    MOVC dst=3 val=16
    ADD dst=3 src=1
    MOVR dst=134 src=3
    TAMA
    MOVC dst=4 val=112
    SUB dst=4 src=1
    MOVR dst=134 src=4
    TAMA
    NOP count=1
    LOOP jmp=@label_0651 count=8
    NOP count=64
.org 0x0686
@label_0686:
    TCOL color=17
    MOVC dst=1 val=64
    ADD dst=0 src=2
    SINL len=1 deg=0
    MOVC dst=3 val=16
    ADD dst=3 src=1
    MOVR dst=134 src=3
    TAMA
    MOVC dst=4 val=112
    SUB dst=4 src=1
    MOVR dst=134 src=4
    TAMA
    NOP count=1
    LOOP jmp=@label_0686 count=8
    NOP count=64
    TNUMR n=1 ns=0
