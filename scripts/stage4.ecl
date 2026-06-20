.header 20
.offset 0 0x0054
.offset 1 0x0094
.offset 2 0x00DB
.offset 3 0x0169
.offset 4 0x01F4
.offset 5 0x0282
.offset 6 0x02BA
.offset 7 0x02CA
.offset 8 0x02DA
.offset 9 0x0360
.offset 10 0x07EF
.offset 11 0x0B82
.offset 12 0x0BB8
.offset 13 0x0054
.offset 14 0x0054
.offset 15 0x0054
.offset 16 0x0054
.offset 17 0x0054
.offset 18 0x0054
.offset 19 0x0054

.org 0x0054
@script_0:
@script_13:  ; shared
@script_14:  ; shared
@script_15:  ; shared
@script_16:  ; shared
@script_17:  ; shared
@script_18:  ; shared
@script_19:  ; shared
    SETUP hp=60 score=500
    RLCHG_ON
    CLIP_ON
    ANM pattern=0 speed=0
    SPDA speed=320
    DEGA angle=64
    TCMD cmd=1
    TDEGA angle=0 dw=5
    TNUMA n=20 ns=0
    TSPDA v=18 a=-4
    TREP rep=30
    TTYPE type=1
    TCOL color=16
    MOV count=50
    CLIP_OFF
    SPDR speed=-64
    ROL deg=4 count=10
    TAMA
    ROL deg=4 count=10
.org 0x008C
@label_008C:
    MOV count=200
    JMP jmp=@label_008C
.org 0x0094
@script_1:
    SETUP hp=30 score=500
    RLCHG_ON
    CLIP_ON
    ANM pattern=1 speed=0
    SPDA speed=576
    DEGS
    TCMD cmd=94
    TDEGA angle=0 dw=5
    TNUMA n=2 ns=2
    TSPDA v=141 a=0
    TTYPE type=0
    TCOL color=5
.org 0x00B7
@label_00B7:
    MOV count=1
    SPDR speed=-18
    LOOP jmp=@label_00B7 count=32
    NOP count=10
    TAMA
    NOP count=10
    CLIP_OFF
    MOV count=1
    SPDR speed=8
    JMP jmp=@label_00B7
.org 0x00DB
@script_2:
    SETUP hp=150 score=500
    RLCHG_ON
    CLIP_ON
    ANM pattern=2 speed=0
    SPDA speed=128
    DEGA angle=64
    TCMD cmd=80
    TDEGA angle=0 dw=10
    TNUMA n=1 ns=0
    TSPDA v=12 a=-2
    TTYPE type=1
    TREP rep=20
    TCOL color=33
    ROL deg=2 count=16
    ROL deg=-2 count=32
    ROL deg=2 count=16
    CLIP_OFF
    JDIF easy=@label_0140 norm=@label_0135 hard=@label_012A luna=@label_011F
.org 0x011F
@label_011F:
    MOVC dst=0 val=14
    JMP jmp=@label_014B
.org 0x012A
@label_012A:
    MOVC dst=0 val=12
    JMP jmp=@label_014B
.org 0x0135
@label_0135:
    MOVC dst=0 val=10
    JMP jmp=@label_014B
.org 0x0140
@label_0140:
    MOVC dst=0 val=6
    JMP jmp=@label_014B
.org 0x014B
@label_014B:
    TDEGS
.org 0x014C
@label_014C:
    TSPDR v=2 a=0
    TAMA2
    NOP count=2
    DEC reg=0
    CMPC reg=0 val=0
    JL jmp=@label_014C
    DEGXU
.org 0x0161
@label_0161:
    MOV count=200
    JMP jmp=@label_0161
.org 0x0169
@script_3:
    SETUP hp=150 score=500
    RLCHG_ON
    CLIP_ON
    ANM pattern=2 speed=0
    SPDA speed=128
    DEGA angle=64
    TCMD cmd=80
    TDEGA angle=0 dw=10
    TNUMA n=1 ns=0
    TSPDA v=22 a=4
    TTYPE type=1
    TREP rep=30
    TCOL color=32
    ROL deg=2 count=16
    ROL deg=-2 count=32
    ROL deg=2 count=16
    CLIP_OFF
    JDIF easy=@label_01CE norm=@label_01C3 hard=@label_01B8 luna=@label_01AD
.org 0x01AD
@label_01AD:
    MOVC dst=0 val=14
    JMP jmp=@label_01D9
.org 0x01B8
@label_01B8:
    MOVC dst=0 val=12
    JMP jmp=@label_01D9
.org 0x01C3
@label_01C3:
    MOVC dst=0 val=10
    JMP jmp=@label_01D9
.org 0x01CE
@label_01CE:
    MOVC dst=0 val=6
    JMP jmp=@label_01D9
.org 0x01D9
@label_01D9:
    TDEGS
.org 0x01DA
@label_01DA:
    TAMA2
    NOP count=5
    DEC reg=0
    CMPC reg=0 val=0
    JL jmp=@label_01DA
    DEGXU
.org 0x01EC
@label_01EC:
    MOV count=200
    JMP jmp=@label_01EC
.org 0x01F4
@script_4:
    SETUP hp=150 score=10000
    RLCHG_ON
    CLIP_ON
    ANM pattern=3 speed=8
    SPDA speed=192
    DEGA angle=64
    LCMD cmd=0
    LLA len=960
    LDEGA angle=64 dw=2
    LNUMA n=1
    LSPDA v=256
    LCOL color=0
    LTYPE type=0
    LWA w=192
    MOVC dst=0 val=1
    JDIF easy=@label_0240 norm=@label_023E hard=@label_023C luna=@label_023A
.org 0x023A
@label_023A:
    INC reg=0
.org 0x023C
@label_023C:
    INC reg=0
.org 0x023E
@label_023E:
    INC reg=0
.org 0x0240
@label_0240:
    INC reg=0
    MOV count=40
    NOP count=10
.org 0x0248
@label_0248:
    DEGX
    MOV count=10
    MOVR dst=1 src=0
    PSE id=3
.org 0x0251
@label_0251:
    LXY x=-10 y=0
    LASER2
    LXY x=10 y=0
    LASER2
    NOP count=3
    DEC reg=1
    CMPC reg=1 val=0
    JL jmp=@label_0251
    NOP count=60
    LOOP jmp=@label_0248 count=2
    DEGA angle=192
    CLIP_OFF
.org 0x027A
@label_027A:
    MOV count=200
    JMP jmp=@label_027A
.org 0x0282
@script_5:
    SETUP hp=12 score=100
    RLCHG_ON
    CLIP_ON
    ANM pattern=4 speed=0
    SPDA speed=192
    DEGS
    DEGR angle=-16
.org 0x0298
@label_0298:
    MOV count=2
    SPDR speed=8
    DEGR angle=1
    CLIP_OFF
    JDSB jmp=@label_02AD
    JMP jmp=@label_0298
.org 0x02AD
@label_02AD:
    MOV count=2
    SPDR speed=8
    JMP jmp=@label_02AD
.org 0x02BA
@script_6:
    CALL jmp=@script_8
    MOVC dst=0 val=0
    CALL jmp=@label_031B
.org 0x02CA
@script_7:
    CALL jmp=@script_8
    MOVC dst=0 val=1
    CALL jmp=@label_031B
.org 0x02DA
@script_8:
    SETUP hp=300 score=1000
    RLCHG_ON
    ANM pattern=5 speed=0
    SPDA speed=384
    DEGA angle=64
    TCMD cmd=0
    TDEGA angle=64 dw=18
    TSPDA v=16 a=-1
    TREP rep=40
    TTYPE type=1
    JDIF easy=@label_030B norm=@label_030F hard=@label_0313 luna=@label_0317
.org 0x030B
@label_030B:
    TNUMA n=3 ns=1
    RET
.org 0x030F
@label_030F:
    TNUMA n=5 ns=1
    RET
.org 0x0313
@label_0313:
    TNUMA n=7 ns=1
    RET
.org 0x0317
@label_0317:
    TNUMA n=9 ns=1
    RET
.org 0x031B
@label_031B:
    MOV count=2
    SPDR speed=-32
    LOOP jmp=@label_031B count=11
.org 0x032A
@label_032A:
    DEGR angle=-7
    NOP count=1
    LOOP jmp=@label_032A count=13
.org 0x0336
@label_0336:
    DEGR angle=7
    TDEGE
    TCOL color=20
    TAMA2
    NOP count=3
    DEGR angle=7
    TDEGE
    MOVR dst=139 src=0
    TAMA2
    NOP count=3
    LOOP jmp=@label_0336 count=13
    NOP count=60
.org 0x0353
@label_0353:
    MOV count=1
    SPDR speed=8
    JMP jmp=@label_0353
.org 0x0360
@script_9:
    SETUP hp=33750 score=100000
    STI jmp=@label_0460 vector=HP val=22500
    STI jmp=@label_0460 vector=TIMER val=6000
    HITSB_OFF
    DAMAGE_OFF
    ANM pattern=6 speed=0
    ANMEX pattern=7
    HITXY w=80 h=16
    SPDA speed=128
    DEGA angle=64
.org 0x0390
@label_0390:
    MXYA x=319 y=100 count=100
    DAMAGE_ON
    MOVC dst=0 val=0
    MOVC dst=1 val=8
.org 0x03A4
@label_03A4:
    CALL jmp=@script_10
    MOVR dst=134 src=0
    ADD dst=0 src=1
    TAMA2
    NOP count=2
    LOOP jmp=@label_03A4 count=16
    CALL jmp=@label_081E
    NOP count=80
    MOVC dst=0 val=128
    MOVC dst=1 val=8
.org 0x03CE
@label_03CE:
    SUB dst=0 src=1
    CALL jmp=@script_10
    MOVR dst=134 src=0
    TAMA2
    NOP count=2
    LOOP jmp=@label_03CE count=16
    CALL jmp=@label_081E
    NOP count=160
    CALL jmp=@label_084E
    NOP count=60
    CALL jmp=@label_085F
    NOP count=100
    CALL jmp=@label_0890
    PSE id=1
    CALL jmp=@label_0AFE
    NOP count=160
    SPDA speed=32
    DEGA angle=192
.org 0x0412
@label_0412:
    MOV count=3
    LLDEGR id=0 deg=1
    LLDEGR id=1 deg=-1
    LOOP jmp=@label_0412 count=5
    LLOPEN id=255
    SPDA speed=192
.org 0x0429
@label_0429:
    PSE id=11
    ENEMYSET dx=150 dy=-70 id=5
    ENEMYSET dx=-150 dy=-70 id=5
    NOP count=2
    DEGR angle=-128
    MOV count=3
    LLDEGR id=0 deg=1
    LLDEGR id=1 deg=-1
    SPDR speed=-24
    DEGR angle=-128
    MOV count=3
    LOOP jmp=@label_0429 count=9
    LLCLOSE id=255
    NOP count=100
    JMP jmp=@label_0390
.org 0x0460
@label_0460:
    SETUP hp=22500 score=100000
    STI jmp=@label_057D vector=HP val=13500
    STI jmp=@label_057D vector=TIMER val=6000
    LLCLOSE id=255
    TCLR
    TXYR dx=0 dy=0
    STG4EFC cmd=1
.org 0x0487
@label_0487:
    MXYA x=319 y=100 count=100
    CALL jmp=@label_0903
    PSE id=1
    CALL jmp=@label_0890
    CALL jmp=@label_0B21
    DEGS
    NOP count=80
    SPDA speed=16
.org 0x04A8
@label_04A8:
    MOV count=3
    LLDEGR id=0 deg=1
    LLDEGR id=1 deg=-1
    LOOP jmp=@label_04A8 count=25
    SPDA speed=192
    LLOPEN id=255
.org 0x04BF
@label_04BF:
    PSE id=11
    ENEMYSET dx=150 dy=-70 id=11
    ENEMYSET dx=-150 dy=-70 id=11
    ENEMYSET dx=150 dy=-70 id=11
    ENEMYSET dx=-150 dy=-70 id=11
    NOP count=4
    DEGR angle=-128
    MOV count=3
    SPDR speed=-16
    LLDEGR id=0 deg=1
    LLDEGR id=1 deg=-1
    DEGR angle=-128
    MOV count=3
    LLDEGR id=0 deg=1
    LLDEGR id=1 deg=-1
    LOOP jmp=@label_04BF count=11
    LLCLOSE id=255
    MXYA x=319 y=100 count=10
    CALL jmp=@label_08B0
    STG4EFC cmd=1
    RND reg=0
    MOD reg=0 div=200
    CMPC reg=0 val=100
    JL jmp=@label_0547
    DEGA angle=236
    SPDA speed=128
    MOV count=60
    DEGR angle=-128
    LDEGA angle=0 dw=6
.org 0x0530
@label_0530:
    MOV count=5
    CALL jmp=@label_0A31
    LDEGR angle=4 dw=0
    LOOP jmp=@label_0530 count=23
    JMP jmp=@label_056D
.org 0x0547
@label_0547:
    DEGA angle=148
    SPDA speed=128
    MOV count=60
    DEGR angle=-128
    LDEGA angle=128 dw=6
.org 0x0556
@label_0556:
    MOV count=5
    CALL jmp=@label_0A31
    LDEGR angle=-4 dw=0
    LOOP jmp=@label_0556 count=23
    JMP jmp=@label_056D
.org 0x056D
@label_056D:
    NOP count=30
    CALL jmp=@label_08EC
    NOP count=20
    JMP jmp=@label_0487
.org 0x057D
@label_057D:
    SETUP hp=13500 score=100000
    STI jmp=@label_071B vector=HP val=2250
    STI jmp=@label_071B vector=TIMER val=6000
    LLCLOSE id=255
    TCLR
    TXYR dx=0 dy=0
    MXYA x=319 y=100 count=100
.org 0x05A9
@label_05A9:
    MOVC dst=1 val=160
    RND reg=0
    MOD reg=0 div=64
    ADD dst=0 src=1
    MOVR dst=145 src=0
    SPDA speed=16
    STG4EFC cmd=1
.org 0x05C4
@label_05C4:
    MOV count=5
    SPDR speed=16
    LOOP jmp=@label_05C4 count=12
    CALL jmp=@label_09AE
    DEGA angle=64
    SPDA speed=16
.org 0x05DF
@label_05DF:
    MOV count=5
    SPDR speed=16
    LOOP jmp=@label_05DF count=12
    CALL jmp=@script_10
    PSE id=1
.org 0x05F5
@label_05F5:
    TAMA2
    NOP count=1
    TDEGR angle=8 dw=0
    LOOP jmp=@label_05F5 count=16
    STG4EFC cmd=2
    NOP count=20
.org 0x0608
@label_0608:
    MOV count=10
    SPDR speed=-16
    CALL jmp=@label_0951
    LOOP jmp=@label_0608 count=10
    RND reg=0
    MOD reg=0 div=180
    CMPC reg=0 val=60
    JS jmp=@label_0646
    CMPC reg=0 val=120
    JS jmp=@label_0652
    MXYA x=279 y=100 count=80
    JMP jmp=@label_0659
.org 0x0646
@label_0646:
    MXYA x=319 y=100 count=80
    JMP jmp=@label_0659
.org 0x0652
@label_0652:
    MXYA x=359 y=100 count=80
.org 0x0659
@label_0659:
    PSE id=1
    NOP count=60
    CALL jmp=@label_08EC
    CALL jmp=@label_0B53
    NOP count=130
    LLOPEN id=255
    SPDA speed=256
    DEGS
    MOVC dst=2 val=16
    MOVR dst=3 src=145
.org 0x067C
@label_067C:
    DEC reg=2
    MOVC dst=0 val=32
    MOVC dst=1 val=4294967264
    ADD dst=0 src=3
    ADD dst=1 src=3
    MOVC dst=4 val=0
.org 0x0696
@label_0696:
    PSE id=11
    ENEMYSETD dx=60 dy=-30 reg=0 id=12
    ENEMYSETD dx=-60 dy=-30 reg=1 id=12
    ADD dst=1 src=2
    SUB dst=0 src=2
    INC reg=4
    CMPC reg=4 val=12
    JS jmp=@label_0696
    NOP count=2
    DEGR angle=-128
    MOV count=4
    SPDR speed=-12
    LLDEGR id=0 deg=2
    LLDEGR id=1 deg=-2
    LLDEGR id=2 deg=1
    LLDEGR id=3 deg=-1
    DEGR angle=-128
    MOV count=4
    NOP count=2
    DEGR angle=-128
    MOV count=4
    SPDR speed=-12
    LLDEGR id=0 deg=2
    LLDEGR id=1 deg=-2
    LLDEGR id=2 deg=1
    LLDEGR id=3 deg=-1
    DEGR angle=-128
    MOV count=4
    LOOP jmp=@label_067C count=8
    LLCLOSE id=255
    NOP count=60
    CALL jmp=@label_0A71
    NOP count=40
    CALL jmp=@label_0A88
    NOP count=10
    CALL jmp=@label_0964
    JMP jmp=@label_05A9
.org 0x071B
@label_071B:
    SETUP hp=2250 score=100000
    CLI vector=HP
    STI jmp=@label_07DD vector=TIMER val=1400
    LLCLOSE id=255
    TCLR
    TXYR dx=0 dy=0
    STG4EFC cmd=5
    MXYA x=319 y=100 count=100
    MOVC dst=0 val=0
    MOVC dst=1 val=5
.org 0x074D
@label_074D:
    CALL jmp=@label_0A20
    TDEGA angle=0 dw=1
.org 0x0755
@label_0755:
    TAMA
    TDEGR angle=16 dw=0
    LOOP jmp=@label_0755 count=15
    CALL jmp=@label_0A05
.org 0x0765
@label_0765:
    MOVR dst=134 src=0
    ADD dst=0 src=1
    PSE id=0
    TAMA
    NOP count=2
    LOOP jmp=@label_0765 count=30
    CALL jmp=@label_0A20
    TDEGA angle=8 dw=1
.org 0x0780
@label_0780:
    TAMA
    TDEGR angle=16 dw=0
    LOOP jmp=@label_0780 count=15
    CALL jmp=@label_0A05
.org 0x0790
@label_0790:
    MOVR dst=134 src=0
    ADD dst=0 src=1
    PSE id=0
    TAMA
    NOP count=2
    LOOP jmp=@label_0790 count=30
    JMP jmp=@label_074D
.org 0x07A8
@label_07A8:
    CALL jmp=@label_0A05
.org 0x07AD
@label_07AD:
    TDEGR angle=7 dw=0
    TAMA
    NOP count=2
    LOOP jmp=@label_07AD count=60
    CALL jmp=@label_0A14
    TREP rep=10
.org 0x07C2
@label_07C2:
    RND reg=0
    MOVR dst=142 src=0
    TDEGR angle=-5 dw=0
    TAMA
    NOP count=10
    LOOP jmp=@label_07C2 count=10
    TNUMR n=1 ns=0
    JMP jmp=@label_07A8
.org 0x07DD
@label_07DD:
    SETUP hp=0 score=0  ; death marker
    TCLR
.org 0x07E7
@label_07E7:
    NOP count=1000
    JMP jmp=@label_07E7
.org 0x07EF
@script_10:
    TCMD cmd=4
    TDEGA angle=0 dw=1
    TSPDA v=13 a=0
    TTYPE type=0
    TCOL color=33
    TOPT opt=64
    JDIF easy=@label_080E norm=@label_0812 hard=@label_0816 luna=@label_081A
.org 0x080E
@label_080E:
    TNUMA n=1 ns=2
    RET
.org 0x0812
@label_0812:
    TNUMA n=2 ns=2
    RET
.org 0x0816
@label_0816:
    TNUMA n=3 ns=2
    RET
.org 0x081A
@label_081A:
    TNUMA n=3 ns=3
    RET
.org 0x081E
@label_081E:
    TCMD cmd=0
    TNUMA n=4 ns=1
    TDEGA angle=0 dw=10
    TSPDA v=18 a=0
    TTYPE type=0
    TOPT opt=0
    TCOL color=21
    TDEGS
.org 0x0830
@label_0830:
    PSE id=11
    TXYR dx=150 dy=-70
    TAMA
    TXYR dx=-150 dy=-70
    TAMA
    TXYR dx=0 dy=0
    NOP count=6
    LOOP jmp=@label_0830 count=3
    RET
.org 0x084E
@label_084E:
    MOVC dst=0 val=0
    MOVC dst=1 val=4
    JMP jmp=@label_0870
.org 0x085F
@label_085F:
    MOVC dst=0 val=128
    MOVC dst=1 val=4294967292
    JMP jmp=@label_0870
.org 0x0870
@label_0870:
    TCMD cmd=0
    TNUMA n=1 ns=1
    TSPDA v=18 a=0
    TTYPE type=0
    TOPT opt=113
    TCOL color=17
.org 0x087E
@label_087E:
    MOVR dst=134 src=0
    ADD dst=0 src=1
    TAMA2
    NOP count=3
    LOOP jmp=@label_087E count=8
    RET
.org 0x0890
@label_0890:
    TCMD cmd=0
    TNUMA n=2 ns=1
    TSPDA v=18 a=0
    TTYPE type=0
    TOPT opt=114
    TCOL color=16
    TDEGA angle=64 dw=178
.org 0x08A1
@label_08A1:
    TAMA2
    TDEGR angle=0 dw=-8
    NOP count=3
    LOOP jmp=@label_08A1 count=12
    RET
.org 0x08B0
@label_08B0:
    TCMD cmd=0
    TNUMA n=5 ns=1
    TSPDA v=18 a=0
    TDEGA angle=0 dw=10
    TTYPE type=0
    TOPT opt=112
    TCOL color=18
    MOVC dst=0 val=4294967276
    MOVC dst=1 val=148
    MOVC dst=2 val=8
.org 0x08D3
@label_08D3:
    MOVR dst=134 src=0
    TAMA2
    MOVR dst=134 src=1
    TAMA2
    ADD dst=0 src=2
    SUB dst=1 src=2
    NOP count=12
    LOOP jmp=@label_08D3 count=4
    RET
.org 0x08EC
@label_08EC:
    TCMD cmd=0
    TNUMA n=8 ns=1
    TSPDA v=18 a=0
    TDEGA angle=0 dw=8
    TTYPE type=0
    TOPT opt=117
    TCOL color=19
    TAMA2
    TDEGR angle=-128 dw=0
    TAMA2
    RET
.org 0x0903
@label_0903:
    TCMD cmd=0
    TNUMA n=8 ns=1
    TSPDA v=22 a=0
    TDEGA angle=64 dw=16
    TTYPE type=0
    TOPT opt=0
    TCOL color=34
    TDEGS
    MOVC dst=1 val=0
    MOVC dst=2 val=8
    MOVR dst=3 src=134
.org 0x0924
@label_0924:
    PSE id=0
    TAMA
    NOP count=3
    LOOP jmp=@label_0924 count=16
.org 0x0931
@label_0931:
    MOVC dst=0 val=10
    SINL len=0 deg=1
    ADD dst=0 src=3
    ADD dst=1 src=2
    MOVR dst=134 src=0
    PSE id=0
    TAMA
    NOP count=3
    LOOP jmp=@label_0931 count=32
    RET
.org 0x0951
@label_0951:
    TCMD cmd=4
    TNUMA n=2 ns=2
    TSPDA v=18 a=0
    TDEGA angle=192 dw=100
    TTYPE type=0
    TOPT opt=114
    TCOL color=16
    TAMA2
    RET
.org 0x0964
@label_0964:
    TCMD cmd=81
    TNUMA n=7 ns=1
    TSPDA v=16 a=0
    TDEGA angle=0 dw=24
    TTYPE type=0
    TOPT opt=0
    TCOL color=34
    RND reg=0
    MOVC dst=1 val=20
    MOVC dst=3 val=7
.org 0x0983
@label_0983:
    MOVR dst=2 src=1
    SINL len=2 deg=0
    MOVR dst=134 src=2
    PSE id=0
    TAMA
    NOP count=4
    ADD dst=0 src=3
    LOOP jmp=@label_0983 count=10
    TSPDR v=1 a=0
    ADD dst=1 src=3
    CMPC reg=1 val=130
    JS jmp=@label_0983
    RET
.org 0x09AE
@label_09AE:
    TCMD cmd=0
    TNUMA n=11 ns=1
    TSPDA v=5 a=16
    TREP rep=20
    TDEGA angle=0 dw=6
    TTYPE type=1
    TOPT opt=0
    TCOL color=21
    TXYR dx=0 dy=80
    TDEGS
    MOVC dst=0 val=6
.org 0x09CD
@label_09CD:
    TAMA
    PSE id=11
    NOP count=5
    TNUMR n=1 ns=0
    DEC reg=0
    CMPC reg=0 val=0
    JL jmp=@label_09CD
    MOVC dst=0 val=6
.org 0x09E9
@label_09E9:
    TAMA
    PSE id=11
    NOP count=5
    TNUMR n=-1 ns=0
    DEC reg=0
    CMPC reg=0 val=0
    JL jmp=@label_09E9
    TXYR dx=0 dy=0
    RET
.org 0x0A05
@label_0A05:
    TCMD cmd=81
    TNUMA n=7 ns=1
    TSPDA v=16 a=0
    TTYPE type=0
    TOPT opt=0
    TCOL color=1
    RET
.org 0x0A14
@label_0A14:
    TCMD cmd=1
    TSPDA v=18 a=0
    TTYPE type=8
    TOPT opt=0
    TCOL color=17
    RET
.org 0x0A20
@label_0A20:
    TCMD cmd=0
    TNUMA n=6 ns=1
    TSPDA v=22 a=-5
    TREP rep=40
    TTYPE type=1
    TOPT opt=0
    TCOL color=17
    RET
.org 0x0A31
@label_0A31:
    LCMD cmd=0
    LLA len=12800
    LNUMA n=1
    LSPDA v=576
    LCOL color=0
    LTYPE type=0
    LWA w=192
    LXY x=0 y=-40
    JDIF easy=@label_0A6D norm=@label_0A68 hard=@label_0A63 luna=@label_0A5E
.org 0x0A5E
@label_0A5E:
    LLR len=1280
.org 0x0A63
@label_0A63:
    LLR len=1280
.org 0x0A68
@label_0A68:
    LLR len=1280
.org 0x0A6D
@label_0A6D:
    PSE id=3
    LASER2
    RET
.org 0x0A71
@label_0A71:
    MOVC dst=0 val=50
    MOVC dst=1 val=78
    MOVC dst=2 val=5
    JMP jmp=@label_0A9F
.org 0x0A88
@label_0A88:
    MOVC dst=0 val=110
    MOVC dst=1 val=18
    MOVC dst=2 val=4294967289
    JMP jmp=@label_0A9F
.org 0x0A9F
@label_0A9F:
    LCMD cmd=0
    LLA len=9600
    LNUMA n=1
    LSPDA v=512
    LCOL color=0
    LTYPE type=0
    LWA w=192
    LDEGA angle=0 dw=4
    JDIF easy=@label_0AD9 norm=@label_0AD4 hard=@label_0ACF luna=@label_0ACA
.org 0x0ACA
@label_0ACA:
    LLR len=1280
.org 0x0ACF
@label_0ACF:
    LLR len=1280
.org 0x0AD4
@label_0AD4:
    LLR len=1280
.org 0x0AD9
@label_0AD9:
    PSE id=3
    LXY x=130 y=-60
    MOVR dst=128 src=0
    LASER2
    LXY x=-130 y=-60
    MOVR dst=128 src=1
    LASER2
    ADD dst=0 src=2
    SUB dst=1 src=2
    NOP count=4
    LOOP jmp=@label_0AD9 count=12
    RET
.org 0x0AFE
@label_0AFE:
    LNUMA n=1
    LSPDA v=64
    LCOL color=1
    LWA w=2240
    LTYPE type=0
    LDEGA angle=35 dw=0
    LXY x=20 y=-40
    LLSET
    LDEGA angle=93 dw=0
    LXY x=-20 y=-40
    LLSET
    RET
.org 0x0B21
@label_0B21:
    LNUMA n=1
    LSPDA v=64
    LCOL color=2
    LWA w=2240
    LTYPE type=0
    LDEGA angle=0 dw=0
    LDEGS
    LDEGR angle=-70 dw=0
    LXY x=20 y=-40
    LLSET
    LDEGR angle=-116 dw=0
    LXY x=-20 y=-40
    LLSET
    LCOL color=1
    LDEGR angle=-70 dw=0
    LXY x=0 y=-40
    LLSET
    RET
.org 0x0B53
@label_0B53:
    LNUMA n=1
    LSPDA v=64
    LCOL color=2
    LWA w=960
    LTYPE type=0
    LXY x=0 y=-40
    LDEGS
    LDEGR angle=-60 dw=0
    LLSET
    LDEGR angle=120 dw=0
    LLSET
    LCOL color=1
    LDEGR angle=-90 dw=0
    LLSET
    LDEGR angle=60 dw=0
    LLSET
    LCOL color=2
    LDEGR angle=-30 dw=0
    LLSET
    RET
.org 0x0B82
@script_11:
    SETUP hp=12 score=100
    RLCHG_ON
    CLIP_ON
    ANM pattern=4 speed=0
    SPDA speed=192
    DEGXD
.org 0x0B96
@label_0B96:
    MOV count=2
    SPDR speed=8
    DEGR angle=1
    CLIP_OFF
    JDSB jmp=@label_0BAB
    JMP jmp=@label_0B96
.org 0x0BAB
@label_0BAB:
    MOV count=2
    SPDR speed=8
    JMP jmp=@label_0BAB
.org 0x0BB8
@script_12:
    SETUP hp=12 score=100
    RLCHG_ON
    ANM pattern=4 speed=0
    SPDA speed=192
    MOV count=16
.org 0x0BCD
@label_0BCD:
    MOV count=2
    SPDR speed=8
    JMP jmp=@label_0BCD
