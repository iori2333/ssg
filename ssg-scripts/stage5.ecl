.header 30
.offset 0 0x007C
.offset 1 0x0176
.offset 2 0x01B7
.offset 3 0x023E
.offset 4 0x027E
.offset 5 0x03A4
.offset 6 0x03C9
.offset 7 0x03C9
.offset 8 0x03C9
.offset 9 0x058D
.offset 10 0x05DA
.offset 11 0x062D
.offset 12 0x067A
.offset 13 0x070A
.offset 14 0x070A
.offset 15 0x070A
.offset 16 0x070A
.offset 17 0x070A
.offset 18 0x070A
.offset 19 0x070A
.offset 20 0x070A
.offset 21 0x0B78
.offset 22 0x0F29
.offset 23 0x0F67
.offset 24 0x1031
.offset 25 0x103C
.offset 26 0x1047
.offset 27 0x10F6
.offset 28 0x112C
.offset 29 0x007C

.org 0x007C
@script_0:
@script_29:  ; shared
    SETUP hp=60 score=500
    CLIP_ON
    DAMAGE_OFF
    HITSB_OFF
    ANM pattern=1 speed=0
    SPDA speed=64
    DEGA angle=64
    MOVR dst=0 src=143
    MOVR dst=1 src=144
    CMPC reg=1 val=4096
    JS jmp=@label_00C4
    CMPC reg=0 val=20416
    JL jmp=@label_00B9
    MOVC dst=2 val=32
    JMP jmp=@label_00CF
.org 0x00B9
@label_00B9:
    MOVC dst=2 val=96
    JMP jmp=@label_00CF
.org 0x00C4
@label_00C4:
    MOVC dst=2 val=64
    JMP jmp=@label_00CF
.org 0x00CF
@label_00CF:
    RND reg=0
    MOD reg=0 div=64
    MOVC dst=1 val=32
    SUB dst=0 src=1
    ADD dst=0 src=2
    MOVR dst=145 src=0
    TCMD cmd=80
    TDEGA angle=0 dw=1
    TDEGE
    TNUMA n=1 ns=1
    TSPDA v=9 a=1
    TTYPE type=0
.org 0x00F4
@label_00F4:
    NOP count=1
    DEGR angle=16
    LOOP jmp=@label_00F4 count=31
    NOP count=30
    DAMAGE_ON
    DEGR angle=-128
    MOVR dst=2 src=145
    DEGR angle=-128
.org 0x010B
@label_010B:
    NOP count=1
    MOVC dst=0 val=256
    MOVC dst=1 val=256
    COSL len=0 deg=2
    SINL len=1 deg=2
    ADD dst=0 src=143
    ADD dst=1 src=144
    MOVR dst=143 src=0
    MOVR dst=144 src=1
    LOOP jmp=@label_010B count=14
    ANM pattern=0 speed=0
.org 0x0136
@label_0136:
    TCOL color=5
    TAMA
    MOV count=1
    SPDR speed=16
    MOV count=1
    SPDR speed=16
    TCOL color=21
    TAMA
    MOV count=1
    SPDR speed=16
    MOV count=1
    SPDR speed=16
    LOOP jmp=@label_0136 count=5
    CLIP_OFF
    JMP jmp=@label_0136
.org 0x0169
@label_0169:
    MOV count=1
    SPDR speed=16
    JMP jmp=@label_0169
.org 0x0176
@script_1:
    SETUP hp=60 score=200
    CLIP_ON
    ANM pattern=2 speed=0
    SPDA speed=192
    MOV count=30
    TCMD cmd=2
    TDEGA angle=0 dw=96
    TDEGS
    TNUMA n=3 ns=1
    TSPDA v=20 a=-3
    TREP rep=60
    TTYPE type=1
.org 0x019B
@label_019B:
    TCOL color=1
    TAMA
    NOP count=2
    TCOL color=17
    TAMA
    NOP count=2
    LOOP jmp=@label_019B count=20
    CLIP_ON
.org 0x01AF
@label_01AF:
    MOV count=200
    JMP jmp=@label_01AF
.org 0x01B7
@script_2:
    SETUP hp=60 score=10000
    CLIP_ON
    ANM pattern=5 speed=2
    SPDA speed=192
    MOV count=30
    TCMD cmd=5
    TDEGA angle=0 dw=96
    TNUMA n=25 ns=5
    TSPDA v=9 a=0
    TCOL color=16
    TTYPE type=0
    DEGS
    LCMD cmd=0
    LLA len=17280
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
    MOVC dst=2 val=10
.org 0x0212
@label_0212:
    MOVR dst=128 src=0
    LASER2
    MOVR dst=128 src=1
    LASER2
    PSE id=3
    SUB dst=0 src=2
    ADD dst=1 src=2
    NOP count=1
    LOOP jmp=@label_0212 count=8
    DEGR angle=-128
    MOV count=15
    TDEGS
    TAMA
    DEGR angle=-128
    CLIP_ON
.org 0x0236
@label_0236:
    MOV count=200
    JMP jmp=@label_0236
.org 0x023E
@script_3:
    SETUP hp=60 score=2000
    CLIP_ON
    RLCHG_ON
    ANM pattern=3 speed=0
    SPDA speed=192
    MOV count=30
    TCMD cmd=1
    TDEGA angle=0 dw=0
    TNUMA n=20 ns=1
    TSPDA v=9 a=-2
    TVDEG vd=1
    TREP rep=1
    TCOL color=4
    TTYPE type=5
.org 0x0267
@label_0267:
    TAMA
    TDEGR angle=-1 dw=0
    NOP count=2
    LOOP jmp=@label_0267 count=10
    CLIP_ON
.org 0x0276
@label_0276:
    MOV count=200
    JMP jmp=@label_0276
.org 0x027E
@script_4:
    SETUP hp=120 score=100
    CLIP_ON
    DAMAGE_OFF
    HITSB_OFF
    ANM pattern=6 speed=6
    SPDA speed=224
    TAUTO interval=1
    TAUTO interval=0
    TCMD cmd=0
    TDEGA angle=0 dw=4
    TNUMA n=5 ns=1
    TSPDA v=8 a=0
    TCOL color=35
    TTYPE type=0
    MOVR dst=0 src=143
    MOVR dst=1 src=144
    MOVC dst=2 val=3200
    MOVC dst=5 val=224
    CMPC reg=0 val=20416
    JS jmp=@label_0315
    MOVC dst=3 val=0
    MOVR dst=4 src=0
    ADD dst=4 src=2
    MOVR dst=143 src=4
    MOVC dst=4 val=4294965248
    MOVR dst=144 src=4
    CALL jmp=@label_036F
.org 0x02DF
@label_02DF:
    CALL jmp=@label_0384
    INC reg=3
    INC reg=3
    MOVC dst=6 val=30
    ADD dst=2 src=6
    CMPC reg=2 val=5120
    JS jmp=@label_02DF
    MOVC dst=2 val=5120
    ANM pattern=4 speed=0
    DAMAGE_ON
    HITSB_ON
    TAUTO interval=14
    LOOP jmp=@label_02DF count=244
    JMP jmp=@label_0363
.org 0x0315
@label_0315:
    MOVC dst=3 val=128
    MOVR dst=4 src=0
    SUB dst=4 src=2
    MOVR dst=143 src=4
    MOVC dst=4 val=4294965248
    MOVR dst=144 src=4
    CALL jmp=@label_036F
.org 0x0332
@label_0332:
    CALL jmp=@label_0384
    DEC reg=3
    DEC reg=3
    MOVC dst=6 val=30
    ADD dst=2 src=6
    CMPC reg=2 val=5120
    JS jmp=@label_0332
    MOVC dst=2 val=5120
    ANM pattern=4 speed=0
    DAMAGE_ON
    HITSB_ON
    TAUTO interval=14
    LOOP jmp=@label_0332 count=244
.org 0x0363
@label_0363:
    RLCHG_ON
    DEGR angle=64
    CLIP_OFF
    MOV count=200
    JMP jmp=@label_0363
.org 0x036F
@label_036F:
    NOP count=1
    MOVR dst=144 src=4
    ADD dst=4 src=5
    CMPR reg0=4 reg1=1
    JS jmp=@label_036F
    MOVR dst=144 src=1
    RET
.org 0x0384
@label_0384:
    NOP count=1
    MOVR dst=4 src=2
    COSL len=4 deg=3
    ADD dst=4 src=0
    MOVR dst=143 src=4
    MOVR dst=4 src=2
    SINL len=4 deg=3
    ADD dst=4 src=1
    MOVR dst=144 src=4
    MOVR dst=145 src=3
    TDEGE
    RET
.org 0x03A4
@script_5:
    SETUP hp=30 score=3000
    CLIP_ON
    RLCHG_ON
    ANM pattern=2 speed=0
    SPDA speed=384
    DEGA angle=96
    MOV count=40
    ROL deg=2 count=96
    CLIP_OFF
.org 0x03C1
@label_03C1:
    MOV count=80
    JMP jmp=@label_03C1
.org 0x03C9
@script_6:
@script_7:  ; shared
@script_8:  ; shared
    SETUP hp=3600 score=3000
    STI jmp=@label_0579 vector=TIMER val=1800
    CLIP_ON
    ITEM type=2
    ANM pattern=8 speed=0
    SPDA speed=128
    MOV count=100
    RND reg=2
    MOD reg=2 div=1000
    CMPC reg=2 val=500
    JL jmp=@label_0408
    MOVC dst=2 val=0
    JMP jmp=@label_040E
.org 0x0408
@label_0408:
    MOVC dst=2 val=10
.org 0x040E
@label_040E:
    TCMD cmd=1
    TDEGA angle=0 dw=0
    TNUMA n=26 ns=1
    TSPDA v=9 a=0
    TCOL color=18
    TTYPE type=0
    TDEGS
    DEGA angle=192
    MOV count=30
    MOVC dst=0 val=32
    MOVC dst=1 val=64
    ENEMYSETD dx=0 dy=0 reg=0 id=10
    ADD dst=0 src=1
    ENEMYSETD dx=0 dy=0 reg=0 id=10
    ADD dst=0 src=1
    ENEMYSETD dx=0 dy=0 reg=0 id=10
    ADD dst=0 src=1
    ENEMYSETD dx=0 dy=0 reg=0 id=10
    ADD dst=0 src=1
.org 0x0457
@label_0457:
    TAMA
    TDEGR angle=4 dw=0
    NOP count=10
    LOOP jmp=@label_0457 count=9
    NOP count=70
    DEGR angle=-128
    MOV count=30
    MOVC dst=0 val=64
    MOVC dst=1 val=85
    ENEMYSETD dx=0 dy=0 reg=0 id=9
    ADD dst=0 src=1
    ENEMYSETD dx=0 dy=0 reg=0 id=9
    ADD dst=0 src=1
    ENEMYSETD dx=0 dy=0 reg=0 id=9
    ADD dst=0 src=1
    NOP count=60
    MOVC dst=1 val=32
    ENEMYSETD dx=0 dy=0 reg=0 id=11
    ADD dst=0 src=1
    ENEMYSETD dx=0 dy=0 reg=0 id=11
    ADD dst=0 src=1
    ENEMYSETD dx=0 dy=0 reg=0 id=11
    ADD dst=0 src=1
    ENEMYSETD dx=0 dy=0 reg=0 id=11
    ADD dst=0 src=1
    ENEMYSETD dx=0 dy=0 reg=0 id=11
    ADD dst=0 src=1
    ENEMYSETD dx=0 dy=0 reg=0 id=11
    ADD dst=0 src=1
    ENEMYSETD dx=0 dy=0 reg=0 id=11
    ADD dst=0 src=1
    ENEMYSETD dx=0 dy=0 reg=0 id=11
    ADD dst=0 src=1
    NOP count=200
    CMPC reg=2 val=5
    JS jmp=@label_0514
    DEGA angle=16
    TDEGA angle=208 dw=96
    MOVC dst=0 val=32
    MOVC dst=2 val=0
    JMP jmp=@label_0525
.org 0x0514
@label_0514:
    DEGA angle=112
    TDEGA angle=176 dw=96
    MOVC dst=0 val=96
    MOVC dst=2 val=10
.org 0x0525
@label_0525:
    TCMD cmd=2
    TNUMA n=6 ns=1
    TSPDA v=7 a=0
    TCOL color=19
    TTYPE type=0
    TXYR dx=0 dy=30
    SPDA speed=256
    MOV count=50
    DEGR angle=-128
    SPDA speed=512
    TAUTO interval=3
.org 0x0547
@label_0547:
    ENEMYSETD dx=0 dy=0 reg=0 id=12
    MOV count=5
    LOOP jmp=@label_0547 count=9
    TAUTO interval=0
    TXYR dx=0 dy=0
    SPDA speed=256
    NOP count=80
    DEGR angle=-128
    MOV count=50
    SPDA speed=128
    NOP count=30
    JMP jmp=@label_040E
.org 0x0579
@label_0579:
    SETUP hp=0 score=0  ; death marker
    CLI vector=TIMER
    TCLR
    NOP count=1000
    JMP jmp=@label_0579
.org 0x058D
@script_9:
    SETUP hp=14999 score=0
    STI jmp=@label_05D8 vector=BOSSLEFT val=0
    ANM pattern=7 speed=4
    CLIP_ON
    DAMAGE_OFF
    HITSB_OFF
    SPDA speed=192
    TCMD cmd=0
    TDEGA angle=0 dw=16
    TNUMA n=3 ns=1
    TSPDA v=8 a=0
    TCOL color=3
    TTYPE type=0
    MOV count=20
    DEGR angle=64
.org 0x05BF
@label_05BF:
    ROL deg=2 count=3
    TDEGE
    TDEGR angle=-64 dw=0
    TAMA
    LOOP jmp=@label_05BF count=64
    NOP count=5
    DEGR angle=64
    MOV count=20
    END
.org 0x05D8
@label_05D8:
    TCLR
    END
.org 0x05DA
@script_10:
    SETUP hp=14999 score=0
    STI jmp=@label_062B vector=BOSSLEFT val=0
    ANM pattern=7 speed=4
    CLIP_ON
    DAMAGE_OFF
    HITSB_OFF
    SPDA speed=192
    LCMD cmd=0
    LLA len=6400
    LDEGA angle=64 dw=10
    LNUMA n=2
    LSPDA v=320
    LCOL color=0
    LTYPE type=0
    LWA w=192
    MOV count=40
.org 0x0615
@label_0615:
    NOP count=3
    LDEGR angle=0 dw=12
    PSE id=3
    LASER2
    LOOP jmp=@label_0615 count=4
    DEGR angle=-128
    MOV count=40
    END
.org 0x062B
@label_062B:
    TCLR
    END
.org 0x062D
@script_11:
    SETUP hp=14999 score=0
    STI jmp=@label_0678 vector=BOSSLEFT val=0
    ANM pattern=7 speed=4
    CLIP_ON
    DAMAGE_OFF
    HITSB_OFF
    SPDA speed=256
    TCMD cmd=0
    TDEGA angle=0 dw=1
    TNUMA n=3 ns=1
    TSPDA v=10 a=0
    TCOL color=16
    TTYPE type=0
    MOV count=20
    DEGR angle=-64
.org 0x065F
@label_065F:
    ROL deg=-2 count=12
    TDEGE
    TDEGR angle=64 dw=0
    TAMA
    LOOP jmp=@label_065F count=12
    NOP count=5
    DEGR angle=-64
    MOV count=20
    END
.org 0x0678
@label_0678:
    TCLR
    END
.org 0x067A
@script_12:
    SETUP hp=14999 score=0
    STI jmp=@label_0708 vector=BOSSLEFT val=0
    ANM pattern=7 speed=4
    CLIP_ON
    DAMAGE_OFF
    HITSB_OFF
    SPDA speed=256
    TCMD cmd=4
    TDEGA angle=0 dw=2
    TNUMA n=1 ns=8
    TSPDA v=10 a=0
    TCOL color=1
    TTYPE type=0
    TDEGE
    JDIF easy=@label_06B9 norm=@label_06C1 hard=@label_06C9 luna=@label_06D1
.org 0x06B9
@label_06B9:
    TNUMA n=1 ns=6
    JMP jmp=@label_06D9
.org 0x06C1
@label_06C1:
    TNUMA n=1 ns=8
    JMP jmp=@label_06D9
.org 0x06C9
@label_06C9:
    TNUMA n=1 ns=10
    JMP jmp=@label_06D9
.org 0x06D1
@label_06D1:
    TNUMA n=1 ns=12
    JMP jmp=@label_06D9
.org 0x06D9
@label_06D9:
    NOP count=30
    MOVR dst=0 src=145
    CMPC reg=0 val=64
    JS jmp=@label_06F9
.org 0x06EA
@label_06EA:
    NOP count=20
    TAMA2
    TDEGR angle=-16 dw=0
    LOOP jmp=@label_06EA count=3
    END
.org 0x06F9
@label_06F9:
    NOP count=20
    TAMA2
    TDEGR angle=16 dw=0
    LOOP jmp=@label_06F9 count=3
    END
.org 0x0708
@label_0708:
    TCLR
    END
.org 0x070A
@script_13:
@script_14:  ; shared
@script_15:  ; shared
@script_16:  ; shared
@script_17:  ; shared
@script_18:  ; shared
@script_19:  ; shared
@script_20:  ; shared
    SETUP hp=40500 score=500000
    CLIP_ON
    DAMAGE_OFF
    ANM pattern=9 speed=0
    SPDA speed=128
    HITXY w=80 h=48
    MOV count=120
    DAMAGE_ON
    RND reg=0
    MOD reg=0 div=5000
    CMPC reg=0 val=2500
    JL jmp=@label_0758
    STI jmp=@label_080D vector=HP val=33000
    STI jmp=@label_080D vector=TIMER val=4800
    MOVC dst=7 val=0
    JMP jmp=@label_0772
.org 0x0758
@label_0758:
    STI jmp=@label_08DF vector=HP val=33000
    STI jmp=@label_08DF vector=TIMER val=4800
    MOVC dst=7 val=5000
.org 0x0772
@label_0772:
    MOVC dst=1 val=3
.org 0x0778
@label_0778:
    PSE id=1
    CALL jmp=@script_21
    CMPC reg=0 val=2500
    JL jmp=@label_07AC
.org 0x078A
@label_078A:
    PSE id=0
    TAMA
    TDEGR angle=-1 dw=0
    NOP count=1
    TAMA
    TDEGR angle=-1 dw=0
    NOP count=1
    LOOP jmp=@label_078A count=10
    MOVC dst=0 val=5000
    JMP jmp=@label_07C9
.org 0x07AC
@label_07AC:
    PSE id=0
    TAMA
    TDEGR angle=1 dw=0
    NOP count=1
    TAMA
    TDEGR angle=1 dw=0
    NOP count=1
    LOOP jmp=@label_07AC count=10
    MOVC dst=0 val=0
.org 0x07C9
@label_07C9:
    NOP count=20
.org 0x07CC
@label_07CC:
    CALL jmp=@label_0B8A
    TAMA
    NOP count=5
    LOOP jmp=@label_07CC count=15
    CALL jmp=@label_0F02
    NOP count=30
    LLOPEN id=255
    NOP count=30
    LLCLOSE id=255
    DEC reg=1
    CMPC reg=1 val=0
    JL jmp=@label_0778
    CALL jmp=@label_0D07
    NOP count=80
    CALL jmp=@label_0D07
    NOP count=80
    JMP jmp=@label_0772
.org 0x080D
@label_080D:
    SETUP hp=33000 score=500000
    STI jmp=@label_0957 vector=HP val=26250
    STI jmp=@label_0957 vector=TIMER val=5200
    LLCLOSE id=255
    TCLR
    TXYR dx=0 dy=0
    NOP count=130
.org 0x0835
@label_0835:
    CALL jmp=@label_0B9C
    MOVC dst=2 val=0
    MOVC dst=5 val=0
    MOVC dst=6 val=0
.org 0x084C
@label_084C:
    PSE id=0
    MOVC dst=3 val=40
    SINL len=3 deg=2
    MOVC dst=4 val=0
    ADD dst=4 src=3
    MOVR dst=134 src=4
    TAMA
    MOVC dst=4 val=128
    SUB dst=4 src=3
    MOVR dst=134 src=4
    TAMA
    NOP count=2
    INC reg=2
    INC reg=2
    INC reg=2
    INC reg=2
    INC reg=5
    CMPC reg=5 val=30
    JS jmp=@label_084C
    INC reg=6
    MOVR dst=5 src=6
    MOD reg=5 div=4
    CMPC reg=5 val=2
    JL jmp=@label_08AF
    MOVC dst=5 val=0
    CALL jmp=@label_0E8C
    JMP jmp=@label_084C
.org 0x08AF
@label_08AF:
    MOVC dst=5 val=0
    CMPC reg=6 val=9
    JL jmp=@label_08CA
    CALL jmp=@label_0F22
    JMP jmp=@label_084C
.org 0x08CA
@label_08CA:
    CALL jmp=@label_0C6B
    NOP count=80
    CALL jmp=@label_0C6B
    NOP count=80
    JMP jmp=@label_0835
.org 0x08DF
@label_08DF:
    SETUP hp=33000 score=500000
    STI jmp=@label_0957 vector=HP val=26250
    STI jmp=@label_0957 vector=TIMER val=5200
    LLCLOSE id=255
    TCLR
    TXYR dx=0 dy=0
    NOP count=130
.org 0x0907
@label_0907:
    CALL jmp=@label_0BF0
    MOVC dst=1 val=3
.org 0x0912
@label_0912:
    NOP count=8
    TCOL color=1
    TAMA
    NOP count=8
    TCOL color=17
    TAMA
    LOOP jmp=@label_0912 count=4
    CALL jmp=@label_0EEE
    DEC reg=1
    CMPC reg=1 val=0
    JL jmp=@label_0912
    NOP count=120
    CALL jmp=@label_0D07
    NOP count=80
    CALL jmp=@label_0D07
    NOP count=80
    CALL jmp=@label_0C0E
    NOP count=80
    JMP jmp=@label_0907
.org 0x0957
@label_0957:
    LLCLOSE id=255
    TCLR
    TXYR dx=0 dy=0
    CMPC reg=7 val=2500
    JS jmp=@label_098C
    SETUP hp=26250 score=500000
    STI jmp=@label_0A8B vector=HP val=14250
    STI jmp=@label_0A8B vector=TIMER val=7600
    JMP jmp=@label_09A9
.org 0x098C
@label_098C:
    SETUP hp=26250 score=500000
    STI jmp=@label_0AB8 vector=HP val=14250
    STI jmp=@label_0AB8 vector=TIMER val=7600
.org 0x09A9
@label_09A9:
    NOP count=130
    RND reg=1
    MOD reg=1 div=5000
    CMPC reg=1 val=2500
    JL jmp=@label_0A18
.org 0x09BF
@label_09BF:
    MOVC dst=1 val=4294967264
    ENEMYSETD dx=0 dy=0 reg=1 id=23
    MOVC dst=1 val=160
    ENEMYSETD dx=0 dy=0 reg=1 id=23
    CALL jmp=@label_0BFE
    TAUTO interval=8
    NOP count=120
.org 0x09E3
@label_09E3:
    ENEMYSETD dx=60 dy=53 reg=1 id=27
    ENEMYSETD dx=-60 dy=53 reg=1 id=27
    NOP count=80
    LOOP jmp=@label_09E3 count=4
    TAUTO interval=0
    NOP count=80
    CALL jmp=@label_0C6B
    NOP count=80
    CALL jmp=@label_0D07
    NOP count=80
    CALL jmp=@label_0D72
    NOP count=80
.org 0x0A18
@label_0A18:
    MOVC dst=1 val=3
.org 0x0A1E
@label_0A1E:
    CALL jmp=@label_0BAC
.org 0x0A23
@label_0A23:
    NOP count=1
    TAMA2
    TDEGR angle=4 dw=0
    LOOP jmp=@label_0A23 count=31
    LXY x=165 y=3
    CALL jmp=@label_0EB5
    NOP count=30
    CALL jmp=@label_0BBE
.org 0x0A43
@label_0A43:
    NOP count=1
    TAMA2
    TDEGR angle=-4 dw=0
    LOOP jmp=@label_0A43 count=31
    LXY x=-165 y=3
    CALL jmp=@label_0EB5
    NOP count=30
    DEC reg=1
    CMPC reg=1 val=0
    JL jmp=@label_0A1E
    NOP count=60
    CALL jmp=@label_0D72
    NOP count=80
    CALL jmp=@label_0D07
    NOP count=80
    CALL jmp=@label_0C6B
    NOP count=80
    JMP jmp=@label_09BF
.org 0x0A8B
@label_0A8B:
    SETUP hp=14250 score=500000
    STI jmp=@label_0AE5 vector=HP val=6000
    STI jmp=@label_0AE5 vector=TIMER val=5200
    LLCLOSE id=255
    TCLR
    TXYR dx=0 dy=0
    NOP count=130
    JMP jmp=@label_0835
.org 0x0AB8
@label_0AB8:
    SETUP hp=14250 score=500000
    STI jmp=@label_0AE5 vector=HP val=6000
    STI jmp=@label_0AE5 vector=TIMER val=5200
    LLCLOSE id=255
    TCLR
    TXYR dx=0 dy=0
    NOP count=130
    JMP jmp=@label_0907
.org 0x0AE5
@label_0AE5:
    SETUP hp=6000 score=500000
    CLI vector=HP
    STI jmp=@label_0B66 vector=TIMER val=3200
    LLCLOSE id=255
    TCLR
    TXYR dx=0 dy=0
    NOP count=130
.org 0x0B05
@label_0B05:
    MOVC dst=1 val=14
.org 0x0B0B
@label_0B0B:
    CALL jmp=@label_0BD0
    TDEGS
    TVDEG vd=4
.org 0x0B13
@label_0B13:
    TAMA
    TDEGR angle=2 dw=0
    NOP count=2
    LOOP jmp=@label_0B13 count=8
    TDEGS
    CALL jmp=@label_0BDF
    MOVR dst=137 src=1
    TAMA
    NOP count=30
    CALL jmp=@label_0BD0
    TDEGS
    TVDEG vd=-4
.org 0x0B36
@label_0B36:
    TAMA
    TDEGR angle=-2 dw=0
    NOP count=2
    LOOP jmp=@label_0B36 count=8
    NOP count=60
    INC reg=1
    INC reg=1
    CMPC reg=1 val=20
    JS jmp=@label_0B0B
    NOP count=60
    CALL jmp=@label_0DE7
    NOP count=80
    JMP jmp=@label_0B05
.org 0x0B66
@label_0B66:
    SETUP hp=0 score=0  ; death marker
    TCLR
.org 0x0B70
@label_0B70:
    NOP count=1000
    JMP jmp=@label_0B70
.org 0x0B78
@script_21:
    TCMD cmd=1
    TDEGA angle=0 dw=0
    TNUMA n=15 ns=0
    TSPDA v=2 a=5
    TREP rep=96
    TCOL color=35
    TTYPE type=1
    RET
.org 0x0B8A
@label_0B8A:
    TCMD cmd=2
    TDEGA angle=64 dw=196
    TNUMA n=10 ns=0
    TSPDA v=214 a=-5
    TREP rep=32
    TCOL color=4
    TTYPE type=1
    RET
.org 0x0B9C
@label_0B9C:
    TCMD cmd=0
    TDEGA angle=0 dw=8
    TNUMA n=2 ns=0
    TSPDA v=22 a=0
    TCOL color=3
    TTYPE type=0
    RET
.org 0x0BAC
@label_0BAC:
    TCMD cmd=0
    TDEGA angle=0 dw=8
    TNUMA n=8 ns=0
    TSPDA v=214 a=-3
    TREP rep=32
    TCOL color=18
    TTYPE type=1
    RET
.org 0x0BBE
@label_0BBE:
    TCMD cmd=0
    TDEGA angle=126 dw=8
    TNUMA n=4 ns=0
    TSPDA v=197 a=3
    TREP rep=96
    TCOL color=17
    TTYPE type=1
    RET
.org 0x0BD0
@label_0BD0:
    TCMD cmd=1
    TTYPE type=4
    TNUMA n=13 ns=0
    TSPDA v=20 a=0
    TCOL color=16
    TREP rep=32
    RET
.org 0x0BDF
@label_0BDF:
    TCMD cmd=5
    TTYPE type=5
    TNUMA n=5 ns=12
    TSPDA v=8 a=-8
    TCOL color=21
    TREP rep=1
    TVDEG vd=8
    RET
.org 0x0BF0
@label_0BF0:
    TCMD cmd=10
    TTYPE type=0
    TNUMA n=4 ns=0
    TDEGA angle=0 dw=64
    TSPDA v=204 a=0
    RET
.org 0x0BFE
@label_0BFE:
    TCMD cmd=2
    TTYPE type=0
    TNUMA n=4 ns=0
    TDEGA angle=64 dw=192
    TSPDA v=72 a=0
    TCOL color=2
    RET
.org 0x0C0E
@label_0C0E:
    TCMD cmd=9
    TTYPE type=1
    TNUMA n=64 ns=0
    TDEGA angle=0 dw=0
    TSPDA v=10 a=4
    TREP rep=64
    TXYR dx=165 dy=3
    TCOL color=17
    TAMA
    NOP count=40
    TXYR dx=-165 dy=3
    TCOL color=18
    TAMA
    NOP count=40
    TXYR dx=165 dy=3
    TCOL color=16
    TAMA
    NOP count=40
    TXYR dx=-165 dy=3
    TCOL color=17
    TAMA
    NOP count=40
    TXYR dx=165 dy=3
    TCOL color=18
    TAMA
    NOP count=40
    TXYR dx=-165 dy=3
    TCOL color=16
    TAMA
    NOP count=40
    TXYR dx=0 dy=0
    TCOL color=20
    TAMA
    NOP count=10
    RET
.org 0x0C6B
@label_0C6B:
    TCMD cmd=2
    TTYPE type=0
    TNUMA n=10 ns=0
    TDEGA angle=0 dw=10
    TSPDA v=8 a=0
    TCOL color=1
    RND reg=1
    MOD reg=1 div=2048
    MOVR dst=134 src=1
    CMPC reg=1 val=1024
    JL jmp=@label_0CB1
.org 0x0C90
@label_0C90:
    TDEGR angle=90 dw=0
    TCOL color=0
    TAMAL
    TDEGR angle=85 dw=0
    TCOL color=1
    TAMAL
    TDEGR angle=85 dw=0
    TCOL color=2
    TAMAL
    NOP count=5
    LOOP jmp=@label_0C90 count=9
    JMP jmp=@label_0CD2
.org 0x0CB1
@label_0CB1:
    TDEGR angle=-90 dw=0
    TCOL color=0
    TAMAL
    TDEGR angle=-85 dw=0
    TCOL color=1
    TAMAL
    TDEGR angle=-85 dw=0
    TCOL color=2
    TAMAL
    NOP count=5
    LOOP jmp=@label_0CB1 count=9
    JMP jmp=@label_0CD2
.org 0x0CD2
@label_0CD2:
    TCMD cmd=5
    TTYPE type=1
    TNUMA n=1 ns=5
    TDEGA angle=0 dw=10
    TSPDA v=18 a=-3
    TREP rep=48
    TCOL color=35
    TDEGS
    TDEGR angle=-64 dw=0
.org 0x0CE7
@label_0CE7:
    TAMA
    TDEGR angle=8 dw=0
    NOP count=2
    LOOP jmp=@label_0CE7 count=15
    TDEGR angle=-4 dw=0
.org 0x0CF8
@label_0CF8:
    TAMA
    TDEGR angle=-8 dw=0
    NOP count=2
    LOOP jmp=@label_0CF8 count=15
    RET
.org 0x0D07
@label_0D07:
    TCMD cmd=2
    TTYPE type=0
    TNUMA n=11 ns=0
    TDEGA angle=0 dw=6
    TSPDA v=11 a=0
    TCOL color=1
    RND reg=1
    MOD reg=1 div=2048
    MOVR dst=134 src=1
    CMPC reg=1 val=1024
    JL jmp=@label_0D4F
.org 0x0D2C
@label_0D2C:
    TDEGR angle=73 dw=0
    TCOL color=0
    TAMAL
    TDEGR angle=64 dw=0
    TCOL color=17
    TAMAL
    TDEGR angle=64 dw=0
    TCOL color=2
    TAMAL
    TDEGR angle=64 dw=0
    TCOL color=21
    TAMAL
    NOP count=6
    LOOP jmp=@label_0D2C count=8
    RET
.org 0x0D4F
@label_0D4F:
    TDEGR angle=-73 dw=0
    TCOL color=16
    TAMAL
    TDEGR angle=-64 dw=0
    TCOL color=1
    TAMAL
    TDEGR angle=-64 dw=0
    TCOL color=18
    TAMAL
    TDEGR angle=-64 dw=0
    TCOL color=5
    TAMAL
    NOP count=6
    LOOP jmp=@label_0D4F count=8
    RET
.org 0x0D72
@label_0D72:
    TCMD cmd=2
    TTYPE type=1
    TNUMA n=10 ns=0
    TDEGA angle=0 dw=10
    TSPDA v=8 a=1
    TREP rep=96
    TCOL color=5
    RND reg=1
    MOD reg=1 div=2048
    MOVR dst=134 src=1
    CMPC reg=1 val=1024
    JL jmp=@label_0DC0
.org 0x0D99
@label_0D99:
    TDEGR angle=-40 dw=0
    TCOL color=5
    TAMAL
    TDEGR angle=85 dw=0
    TAMAL
    TDEGR angle=85 dw=0
    TAMAL
    TDEGR angle=-128 dw=0
    TCOL color=21
    TAMAL
    TDEGR angle=85 dw=0
    TAMAL
    TDEGR angle=85 dw=0
    TAMAL
    NOP count=12
    LOOP jmp=@label_0D99 count=8
    RET
.org 0x0DC0
@label_0DC0:
    TDEGR angle=-46 dw=0
    TCOL color=5
    TAMAL
    TDEGR angle=85 dw=0
    TAMAL
    TDEGR angle=85 dw=0
    TAMAL
    TDEGR angle=-128 dw=0
    TCOL color=21
    TAMAL
    TDEGR angle=85 dw=0
    TAMAL
    TDEGR angle=85 dw=0
    TAMAL
    NOP count=12
    LOOP jmp=@label_0DC0 count=8
    RET
.org 0x0DE7
@label_0DE7:
    TCMD cmd=1
    TTYPE type=0
    TNUMA n=5 ns=0
    TDEGA angle=0 dw=0
    TSPDA v=17 a=0
    TCOL color=17
    TDEGS
    TDEGR angle=-128 dw=0
.org 0x0DFA
@label_0DFA:
    TAMA
    NOP count=2
    LOOP jmp=@label_0DFA count=15
.org 0x0E05
@label_0E05:
    TAMA
    TDEGR angle=2 dw=0
    TAMA
    TDEGR angle=2 dw=0
    NOP count=2
    LOOP jmp=@label_0E05 count=5
.org 0x0E17
@label_0E17:
    TAMA
    NOP count=2
    LOOP jmp=@label_0E17 count=15
.org 0x0E22
@label_0E22:
    TAMA
    TDEGR angle=-3 dw=0
    TAMA
    TDEGR angle=-3 dw=0
    NOP count=2
    LOOP jmp=@label_0E22 count=10
.org 0x0E34
@label_0E34:
    TAMA
    NOP count=2
    LOOP jmp=@label_0E34 count=20
.org 0x0E3F
@label_0E3F:
    TAMA
    TDEGR angle=2 dw=0
    TAMA
    TDEGR angle=2 dw=0
    NOP count=2
    LOOP jmp=@label_0E3F count=15
.org 0x0E51
@label_0E51:
    TAMA
    NOP count=2
    LOOP jmp=@label_0E51 count=25
.org 0x0E5C
@label_0E5C:
    TAMA
    TDEGR angle=-3 dw=0
    TAMA
    TDEGR angle=-3 dw=0
    NOP count=2
    LOOP jmp=@label_0E5C count=20
.org 0x0E6E
@label_0E6E:
    TAMA
    NOP count=2
    LOOP jmp=@label_0E6E count=25
.org 0x0E79
@label_0E79:
    TAMA
    TDEGR angle=3 dw=0
    TAMA
    TDEGR angle=3 dw=0
    NOP count=2
    LOOP jmp=@label_0E79 count=25
    RET
.org 0x0E8C
@label_0E8C:
    LCMD cmd=8
    LLA len=8320
    LDEGA angle=0 dw=18
    LNUMA n=5
    LSPDA v=512
    LCOL color=0
    LTYPE type=1
    LWA w=192
    PSE id=3
    LXY x=60 y=53
    LASER
    LXY x=-60 y=53
    LASER
    RET
.org 0x0EB5
@label_0EB5:
    LCMD cmd=8
    LLA len=11520
    LDEGA angle=0 dw=10
    LNUMA n=3
    LSPDA v=384
    LCOL color=0
    LTYPE type=0
    LWA w=192
    PSE id=3
    LASER
    RET
    LCMD cmd=8
    LLA len=1280
    LDEGA angle=0 dw=12
    LNUMA n=6
    LSPDA v=448
    LCOL color=0
    LTYPE type=0
    LWA w=192
    RET
.org 0x0EEE
@label_0EEE:
    LTYPE type=1
    LCOL color=0
    LDEGA angle=0 dw=12
    LNUMA n=4
    LXY x=0 y=0
    LDEGS
    LDEGR angle=-128 dw=0
    HLASER
    RET
.org 0x0F02
@label_0F02:
    LNUMA n=1
    LSPDA v=64
    LCOL color=3
    LWA w=960
    LTYPE type=3
    LDEGA angle=0 dw=0
    LXY x=165 y=3
    LLSET
    LXY x=-165 y=3
    LLSET
    RET
.org 0x0F22
@label_0F22:
    ENEMYSET dx=0 dy=0 id=22
    RET
.org 0x0F29
@script_22:
    SETUP hp=14999 score=0
    STI jmp=@label_0F64 vector=BOSSLEFT val=0
    ANM pattern=0 speed=0
    DRAW_OFF
    CLIP_ON
    DAMAGE_OFF
    HITSB_OFF
    LNUMA n=1
    LSPDA v=64
    LCOL color=0
    LWA w=960
    LTYPE type=3
    LDEGA angle=0 dw=0
    LXY x=0 y=0
    LLSET
    NOP count=30
    LLOPEN id=255
    NOP count=150
.org 0x0F64
@label_0F64:
    LLCLOSE id=255
    END
.org 0x0F67
@script_23:
    SETUP hp=14999 score=0
    STI jmp=@label_0FF9 vector=BOSSLEFT val=0
    ANM pattern=5 speed=2
    CLIP_ON
    DAMAGE_OFF
    HITSB_OFF
    MOVR dst=0 src=145
    MOVC dst=1 val=20416
    MOVC dst=2 val=15296
    MOVC dst=3 val=0
    MOVC dst=6 val=128
.org 0x0F9B
@label_0F9B:
    NOP count=1
    ADD dst=3 src=6
    MOVR dst=4 src=3
    MOVR dst=5 src=3
    COSL len=4 deg=0
    SINL len=5 deg=0
    ADD dst=4 src=1
    ADD dst=5 src=2
    MOVR dst=143 src=4
    MOVR dst=144 src=5
    CMPC reg=3 val=10240
    JS jmp=@label_0F9B
    ENEMYSETD dx=0 dy=0 reg=0 id=24
    ENEMYSETD dx=0 dy=0 reg=0 id=25
    NOP count=40
    LNUMA n=1
    LSPDA v=64
    LCOL color=4
    LWA w=640
    LTYPE type=2
    LDEGR angle=0 dw=0
    LXY x=0 y=0
    DEGR angle=-128
    CALL jmp=@label_0FFC
    CALL jmp=@label_0FFC
.org 0x0FF9
@label_0FF9:
    LLCLOSE id=255
    END
.org 0x0FFC
@label_0FFC:
    NOP count=30
    LLSET
    NOP count=1
    DEGR angle=1
    INC reg=0
    MOVR dst=4 src=3
    MOVR dst=5 src=3
    COSL len=4 deg=0
    SINL len=5 deg=0
    ADD dst=4 src=1
    ADD dst=5 src=2
    MOVR dst=143 src=4
    MOVR dst=144 src=5
    LOOP jmp=0x1000 count=14
    NOP count=60
    LLOPEN id=255
    NOP count=100
    LLCLOSE id=255
    RET
.org 0x1031
@script_24:
    MOVC dst=6 val=1
    JMP jmp=@script_26
.org 0x103C
@script_25:
    MOVC dst=6 val=4294967295
    JMP jmp=@script_26
.org 0x1047
@script_26:
    SETUP hp=14999 score=0
    STI jmp=@label_10BE vector=BOSSLEFT val=0
    ANM pattern=5 speed=2
    CLIP_ON
    DAMAGE_OFF
    HITSB_OFF
    MOVR dst=0 src=145
    MOVC dst=1 val=20416
    MOVC dst=2 val=15296
    MOVC dst=3 val=10240
.org 0x1075
@label_1075:
    NOP count=1
    ADD dst=0 src=6
    MOVR dst=4 src=3
    MOVR dst=5 src=3
    COSL len=4 deg=0
    SINL len=5 deg=0
    ADD dst=4 src=1
    ADD dst=5 src=2
    MOVR dst=143 src=4
    MOVR dst=144 src=5
    LOOP jmp=@label_1075 count=39
    LNUMA n=1
    LSPDA v=64
    LCOL color=4
    LWA w=640
    LTYPE type=2
    LDEGR angle=0 dw=0
    LXY x=0 y=0
    DEGR angle=-128
    CALL jmp=@label_10C1
    CALL jmp=@label_10C1
.org 0x10BE
@label_10BE:
    LLCLOSE id=255
    END
.org 0x10C1
@label_10C1:
    NOP count=30
    LLSET
    NOP count=1
    DEGR angle=1
    INC reg=0
    MOVR dst=4 src=3
    MOVR dst=5 src=3
    COSL len=4 deg=0
    SINL len=5 deg=0
    ADD dst=4 src=1
    ADD dst=5 src=2
    MOVR dst=143 src=4
    MOVR dst=144 src=5
    LOOP jmp=0x10C5 count=14
    NOP count=60
    LLOPEN id=255
    NOP count=100
    LLCLOSE id=255
    RET
.org 0x10F6
@script_27:
    SETUP hp=14999 score=0
    STI jmp=@label_112B vector=BOSSLEFT val=0
    ANM pattern=0 speed=0
    DRAW_OFF
    CLIP_ON
    DAMAGE_OFF
    HITSB_OFF
    TCMD cmd=0
    TDEGA angle=0 dw=12
    TNUMA n=2 ns=0
    TSPDA v=22 a=0
    TCOL color=33
    TTYPE type=0
    TDEGS
.org 0x1120
@label_1120:
    NOP count=2
    TAMA
    LOOP jmp=@label_1120 count=20
.org 0x112B
@label_112B:
    END
.org 0x112C
@script_28:
    SETUP hp=14999 score=0
    STI jmp=@label_11D4 vector=BOSSLEFT val=0
    ANM pattern=0 speed=0
    CLIP_ON
    DAMAGE_OFF
    HITSB_OFF
    MOVC dst=4 val=0
    MOVC dst=6 val=128
.org 0x1151
@label_1151:
    NOP count=1
    ADD dst=4 src=6
    MOVR dst=0 src=145
    MOVC dst=1 val=20416
    MOVC dst=2 val=15296
    MOVC dst=5 val=128
    MOVC dst=3 val=10240
    COSL len=3 deg=0
    ADD dst=1 src=3
    MOVC dst=3 val=10240
    SINL len=3 deg=0
    ADD dst=2 src=3
    DEGR angle=64
    MOVR dst=0 src=145
    DEGR angle=-64
    MOVR dst=3 src=4
    COSL len=3 deg=0
    ADD dst=1 src=3
    MOVR dst=143 src=1
    MOVR dst=3 src=4
    SINL len=3 deg=0
    ADD dst=2 src=3
    MOVR dst=144 src=2
    CMPC reg=4 val=3200
    JS jmp=@label_1151
    NOP count=120
    LNUMA n=1
    LSPDA v=64
    LCOL color=4
    LWA w=640
    LTYPE type=2
    LDEGR angle=0 dw=0
    LXY x=0 y=0
    DEGR angle=-128
    LLSET
    NOP count=60
    LLOPEN id=255
    NOP count=100
.org 0x11D4
@label_11D4:
    LLCLOSE id=255
    END
