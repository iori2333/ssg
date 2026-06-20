.header 60
.offset 0 0x00F4
.offset 1 0x010A
.offset 2 0x05C4
.offset 3 0x0909
.offset 4 0x098F
.offset 5 0x09FB
.offset 6 0x0A6A
.offset 7 0x0A6A
.offset 8 0x0A6A
.offset 9 0x0A6A
.offset 10 0x0A6A
.offset 11 0x0A6A
.offset 12 0x0A6A
.offset 13 0x0A6A
.offset 14 0x0A6A
.offset 15 0x0A6A
.offset 16 0x0A6A
.offset 17 0x0A6A
.offset 18 0x0B37
.offset 19 0x0B88
.offset 20 0x0BD3
.offset 21 0x0BE9
.offset 22 0x14C4
.offset 23 0x1B37
.offset 24 0x1B53
.offset 25 0x1B7B
.offset 26 0x1BA3
.offset 27 0x1BCB
.offset 28 0x1BF3
.offset 29 0x1C0F
.offset 30 0x1C7A
.offset 31 0x1CD8
.offset 32 0x1D16
.offset 33 0x1D16
.offset 34 0x1D16
.offset 35 0x1D57
.offset 36 0x1D9E
.offset 37 0x1DE5
.offset 38 0x1E25
.offset 39 0x1E90
.offset 40 0x1E97
.offset 41 0x1E9E
.offset 42 0x1EA5
.offset 43 0x1EAC
.offset 44 0x1EB3
.offset 45 0x1EFE
.offset 46 0x1F50
.offset 47 0x20E1
.offset 48 0x219C
.offset 49 0x21BE
.offset 50 0x21F6
.offset 51 0x22AF
.offset 52 0x2305
.offset 53 0x236C
.offset 54 0x00F4
.offset 55 0x00F4
.offset 56 0x00F4
.offset 57 0x00F4
.offset 58 0x00F4
.offset 59 0x00F4

.org 0x00F4
@script_0:
@script_54:  ; shared
@script_55:  ; shared
@script_56:  ; shared
@script_57:  ; shared
@script_58:  ; shared
@script_59:  ; shared
    SETUP hp=100000 score=100
    DAMAGE_OFF
    HITSB_OFF
.org 0x00FF
@label_00FF:
    ANM pattern=0 speed=10
    NOP count=60
    JMP jmp=@label_00FF
.org 0x010A
@script_1:
    SETUP hp=12000 score=50000
    STI jmp=@label_02C6 vector=HP val=8000
    STI jmp=@label_02C8 vector=TIMER val=3600
    ITEM type=2
    INT id=5
    ANM pattern=0 speed=10
    NOP count=60
    CALL jmp=@label_06C5
    NOP count=32
    PSE id=1
    MXYA x=319 y=89 count=36
    CALL jmp=@label_06CC
    NOP count=36
    CALL jmp=@label_06D1
    RND reg=4
    MOVC dst=6 val=0
.org 0x0157
@label_0157:
    NOP count=80
    DEGX2
    CALL jmp=@label_069B
    SPDA speed=320
    ACC accel=-6 count=60
    RND reg=5
    MOD reg=5 div=3
    ADD dst=4 src=5
    INC reg=4
    MOD reg=4 div=4
    CMPC reg=4 val=0
    JEQ jmp=@label_01CB
    CMPC reg=4 val=1
    JEQ jmp=@label_01BC
    CMPC reg=4 val=2
    JEQ jmp=@label_01AC
    CALL jmp=@label_06D8
    CALL jmp=@script_2
    JMP jmp=@label_01DB
.org 0x01AC
@label_01AC:
    DEGX2
    CALL jmp=@label_069B
    CALL jmp=@label_060B
    JMP jmp=@label_01DB
.org 0x01BC
@label_01BC:
    CALL jmp=@label_06D8
    CALL jmp=@label_0629
    JMP jmp=@label_01DB
.org 0x01CB
@label_01CB:
    DEGX2
    CALL jmp=@label_069B
    CALL jmp=@label_0675
    JMP jmp=@label_01DB
.org 0x01DB
@label_01DB:
    CALL jmp=@label_06D1
    INC reg=6
    CMPC reg=6 val=1
    JL jmp=@label_01F2
    JMP jmp=@label_0157
.org 0x01F2
@label_01F2:
    PSE id=1
    RND reg=4
    NOP count=180
    INT id=3
    STI jmp=@label_02AF vector=BITLEFT val=0
    NOP count=30
.org 0x0208
@label_0208:
    DEGX2
    CALL jmp=@label_069B
    SPDA speed=320
    ACC accel=-6 count=60
    CALL jmp=@label_06D8
    RND reg=5
    MOD reg=5 div=5
    ADD dst=4 src=5
    INC reg=4
    MOD reg=4 div=6
    CMPC reg=4 val=0
    JEQ jmp=@label_0298
    CMPC reg=4 val=1
    JEQ jmp=@label_028E
    CMPC reg=4 val=2
    JEQ jmp=@label_0284
    CMPC reg=4 val=3
    JEQ jmp=@label_027A
    CMPC reg=4 val=4
    JEQ jmp=@label_0270
    CALL jmp=@label_0735
    JMP jmp=@label_02A2
.org 0x0270
@label_0270:
    CALL jmp=@label_06E4
    JMP jmp=@label_02A2
.org 0x027A
@label_027A:
    CALL jmp=@label_073E
    JMP jmp=@label_02A2
.org 0x0284
@label_0284:
    CALL jmp=@label_0819
    JMP jmp=@label_02A2
.org 0x028E
@label_028E:
    CALL jmp=@label_089D
    JMP jmp=@label_02A2
.org 0x0298
@label_0298:
    CALL jmp=@label_08B9
    JMP jmp=@label_02A2
.org 0x02A2
@label_02A2:
    CALL jmp=@label_06D1
    NOP count=60
    JMP jmp=@label_0208
.org 0x02AF
@label_02AF:
    CLI vector=BITLEFT
    CALL jmp=@label_06D1
    NOP count=100
    RND reg=4
    MOVC dst=6 val=0
    JMP jmp=@label_0157
.org 0x02C6
@label_02C6:
    T2ITEM pct=3
.org 0x02C8
@label_02C8:
    SETUP hp=8000 score=50000
    STI jmp=@label_049C vector=HP val=1500
    STI jmp=@label_049E vector=TIMER val=9000
    CLI vector=BITLEFT
    TCLR
    DAMAGE_OFF
    SPDA speed=80
    DEGA angle=192
    CALL jmp=@label_06DF
    MOV count=36
    NOP count=10
    DAMAGE_ON
    CALL jmp=@label_06C5
    NOP count=32
    PSE id=1
    MXYA x=319 y=89 count=36
    CALL jmp=@label_06CC
    NOP count=36
    CALL jmp=@label_06D1
    RND reg=4
    MOVC dst=6 val=0
.org 0x0322
@label_0322:
    NOP count=50
    DEGX2
    CALL jmp=@label_069B
    SPDA speed=320
    ACC accel=-6 count=60
    RND reg=5
    MOD reg=5 div=3
    ADD dst=4 src=5
    INC reg=4
    MOD reg=4 div=4
    CMPC reg=4 val=0
    JEQ jmp=@label_0396
    CMPC reg=4 val=1
    JEQ jmp=@label_0387
    CMPC reg=4 val=2
    JEQ jmp=@label_0377
    CALL jmp=@label_06D8
    CALL jmp=@script_2
    JMP jmp=@label_03A6
.org 0x0377
@label_0377:
    DEGX2
    CALL jmp=@label_069B
    CALL jmp=@label_060B
    JMP jmp=@label_03A6
.org 0x0387
@label_0387:
    CALL jmp=@label_06D8
    CALL jmp=@label_0629
    JMP jmp=@label_03A6
.org 0x0396
@label_0396:
    DEGX2
    CALL jmp=@label_069B
    CALL jmp=@label_0675
    JMP jmp=@label_03A6
.org 0x03A6
@label_03A6:
    CALL jmp=@label_06D1
    INC reg=6
    CMPC reg=6 val=3
    JL jmp=@label_03BD
    JMP jmp=@label_0322
.org 0x03BD
@label_03BD:
    PSE id=1
    RND reg=4
    NOP count=180
    INT id=4
    STI jmp=@label_0485 vector=BITLEFT val=0
    NOP count=30
.org 0x03D3
@label_03D3:
    DEGX2
    CALL jmp=@label_069B
    SPDA speed=320
    ACC accel=-6 count=60
    CALL jmp=@label_06D8
    RND reg=5
    MOD reg=5 div=5
    ADD dst=4 src=5
    INC reg=4
    MOD reg=4 div=6
    CMPC reg=4 val=0
    JEQ jmp=@label_043C
    CMPC reg=4 val=1
    JEQ jmp=@label_0446
    CMPC reg=4 val=2
    JEQ jmp=@label_0450
    CMPC reg=4 val=3
    JEQ jmp=@label_045A
    CMPC reg=4 val=4
    JEQ jmp=@label_0464
    CMPC reg=4 val=5
    JEQ jmp=@label_046E
.org 0x043C
@label_043C:
    CALL jmp=@label_0735
    JMP jmp=@label_0478
.org 0x0446
@label_0446:
    CALL jmp=@label_06E4
    JMP jmp=@label_0478
.org 0x0450
@label_0450:
    CALL jmp=@label_073E
    JMP jmp=@label_0478
.org 0x045A
@label_045A:
    CALL jmp=@label_0819
    JMP jmp=@label_0478
.org 0x0464
@label_0464:
    CALL jmp=@label_089D
    JMP jmp=@label_0478
.org 0x046E
@label_046E:
    CALL jmp=@label_08B9
    JMP jmp=@label_0478
.org 0x0478
@label_0478:
    CALL jmp=@label_06D1
    NOP count=60
    JMP jmp=@label_03D3
.org 0x0485
@label_0485:
    CLI vector=BITLEFT
    CALL jmp=@label_06D1
    NOP count=100
    RND reg=4
    MOVC dst=6 val=0
    JMP jmp=@label_0322
.org 0x049C
@label_049C:
    T2ITEM pct=3
.org 0x049E
@label_049E:
    SETUP hp=1500 score=50000
    CLI vector=BITLEFT
    CLI vector=HP
    STI jmp=@label_05B1 vector=TIMER val=3600
    TCLR
    DAMAGE_OFF
    SPDA speed=80
    DEGA angle=192
    CALL jmp=@label_06DF
    DAMAGE_OFF
    MOV count=36
    NOP count=10
    CALL jmp=@label_06C5
    DAMAGE_OFF
    NOP count=32
    PSE id=1
    MXYA x=319 y=89 count=36
.org 0x04DC
@label_04DC:
    DRAW_OFF
    NOP count=6
    DRAW_ON
    NOP count=6
    LOOP jmp=@label_04DC count=2
.org 0x04EB
@label_04EB:
    DRAW_OFF
    NOP count=4
    DRAW_ON
    NOP count=4
    LOOP jmp=@label_04EB count=4
.org 0x04FA
@label_04FA:
    DRAW_OFF
    NOP count=2
    DRAW_ON
    NOP count=2
    LOOP jmp=@label_04FA count=8
    BOSSSET id=17
    DAMAGE_ON
    DEGX2
    CALL jmp=@label_069B
    SPDA speed=320
    ACC accel=-6 count=60
    CALL jmp=@label_06CC
    NOP count=36
.org 0x0523
@label_0523:
    NOP count=50
    DEGX2
    CALL jmp=@label_069B
    SPDA speed=320
    ACC accel=-6 count=60
    RND reg=5
    MOD reg=5 div=3
    ADD dst=4 src=5
    INC reg=4
    MOD reg=4 div=4
    CMPC reg=4 val=0
    JEQ jmp=@label_0597
    CMPC reg=4 val=1
    JEQ jmp=@label_0588
    CMPC reg=4 val=2
    JEQ jmp=@label_0578
    CALL jmp=@label_06D8
    CALL jmp=@script_2
    JMP jmp=@label_05A7
.org 0x0578
@label_0578:
    DEGX2
    CALL jmp=@label_069B
    CALL jmp=@label_060B
    JMP jmp=@label_05A7
.org 0x0588
@label_0588:
    CALL jmp=@label_06D8
    CALL jmp=@label_0629
    JMP jmp=@label_05A7
.org 0x0597
@label_0597:
    DEGX2
    CALL jmp=@label_069B
    CALL jmp=@label_0675
    JMP jmp=@label_05A7
.org 0x05A7
@label_05A7:
    CALL jmp=@label_06D1
    JMP jmp=@label_0523
.org 0x05B1
@label_05B1:
    SETUP hp=0 score=0  ; death marker
    CLI vector=TIMER
    NOP count=1000
    JMP jmp=@label_05B1
.org 0x05C4
@script_2:
    RND reg=0
    MOD reg=0 div=65536
    CMPC reg=0 val=32768
    JL jmp=@label_05E8
    MOVC dst=0 val=4294967280
    MOVC dst=1 val=16
    JMP jmp=@label_05F4
.org 0x05E8
@label_05E8:
    MOVC dst=0 val=144
    MOVC dst=1 val=4294967280
.org 0x05F4
@label_05F4:
    PSE id=4
    ENEMYSETD dx=0 dy=0 reg=0 id=18
    NOP count=4
    ADD dst=0 src=1
    LOOP jmp=@label_05F4 count=10
    RET
.org 0x060B
@label_060B:
    SPDA speed=320
.org 0x0610
@label_0610:
    TDEGS
    MOVR dst=0 src=134
    PSE id=4
    ENEMYSETD dx=0 dy=0 reg=0 id=18
    ACC accel=-6 count=5
    LOOP jmp=@label_0610 count=12
    RET
.org 0x0629
@label_0629:
    MOVC dst=1 val=40
    MOVC dst=2 val=120
.org 0x0635
@label_0635:
    TDEGS
    MOVR dst=0 src=134
    PSE id=4
    ENEMYSETD dx=0 dy=0 reg=0 id=18
    ADD dst=0 src=1
    ENEMYSETD dx=0 dy=0 reg=0 id=18
    ADD dst=0 src=1
    ENEMYSETD dx=0 dy=0 reg=0 id=18
    SUB dst=0 src=2
    ENEMYSETD dx=0 dy=0 reg=0 id=18
    SUB dst=0 src=1
    ENEMYSETD dx=0 dy=0 reg=0 id=18
    NOP count=12
    LOOP jmp=@label_0635 count=5
    RET
.org 0x0675
@label_0675:
    SPDA speed=320
    MOVR dst=0 src=134
    MOVC dst=1 val=32
.org 0x0683
@label_0683:
    PSE id=4
    ENEMYSETD dx=0 dy=0 reg=0 id=19
    ACC accel=-6 count=12
    ADD dst=0 src=1
    LOOP jmp=@label_0683 count=5
    RET
.org 0x069B
@label_069B:
    DAMAGE_ON
    MOVC dst=0 val=64
    MOVR dst=1 src=145
    ADD dst=1 src=0
    MOD reg=1 div=256
    CMPC reg=1 val=128
    JL jmp=@label_06BF
    ANM pattern=5 speed=6
    ANMEX pattern=10
    RET
.org 0x06BF
@label_06BF:
    ANM pattern=4 speed=6
    ANMEX pattern=9
    RET
.org 0x06C5
@label_06C5:
    DAMAGE_ON
    ANM pattern=0 speed=10
    ANMEX pattern=7
    RET
.org 0x06CC
@label_06CC:
    DAMAGE_OFF
    ANM pattern=2 speed=6
    RET
.org 0x06D1
@label_06D1:
    DAMAGE_ON
    ANM pattern=1 speed=10
    ANMEX pattern=7
    RET
.org 0x06D8
@label_06D8:
    DAMAGE_ON
    ANM pattern=3 speed=6
    ANMEX pattern=8
    RET
.org 0x06DF
@label_06DF:
    DAMAGE_OFF
    ANM pattern=6 speed=6
    RET
.org 0x06E4
@label_06E4:
    BITATTACK jmp=0x0004
    NOP count=10
    BITCMD cmd=1 val=-3
    NOP count=10
    BITCMD cmd=1 val=4
    NOP count=20
    BITCMD cmd=1 val=5
    NOP count=30
    BITCMD cmd=1 val=6
    NOP count=40
    BITCMD cmd=1 val=5
    NOP count=30
    BITCMD cmd=1 val=4
    NOP count=20
    BITCMD cmd=1 val=3
    NOP count=10
    BITCMD cmd=1 val=2
    NOP count=10
    RET
.org 0x0735
@label_0735:
    BITATTACK jmp=0x0005
    NOP count=190
    RET
.org 0x073E
@label_073E:
    TCMD cmd=6
    TNUMA n=8 ns=2
    TDEGA angle=0 dw=96
    TSPDA v=222 a=-8
    TREP rep=45
    TTYPE type=1
    TOPT opt=0
    RND reg=0
    MOD reg=0 div=65536
    CMPC reg=0 val=32768
    JL jmp=@label_0773
    MOVC dst=0 val=12
    MOVC dst=1 val=0
    JMP jmp=@label_077F
.org 0x0773
@label_0773:
    MOVC dst=0 val=4294967284
    MOVC dst=1 val=128
.org 0x077F
@label_077F:
    BITCMD cmd=3 val=9600
    BITCMD cmd=1 val=-4
    BITLASER cmd=3
    TCOL color=5
    MOVR dst=134 src=1
    ADD dst=1 src=0
    TAMA
    NOP count=6
    TCOL color=21
    MOVR dst=134 src=1
    ADD dst=1 src=0
    TAMA
    NOP count=6
    TCOL color=5
    MOVR dst=134 src=1
    ADD dst=1 src=0
    TAMA
    NOP count=6
    TCOL color=21
    MOVR dst=134 src=1
    ADD dst=1 src=0
    TAMA
    NOP count=6
    TCOL color=5
    MOVR dst=134 src=1
    ADD dst=1 src=0
    TAMA
    NOP count=6
    TCOL color=21
    MOVR dst=134 src=1
    ADD dst=1 src=0
    TAMA
    NOP count=6
    TCOL color=5
    MOVR dst=134 src=1
    ADD dst=1 src=0
    TAMA
    NOP count=6
    TCOL color=21
    MOVR dst=134 src=1
    ADD dst=1 src=0
    TAMA
    NOP count=6
    TCOL color=5
    MOVR dst=134 src=1
    ADD dst=1 src=0
    TAMA
    NOP count=6
    TCOL color=21
    MOVR dst=134 src=1
    ADD dst=1 src=0
    TAMA
    NOP count=6
    BITCMD cmd=1 val=2
    BITLASER cmd=0
    NOP count=80
    BITLASER cmd=1
    BITCMD cmd=3 val=5120
    RET
.org 0x0819
@label_0819:
    TCMD cmd=0
    TDEGA angle=0 dw=3
    TNUMA n=1 ns=0
    TSPDA v=29 a=-6
    TREP rep=20
    TTYPE type=1
    TOPT opt=0
    TCOL color=17
    BITCMD cmd=3 val=2560
    BITCMD cmd=1 val=-2
    BITLASER cmd=4
    TDEGS
    TAMA2
    TNUMR n=3 ns=0
    NOP count=10
    TAMA2
    TNUMR n=3 ns=0
    NOP count=10
    TAMA2
    TNUMR n=3 ns=0
    NOP count=10
    TAMA2
    TNUMR n=3 ns=0
    NOP count=10
    TAMA2
    TNUMR n=3 ns=0
    NOP count=10
    TAMA2
    TNUMR n=3 ns=0
    NOP count=10
    BITLASER cmd=0
    TAMA2
    TNUMR n=3 ns=0
    NOP count=10
    TAMA2
    TNUMR n=3 ns=0
    NOP count=10
    TAMA2
    TNUMR n=3 ns=0
    NOP count=10
    TAMA2
    TNUMR n=3 ns=0
    NOP count=10
    TAMA2
    TNUMR n=3 ns=0
    NOP count=10
    TAMA2
    TNUMR n=3 ns=0
    NOP count=10
    NOP count=20
    BITLASER cmd=1
    BITCMD cmd=3 val=5120
    RET
.org 0x089D
@label_089D:
    BITLASER cmd=5
    NOP count=60
    BITCMD cmd=1 val=0
    NOP count=20
    BITLASER cmd=0
    NOP count=20
    BITCMD cmd=1 val=2
    BITLASER cmd=1
    RET
.org 0x08B9
@label_08B9:
    LTYPE type=1
    LCOL color=0
    LNUMA n=2
    LXY x=0 y=0
    LDEGA angle=0 dw=48
    BITCMD cmd=3 val=11520
    NOP count=30
    BITCMD cmd=4 val=0
    LDEGS
    LDEGR angle=-128 dw=0
    HLASER
    NOP count=20
    LDEGS
    LDEGR angle=-128 dw=0
    HLASER
    NOP count=20
    LDEGS
    LDEGR angle=-128 dw=0
    HLASER
    NOP count=20
    LDEGS
    LDEGR angle=-128 dw=0
    HLASER
    NOP count=20
    BITCMD cmd=3 val=1280
    NOP count=80
    BITCMD cmd=3 val=5120
    NOP count=20
    RET
.org 0x0909
@script_3:
    DAMAGE_OFF
    HITSB_ON
    CLIP_ON
    CMPC reg=0 val=0
    JEQ jmp=@label_095F
    CMPC reg=0 val=1
    JEQ jmp=@label_0967
    CMPC reg=0 val=2
    JEQ jmp=@label_096F
    CMPC reg=0 val=3
    JEQ jmp=@label_0977
    CMPC reg=0 val=4
    JEQ jmp=@label_097F
    CMPC reg=0 val=5
    JEQ jmp=@label_0987
    ANM pattern=0 speed=10
.org 0x0951
@label_0951:
    ANMEX pattern=17
    NOP count=1
    DAMAGE_ON
.org 0x0957
@label_0957:
    NOP count=9999
    JMP jmp=@label_0957
.org 0x095F
@label_095F:
    ANM pattern=12 speed=6
    JMP jmp=@label_0951
.org 0x0967
@label_0967:
    ANM pattern=11 speed=6
    JMP jmp=@label_0951
.org 0x096F
@label_096F:
    ANM pattern=13 speed=6
    JMP jmp=@label_0951
.org 0x0977
@label_0977:
    ANM pattern=14 speed=6
    JMP jmp=@label_0951
.org 0x097F
@label_097F:
    ANM pattern=15 speed=6
    JMP jmp=@label_0951
.org 0x0987
@label_0987:
    ANM pattern=16 speed=6
    JMP jmp=@label_0951
.org 0x098F
@script_4:
    TCMD cmd=0
    TDEGA angle=0 dw=7
    TNUMA n=4 ns=0
    TSPDA v=12 a=0
    TTYPE type=0
    TOPT opt=0
    TCOL color=1
    TDEGE
    CMPC reg=1 val=6
    JEQ jmp=@label_09E7
    CMPC reg=1 val=5
    JEQ jmp=@label_09E4
    CMPC reg=1 val=4
    JEQ jmp=@label_09E1
    CMPC reg=1 val=3
    JEQ jmp=@label_09DE
    CMPC reg=1 val=2
    JEQ jmp=@label_09DB
    TNUMR n=1 ns=0
.org 0x09DB
@label_09DB:
    TNUMR n=1 ns=0
.org 0x09DE
@label_09DE:
    TNUMR n=1 ns=0
.org 0x09E1
@label_09E1:
    TNUMR n=1 ns=0
.org 0x09E4
@label_09E4:
    TNUMR n=1 ns=0
.org 0x09E7
@label_09E7:
    TDEGE
    TAMA
    NOP count=10
    LOOP jmp=@label_09E7 count=24
.org 0x09F3
@label_09F3:
    NOP count=9999
    JMP jmp=@label_09F3
.org 0x09FB
@script_5:
    TCMD cmd=9
    TDEGA angle=0 dw=7
    TNUMA n=16 ns=0
    TSPDA v=18 a=-3
    TTYPE type=1
    TREP rep=50
    TOPT opt=0
    TCOL color=21
    TDEGE
    CMPC reg=1 val=6
    JEQ jmp=@label_0A57
    CMPC reg=1 val=5
    JEQ jmp=@label_0A54
    CMPC reg=1 val=4
    JEQ jmp=@label_0A51
    CMPC reg=1 val=3
    JEQ jmp=@label_0A4E
    CMPC reg=1 val=2
    JEQ jmp=@label_0A4B
    TNUMR n=15 ns=1
    TCMD cmd=13
.org 0x0A4B
@label_0A4B:
    TNUMR n=15 ns=0
.org 0x0A4E
@label_0A4E:
    TNUMR n=3 ns=0
.org 0x0A51
@label_0A51:
    TNUMR n=3 ns=0
.org 0x0A54
@label_0A54:
    TNUMR n=3 ns=0
.org 0x0A57
@label_0A57:
    TAMA
    NOP count=24
    LOOP jmp=@label_0A57 count=6
.org 0x0A62
@label_0A62:
    NOP count=9999
    JMP jmp=@label_0A62
.org 0x0A6A
@script_6:
@script_7:  ; shared
@script_8:  ; shared
@script_9:  ; shared
@script_10:  ; shared
@script_11:  ; shared
@script_12:  ; shared
@script_13:  ; shared
@script_14:  ; shared
@script_15:  ; shared
@script_16:  ; shared
@script_17:  ; shared
    SETUP hp=750 score=50000
    DAMAGE_ON
    DEGX2
    CALL jmp=@label_069B
    SPDA speed=320
    ACC accel=-6 count=60
    CALL jmp=@label_06C5
    NOP count=32
    PSE id=1
    MXYA x=319 y=89 count=36
    CALL jmp=@label_06CC
    NOP count=36
    CALL jmp=@label_06D1
    RND reg=4
    MOVC dst=6 val=0
.org 0x0AA9
@label_0AA9:
    NOP count=80
    DEGX2
    CALL jmp=@label_069B
    SPDA speed=320
    ACC accel=-6 count=60
    RND reg=5
    MOD reg=5 div=3
    ADD dst=4 src=5
    INC reg=4
    MOD reg=4 div=4
    CMPC reg=4 val=0
    JEQ jmp=@label_0B1D
    CMPC reg=4 val=1
    JEQ jmp=@label_0B0E
    CMPC reg=4 val=2
    JEQ jmp=@label_0AFE
    CALL jmp=@label_06D8
    CALL jmp=@script_2
    JMP jmp=@label_0B2D
.org 0x0AFE
@label_0AFE:
    DEGX2
    CALL jmp=@label_069B
    CALL jmp=@label_060B
    JMP jmp=@label_0B2D
.org 0x0B0E
@label_0B0E:
    CALL jmp=@label_06D8
    CALL jmp=@label_0629
    JMP jmp=@label_0B2D
.org 0x0B1D
@label_0B1D:
    DEGX2
    CALL jmp=@label_069B
    CALL jmp=@label_0675
    JMP jmp=@label_0B2D
.org 0x0B2D
@label_0B2D:
    CALL jmp=@label_06D1
    JMP jmp=@label_0AA9
.org 0x0B37
@script_18:
    SETUP hp=9999 score=0
    STI jmp=@label_0B86 vector=BOSSLEFT val=0
    ANM pattern=0 speed=0
    CLIP_ON
    DAMAGE_OFF
    HITSB_OFF
    DRAW_OFF
    TTYPE type=0
    TOPT opt=0
    TCMD cmd=0
    TDEGA angle=0 dw=32
    TNUMA n=1 ns=0
    TCOL color=48
    TSPDA v=36 a=0
    TDEGE
    TAMA2
    NOP count=1
    TCMD cmd=6
    TDEGA angle=0 dw=17
    TNUMA n=6 ns=2
    TCOL color=50
    TSPDA v=227 a=0
    TDEGE
    TAMAEX
    TCMD cmd=6
    TDEGA angle=0 dw=35
    TNUMA n=6 ns=2
    TCOL color=51
    TSPDA v=227 a=0
    TDEGE
    TAMAEX
    END
.org 0x0B86
@label_0B86:
    TCLR
    END
.org 0x0B88
@script_19:
    SETUP hp=9999 score=0
    STI jmp=@label_0BD1 vector=BOSSLEFT val=0
    ANM pattern=0 speed=0
    CLIP_ON
    DAMAGE_OFF
    HITSB_OFF
    DRAW_OFF
    MOVR dst=0 src=145
    MOVC dst=1 val=64
    ENEMYSETD dx=0 dy=0 reg=0 id=18
    ADD dst=0 src=1
    ENEMYSETD dx=0 dy=0 reg=0 id=18
    ADD dst=0 src=1
    ENEMYSETD dx=0 dy=0 reg=0 id=18
    ADD dst=0 src=1
    ENEMYSETD dx=0 dy=0 reg=0 id=18
    END
.org 0x0BD1
@label_0BD1:
    TCLR
    END
.org 0x0BD3
@script_20:
    SETUP hp=100000 score=100
    DAMAGE_OFF
    HITSB_OFF
.org 0x0BDE
@label_0BDE:
    ANM pattern=18 speed=10
    NOP count=60
    JMP jmp=@label_0BDE
.org 0x0BE9
@script_21:
    SETUP hp=20500 score=900000
    INT id=6
    CALL jmp=@label_1A2B
    MXYA x=319 y=89 count=36
    STI jmp=@label_0C4F vector=HP val=15700
    STI jmp=@label_0C51 vector=TIMER val=4200
.org 0x0C14
@label_0C14:
    CALL jmp=@label_1A9B
    CALL jmp=@label_1A6C
    CALL jmp=@label_168E
    NOP count=32
    CALL jmp=@label_1A9B
    CALL jmp=@label_1A6C
    CALL jmp=@script_22
    NOP count=32
    CALL jmp=@label_1A9B
    CALL jmp=@label_1A6C
    CALL jmp=@label_1819
    NOP count=32
    JMP jmp=@label_0C14
.org 0x0C4F
@label_0C4F:
    T2ITEM pct=1
.org 0x0C51
@label_0C51:
    SETUP hp=15700 score=900000
    STI jmp=@label_0F65 vector=HP val=12700
    STI jmp=@label_0F67 vector=TIMER val=6000
    TCLR
    TXYR dx=0 dy=0
    TAUTO interval=0
    CALL jmp=@label_1A7F
    MXYA x=319 y=89 count=120
    CALL jmp=@label_1A2B
    RND reg=7
    MOD reg=7 div=65536
    CMPC reg=7 val=32768
    JL jmp=@label_0CA5
    MOVC dst=7 val=1
    JMP jmp=@label_0CAB
.org 0x0CA5
@label_0CA5:
    MOVC dst=7 val=0
.org 0x0CAB
@label_0CAB:
    CALL jmp=@label_1AE9
    XYA x=319 y=89
    CALL jmp=@label_1B18
    CMPC reg=7 val=0
    JEQ jmp=@label_0E15
    CALL jmp=@label_1A8A
    CALL jmp=@label_1AE9
    XYA x=198 y=48
    CALL jmp=@label_1B18
    CALL jmp=@label_1992
    CALL jmp=@label_1579
    TAUTO interval=5
    DEGA angle=32
    CALL jmp=@label_1A85
    PSE id=14
    SPDA speed=960
    MOV count=12
    ACC accel=-90 count=8
    TAUTO interval=0
    CALL jmp=@label_1AE9
    MOVC dst=0 val=3072
    MOVR dst=144 src=0
    CALL jmp=@label_1B18
    CALL jmp=@label_1992
    CALL jmp=@label_1579
    TAUTO interval=5
    DEGA angle=85
    CALL jmp=@label_1A85
    PSE id=14
    SPDA speed=960
    MOV count=9
    ACC accel=-90 count=8
    TAUTO interval=0
    CALL jmp=@label_1AE9
    MOVC dst=0 val=3072
    MOVR dst=144 src=0
    CALL jmp=@label_1B18
    CALL jmp=@label_1992
    CALL jmp=@label_1579
    TAUTO interval=5
    DEGA angle=32
    CALL jmp=@label_1A85
    PSE id=14
    SPDA speed=960
    MOV count=12
    ACC accel=-90 count=8
    TAUTO interval=0
    CALL jmp=@label_1AE9
    MOVC dst=0 val=3072
    MOVR dst=144 src=0
    CALL jmp=@label_1B18
    CALL jmp=@label_1992
    CALL jmp=@label_1579
    TAUTO interval=5
    DEGA angle=85
    CALL jmp=@label_1A85
    PSE id=14
    SPDA speed=960
    MOV count=9
    ACC accel=-90 count=8
    TAUTO interval=0
    CALL jmp=@label_1AE9
    MOVC dst=0 val=3072
    MOVR dst=144 src=0
    CALL jmp=@label_1B18
    CALL jmp=@label_1992
    CALL jmp=@label_1579
    TAUTO interval=5
    DEGA angle=85
    CALL jmp=@label_1A85
    PSE id=14
    SPDA speed=960
    MOV count=9
    ACC accel=-90 count=8
    TAUTO interval=0
    CALL jmp=@label_1AE9
    MOVC dst=0 val=3072
    MOVR dst=144 src=0
    CALL jmp=@label_1B18
    CALL jmp=@label_1992
    CALL jmp=@label_1579
    TAUTO interval=5
    DEGA angle=32
    CALL jmp=@label_1A85
    PSE id=14
    SPDA speed=960
    MOV count=12
    ACC accel=-90 count=8
    TAUTO interval=0
    MOVC dst=7 val=0
    JMP jmp=@label_0CAB
.org 0x0E15
@label_0E15:
    CALL jmp=@label_1A8A
    CALL jmp=@label_1AE9
    XYA x=441 y=48
    CALL jmp=@label_1B18
    CALL jmp=@label_1992
    CALL jmp=@label_1579
    TAUTO interval=5
    DEGA angle=96
    CALL jmp=@label_1A85
    PSE id=14
    SPDA speed=960
    MOV count=12
    ACC accel=-90 count=8
    TAUTO interval=0
    CALL jmp=@label_1AE9
    MOVC dst=0 val=3072
    MOVR dst=144 src=0
    CALL jmp=@label_1B18
    CALL jmp=@label_1992
    CALL jmp=@label_1579
    TAUTO interval=5
    DEGA angle=43
    CALL jmp=@label_1A85
    PSE id=14
    SPDA speed=960
    MOV count=9
    ACC accel=-90 count=8
    TAUTO interval=0
    CALL jmp=@label_1AE9
    MOVC dst=0 val=3072
    MOVR dst=144 src=0
    CALL jmp=@label_1B18
    CALL jmp=@label_1992
    CALL jmp=@label_1579
    TAUTO interval=5
    DEGA angle=96
    CALL jmp=@label_1A85
    PSE id=14
    SPDA speed=960
    MOV count=12
    ACC accel=-90 count=8
    TAUTO interval=0
    CALL jmp=@label_1AE9
    MOVC dst=0 val=3072
    MOVR dst=144 src=0
    CALL jmp=@label_1B18
    CALL jmp=@label_1992
    CALL jmp=@label_1579
    TAUTO interval=5
    DEGA angle=43
    CALL jmp=@label_1A85
    PSE id=14
    SPDA speed=960
    MOV count=9
    ACC accel=-90 count=8
    TAUTO interval=0
    CALL jmp=@label_1AE9
    MOVC dst=0 val=3072
    MOVR dst=144 src=0
    CALL jmp=@label_1B18
    CALL jmp=@label_1992
    CALL jmp=@label_1579
    TAUTO interval=5
    DEGA angle=43
    CALL jmp=@label_1A85
    PSE id=14
    SPDA speed=960
    MOV count=9
    ACC accel=-90 count=8
    TAUTO interval=0
    CALL jmp=@label_1AE9
    MOVC dst=0 val=3072
    MOVR dst=144 src=0
    CALL jmp=@label_1B18
    CALL jmp=@label_1992
    CALL jmp=@label_1579
    TAUTO interval=5
    DEGA angle=96
    CALL jmp=@label_1A85
    PSE id=14
    SPDA speed=960
    MOV count=12
    ACC accel=-90 count=8
    TAUTO interval=0
    MOVC dst=7 val=1
    JMP jmp=@label_0CAB
.org 0x0F65
@label_0F65:
    T2ITEM pct=1
.org 0x0F67
@label_0F67:
    SETUP hp=12700 score=900000
    STI jmp=@label_100D vector=HP val=8200
    STI jmp=@label_100F vector=TIMER val=4200
    TCLR
    TXYR dx=0 dy=0
    TAUTO interval=0
    CALL jmp=@label_1A7F
    MXYA x=319 y=89 count=120
    CALL jmp=@label_1A2B
.org 0x0F9D
@label_0F9D:
    CALL jmp=@label_1A9B
    CALL jmp=@label_1A57
    CALL jmp=@label_1718
    NOP count=32
    CALL jmp=@label_1A9B
    CALL jmp=@label_1A3E
    CALL jmp=@label_191F
    CALL jmp=@label_14F6
    CALL jmp=@label_1A2B
    NOP count=28
    CALL jmp=@label_1A33
    CALL jmp=@label_191F
    CALL jmp=@label_14F6
    CALL jmp=@label_1A2B
    NOP count=28
    CALL jmp=@label_1A33
    CALL jmp=@label_191F
    CALL jmp=@label_14F6
    CALL jmp=@label_1A2B
    NOP count=28
    CALL jmp=@label_1A9B
    CALL jmp=@label_1A3E
    CALL jmp=@label_19CC
    JMP jmp=@label_0F9D
.org 0x100D
@label_100D:
    T2ITEM pct=1
.org 0x100F
@label_100F:
    SETUP hp=8200 score=900000
    STI jmp=@label_11E3 vector=HP val=5200
    STI jmp=@label_11E5 vector=TIMER val=6000
    TCLR
    TXYR dx=0 dy=0
    TAUTO interval=0
    CALL jmp=@label_1A7F
    MXYA x=319 y=89 count=120
    CALL jmp=@label_1A2B
    RND reg=6
    MOD reg=6 div=65536
    CMPC reg=6 val=32768
    JL jmp=@label_1072
    CALL jmp=@label_1AE9
    XYA x=319 y=49
    CALL jmp=@label_1B18
    MOVC dst=6 val=1
    JMP jmp=@label_1078
.org 0x1072
@label_1072:
    MOVC dst=6 val=0
.org 0x1078
@label_1078:
    CMPC reg=6 val=1
    JEQ jmp=@label_1133
    CALL jmp=@label_1A8A
    DEGA angle=32
    MOVC dst=7 val=16
.org 0x1090
@label_1090:
    DEGR angle=16
    CALL jmp=@label_1A85
    CALL jmp=@label_1590
    TAUTO interval=1
    PSE id=14
    SPDA speed=1536
    MOV count=1
    TDEGR angle=4 dw=0
    MOV count=1
    TDEGR angle=4 dw=0
    MOV count=1
    TDEGR angle=4 dw=0
    MOV count=1
    TDEGR angle=4 dw=0
    MOV count=1
    TDEGR angle=-4 dw=0
    MOV count=1
    TDEGR angle=-4 dw=0
    MOV count=1
    TDEGR angle=-4 dw=0
    MOV count=1
    TDEGR angle=-4 dw=0
    ACC accel=-90 count=8
    TAUTO interval=0
    CALL jmp=@label_1AE9
    MOVC dst=0 val=160
    MOVR dst=2 src=145
    ADD dst=2 src=0
    MOVC dst=0 val=20416
    MOVC dst=1 val=9600
    COSL len=1 deg=2
    ADD dst=0 src=1
    MOVR dst=143 src=0
    MOVC dst=0 val=12736
    MOVC dst=1 val=9600
    SINL len=1 deg=2
    ADD dst=0 src=1
    MOVR dst=144 src=0
    CALL jmp=@label_1B18
    DEC reg=7
    CMPC reg=7 val=0
    JL jmp=@label_1090
    MOVC dst=6 val=1
    JMP jmp=@label_1078
.org 0x1133
@label_1133:
    CALL jmp=@label_1A8A
    DEGA angle=96
    MOVC dst=7 val=16
.org 0x1140
@label_1140:
    DEGR angle=-16
    CALL jmp=@label_1A85
    CALL jmp=@label_1590
    TAUTO interval=1
    PSE id=14
    SPDA speed=1536
    MOV count=1
    TDEGR angle=-4 dw=0
    MOV count=1
    TDEGR angle=-4 dw=0
    MOV count=1
    TDEGR angle=-4 dw=0
    MOV count=1
    TDEGR angle=-4 dw=0
    MOV count=1
    TDEGR angle=4 dw=0
    MOV count=1
    TDEGR angle=4 dw=0
    MOV count=1
    TDEGR angle=4 dw=0
    MOV count=1
    TDEGR angle=4 dw=0
    ACC accel=-90 count=8
    TAUTO interval=0
    CALL jmp=@label_1AE9
    MOVC dst=0 val=160
    MOVR dst=2 src=145
    SUB dst=2 src=0
    MOVC dst=0 val=20416
    MOVC dst=1 val=9600
    COSL len=1 deg=2
    ADD dst=0 src=1
    MOVR dst=143 src=0
    MOVC dst=0 val=12736
    MOVC dst=1 val=9600
    SINL len=1 deg=2
    ADD dst=0 src=1
    MOVR dst=144 src=0
    CALL jmp=@label_1B18
    DEC reg=7
    CMPC reg=7 val=0
    JL jmp=@label_1140
    MOVC dst=6 val=0
    JMP jmp=@label_1078
.org 0x11E3
@label_11E3:
    T2ITEM pct=1
.org 0x11E5
@label_11E5:
    SETUP hp=5200 score=900000
    STI jmp=@label_12F8 vector=HP val=5000
    CLI vector=TIMER
    TCLR
    TXYR dx=0 dy=0
    TAUTO interval=0
    CALL jmp=@label_1A7F
    MXYA x=319 y=89 count=120
    CALL jmp=@label_1664
    TAUTO interval=10
    SPDA speed=192
    DEGA angle=0
    ROL deg=2 count=30
    CALL jmp=@label_1679
    SPDA speed=256
    ROL deg=-3 count=90
    CALL jmp=@label_1664
    SPDA speed=128
    ROL deg=1 count=180
    CALL jmp=@label_1679
    SPDA speed=192
    ROL deg=-3 count=50
    CALL jmp=@label_1664
    SPDA speed=128
    ROL deg=-1 count=60
    TAUTO interval=8
    SPDA speed=128
    ROL deg=-1 count=80
    CALL jmp=@label_1679
    SPDA speed=64
    ROL deg=-8 count=20
    MXYA x=319 y=119 count=20
    TAUTO interval=0
    NOP count=40
    DEGA angle=0
    WAVX vx=0 amp=80 vd=-4 count=32
    NOP count=10
    DEGA angle=0
    WAVX vx=0 amp=60 vd=-4 count=32
    ENEMYSET dx=-250 dy=-80 id=52
    ENEMYSET dx=-250 dy=0 id=52
    ENEMYSET dx=-250 dy=80 id=52
    ENEMYSET dx=-250 dy=160 id=52
    ENEMYSET dx=-250 dy=240 id=52
    ENEMYSET dx=-250 dy=320 id=52
    ENEMYSET dx=-250 dy=400 id=52
    ENEMYSET dx=250 dy=-40 id=53
    ENEMYSET dx=250 dy=40 id=53
    ENEMYSET dx=250 dy=120 id=53
    ENEMYSET dx=250 dy=200 id=53
    ENEMYSET dx=250 dy=280 id=53
    ENEMYSET dx=250 dy=360 id=53
    NOP count=190
    CALL jmp=@label_164F
    TAUTO interval=30
    NOP count=800
    TAUTO interval=0
    JMP jmp=@label_12F8
.org 0x12F8
@label_12F8:
    T2ITEM pct=1
    SETUP hp=5000 score=900000
    STI jmp=@label_13B5 vector=HP val=3000
    STI jmp=@label_13B7 vector=TIMER val=4200
    TCLR
    TXYR dx=0 dy=0
    TAUTO interval=0
    CALL jmp=@label_1A7F
    MXYA x=319 y=89 count=120
    CALL jmp=@label_1A2B
.org 0x1330
@label_1330:
    CALL jmp=@label_1A9B
    CALL jmp=@label_1A6C
    CALL jmp=@label_1790
    NOP count=32
    CALL jmp=@label_1A9B
    CALL jmp=@label_1A3E
    CALL jmp=@label_193D
    NOP count=15
    CALL jmp=@label_193D
    NOP count=15
    CALL jmp=@label_193D
    NOP count=15
    CALL jmp=@label_1A9B
    CALL jmp=@label_1A3E
    CALL jmp=@label_193D
    NOP count=15
    CALL jmp=@label_193D
    NOP count=15
    CALL jmp=@label_193D
    NOP count=15
    CALL jmp=@label_1A9B
    CALL jmp=@label_1A3E
    CALL jmp=@label_193D
    NOP count=15
    CALL jmp=@label_193D
    NOP count=15
    CALL jmp=@label_193D
    NOP count=15
    CALL jmp=@label_1A9B
    NOP count=32
    JMP jmp=@label_1330
.org 0x13B5
@label_13B5:
    T2ITEM pct=1
.org 0x13B7
@label_13B7:
    SETUP hp=3000 score=900000
    STI jmp=@label_14B1 vector=TIMER val=6000
    CLI vector=HP
    TCLR
    TXYR dx=0 dy=0
    TAUTO interval=0
    CALL jmp=@label_1A7F
    MXYA x=319 y=89 count=120
    CALL jmp=@label_1A2B
    CALL jmp=@label_1AE9
    XYA x=319 y=49
    CALL jmp=@label_1B18
.org 0x13F4
@label_13F4:
    CALL jmp=@label_1A8A
    DEGA angle=85
    MOVC dst=7 val=6
.org 0x1401
@label_1401:
    CALL jmp=@label_1A85
    PSE id=14
    SPDA speed=960
    CALL jmp=@label_1638
    TNUMR n=3 ns=-5
    TCOL color=16
    TSPDR v=10 a=0
    TDEGR angle=0 dw=30
    TAUTO interval=10
    MOV count=13
    ACC accel=-90 count=8
    TAUTO interval=0
    CALL jmp=@label_1A2B
    CALL jmp=@label_15ED
    DEGR angle=-85
    CALL jmp=@label_1638
    TAUTO interval=4
    CALL jmp=@label_1A85
    PSE id=14
    SPDA speed=960
    MOV count=13
    ACC accel=-90 count=8
    TAUTO interval=0
    CALL jmp=@label_1AE9
    MOVR dst=0 src=145
    MOVC dst=1 val=149
    ADD dst=0 src=1
    MOVC dst=1 val=43
    MOVR dst=2 src=145
    ADD dst=2 src=1
    MOVR dst=145 src=2
    MOVC dst=2 val=20416
    MOVC dst=1 val=9600
    COSL len=1 deg=0
    ADD dst=1 src=2
    MOVR dst=143 src=1
    MOVC dst=2 val=12736
    MOVC dst=1 val=9600
    SINL len=1 deg=0
    ADD dst=1 src=2
    MOVR dst=144 src=1
    CALL jmp=@label_1B18
    DEC reg=7
    CMPC reg=7 val=0
    JL jmp=@label_1401
    JMP jmp=@label_13F4
.org 0x14B1
@label_14B1:
    SETUP hp=0 score=0  ; death marker
    CLI vector=TIMER
    NOP count=1000
    JMP jmp=@label_14B1
.org 0x14C4
@script_22:
    TCMD cmd=94
    TNUMA n=5 ns=3
    TSPDA v=215 a=-6
    TTYPE type=1
    TDEGA angle=0 dw=255
    TREP rep=66
.org 0x14D3
@label_14D3:
    PSE id=0
    NOP count=3
    TCOL color=0
    TXYR dx=16 dy=-32
    TAMA2
    NOP count=3
    TCOL color=21
    TXYR dx=-16 dy=-16
    TAMA2
    LOOP jmp=@label_14D3 count=46
    NOP count=10
    RET
.org 0x14F6
@label_14F6:
    TCMD cmd=84
    TNUMA n=1 ns=6
    TSPDA v=24 a=0
    TTYPE type=0
    TDEGA angle=0 dw=1
    TXYR dx=0 dy=0
.org 0x1508
@label_1508:
    TDEGS
    TDEGR angle=-2 dw=0
    PSE id=0
    TCOL color=16
    TAMA2
    NOP count=2
    TDEGR angle=1 dw=0
    PSE id=0
    TCOL color=21
    TAMA2
    NOP count=2
    TDEGR angle=1 dw=0
    PSE id=0
    TCOL color=16
    TAMA2
    NOP count=2
    TDEGR angle=1 dw=0
    PSE id=0
    TCOL color=21
    TAMA2
    NOP count=2
    TDEGR angle=1 dw=0
    NOP count=12
    TDEGS
    TDEGR angle=2 dw=0
    PSE id=0
    TCOL color=16
    TAMA2
    NOP count=2
    TDEGR angle=-1 dw=0
    PSE id=0
    TCOL color=21
    TAMA2
    NOP count=2
    TDEGR angle=-1 dw=0
    PSE id=0
    TCOL color=16
    TAMA2
    NOP count=2
    TDEGR angle=-1 dw=0
    PSE id=0
    TCOL color=21
    TAMA2
    NOP count=2
    TDEGR angle=-1 dw=0
    NOP count=12
    LOOP jmp=@label_1508 count=2
    NOP count=20
    RET
.org 0x1579
@label_1579:
    TCMD cmd=12
    TNUMA n=8 ns=5
    TSPDA v=225 a=-20
    TREP rep=50
    TTYPE type=1
    TDEGA angle=128 dw=12
    TXYR dx=0 dy=0
    TCOL color=21
    RET
.org 0x1590
@label_1590:
    CMPC reg=7 val=5
    JS jmp=@label_15D5
    CMPC reg=7 val=10
    JL jmp=@label_15BE
    TCMD cmd=81
    TNUMA n=20 ns=0
    TSPDA v=0 a=18
    TREP rep=27
    TTYPE type=1
    TDEGA angle=0 dw=32
    TXYR dx=0 dy=0
    TCOL color=16
    TDEGE
    RET
.org 0x15BE
@label_15BE:
    TCMD cmd=88
    TNUMA n=10 ns=0
    TSPDA v=0 a=16
    TREP rep=30
    TTYPE type=1
    TDEGA angle=0 dw=30
    TXYR dx=0 dy=0
    TCOL color=21
    RET
.org 0x15D5
@label_15D5:
    TCMD cmd=81
    TNUMA n=7 ns=0
    TSPDA v=0 a=16
    TREP rep=46
    TTYPE type=1
    TDEGA angle=0 dw=1
    TXYR dx=0 dy=0
    TCOL color=18
    TDEGE
    RET
.org 0x15ED
@label_15ED:
    TCMD cmd=4
    TNUMA n=2 ns=11
    TTYPE type=0
    TSPDA v=21 a=0
    TXYR dx=0 dy=0
    TCOL color=67
    MOVC dst=6 val=220
    MOVC dst=5 val=15
    TDEGS
.org 0x160B
@label_160B:
    NOP count=2
    MOVR dst=135 src=6
    SUB dst=6 src=5
    PSE id=0
    PSE id=0
    TAMA2
    TDEGR angle=0 dw=-6
    TAMA2
    TDEGR angle=0 dw=-6
    TAMA2
    TDEGR angle=0 dw=-6
    TAMA2
    TDEGR angle=0 dw=-6
    TAMA2
    TDEGR angle=0 dw=-6
    TAMA2
    TDEGR angle=0 dw=-6
    LOOP jmp=@label_160B count=12
    RET
.org 0x1638
@label_1638:
    TCMD cmd=12
    TNUMA n=1 ns=8
    TTYPE type=1
    TSPDA v=23 a=-29
    TXYR dx=0 dy=0
    TREP rep=30
    TCOL color=21
    TDEGA angle=128 dw=0
    RET
.org 0x164F
@label_164F:
    TCMD cmd=12
    TNUMA n=29 ns=1
    TTYPE type=0
    TSPDA v=11 a=0
    TXYR dx=0 dy=0
    TCOL color=17
    TDEGA angle=0 dw=8
    RET
.org 0x1664
@label_1664:
    TCMD cmd=14
    TNUMA n=2 ns=3
    TTYPE type=0
    TSPDA v=9 a=0
    TXYR dx=0 dy=0
    TCOL color=17
    TDEGA angle=0 dw=120
    RET
.org 0x1679
@label_1679:
    TCMD cmd=13
    TNUMA n=16 ns=1
    TTYPE type=0
    TSPDA v=14 a=0
    TXYR dx=0 dy=0
    TCOL color=3
    TDEGA angle=0 dw=0
    RET
.org 0x168E
@label_168E:
    TCMD cmd=1
    TNUMA n=8 ns=0
    TSPDA v=26 a=0
    TTYPE type=4
    TREP rep=50
    TXYR dx=0 dy=0
    TDEGA angle=0 dw=6
    RND reg=0
    MOVR dst=134 src=0
    MOD reg=0 div=65536
    CMPC reg=0 val=32768
    JL jmp=@label_16E8
.org 0x16B8
@label_16B8:
    PSE id=0
    NOP count=2
    TCOL color=64
    TVDEG vd=-1
    TDEGR angle=4 dw=0
    TAMA2
    TCOL color=65
    TVDEG vd=1
    TDEGR angle=4 dw=0
    TAMA2
    TCOL color=66
    TVDEG vd=-1
    TDEGR angle=4 dw=0
    TAMA2
    TCOL color=67
    TVDEG vd=1
    TDEGR angle=4 dw=0
    TAMA2
    TDEGR angle=32 dw=0
    LOOP jmp=@label_16B8 count=80
    RET
.org 0x16E8
@label_16E8:
    PSE id=0
    NOP count=2
    TCOL color=64
    TVDEG vd=1
    TDEGR angle=-4 dw=0
    TAMA2
    TCOL color=65
    TVDEG vd=-1
    TDEGR angle=-4 dw=0
    TAMA2
    TCOL color=66
    TVDEG vd=1
    TDEGR angle=-4 dw=0
    TAMA2
    TCOL color=67
    TVDEG vd=-1
    TDEGR angle=-4 dw=0
    TAMA2
    TDEGR angle=-32 dw=0
    LOOP jmp=@label_16E8 count=80
    RET
.org 0x1718
@label_1718:
    TCMD cmd=0
    TNUMA n=9 ns=0
    TSPDA v=34 a=0
    TTYPE type=0
    TXYR dx=0 dy=0
    TDEGA angle=0 dw=6
    RND reg=0
    MOVR dst=134 src=0
    MOD reg=0 div=65536
    CMPC reg=0 val=32768
    JL jmp=@label_1768
.org 0x1740
@label_1740:
    NOP count=2
    PSE id=0
    TCOL color=64
    TDEGR angle=64 dw=0
    TAMA2
    TCOL color=65
    TDEGR angle=64 dw=0
    TAMA2
    TCOL color=66
    TDEGR angle=64 dw=0
    TAMA2
    TCOL color=67
    TDEGR angle=64 dw=0
    TAMA2
    TDEGR angle=3 dw=0
    LOOP jmp=@label_1740 count=80
    RET
.org 0x1768
@label_1768:
    NOP count=2
    PSE id=0
    TCOL color=64
    TDEGR angle=64 dw=0
    TAMA2
    TCOL color=65
    TDEGR angle=64 dw=0
    TAMA2
    TCOL color=66
    TDEGR angle=64 dw=0
    TAMA2
    TCOL color=67
    TDEGR angle=64 dw=0
    TAMA2
    TDEGR angle=-3 dw=0
    LOOP jmp=@label_1768 count=80
    RET
.org 0x1790
@label_1790:
    TCMD cmd=10
    TNUMA n=5 ns=0
    TSPDA v=158 a=-10
    TTYPE type=1
    TREP rep=25
    TXYR dx=0 dy=0
    TDEGA angle=0 dw=235
    MOVC dst=0 val=7
    RND reg=1
    MOVC dst=2 val=85
    MOVC dst=3 val=16
.org 0x17B8
@label_17B8:
    TCOL color=16
    TAMA2
    NOP count=2
    TCOL color=0
    TAMA2
    NOP count=2
    TCOL color=21
    TAMA2
    NOP count=2
    TCOL color=5
    TAMA2
    NOP count=2
    LOOP jmp=@label_17B8 count=4
    TSPDR v=0 a=2
    TDEGR angle=0 dw=-20
    CMPC reg=0 val=3
    JS jmp=@label_1808
    PSE id=17
    ENEMYSETD dx=0 dy=0 reg=1 id=23
    ADD dst=1 src=2
    ENEMYSETD dx=0 dy=0 reg=1 id=23
    ADD dst=1 src=2
    ENEMYSETD dx=0 dy=0 reg=1 id=23
    ADD dst=1 src=3
.org 0x1808
@label_1808:
    NOP count=20
    DEC reg=0
    CMPC reg=0 val=0
    JL jmp=@label_17B8
    RET
.org 0x1819
@label_1819:
    TCMD cmd=1
    TNUMA n=8 ns=0
    TSPDA v=20 a=0
    TTYPE type=0
    TXYR dx=0 dy=0
    TDEGA angle=0 dw=6
    TCOL color=65
    MOVC dst=1 val=0
    RND reg=0
    MOVR dst=134 src=0
    MOD reg=0 div=65536
    CMPC reg=0 val=32768
    JL jmp=@label_18B4
    MOVC dst=0 val=4294967252
    MOVC dst=2 val=4294967286
.org 0x1855
@label_1855:
    NOP count=2
    PSE id=0
    TDEGR angle=32 dw=0
    TAMA2
    TDEGR angle=9 dw=0
    INC reg=1
    CMPC reg=1 val=5
    JS jmp=@label_1855
    MOVC dst=1 val=0
    TSPDR v=1 a=0
    PSE id=17
    ENEMYSETD dx=0 dy=0 reg=0 id=23
    ADD dst=0 src=2
    LOOP jmp=@label_1855 count=4
.org 0x188A
@label_188A:
    NOP count=2
    PSE id=0
    TDEGR angle=32 dw=0
    TAMA2
    TDEGR angle=9 dw=0
    INC reg=1
    CMPC reg=1 val=5
    JS jmp=@label_188A
    MOVC dst=1 val=0
    TSPDR v=1 a=0
    LOOP jmp=@label_188A count=13
    RET
.org 0x18B4
@label_18B4:
    MOVC dst=0 val=4294967212
    MOVC dst=2 val=10
.org 0x18C0
@label_18C0:
    NOP count=2
    PSE id=0
    TDEGR angle=32 dw=0
    TAMA2
    TDEGR angle=-9 dw=0
    INC reg=1
    CMPC reg=1 val=5
    JS jmp=@label_18C0
    MOVC dst=1 val=0
    TSPDR v=1 a=0
    PSE id=17
    ENEMYSETD dx=0 dy=0 reg=0 id=23
    ADD dst=0 src=2
    LOOP jmp=@label_18C0 count=4
.org 0x18F5
@label_18F5:
    NOP count=2
    PSE id=0
    TDEGR angle=32 dw=0
    TAMA2
    TDEGR angle=-9 dw=0
    INC reg=1
    CMPC reg=1 val=5
    JS jmp=@label_18F5
    MOVC dst=1 val=0
    TSPDR v=1 a=0
    LOOP jmp=@label_18F5 count=13
    RET
.org 0x191F
@label_191F:
    RND reg=0
    MOVC dst=1 val=16
    PSE id=17
.org 0x1929
@label_1929:
    PSE id=17
    ENEMYSETD dx=0 dy=0 reg=0 id=23
    ADD dst=0 src=1
    LOOP jmp=@label_1929 count=31
    RET
.org 0x193D
@label_193D:
    MOVC dst=1 val=32
    RND reg=0
    MOD reg=0 div=65536
    CMPC reg=0 val=32768
    JL jmp=@label_1974
.org 0x1956
@label_1956:
    PSE id=17
    ENEMYSETD dx=0 dy=0 reg=0 id=24
    ADD dst=0 src=1
    ENEMYSETD dx=0 dy=0 reg=0 id=26
    ADD dst=0 src=1
    LOOP jmp=@label_1956 count=3
    RET
.org 0x1974
@label_1974:
    PSE id=17
    ENEMYSETD dx=0 dy=0 reg=0 id=25
    ADD dst=0 src=1
    ENEMYSETD dx=0 dy=0 reg=0 id=27
    ADD dst=0 src=1
    LOOP jmp=@label_1974 count=3
    RET
.org 0x1992
@label_1992:
    MOVC dst=1 val=16
    MOVC dst=2 val=8
    DEGS
    MOVR dst=3 src=145
.org 0x19A2
@label_19A2:
    PSE id=17
    MOVR dst=0 src=3
    ADD dst=0 src=2
    ENEMYSETD dx=0 dy=0 reg=0 id=28
    MOVR dst=0 src=3
    SUB dst=0 src=2
    ENEMYSETD dx=0 dy=0 reg=0 id=28
    ADD dst=2 src=1
    NOP count=8
    LOOP jmp=@label_19A2 count=7
    RET
.org 0x19CC
@label_19CC:
    RND reg=0
    MOD reg=0 div=65536
    CMPC reg=0 val=32768
    JL jmp=@label_1A05
    MOVC dst=0 val=0
    MOVC dst=1 val=16
.org 0x19EB
@label_19EB:
    PSE id=17
    ENEMYSETD dx=0 dy=0 reg=0 id=29
    ADD dst=0 src=1
    NOP count=16
    LOOP jmp=@label_19EB count=8
    NOP count=60
    RET
.org 0x1A05
@label_1A05:
    MOVC dst=0 val=128
    MOVC dst=1 val=4294967280
.org 0x1A11
@label_1A11:
    PSE id=17
    ENEMYSETD dx=0 dy=0 reg=0 id=29
    ADD dst=0 src=1
    NOP count=16
    LOOP jmp=@label_1A11 count=8
    NOP count=60
    RET
.org 0x1A2B
@label_1A2B:
    DRAW_ON
    DAMAGE_ON
    ANM pattern=18 speed=10
    ANMEX pattern=22
    RET
.org 0x1A33
@label_1A33:
    DRAW_ON
    DAMAGE_ON
    ANM pattern=19 speed=6
    ANMEX pattern=23
    NOP count=10
    RET
.org 0x1A3E
@label_1A3E:
    DRAW_ON
    DAMAGE_ON
    CEFC x=-26 y=40 type=1
    ANM pattern=19 speed=0
    ANMEX pattern=23
    NOP count=145
    ANM pattern=19 speed=6
    ANMEX pattern=23
    NOP count=10
    RET
.org 0x1A57
@label_1A57:
    DRAW_ON
    DAMAGE_ON
    ANM pattern=27 speed=0
    ANMEX pattern=24
    PSE id=18
    NOP count=145
    ANM pattern=20 speed=6
    ANMEX pattern=25
    NOP count=10
    RET
.org 0x1A6C
@label_1A6C:
    DRAW_ON
    DAMAGE_ON
    ANM pattern=27 speed=0
    ANMEX pattern=24
    NOP count=35
    ANM pattern=20 speed=6
    ANMEX pattern=25
    NOP count=10
    RET
.org 0x1A7F
@label_1A7F:
    DRAW_ON
    DAMAGE_OFF
    ANM pattern=21 speed=2
    RET
.org 0x1A85
@label_1A85:
    DRAW_ON
    ANM pattern=26 speed=0
    RET
.org 0x1A8A
@label_1A8A:
    DRAW_ON
    DAMAGE_ON
    ANM pattern=27 speed=0
    ANMEX pattern=24
    CEFC x=0 y=0 type=1
    NOP count=145
    RET
.org 0x1A9B
@label_1A9B:
    DRAW_ON
    ANM pattern=18 speed=10
    ANMEX pattern=22
    NOP count=32
    ANM pattern=27 speed=0
    ANMEX pattern=24
    CEFC x=0 y=0 type=3
    PSE id=17
.org 0x1AB1
@label_1AB1:
    DRAW_ON
    NOP count=2
    DRAW_OFF
    NOP count=2
    LOOP jmp=@label_1AB1 count=6
    DAMAGE_OFF
    NOP count=20
    PSE id=19
    NOP count=5
    XYRND
    CEFC x=0 y=0 type=2
    NOP count=20
    DAMAGE_ON
.org 0x1AD4
@label_1AD4:
    DRAW_OFF
    NOP count=2
    DRAW_ON
    NOP count=2
    LOOP jmp=@label_1AD4 count=6
    ANM pattern=18 speed=10
    ANMEX pattern=22
    RET
.org 0x1AE9
@label_1AE9:
    DRAW_ON
    ANM pattern=18 speed=10
    ANMEX pattern=22
    NOP count=32
    ANM pattern=27 speed=0
    ANMEX pattern=24
    CEFC x=0 y=0 type=3
    PSE id=17
.org 0x1AFF
@label_1AFF:
    DRAW_ON
    NOP count=2
    DRAW_OFF
    NOP count=2
    LOOP jmp=@label_1AFF count=6
    DAMAGE_OFF
    NOP count=20
    PSE id=19
    NOP count=5
    RET
.org 0x1B18
@label_1B18:
    CEFC x=0 y=0 type=2
    NOP count=20
    DAMAGE_ON
.org 0x1B22
@label_1B22:
    DRAW_OFF
    NOP count=2
    DRAW_ON
    NOP count=2
    LOOP jmp=@label_1B22 count=6
    ANM pattern=18 speed=10
    ANMEX pattern=22
    RET
.org 0x1B37
@script_23:
    SETUP hp=99999 score=99999
    CLIP_ON
    ANM pattern=28 speed=3
    SPDA speed=192
    GRAX gravity=2
.org 0x1B4B
@label_1B4B:
    NOP count=34463
    JMP jmp=@label_1B4B
.org 0x1B53
@script_24:
    SETUP hp=99999 score=99999
    CLIP_ON
    ANM pattern=29 speed=4
    SPDA speed=128
    MOV count=10
    ROL deg=3 count=70
    SPDA speed=256
    GRAX gravity=2
.org 0x1B73
@label_1B73:
    NOP count=34463
    JMP jmp=@label_1B73
.org 0x1B7B
@script_25:
    SETUP hp=99999 score=99999
    CLIP_ON
    ANM pattern=29 speed=4
    SPDA speed=128
    MOV count=10
    ROL deg=-3 count=70
    SPDA speed=256
    GRAX gravity=2
.org 0x1B9B
@label_1B9B:
    NOP count=34463
    JMP jmp=@label_1B9B
.org 0x1BA3
@script_26:
    SETUP hp=99999 score=99999
    CLIP_ON
    ANM pattern=30 speed=4
    SPDA speed=128
    MOV count=10
    ROL deg=3 count=70
    SPDA speed=256
    GRAX gravity=2
.org 0x1BC3
@label_1BC3:
    NOP count=34463
    JMP jmp=@label_1BC3
.org 0x1BCB
@script_27:
    SETUP hp=99999 score=99999
    CLIP_ON
    ANM pattern=30 speed=4
    SPDA speed=128
    MOV count=10
    ROL deg=-3 count=70
    SPDA speed=256
    GRAX gravity=2
.org 0x1BEB
@label_1BEB:
    NOP count=34463
    JMP jmp=@label_1BEB
.org 0x1BF3
@script_28:
    SETUP hp=99999 score=99999
    CLIP_ON
    ANM pattern=31 speed=4
    SPDA speed=192
    GRAX gravity=3
.org 0x1C07
@label_1C07:
    NOP count=34463
    JMP jmp=@label_1C07
.org 0x1C0F
@script_29:
    SETUP hp=99999 score=99999
    CLIP_ON
    ANM pattern=32 speed=4
    TCMD cmd=1
    TNUMA n=5 ns=0
    TSPDA v=29 a=-11
    TTYPE type=1
    TREP rep=35
    TXYR dx=0 dy=0
    TCOL color=17
    SPDA speed=384
    ACC accel=-6 count=60
    RND reg=0
    MOD reg=0 div=65536
    CMPC reg=0 val=32768
    JL jmp=@label_1C60
.org 0x1C4B
@label_1C4B:
    PSE id=4
    TAMA2
    NOP count=6
    TDEGR angle=8 dw=0
    LOOP jmp=@label_1C4B count=15
    JMP jmp=@label_1C70
.org 0x1C60
@label_1C60:
    PSE id=4
    TAMA2
    NOP count=6
    TDEGR angle=-8 dw=0
    LOOP jmp=@label_1C60 count=15
.org 0x1C70
@label_1C70:
    CLIP_OFF
    ACC accel=6 count=6000
    JMP jmp=@label_1C70
.org 0x1C7A
@script_30:
    SETUP hp=80 score=1000
    RLCHG_ON
    CLIP_ON
    ANM pattern=33 speed=0
    TCMD cmd=12
    TDEGA angle=0 dw=5
    TNUMA n=11 ns=2
    TSPDA v=27 a=0
    TTYPE type=0
    TCOL color=16
    TAUTO interval=50
    DEGA angle=112
    SPDA speed=128
    MOV count=30
    SPDA speed=64
    LROL vx=-24 vy=24 deg=-2 count=128
    CLIP_OFF
    DEGA angle=16
    SPDA speed=128
    MOV count=30
    SPDA speed=64
    LROL vx=24 vy=24 deg=-2 count=128
.org 0x1CD0
@label_1CD0:
    MOV count=1000
    JMP jmp=@label_1CD0
.org 0x1CD8
@script_31:
    SETUP hp=12 score=1000
    CLIP_ON
    ANM pattern=34 speed=0
    TCMD cmd=89
    TDEGA angle=0 dw=20
    TNUMA n=1 ns=0
    TSPDA v=22 a=0
    TTYPE type=0
    TCOL color=19
    DEGA angle=64
    SPDA speed=320
    MOV count=40
    CLIP_OFF
    TAUTO interval=20
    RLCHG_ON
    ROL deg=2 count=64
    MOV count=20
    RLCHG_ON
    ROL deg=2 count=64
.org 0x1D0E
@label_1D0E:
    MOV count=1000
    JMP jmp=@label_1D0E
.org 0x1D16
@script_32:
@script_33:  ; shared
@script_34:  ; shared
    SETUP hp=10000 score=10000
    CLIP_ON
    ANM pattern=37 speed=0
    TCMD cmd=81
    TDEGA angle=64 dw=1
    TNUMA n=1 ns=0
    TSPDA v=28 a=0
    TTYPE type=0
    TCOL color=5
    SPDA speed=512
    ACC accel=-8 count=30
    PSE id=11
.org 0x1D3D
@label_1D3D:
    TAMA
    NOP count=1
    LOOP jmp=@label_1D3D count=64
    SPDA speed=64
    CLIP_OFF
.org 0x1D4E
@label_1D4E:
    ACC accel=-8 count=200
    JMP jmp=@label_1D4E
.org 0x1D57
@script_35:
    SETUP hp=10000 score=10000
    CLIP_ON
    RLCHG_ON
    ANM pattern=37 speed=0
    TCMD cmd=81
    TDEGA angle=64 dw=1
    TNUMA n=1 ns=0
    TSPDA v=28 a=0
    TTYPE type=0
    TCOL color=5
    DEGA angle=96
    SPDA speed=512
    ACC accel=-8 count=30
    TDEGE
    PSE id=11
    PSE id=11
.org 0x1D84
@label_1D84:
    TAMA
    NOP count=1
    LOOP jmp=@label_1D84 count=64
    SPDA speed=64
    CLIP_OFF
.org 0x1D95
@label_1D95:
    ACC accel=-8 count=200
    JMP jmp=@label_1D95
.org 0x1D9E
@script_36:
    SETUP hp=10000 score=10000
    CLIP_ON
    RLCHG_ON
    ANM pattern=37 speed=0
    TCMD cmd=81
    TDEGA angle=64 dw=1
    TNUMA n=1 ns=0
    TSPDA v=28 a=0
    TTYPE type=0
    TCOL color=5
    DEGA angle=128
    SPDA speed=512
    ACC accel=-8 count=30
    TDEGE
    PSE id=11
    PSE id=11
.org 0x1DCB
@label_1DCB:
    TAMA
    NOP count=1
    LOOP jmp=@label_1DCB count=64
    SPDA speed=64
    CLIP_OFF
.org 0x1DDC
@label_1DDC:
    ACC accel=-8 count=200
    JMP jmp=@label_1DDC
.org 0x1DE5
@script_37:
    SETUP hp=24 score=600
    RLCHG_ON
    CLIP_ON
    ANM pattern=35 speed=0
    SPDA speed=256
    DEGS
    LCMD cmd=8
    LLA len=7680
    LDEGA angle=0 dw=16
    LNUMA n=5
    LSPDA v=704
    LCOL color=0
    LTYPE type=0
    LWA w=192
    ACC accel=4 count=10
    DEGS
    CLIP_OFF
    PSE id=3
    LASER
.org 0x1E1C
@label_1E1C:
    ACC accel=2 count=1000
    JMP jmp=@label_1E1C
.org 0x1E25
@script_38:
    SETUP hp=120 score=1000
    RLCHG_ON
    CLIP_ON
    ANM pattern=36 speed=0
    SPDA speed=320
    DEGA angle=64
    TCMD cmd=0
    TNUMA n=2 ns=0
    TSPDA v=9 a=0
    TTYPE type=0
    ACC accel=-6 count=30
    MOVC dst=0 val=3
.org 0x1E4E
@label_1E4E:
    DEC reg=0
    CMPC reg=0 val=0
    JEQ jmp=@label_1E80
    NOP count=20
    TDEGA angle=0 dw=0
    TDEGS
.org 0x1E62
@label_1E62:
    TAMA2
    NOP count=8
    TDEGR angle=0 dw=10
    LOOP jmp=@label_1E62 count=5
    DEGR angle=-128
    SPDA speed=320
    ACC accel=-6 count=30
    JMP jmp=@label_1E4E
.org 0x1E80
@label_1E80:
    CLIP_OFF
    DEGXU
    SPDA speed=0
.org 0x1E87
@label_1E87:
    ACC accel=4 count=1000
    JMP jmp=@label_1E87
.org 0x1E90
@script_39:
    TCOL color=0
    JMP jmp=@script_38
.org 0x1E97
@script_40:
    TCOL color=1
    JMP jmp=@script_38
.org 0x1E9E
@script_41:
    TCOL color=2
    JMP jmp=@script_38
.org 0x1EA5
@script_42:
    TCOL color=3
    JMP jmp=@script_38
.org 0x1EAC
@script_43:
    TCOL color=5
    JMP jmp=@script_38
.org 0x1EB3
@script_44:
    SETUP hp=7 score=600
    RLCHG_ON
    CLIP_ON
    ANM pattern=35 speed=0
    LCMD cmd=0
    LLA len=19200
    LDEGA angle=0 dw=16
    LNUMA n=1
    LSPDA v=576
    LCOL color=0
    LTYPE type=0
    LWA w=192
    SPDA speed=512
    DEGS
    ACC accel=-10 count=20
    MOVR dst=128 src=145
    LASER2
    PSE id=3
    NOP count=35
    DEGS
    SPDA speed=0
    CLIP_OFF
.org 0x1EF5
@label_1EF5:
    ACC accel=6 count=100
    JMP jmp=@label_1EF5
.org 0x1EFE
@script_45:
    SETUP hp=80 score=10000
    RLCHG_ON
    CLIP_ON
    ANM pattern=38 speed=0
    ANMEX pattern=41
    TCMD cmd=13
    TNUMA n=48 ns=1
    TSPDA v=16 a=0
    TTYPE type=0
    TCOL color=21
    TDEGA angle=0 dw=0
    LDEGA angle=64 dw=0
    LNUMA n=1
    LSPDA v=64
    LCOL color=2
    LWA w=1280
    LTYPE type=0
    LLSET
    MXS count=60
    TAMA
    NOP count=30
    LLOPEN id=255
    NOP count=20
    LLCLOSE id=255
    SPDA speed=0
    CLIP_OFF
    DEGA angle=192
.org 0x1F47
@label_1F47:
    ACC accel=8 count=1000
    JMP jmp=@label_1F47
.org 0x1F50
@script_46:
    SETUP hp=2900 score=500000
    STI jmp=@label_2001 vector=HP val=900
    ITEM type=2
    RLCHG_ON
    CLIP_ON
    ANM pattern=39 speed=16
    ANMEX pattern=40
    DEGA angle=64
    SPDA speed=512
    ACC accel=-12 count=60
.org 0x1F77
@label_1F77:
    MOVC dst=0 val=3
.org 0x1F7D
@label_1F7D:
    CALL jmp=@script_47
.org 0x1F82
@label_1F82:
    CALL jmp=@label_2161
    PSE id=14
    DEGS
    SPDA speed=960
    ACC accel=-32 count=30
    ANM pattern=39 speed=2
    ANMEX pattern=40
    LLOPEN id=255
    NOP count=50
    LLCLOSE id=255
    ANM pattern=39 speed=16
    ANMEX pattern=40
    DEGR angle=-128
    SPDA speed=960
    ACC accel=-32 count=30
    TAUTO interval=0
    NOP count=15
    DEC reg=0
    CMPC reg=0 val=1
    JEQ jmp=@label_1FCB
    JS jmp=@label_1FD2
    JMP jmp=@label_1F7D
.org 0x1FCB
@label_1FCB:
    PSE id=1
    JMP jmp=@label_1F82
.org 0x1FD2
@label_1FD2:
    NOP count=55
.org 0x1FD5
@label_1FD5:
    CALL jmp=@label_20F5
    NOP count=5
    CALL jmp=@label_20F5
    NOP count=5
    CALL jmp=@label_20F5
    NOP count=5
    CALL jmp=@label_213E
    LOOP jmp=@label_1FD5 count=12
    NOP count=50
    JMP jmp=@label_1F77
.org 0x2001
@label_2001:
    CLI vector=HP
    TCLR
    LLCLOSE id=255
    TAUTO interval=0
    ANM pattern=39 speed=16
    ANMEX pattern=40
    DAMAGE_OFF
    MXYA x=319 y=50 count=40
    DAMAGE_ON
.org 0x2016
@label_2016:
    PSE id=1
    CALL jmp=@label_2181
    NOP count=20
    MOVR dst=1 src=128
    CMPC reg=1 val=64
    JS jmp=@label_207A
.org 0x202E
@label_202E:
    NOP count=2
    PSE id=12
    CALL jmp=@label_210A
    LOOP jmp=@label_202E count=12
.org 0x203F
@label_203F:
    NOP count=2
    PSE id=12
    CALL jmp=@label_2124
    LOOP jmp=@label_203F count=12
.org 0x2050
@label_2050:
    NOP count=2
    PSE id=12
    CALL jmp=@label_210A
    LOOP jmp=@label_2050 count=12
.org 0x2061
@label_2061:
    NOP count=2
    PSE id=12
    CALL jmp=@label_2124
    LOOP jmp=@label_2061 count=12
    NOP count=80
    JMP jmp=@label_20C1
.org 0x207A
@label_207A:
    NOP count=2
    PSE id=12
    CALL jmp=@label_2124
    LOOP jmp=@label_207A count=12
.org 0x208B
@label_208B:
    NOP count=2
    PSE id=12
    CALL jmp=@label_210A
    LOOP jmp=@label_208B count=12
.org 0x209C
@label_209C:
    NOP count=2
    PSE id=12
    CALL jmp=@label_2124
    LOOP jmp=@label_209C count=12
.org 0x20AD
@label_20AD:
    NOP count=2
    PSE id=12
    CALL jmp=@label_210A
    LOOP jmp=@label_20AD count=12
    NOP count=80
.org 0x20C1
@label_20C1:
    ANM pattern=39 speed=2
    ANMEX pattern=40
    LLOPEN id=255
    NOP count=80
    LLCLOSE id=255
    ANM pattern=39 speed=16
    ANMEX pattern=40
    DEGX2
    SPDA speed=320
    ACC accel=-6 count=50
    JMP jmp=@label_2016
.org 0x20E1
@script_47:
    TCMD cmd=0
    TDEGA angle=192 dw=15
    TNUMA n=7 ns=2
    TSPDA v=217 a=-6
    TREP rep=1
    TTYPE type=2
    TCOL color=17
    TAUTO interval=20
    RET
.org 0x20F5
@label_20F5:
    TCMD cmd=94
    TDEGA angle=0 dw=60
    TNUMA n=1 ns=1
    TSPDA v=217 a=-8
    TREP rep=40
    TTYPE type=1
    TCOL color=18
    PSE id=11
    TAMA
    RET
.org 0x210A
@label_210A:
    TCMD cmd=1
    TDEGR angle=-1 dw=0
    TNUMA n=23 ns=1
    TSPDA v=28 a=0
    TREP rep=20
    TTYPE type=4
    TCOL color=17
    TVDEG vd=2
    TAMA2
    TCOL color=18
    TVDEG vd=-2
    TAMA2
    RET
.org 0x2124
@label_2124:
    TCMD cmd=1
    TDEGR angle=1 dw=0
    TNUMA n=23 ns=1
    TSPDA v=28 a=0
    TREP rep=20
    TTYPE type=4
    TCOL color=17
    TVDEG vd=-2
    TAMA2
    TCOL color=18
    TVDEG vd=2
    TAMA2
    RET
.org 0x213E
@label_213E:
    LCMD cmd=8
    LLA len=1920
    LDEGA angle=0 dw=5
    LNUMA n=32
    LSPDA v=576
    LCOL color=0
    LTYPE type=0
    LWA w=192
    LXY x=0 y=0
    PSE id=3
    LASER2
    RET
.org 0x2161
@label_2161:
    LDEGA angle=64 dw=0
    LNUMA n=1
    LSPDA v=64
    LCOL color=2
    LWA w=1024
    LTYPE type=0
    LXY x=28 y=22
    LLSET
    LXY x=-28 y=22
    LLSET
    RET
.org 0x2181
@label_2181:
    LDEGA angle=64 dw=0
    LNUMA n=1
    LSPDA v=384
    LCOL color=3
    LWA w=9600
    LTYPE type=0
    LXY x=0 y=0
    LDEGS
    LLSET
    RET
.org 0x219C
@script_48:
    SETUP hp=8 score=600
    RLCHG_ON
    CLIP_ON
    ANM pattern=35 speed=0
    SPDA speed=256
    DEGS
    ACC accel=4 count=10
    CLIP_OFF
.org 0x21B5
@label_21B5:
    ACC accel=4 count=1000
    JMP jmp=@label_21B5
.org 0x21BE
@script_49:
    SETUP hp=90 score=800
    RLCHG_ON
    CLIP_ON
    ANM pattern=36 speed=0
    SPDA speed=320
    DEGA angle=64
    TCOL color=16
    TCMD cmd=2
    TNUMA n=5 ns=0
    TSPDA v=198 a=0
    TTYPE type=0
    TDEGA angle=64 dw=255
    ACC accel=-6 count=30
    TAUTO interval=10
    NOP count=180
    TAUTO interval=0
.org 0x21ED
@label_21ED:
    ACC accel=4 count=1000
    JMP jmp=@label_21ED
.org 0x21F6
@script_50:
    SETUP hp=200 score=50000
    RLCHG_ON
    CLIP_ON
    ITEM type=3
    ANM pattern=38 speed=0
    ANMEX pattern=41
    TCMD cmd=2
    TNUMA n=5 ns=0
    TSPDA v=220 a=0
    TTYPE type=0
    TDEGA angle=192 dw=190
    TCOL color=19
    TAUTO interval=2
    LDEGA angle=64 dw=0
    LNUMA n=1
    LSPDA v=64
    LCOL color=4
    LWA w=1280
    LTYPE type=0
    MOVC dst=0 val=128
    ENEMYSETD dx=-20 dy=-30 reg=0 id=51
    MOVC dst=0 val=0
    ENEMYSETD dx=20 dy=-30 reg=0 id=51
    DEGA angle=64
    SPDA speed=320
    ACC accel=-6 count=60
    NOP count=10
    LLSET
    LLSET
    LLSET
    LLSET
    LCMD cmd=0
    LLA len=8320
    LNUMA n=2
    LSPDA v=512
    LCOL color=0
    LTYPE type=1
    LWA w=192
    LXY x=0 y=5
    LDEGA angle=64 dw=64
.org 0x2277
@label_2277:
    NOP count=3
    PSE id=3
    LASER2
    LDEGR angle=0 dw=-8
    LOOP jmp=@label_2277 count=6
.org 0x2287
@label_2287:
    NOP count=3
    PSE id=3
    LASER2
    LDEGR angle=0 dw=8
    LOOP jmp=@label_2287 count=6
    LLOPEN id=255
    NOP count=140
    LLCLOSE id=255
    SPDA speed=0
    CLIP_OFF
    DEGA angle=192
.org 0x22A6
@label_22A6:
    ACC accel=8 count=1000
    JMP jmp=@label_22A6
.org 0x22AF
@script_51:
    SETUP hp=99999 score=0
    RLCHG_ON
    CLIP_ON
    DAMAGE_OFF
    ANM pattern=42 speed=2
    LDEGA angle=64 dw=0
    LNUMA n=1
    LSPDA v=64
    LCOL color=2
    LWA w=960
    LTYPE type=0
    MOVR dst=0 src=145
    DEGA angle=64
    SPDA speed=448
    ACC accel=-8 count=40
    LLSET
    MOVR dst=145 src=0
    SPDA speed=448
    ACC accel=-9 count=34
    LLOPEN id=255
    NOP count=80
    LLCLOSE id=255
    CLIP_OFF
    RLCHG_ON
    DEGR angle=-48
    SPDA speed=64
.org 0x22FC
@label_22FC:
    ACC accel=8 count=1000
    JMP jmp=@label_22FC
.org 0x2305
@script_52:
    SETUP hp=99999 score=99999
    CLIP_ON
    ANM pattern=28 speed=4
    TCMD cmd=81
    TDEGA angle=128 dw=1
    TNUMA n=1 ns=0
    TSPDA v=28 a=0
    TTYPE type=0
    TCOL color=5
    DEGA angle=0
    MOVC dst=0 val=2
    SPDA speed=256
    ACC accel=-6 count=30
    NOP count=60
.org 0x2335
@label_2335:
    SPDA speed=384
    ACC accel=-3 count=92
    NOP count=120
.org 0x2341
@label_2341:
    TAMA2
    NOP count=3
    LOOP jmp=@label_2341 count=30
    NOP count=120
    TDEGR angle=-128 dw=0
    DEGR angle=-128
    DEC reg=0
    CMPC reg=0 val=0
    JL jmp=@label_2335
    DEGR angle=-128
    MOV count=100
    END
    JMP jmp=0x2363
.org 0x236C
@script_53:
    SETUP hp=99999 score=99999
    CLIP_ON
    ANM pattern=28 speed=4
    TCMD cmd=81
    TDEGA angle=0 dw=1
    TNUMA n=1 ns=0
    TSPDA v=28 a=0
    TTYPE type=0
    TCOL color=5
    DEGA angle=128
    MOVC dst=0 val=2
    SPDA speed=256
    ACC accel=-6 count=30
    NOP count=60
.org 0x239C
@label_239C:
    SPDA speed=384
    ACC accel=-3 count=92
    NOP count=120
.org 0x23A8
@label_23A8:
    TAMA2
    NOP count=3
    LOOP jmp=@label_23A8 count=30
    NOP count=120
    TDEGR angle=-128 dw=0
    DEGR angle=-128
    DEC reg=0
    CMPC reg=0 val=0
    JL jmp=@label_239C
    DEGR angle=-128
    MOV count=100
    END
    JMP jmp=0x23CA
