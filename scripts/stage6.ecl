.header 30
.offset 0 0x007C
.offset 1 0x02D4
.offset 2 0x03AA
.offset 3 0x0515
.offset 4 0x0631
.offset 5 0x071B
.offset 6 0x0794
.offset 7 0x0807
.offset 8 0x0807
.offset 9 0x0807
.offset 10 0x0AB5
.offset 11 0x0ED8
.offset 12 0x0F44
.offset 13 0x0F91
.offset 14 0x103B
.offset 15 0x1093
.offset 16 0x1182
.offset 17 0x12FF
.offset 18 0x1D50
.offset 19 0x1DA0
.offset 20 0x1E05
.offset 21 0x007C
.offset 22 0x007C
.offset 23 0x007C
.offset 24 0x007C
.offset 25 0x007C
.offset 26 0x007C
.offset 27 0x007C
.offset 28 0x007C
.offset 29 0x007C

.org 0x007C
@script_0:
@script_21:  ; shared
@script_22:  ; shared
@script_23:  ; shared
@script_24:  ; shared
@script_25:  ; shared
@script_26:  ; shared
@script_27:  ; shared
@script_28:  ; shared
@script_29:  ; shared
    SETUP hp=18000 score=1000000
    STI jmp=@label_02BB vector=HP val=0
    STI jmp=@label_02BB vector=TIMER val=5600
    CLIP_ON
    DAMAGE_OFF
    HITSB_OFF
    ANM pattern=4 speed=0
    SPDA speed=128
    MOV count=180
    HITXY w=70 h=20
    DAMAGE_ON
    RND reg=6
    MOD reg=6 div=8
    RND reg=7
    MOD reg=7 div=3
.org 0x00BD
@label_00BD:
    CALL jmp=@label_0175
    INC reg=7
    MOD reg=7 div=3
    CMPC reg=0 val=0
    JEQ jmp=@label_00F5
    CMPC reg=0 val=1
    JEQ jmp=@label_010A
    CMPC reg=0 val=2
    JEQ jmp=@label_011F
    JMP jmp=@label_0148
    JMP jmp=@label_00BD
.org 0x00F5
@label_00F5:
    CALL jmp=@script_1
    NOP count=90
    CALL jmp=@label_0337
    NOP count=90
    JMP jmp=@label_00BD
.org 0x010A
@label_010A:
    CALL jmp=@script_2
    NOP count=90
    CALL jmp=@label_0438
    NOP count=90
    JMP jmp=@label_00BD
.org 0x011F
@label_011F:
    CALL jmp=@script_3
    CALL jmp=@label_055E
    NOP count=20
    LOOP jmp=@label_011F count=5
    CALL jmp=@label_05D5
    NOP count=90
    CALL jmp=@label_059E
    NOP count=90
    JMP jmp=@label_00BD
.org 0x0148
@label_0148:
    CALL jmp=@script_4
    NOP count=270
    CALL jmp=@label_068A
    NOP count=90
    CALL jmp=@label_068A
    NOP count=80
    CALL jmp=@label_068A
    NOP count=70
    CALL jmp=@label_068A
    NOP count=270
    JMP jmp=@label_00BD
.org 0x0175
@label_0175:
    CMPC reg=6 val=0
    JEQ jmp=@label_01C7
    CMPC reg=6 val=1
    JEQ jmp=@label_01E2
    CMPC reg=6 val=2
    JEQ jmp=@label_01FD
    CMPC reg=6 val=3
    JEQ jmp=@label_0218
    CMPC reg=6 val=4
    JEQ jmp=@label_0233
    CMPC reg=6 val=5
    JEQ jmp=@label_024E
    CMPC reg=6 val=6
    JEQ jmp=@label_0269
    JMP jmp=@label_0284
.org 0x01C7
@label_01C7:
    CMPC reg=7 val=0
    JEQ jmp=@label_029F
    CMPC reg=7 val=1
    JEQ jmp=@label_02A6
    JMP jmp=@label_02AD
.org 0x01E2
@label_01E2:
    CMPC reg=7 val=0
    JEQ jmp=@label_029F
    CMPC reg=7 val=1
    JEQ jmp=@label_02A6
    JMP jmp=@label_02B4
.org 0x01FD
@label_01FD:
    CMPC reg=7 val=0
    JEQ jmp=@label_029F
    CMPC reg=7 val=1
    JEQ jmp=@label_02AD
    JMP jmp=@label_02A6
.org 0x0218
@label_0218:
    CMPC reg=7 val=0
    JEQ jmp=@label_029F
    CMPC reg=7 val=1
    JEQ jmp=@label_02AD
    JMP jmp=@label_02B4
.org 0x0233
@label_0233:
    CMPC reg=7 val=0
    JEQ jmp=@label_029F
    CMPC reg=7 val=1
    JEQ jmp=@label_02B4
    JMP jmp=@label_02A6
.org 0x024E
@label_024E:
    CMPC reg=7 val=0
    JEQ jmp=@label_029F
    CMPC reg=7 val=1
    JEQ jmp=@label_02B4
    JMP jmp=@label_02AD
.org 0x0269
@label_0269:
    CMPC reg=7 val=0
    JEQ jmp=@label_02A6
    CMPC reg=7 val=1
    JEQ jmp=@label_02AD
    JMP jmp=@label_02B4
.org 0x0284
@label_0284:
    CMPC reg=7 val=0
    JEQ jmp=@label_02A6
    CMPC reg=7 val=1
    JEQ jmp=@label_02B4
    JMP jmp=@label_02AD
.org 0x029F
@label_029F:
    MOVC dst=0 val=0
    RET
.org 0x02A6
@label_02A6:
    MOVC dst=0 val=1
    RET
.org 0x02AD
@label_02AD:
    MOVC dst=0 val=2
    RET
.org 0x02B4
@label_02B4:
    MOVC dst=0 val=3
    RET
.org 0x02BB
@label_02BB:
    CLI vector=HP
    CLI vector=TIMER
    LLCLOSE id=255
    TCLR
    TXYR dx=0 dy=0
    NOP count=130
    SETUP hp=0 score=0  ; death marker
    END
.org 0x02D4
@script_1:
    LCMD cmd=0
    LLA len=7040
    LDEGA angle=0 dw=28
    LNUMA n=2
    LSPDA v=384
    LCOL color=0
    LTYPE type=0
    TOPT opt=0
    LWA w=192
    LDEGS
    MOVR dst=1 src=128
    MOVC dst=2 val=80
    MOVC dst=3 val=8
.org 0x0300
@label_0300:
    PSE id=3
    MOVR dst=4 src=1
    ADD dst=4 src=2
    MOVR dst=128 src=4
    LXY x=-98 y=-34
    LASER
    MOVR dst=4 src=1
    SUB dst=4 src=2
    MOVR dst=128 src=4
    LXY x=98 y=-34
    LASER
    NOP count=4
    SUB dst=2 src=3
    CMPC reg=2 val=4294967216
    JL jmp=@label_0300
    LXY x=0 y=0
    RET
.org 0x0337
@label_0337:
    TCMD cmd=1
    TDEGA angle=0 dw=0
    TNUMA n=8 ns=0
    TSPDA v=9 a=0
    TTYPE type=0
    TOPT opt=0
    MOVC dst=1 val=4
.org 0x034C
@label_034C:
    MOVC dst=2 val=10
.org 0x0352
@label_0352:
    NOP count=3
    TCOL color=1
    TAMA
    TDEGR angle=3 dw=0
    NOP count=3
    TCOL color=17
    TAMA
    TDEGR angle=3 dw=0
    DEC reg=2
    CMPC reg=2 val=0
    JL jmp=@label_0352
    TSPDR v=2 a=0
    MOVC dst=2 val=10
.org 0x037A
@label_037A:
    NOP count=3
    TCOL color=1
    TAMA
    TDEGR angle=-3 dw=0
    NOP count=3
    TCOL color=17
    TAMA
    TDEGR angle=-3 dw=0
    DEC reg=2
    CMPC reg=2 val=0
    JL jmp=@label_037A
    DEC reg=1
    CMPC reg=1 val=0
    TSPDR v=2 a=0
    JL jmp=@label_034C
    RET
.org 0x03AA
@script_2:
    LNUMA n=1
    LSPDA v=64
    LCOL color=0
    LWA w=960
    LTYPE type=0
    TOPT opt=0
    LXY x=0 y=0
    RND reg=1
    MOD reg=1 div=65536
    CMPC reg=1 val=32768
    JL jmp=@label_0406
    MOVC dst=1 val=0
    ENEMYSETD dx=0 dy=0 reg=1 id=5
    MOVC dst=1 val=80
    LDEGA angle=0 dw=0
    LLSET
    NOP count=40
    LLOPEN id=255
.org 0x03F0
@label_03F0:
    NOP count=2
    LLDEGR id=0 deg=1
    DEC reg=1
    CMPC reg=1 val=0
    JL jmp=@label_03F0
    LLCLOSE id=255
    RET
.org 0x0406
@label_0406:
    MOVC dst=1 val=128
    ENEMYSETD dx=0 dy=0 reg=1 id=5
    MOVC dst=1 val=80
    LDEGA angle=128 dw=0
    LLSET
    NOP count=40
    LLOPEN id=255
.org 0x0422
@label_0422:
    NOP count=2
    LLDEGR id=0 deg=-1
    DEC reg=1
    CMPC reg=1 val=0
    JL jmp=@label_0422
    LLCLOSE id=255
    RET
.org 0x0438
@label_0438:
    TCMD cmd=0
    TDEGA angle=0 dw=8
    TNUMA n=3 ns=0
    TSPDA v=24 a=0
    TTYPE type=0
    TOPT opt=0
    TCOL color=17
    ENEMYSET dx=0 dy=0 id=6
    NOP count=50
    TDEGS
    MOVR dst=1 src=134
    MOVC dst=2 val=40
    MOVC dst=3 val=10
.org 0x0462
@label_0462:
    MOVR dst=4 src=1
    ADD dst=4 src=2
    MOVR dst=134 src=4
    TAMA
    MOVR dst=4 src=1
    SUB dst=4 src=2
    MOVR dst=134 src=4
    TAMA
    DEC reg=2
    DEC reg=2
    NOP count=2
    DEC reg=3
    CMPC reg=3 val=0
    JL jmp=@label_0462
    MOVC dst=3 val=30
.org 0x0490
@label_0490:
    MOVR dst=4 src=1
    ADD dst=4 src=2
    MOVR dst=134 src=4
    TAMA
    MOVR dst=4 src=1
    SUB dst=4 src=2
    MOVR dst=134 src=4
    TAMA
    INC reg=2
    INC reg=2
    NOP count=2
    DEC reg=3
    CMPC reg=3 val=0
    JL jmp=@label_0490
    MOVC dst=3 val=30
.org 0x04BE
@label_04BE:
    MOVR dst=4 src=1
    ADD dst=4 src=2
    MOVR dst=134 src=4
    TAMA
    MOVR dst=4 src=1
    SUB dst=4 src=2
    MOVR dst=134 src=4
    TAMA
    DEC reg=2
    DEC reg=2
    NOP count=2
    DEC reg=3
    CMPC reg=3 val=0
    JL jmp=@label_04BE
    MOVC dst=3 val=30
.org 0x04EC
@label_04EC:
    MOVR dst=4 src=1
    ADD dst=4 src=2
    MOVR dst=134 src=4
    TAMA
    MOVR dst=4 src=1
    SUB dst=4 src=2
    MOVR dst=134 src=4
    TAMA
    INC reg=2
    INC reg=2
    NOP count=2
    DEC reg=3
    CMPC reg=3 val=0
    JL jmp=@label_04EC
    RET
.org 0x0515
@script_3:
    TCMD cmd=1
    TDEGA angle=0 dw=0
    TNUMA n=8 ns=0
    TSPDA v=12 a=0
    TCOL color=0
    TTYPE type=0
    TOPT opt=0
    RND reg=1
    MOD reg=1 div=256
    MOVR dst=134 src=1
    MOVC dst=2 val=0
.org 0x0537
@label_0537:
    MOVR dst=3 src=1
    ADD dst=3 src=2
    MOVR dst=134 src=3
    TAMA
    MOVR dst=3 src=1
    SUB dst=3 src=2
    MOVR dst=134 src=3
    TAMA
    INC reg=2
    INC reg=2
    NOP count=3
    CMPC reg=2 val=28
    JS jmp=@label_0537
    RET
.org 0x055E
@label_055E:
    TCMD cmd=1
    TDEGA angle=0 dw=0
    TNUMA n=4 ns=0
    TSPDA v=18 a=0
    TCOL color=17
    TTYPE type=0
    TOPT opt=0
    TDEGS
    MOVR dst=1 src=134
    MOVC dst=2 val=0
.org 0x0579
@label_0579:
    MOVR dst=3 src=1
    ADD dst=3 src=2
    MOVR dst=134 src=3
    TAMA
    MOVR dst=3 src=1
    SUB dst=3 src=2
    MOVR dst=134 src=3
    TAMA
    INC reg=2
    NOP count=3
    CMPC reg=2 val=10
    JS jmp=@label_0579
    RET
.org 0x059E
@label_059E:
    TCMD cmd=5
    TDEGA angle=0 dw=0
    TNUMA n=6 ns=6
    TSPDA v=16 a=0
    TCOL color=33
    TTYPE type=4
    TOPT opt=0
    TREP rep=40
    RND reg=1
    MOD reg=1 div=256
    MOVR dst=134 src=1
    TAUTO interval=20
    TVDEG vd=-3
    NOP count=100
    TVDEG vd=3
    NOP count=100
    TVDEG vd=-3
    NOP count=100
    TVDEG vd=3
    NOP count=100
    TAUTO interval=0
    RET
.org 0x05D5
@label_05D5:
    TCMD cmd=1
    TNUMA n=4 ns=0
    TSPDA v=16 a=0
    TCOL color=5
    TTYPE type=0
    TOPT opt=0
    MOVC dst=1 val=64
    MOVC dst=2 val=0
    MOVC dst=4 val=9
.org 0x05F5
@label_05F5:
    MOVR dst=3 src=1
    ADD dst=3 src=2
    MOVR dst=134 src=3
    TXYR dx=98 dy=-34
    TAMA
    MOVR dst=3 src=1
    SUB dst=3 src=2
    MOVR dst=134 src=3
    TXYR dx=-98 dy=-34
    TAMA
    NOP count=2
    LOOP jmp=@label_05F5 count=4
    ADD dst=2 src=4
    CMPC reg=2 val=512
    JS jmp=@label_05F5
    TXYR dx=0 dy=0
    RET
.org 0x0631
@script_4:
    TCMD cmd=1
    TSPDA v=19 a=-4
    TREP rep=40
    TCOL color=5
    TTYPE type=1
    TOPT opt=96
    MOVC dst=1 val=40
    JDIF easy=@label_0655 norm=@label_065D hard=@label_0665 luna=@label_066D
.org 0x0655
@label_0655:
    TNUMA n=3 ns=0
    JMP jmp=@label_0675
.org 0x065D
@label_065D:
    TNUMA n=4 ns=0
    JMP jmp=@label_0675
.org 0x0665
@label_0665:
    TNUMA n=5 ns=0
    JMP jmp=@label_0675
.org 0x066D
@label_066D:
    TNUMA n=6 ns=0
    JMP jmp=@label_0675
.org 0x0675
@label_0675:
    TAMA2
    TDEGR angle=4 dw=0
    NOP count=3
    DEC reg=1
    CMPC reg=1 val=0
    JL jmp=@label_0675
    RET
.org 0x068A
@label_068A:
    TCMD cmd=1
    TSPDA v=19 a=-1
    TREP rep=120
    TTYPE type=1
    TOPT opt=80
    JDIF easy=@label_06A6 norm=@label_06AE hard=@label_06B6 luna=@label_06BE
.org 0x06A6
@label_06A6:
    TNUMA n=6 ns=0
    JMP jmp=@label_06C6
.org 0x06AE
@label_06AE:
    TNUMA n=8 ns=0
    JMP jmp=@label_06C6
.org 0x06B6
@label_06B6:
    TNUMA n=10 ns=0
    JMP jmp=@label_06C6
.org 0x06BE
@label_06BE:
    TNUMA n=12 ns=0
    JMP jmp=@label_06C6
.org 0x06C6
@label_06C6:
    TCOL color=32
    MOVC dst=1 val=6
.org 0x06CE
@label_06CE:
    TAMA2
    TDEGR angle=-2 dw=0
    NOP count=1
    DEC reg=1
    CMPC reg=1 val=0
    JL jmp=@label_06CE
    TCOL color=33
    MOVC dst=1 val=6
.org 0x06EA
@label_06EA:
    TAMA2
    TDEGR angle=-2 dw=0
    NOP count=1
    DEC reg=1
    CMPC reg=1 val=0
    JL jmp=@label_06EA
    TCOL color=34
    MOVC dst=1 val=6
.org 0x0706
@label_0706:
    TAMA2
    TDEGR angle=-2 dw=0
    NOP count=1
    DEC reg=1
    CMPC reg=1 val=0
    JL jmp=@label_0706
    RET
.org 0x071B
@script_5:
    SETUP hp=14999 score=0
    STI jmp=@label_0791 vector=BOSSLEFT val=0
    ANM pattern=0 speed=0
    DRAW_OFF
    CLIP_ON
    DAMAGE_OFF
    HITSB_OFF
    TCMD cmd=1
    TDEGA angle=0 dw=0
    TNUMA n=18 ns=0
    TSPDA v=12 a=0
    TTYPE type=4
    TREP rep=60
    TCOL color=21
    MOVR dst=0 src=145
    CMPC reg=0 val=64
    JL jmp=@label_0775
    TVDEG vd=-2
    MOVC dst=1 val=28
.org 0x075C
@label_075C:
    TAMA
    TDEGR angle=1 dw=0
    NOP count=4
    DEC reg=1
    CMPC reg=1 val=0
    JL jmp=@label_075C
    JMP jmp=@label_0791
.org 0x0775
@label_0775:
    TVDEG vd=2
    MOVC dst=1 val=20
.org 0x077D
@label_077D:
    TAMA
    TDEGR angle=-1 dw=0
    NOP count=4
    DEC reg=1
    CMPC reg=1 val=0
    JL jmp=@label_077D
.org 0x0791
@label_0791:
    CLI vector=BOSSLEFT
    END
.org 0x0794
@script_6:
    SETUP hp=14999 score=0
    STI jmp=@label_0804 vector=BOSSLEFT val=0
    ANM pattern=0 speed=0
    DRAW_OFF
    CLIP_ON
    DAMAGE_OFF
    HITSB_OFF
    TCMD cmd=9
    TDEGA angle=0 dw=0
    TNUMA n=16 ns=0
    TSPDA v=19 a=0
    TTYPE type=8
    TREP rep=2
    TCOL color=3
.org 0x07BF
@label_07BF:
    JDIF easy=@label_07D0 norm=@label_07D0 hard=@label_07E5 luna=@label_07E5
.org 0x07D0
@label_07D0:
    TDEGS
    MOVR dst=142 src=134
    TAMA2
    NOP count=16
    TDEGS
    MOVR dst=142 src=134
    TAMA2
    NOP count=16
    JMP jmp=@label_07FD
.org 0x07E5
@label_07E5:
    TDEGS
    MOVR dst=142 src=134
    TAMA2
    NOP count=10
    TDEGS
    MOVR dst=142 src=134
    TAMA2
    NOP count=10
    TDEGS
    MOVR dst=142 src=134
    TAMA2
    NOP count=12
.org 0x07FD
@label_07FD:
    LOOP jmp=@label_07BF count=7
.org 0x0804
@label_0804:
    CLI vector=BOSSLEFT
    END
.org 0x0807
@script_7:
@script_8:  ; shared
@script_9:  ; shared
    SETUP hp=12249 score=1000000
    STI jmp=@label_08DB vector=HP val=9249
    STI jmp=@label_08DB vector=TIMER val=5000
    CLIP_ON
    HITSB_OFF
    ANM pattern=2 speed=2
    INT id=1
    MXYA x=319 y=159 count=60
    ANM pattern=0 speed=6
    NOP count=20
.org 0x0838
@label_0838:
    SPDA speed=128
    MOVC dst=1 val=128
.org 0x0843
@label_0843:
    RND reg=2
    DEGXU
    NOP count=30
    CALL jmp=@label_0DA2
    ANM pattern=3 speed=5
    MOV count=30
    ENEMYSETD dx=0 dy=-30 reg=2 id=14
    NOP count=30
    ADD dst=2 src=1
    DEGXD
    NOP count=30
    ANM pattern=3 speed=5
    MOV count=30
    ENEMYSETD dx=0 dy=-30 reg=2 id=14
    NOP count=30
    LOOP jmp=@label_0843 count=1
    ANM pattern=1 speed=6
    MXYA x=319 y=50 count=60
    ANM pattern=2 speed=2
.org 0x0889
@label_0889:
    DEGS
    MOVR dst=1 src=145
    ENEMYSETD dx=0 dy=-40 reg=1 id=13
    NOP count=20
    DEGS
    MOVR dst=1 src=145
    ENEMYSETD dx=0 dy=-40 reg=1 id=13
    NOP count=20
    LOOP jmp=@label_0889 count=1
    CALL jmp=@label_0CF6
    NOP count=100
    MXYA x=319 y=159 count=60
    CALL jmp=@label_0DC1
    NOP count=60
    MXYA x=319 y=99 count=60
    CALL jmp=@label_0DF4
    MXYA x=319 y=159 count=60
    JMP jmp=@label_0838
.org 0x08DB
@label_08DB:
    SETUP hp=9249 score=1000000
    STI jmp=@label_0A71 vector=HP val=3249
    STI jmp=@label_0A71 vector=TIMER val=6000
    LLCLOSE id=255
    TCLR
    TXYR dx=0 dy=0
    RND reg=1
    MOD reg=1 div=65536
    CMPC reg=1 val=32768
    JL jmp=@label_092A
    MXYA x=319 y=120 count=120
.org 0x091A
@label_091A:
    CALL jmp=@label_0C2B
    NOP count=60
    CALL jmp=@script_10
    NOP count=80
.org 0x092A
@label_092A:
    ANM pattern=1 speed=6
    MXYA x=319 y=169 count=120
    RND reg=1
    MOD reg=1 div=65536
    CMPC reg=1 val=32768
    JL jmp=@label_09BE
    CALL jmp=@label_0BA8
    DEGA angle=0
    ANM pattern=5 speed=4
    WAVX vx=256 amp=80 vd=-3 count=40
    CALL jmp=@label_0BA8
    DEGA angle=0
    ANM pattern=5 speed=4
    WAVX vx=-384 amp=80 vd=-3 count=40
    CALL jmp=@label_0BA8
    DEGA angle=0
    ANM pattern=5 speed=4
    WAVX vx=-128 amp=40 vd=-3 count=40
    CALL jmp=@label_0BA8
    DEGA angle=0
    ANM pattern=5 speed=4
    WAVX vx=384 amp=60 vd=-3 count=40
    CALL jmp=@label_0BA8
    DEGA angle=0
    ANM pattern=5 speed=3
    WAVX vx=128 amp=30 vd=-4 count=30
    CALL jmp=@label_0BA8
    DEGA angle=0
    ANM pattern=5 speed=4
    WAVX vx=-256 amp=80 vd=-3 count=40
    JMP jmp=@label_0A30
.org 0x09BE
@label_09BE:
    CALL jmp=@label_0BA8
    DEGA angle=0
    ANM pattern=5 speed=4
    WAVX vx=-256 amp=80 vd=-3 count=40
    CALL jmp=@label_0BA8
    DEGA angle=0
    ANM pattern=5 speed=4
    WAVX vx=384 amp=80 vd=-3 count=40
    CALL jmp=@label_0BA8
    DEGA angle=0
    ANM pattern=5 speed=4
    WAVX vx=128 amp=40 vd=-3 count=40
    CALL jmp=@label_0BA8
    DEGA angle=0
    ANM pattern=5 speed=4
    WAVX vx=-384 amp=60 vd=-3 count=40
    CALL jmp=@label_0BA8
    DEGA angle=0
    ANM pattern=5 speed=3
    WAVX vx=-128 amp=30 vd=-4 count=30
    CALL jmp=@label_0BA8
    DEGA angle=0
    ANM pattern=5 speed=4
    WAVX vx=256 amp=80 vd=-3 count=40
.org 0x0A30
@label_0A30:
    CALL jmp=@label_0BA8
    DEGS
    SPDA speed=192
    MOV count=60
    CALL jmp=@label_0BA8
    CALL jmp=@label_0BA8
    DEGR angle=-128
    SPDA speed=256
    ANM pattern=0 speed=6
    MOV count=40
    CALL jmp=@label_0BA8
    CALL jmp=@label_0BA8
    CALL jmp=@label_0BA8
    CALL jmp=@label_0BA8
    NOP count=40
    JMP jmp=@label_091A
.org 0x0A71
@label_0A71:
    SETUP hp=3249 score=1000000
    CLI vector=HP
    STI jmp=@label_0AA2 vector=TIMER val=3600
    LLCLOSE id=255
    TCLR
    TXYR dx=0 dy=0
    MXYA x=319 y=120 count=120
.org 0x0A95
@label_0A95:
    CALL jmp=@label_0E5D
    NOP count=100
    JMP jmp=@label_0A95
.org 0x0AA2
@label_0AA2:
    SETUP hp=0 score=0  ; death marker
    CLI vector=TIMER
    NOP count=1000
    JMP jmp=@label_0AA2
.org 0x0AB5
@script_10:
    TSPDA v=11 a=0
    TTYPE type=0
    TOPT opt=0
    JDIF easy=@label_0ACD norm=@label_0AD8 hard=@label_0AE3 luna=@label_0AEE
.org 0x0ACD
@label_0ACD:
    TDEGA angle=0 dw=10
    TNUMA n=5 ns=0
    JMP jmp=@label_0AF9
.org 0x0AD8
@label_0AD8:
    TDEGA angle=0 dw=8
    TNUMA n=7 ns=0
    JMP jmp=@label_0AF9
.org 0x0AE3
@label_0AE3:
    TDEGA angle=0 dw=5
    TNUMA n=10 ns=0
    JMP jmp=@label_0AF9
.org 0x0AEE
@label_0AEE:
    TDEGA angle=0 dw=4
    TNUMA n=13 ns=0
    JMP jmp=@label_0AF9
.org 0x0AF9
@label_0AF9:
    TDEGS
    LTYPE type=1
    LCOL color=0
    LNUMA n=1
    LXY x=0 y=0
    MOVC dst=1 val=5
    RND reg=2
    MOD reg=2 div=256
    CMPC reg=2 val=128
    JL jmp=@label_0B66
    TDEGR angle=-90 dw=0
.org 0x0B21
@label_0B21:
    TCOL color=21
    TAMAL
    TDEGR angle=50 dw=0
    TCOL color=20
    TAMAL
    TDEGR angle=50 dw=0
    TCOL color=21
    TAMAL
    TDEGR angle=-96 dw=0
    LDEGS
    LDEGR angle=-108 dw=0
    HLASER
    NOP count=16
    TCOL color=21
    TAMAL
    TDEGR angle=50 dw=0
    TCOL color=20
    TAMAL
    TDEGR angle=50 dw=0
    TCOL color=21
    TAMAL
    TDEGR angle=-96 dw=0
    LDEGS
    LDEGR angle=108 dw=0
    HLASER
    NOP count=16
    DEC reg=1
    CMPC reg=1 val=0
    JL jmp=@label_0B21
    RET
    TDEGR angle=90 dw=0
.org 0x0B66
@label_0B66:
    TCOL color=21
    TAMAL
    TDEGR angle=50 dw=0
    TCOL color=20
    TAMAL
    TDEGR angle=50 dw=0
    TCOL color=21
    TAMAL
    TDEGR angle=-104 dw=0
    LDEGS
    LDEGR angle=108 dw=0
    HLASER
    NOP count=16
    TCOL color=21
    TAMAL
    TDEGR angle=50 dw=0
    TCOL color=20
    TAMAL
    TDEGR angle=50 dw=0
    TCOL color=21
    TAMAL
    TDEGR angle=-104 dw=0
    LDEGS
    LDEGR angle=-108 dw=0
    HLASER
    NOP count=16
    DEC reg=1
    CMPC reg=1 val=0
    JL jmp=@label_0B66
    RET
.org 0x0BA8
@label_0BA8:
    TCMD cmd=0
    TNUMA n=4 ns=0
    TDEGA angle=0 dw=128
    TSPDA v=30 a=0
    TTYPE type=0
    TOPT opt=0
    TCOL color=34
    TOPT opt=0
    LCMD cmd=8
    LLA len=3840
    LDEGA angle=0 dw=10
    LNUMA n=5
    LSPDA v=384
    LCOL color=0
    LTYPE type=0
    TOPT opt=0
    LWA w=192
    MOVC dst=2 val=50
    RND reg=1
    MOD reg=1 div=28
    ADD dst=1 src=2
    MOVR dst=134 src=1
    MOVC dst=1 val=12
.org 0x0BF1
@label_0BF1:
    NOP count=1
    TAMA2
    TDEGR angle=0 dw=-2
    TAMA2
    TDEGR angle=0 dw=-2
    DEC reg=1
    CMPC reg=1 val=0
    JL jmp=@label_0BF1
    PSE id=3
    LASER
    MOVC dst=1 val=12
.org 0x0C12
@label_0C12:
    NOP count=1
    TAMA2
    TDEGR angle=0 dw=-2
    TAMA2
    TDEGR angle=0 dw=-2
    DEC reg=1
    CMPC reg=1 val=0
    JL jmp=@label_0C12
    RET
.org 0x0C2B
@label_0C2B:
    MXYA x=319 y=100 count=60
    DEGA angle=77
    TCMD cmd=14
    TSPDA v=198 a=2
    TTYPE type=7
    TCOL color=1
    TOPT opt=0
    TNUMA n=2 ns=6
    TAUTO interval=20
    TDEGA angle=128 dw=100
    TXYR dx=0 dy=-32
    MOVC dst=2 val=6
.org 0x0C52
@label_0C52:
    SPDA speed=448
    MOVC dst=1 val=50
.org 0x0C5D
@label_0C5D:
    MOV count=1
    SPDR speed=-8
    DEC reg=1
    CMPC reg=1 val=0
    JL jmp=@label_0C5D
    ENEMYSETD dx=0 dy=0 reg=2 id=11
    DEC reg=2
    DEGR angle=-102
    LOOP jmp=@label_0C52 count=4
    TAUTO interval=0
    TXYR dx=0 dy=0
    ANM pattern=1 speed=7
    MXYA x=319 y=220 count=50
    ANM pattern=2 speed=2
    NOP count=180
    ANM pattern=0 speed=8
    TCMD cmd=9
    TSPDA v=10 a=6
    TREP rep=60
    TTYPE type=1
    TCOL color=3
    TOPT opt=0
    TNUMA n=64 ns=0
    TDEGA angle=0 dw=0
    RND reg=2
    MOD reg=2 div=65536
    CMPC reg=2 val=32768
    JL jmp=@label_0CD0
    MXYA x=178 y=119 count=60
    JMP jmp=@label_0CD7
.org 0x0CD0
@label_0CD0:
    MXYA x=461 y=119 count=60
.org 0x0CD7
@label_0CD7:
    ANM pattern=3 speed=4
    NOP count=20
    TAMA
    NOP count=30
    ANM pattern=3 speed=4
    NOP count=20
    TAMA
    NOP count=30
    ANM pattern=3 speed=4
    NOP count=20
    TAMA
    NOP count=30
    RET
.org 0x0CF6
@label_0CF6:
    SPDA speed=256
    TCMD cmd=9
    TSPDA v=201 a=2
    TTYPE type=0
    TCOL color=17
    TOPT opt=0
    TNUMA n=30 ns=0
    TDEGA angle=0 dw=0
    TXYR dx=0 dy=-32
    RND reg=2
    MOD reg=2 div=65536
    CMPC reg=2 val=32768
    JL jmp=@label_0D5F
    MXYA x=491 y=70 count=120
    ANM pattern=0 speed=12
    DEGA angle=96
    MOVR dst=1 src=145
    MOVC dst=2 val=4294967232
    ADD dst=1 src=2
    MOVC dst=2 val=3
    TAUTO interval=10
.org 0x0D44
@label_0D44:
    MOV count=6
    ENEMYSETD dx=0 dy=-40 reg=1 id=12
    DEGR angle=3
    ADD dst=1 src=2
    LOOP jmp=@label_0D44 count=15
    JMP jmp=@label_0D9A
.org 0x0D5F
@label_0D5F:
    MXYA x=148 y=70 count=120
    ANM pattern=0 speed=12
    DEGA angle=32
    MOVR dst=1 src=145
    MOVC dst=2 val=64
    ADD dst=1 src=2
    MOVC dst=2 val=4294967293
    TAUTO interval=10
.org 0x0D7F
@label_0D7F:
    MOV count=6
    ENEMYSETD dx=0 dy=-40 reg=1 id=12
    DEGR angle=-3
    ADD dst=1 src=2
    LOOP jmp=@label_0D7F count=15
    JMP jmp=@label_0D9A
.org 0x0D9A
@label_0D9A:
    TAUTO interval=0
    TXYR dx=0 dy=0
    RET
.org 0x0DA2
@label_0DA2:
    TCMD cmd=8
    TSPDA v=202 a=3
    TTYPE type=7
    TREP rep=120
    TCOL color=21
    TOPT opt=0
    TNUMA n=32 ns=0
    TDEGA angle=128 dw=4
    TXYR dx=0 dy=-32
    TAMA
    TXYR dx=0 dy=0
    RET
.org 0x0DC1
@label_0DC1:
    TCMD cmd=8
    TSPDA v=18 a=1
    TTYPE type=0
    TCOL color=35
    TOPT opt=0
    TNUMA n=238 ns=0
    TDEGA angle=128 dw=1
.org 0x0DD2
@label_0DD2:
    TNUMR n=2 ns=0
    NOP count=10
    TXYR dx=-100 dy=-50
    TAMA
    NOP count=10
    TXYR dx=100 dy=-50
    TAMA
    LOOP jmp=@label_0DD2 count=6
    TXYR dx=0 dy=0
    RET
.org 0x0DF4
@label_0DF4:
    TCMD cmd=12
    TSPDA v=10 a=1
    TTYPE type=0
    TOPT opt=0
    TNUMA n=1 ns=25
    TDEGA angle=0 dw=1
    TCOL color=32
.org 0x0E05
@label_0E05:
    TXYR dx=-40 dy=-50
    TAMA
    TXYR dx=40 dy=-50
    TAMA
    NOP count=15
    LOOP jmp=@label_0E05 count=5
    NOP count=40
    TNUMR n=0 ns=-5
    TCOL color=33
.org 0x0E23
@label_0E23:
    TXYR dx=-80 dy=-30
    TAMA
    TXYR dx=80 dy=-30
    TAMA
    NOP count=10
    LOOP jmp=@label_0E23 count=4
    NOP count=40
    TNUMR n=0 ns=-5
    TCOL color=34
.org 0x0E41
@label_0E41:
    TXYR dx=-100 dy=-20
    TAMA
    TXYR dx=100 dy=-20
    TAMA
    NOP count=5
    LOOP jmp=@label_0E41 count=3
    TXYR dx=0 dy=0
    RET
.org 0x0E5D
@label_0E5D:
    MXYA x=319 y=100 count=60
    DEGA angle=43
    MOVC dst=2 val=4
.org 0x0E6C
@label_0E6C:
    SPDA speed=448
    MOVC dst=1 val=70
.org 0x0E77
@label_0E77:
    MOV count=1
    SPDR speed=-8
    DEC reg=1
    CMPC reg=1 val=0
    JL jmp=@label_0E77
    ENEMYSETD dx=0 dy=0 reg=2 id=15
    DEC reg=2
    DEGR angle=86
    LOOP jmp=@label_0E6C count=2
    ANM pattern=1 speed=7
    MXYA x=319 y=220 count=70
    ANM pattern=2 speed=2
    NOP count=600
    ANM pattern=0 speed=8
    TCMD cmd=12
    TSPDA v=12 a=0
    TTYPE type=0
    TCOL color=34
    TOPT opt=0
    TNUMA n=9 ns=4
    TDEGA angle=1 dw=16
.org 0x0EC2
@label_0EC2:
    TAMA
    TDEGR angle=-2 dw=0
    NOP count=3
    TAMA
    TDEGR angle=2 dw=0
    NOP count=3
    LOOP jmp=@label_0EC2 count=10
    RET
.org 0x0ED8
@script_11:
    SETUP hp=750 score=50000
    STI jmp=@label_0F41 vector=BOSSLEFT val=0
    CLIP_ON
    DAMAGE_ON
    HITSB_OFF
    ANM pattern=6 speed=5
    TCMD cmd=9
    TSPDA v=19 a=-8
    TREP rep=25
    TTYPE type=1
    TCOL color=21
    TOPT opt=0
    TNUMA n=46 ns=0
    LNUMA n=1
    LSPDA v=64
    LCOL color=0
    LWA w=384
    LTYPE type=0
    TOPT opt=0
    LXY x=0 y=0
    MOVR dst=1 src=145
.org 0x0F1B
@label_0F1B:
    NOP count=50
    DEC reg=1
    CMPC reg=1 val=0
    JL jmp=@label_0F1B
    TAMA
    NOP count=80
    LDEGS
    LLSET
    NOP count=60
    LLOPEN id=255
    NOP count=60
    LLCLOSE id=255
    ANM pattern=7 speed=5
    NOP count=55
.org 0x0F41
@label_0F41:
    CLI vector=BOSSLEFT
    END
.org 0x0F44
@script_12:
    SETUP hp=750 score=50000
    STI jmp=@label_0F8E vector=BOSSLEFT val=0
    CLIP_ON
    DAMAGE_ON
    HITSB_OFF
    ANM pattern=6 speed=5
    LNUMA n=1
    LSPDA v=64
    LCOL color=0
    LWA w=384
    LTYPE type=0
    TOPT opt=0
    LXY x=0 y=0
    LDEGA angle=0 dw=0
    MOVR dst=0 src=145
    MOVR dst=128 src=0
    LLSET
    NOP count=30
    LLOPEN id=255
    NOP count=40
    LLCLOSE id=255
    ANM pattern=7 speed=5
    NOP count=55
.org 0x0F8E
@label_0F8E:
    CLI vector=BOSSLEFT
    END
.org 0x0F91
@script_13:
    SETUP hp=14999 score=0
    STI jmp=@label_1038 vector=BOSSLEFT val=0
    CLIP_ON
    DAMAGE_OFF
    HITSB_OFF
    DRAW_OFF
    TCMD cmd=2
    TSPDA v=224 a=0
    TTYPE type=0
    TOPT opt=0
    TNUMA n=1 ns=0
    TDEGA angle=0 dw=1
    TDEGE
    JDIF easy=@label_0FC9 norm=@label_0FD4 hard=@label_0FDF luna=@label_0FEA
.org 0x0FC9
@label_0FC9:
    TDEGR angle=0 dw=0
    TNUMR n=0 ns=0
    JMP jmp=@label_0FF5
.org 0x0FD4
@label_0FD4:
    TDEGR angle=0 dw=2
    TNUMR n=2 ns=0
    JMP jmp=@label_0FF5
.org 0x0FDF
@label_0FDF:
    TDEGR angle=0 dw=4
    TNUMR n=3 ns=0
    JMP jmp=@label_0FF5
.org 0x0FEA
@label_0FEA:
    TDEGR angle=0 dw=6
    TNUMR n=4 ns=0
    JMP jmp=@label_0FF5
.org 0x0FF5
@label_0FF5:
    TCOL color=0
    TAMA2
    NOP count=1
    TCOL color=16
    TAMA2
    NOP count=1
    TDEGR angle=0 dw=1
    TCOL color=0
    TAMA2
    NOP count=1
    TCOL color=16
    TAMA2
    NOP count=1
    TDEGR angle=0 dw=1
    LOOP jmp=@label_0FF5 count=4
    TDEGR angle=0 dw=-2
    TCOL color=0
    TAMA2
    NOP count=1
    TCOL color=16
    TAMA2
    NOP count=1
    TDEGR angle=0 dw=-1
    TCOL color=0
    TAMA2
    NOP count=1
    TCOL color=16
    TAMA2
    NOP count=1
.org 0x1038
@label_1038:
    CLI vector=BOSSLEFT
    END
.org 0x103B
@script_14:
    SETUP hp=14999 score=0
    STI jmp=@label_1090 vector=BOSSLEFT val=0
    CLIP_ON
    DAMAGE_OFF
    HITSB_OFF
    DRAW_OFF
    TCMD cmd=1
    TSPDA v=24 a=0
    TTYPE type=0
    TOPT opt=0
    TNUMA n=16 ns=0
    TCOL color=33
    RND reg=0
    MOVR dst=134 src=0
    MOVR dst=0 src=145
    CMPC reg=0 val=128
    JL jmp=@label_1082
.org 0x1073
@label_1073:
    TAMA
    TDEGR angle=1 dw=0
    NOP count=1
    LOOP jmp=@label_1073 count=13
    END
.org 0x1082
@label_1082:
    TAMA
    TDEGR angle=-1 dw=0
    NOP count=1
    LOOP jmp=@label_1082 count=13
.org 0x1090
@label_1090:
    CLI vector=BOSSLEFT
    END
.org 0x1093
@script_15:
    SETUP hp=750 score=50000
    STI jmp=@label_117F vector=BOSSLEFT val=0
    CLIP_ON
    DAMAGE_ON
    HITSB_OFF
    ANM pattern=6 speed=5
    TCMD cmd=1
    TSPDA v=9 a=-8
    TTYPE type=4
    TVDEG vd=-4
    TREP rep=30
    TCOL color=21
    TOPT opt=0
    JDIF easy=@label_10CC norm=@label_10D4 hard=@label_10DC luna=@label_10E4
.org 0x10CC
@label_10CC:
    TNUMA n=7 ns=0
    JMP jmp=@label_10EC
.org 0x10D4
@label_10D4:
    TNUMA n=9 ns=0
    JMP jmp=@label_10EC
.org 0x10DC
@label_10DC:
    TNUMA n=11 ns=0
    JMP jmp=@label_10EC
.org 0x10E4
@label_10E4:
    TNUMA n=13 ns=0
    JMP jmp=@label_10EC
.org 0x10EC
@label_10EC:
    LNUMA n=1
    LSPDA v=64
    LCOL color=0
    LWA w=384
    LTYPE type=0
    TOPT opt=0
    LXY x=0 y=0
    MOVR dst=1 src=145
.org 0x1106
@label_1106:
    NOP count=70
    DEC reg=1
    CMPC reg=1 val=0
    JL jmp=@label_1106
.org 0x1116
@label_1116:
    TAMA2
    TDEGR angle=3 dw=0
    NOP count=8
    LOOP jmp=@label_1116 count=10
    NOP count=30
    TVDEG vd=5
    TCOL color=17
.org 0x112B
@label_112B:
    TAMA2
    TDEGR angle=-3 dw=0
    NOP count=8
    LOOP jmp=@label_112B count=8
    NOP count=30
    TCOL color=21
    TVDEG vd=-6
.org 0x1140
@label_1140:
    TAMA2
    TDEGR angle=4 dw=0
    NOP count=8
    LOOP jmp=@label_1140 count=6
    NOP count=30
    TCOL color=17
    TVDEG vd=6
.org 0x1155
@label_1155:
    TAMA2
    TDEGR angle=-4 dw=0
    NOP count=8
    LOOP jmp=@label_1155 count=6
    NOP count=120
    LDEGS
    LDEGR angle=3 dw=0
    LLSET
    LDEGR angle=-6 dw=0
    LLSET
    NOP count=90
    LLOPEN id=255
    NOP count=60
    LLCLOSE id=255
    ANM pattern=7 speed=5
    NOP count=55
.org 0x117F
@label_117F:
    CLI vector=BOSSLEFT
    END
.org 0x1182
@script_16:
    SETUP hp=21000 score=1000000
    STI jmp=@label_1216 vector=HP val=12000
    STI jmp=@label_1216 vector=TIMER val=9000
    CLIP_ON
    HITSB_OFF
    ANM pattern=2 speed=2
    INT id=2
    MXYA x=319 y=120 count=120
.org 0x11AD
@label_11AD:
    SPDA speed=64
    DEGA angle=192
    MOV count=60
    CALL jmp=@label_1513
    NOP count=60
    CALL jmp=@label_14AE
    DEGA angle=64
    MOV count=60
    NOP count=120
    CALL jmp=@script_17
    NOP count=80
    CALL jmp=@label_1443
    NOP count=120
    RND reg=1
    MOD reg=1 div=65536
    CMPC reg=1 val=32768
    JL jmp=@label_11FB
    MXYA x=198 y=80 count=60
    JMP jmp=@label_1202
.org 0x11FB
@label_11FB:
    MXYA x=441 y=80 count=60
.org 0x1202
@label_1202:
    CALL jmp=@label_13AD
    MXYA x=319 y=120 count=120
    NOP count=60
    JMP jmp=@label_11AD
.org 0x1216
@label_1216:
    SETUP hp=12000 score=1000000
    STI jmp=@label_1280 vector=HP val=4500
    STI jmp=@label_1280 vector=TIMER val=6000
    LLCLOSE id=255
    TCLR
    TXYR dx=0 dy=0
    LXY x=0 y=0
    MXYA x=319 y=120 count=120
.org 0x1247
@label_1247:
    CALL jmp=@label_1A67
    NOP count=60
    CALL jmp=@label_1A9D
    NOP count=60
    CALL jmp=@label_1A20
    NOP count=60
    CALL jmp=@label_189F
    NOP count=40
    CALL jmp=@label_1958
    MXYA x=319 y=120 count=120
    CALL jmp=@label_1690
    NOP count=60
    JMP jmp=@label_1247
.org 0x1280
@label_1280:
    SETUP hp=4500 score=1000000
    CLI vector=HP
    STI jmp=@label_12EE vector=TIMER val=2000
    LLCLOSE id=255
    TCLR
    TXYR dx=0 dy=0
    LXY x=0 y=0
    MXYA x=319 y=120 count=120
.org 0x12A9
@label_12A9:
    DEGA angle=0
    WAVX vx=128 amp=80 vd=-1 count=80
    CALL jmp=@label_1B6A
    DEGA angle=0
    WAVX vx=-128 amp=80 vd=1 count=80
    CALL jmp=@label_1B2F
    DEGA angle=0
    WAVX vx=-128 amp=80 vd=-1 count=80
    CALL jmp=@label_1B6A
    DEGA angle=0
    WAVX vx=128 amp=80 vd=1 count=80
    CALL jmp=@label_1CBA
    JMP jmp=@label_12A9
.org 0x12EE
@label_12EE:
    SETUP hp=0 score=0  ; death marker
    NOP count=10000
    JMP jmp=@label_12EE
.org 0x12FF
@script_17:
    TCMD cmd=1
    TDEGA angle=0 dw=0
    TNUMA n=4 ns=0
    TSPDA v=20 a=-9
    TTYPE type=1
    TOPT opt=0
    TREP rep=60
    TDEGS
    JDIF easy=@label_1322 norm=@label_132A hard=@label_1332 luna=@label_133A
.org 0x1322
@label_1322:
    TNUMA n=3 ns=0
    JMP jmp=@label_1342
.org 0x132A
@label_132A:
    TNUMA n=4 ns=0
    JMP jmp=@label_1342
.org 0x1332
@label_1332:
    TNUMA n=6 ns=0
    JMP jmp=@label_1342
.org 0x133A
@label_133A:
    TNUMA n=7 ns=0
    JMP jmp=@label_1342
.org 0x1342
@label_1342:
    MOVC dst=1 val=30
    RND reg=2
    MOD reg=2 div=10000
    CMPC reg=2 val=5000
    JL jmp=@label_1384
.org 0x135B
@label_135B:
    TCOL color=16
    TAMA2
    TDEGR angle=8 dw=0
    TCOL color=17
    TAMA2
    TDEGR angle=8 dw=0
    TCOL color=18
    TAMA2
    TDEGR angle=8 dw=0
    TCOL color=19
    TAMA2
    TDEGR angle=-21 dw=0
    NOP count=3
    DEC reg=1
    CMPC reg=1 val=0
    JL jmp=@label_135B
    RET
.org 0x1384
@label_1384:
    TCOL color=16
    TAMA2
    TDEGR angle=-8 dw=0
    TCOL color=17
    TAMA2
    TDEGR angle=-8 dw=0
    TCOL color=18
    TAMA2
    TDEGR angle=-8 dw=0
    TCOL color=19
    TAMA2
    TDEGR angle=21 dw=0
    NOP count=3
    DEC reg=1
    CMPC reg=1 val=0
    JL jmp=@label_1384
    RET
.org 0x13AD
@label_13AD:
    TCMD cmd=85
    TSPDA v=14 a=1
    TTYPE type=0
    TOPT opt=0
    TDEGA angle=0 dw=0
    RND reg=0
    MOVR dst=134 src=0
    JDIF easy=@label_13CF norm=@label_13D7 hard=@label_13DF luna=@label_13E7
.org 0x13CF
@label_13CF:
    TNUMA n=3 ns=10
    JMP jmp=@label_13EF
.org 0x13D7
@label_13D7:
    TNUMA n=4 ns=11
    JMP jmp=@label_13EF
.org 0x13DF
@label_13DF:
    TNUMA n=5 ns=12
    JMP jmp=@label_13EF
.org 0x13E7
@label_13E7:
    TNUMA n=6 ns=12
    JMP jmp=@label_13EF
.org 0x13EF
@label_13EF:
    TCOL color=32
    PSE id=0
    TAMA2
    TDEGR angle=4 dw=0
    NOP count=2
    TCOL color=33
    PSE id=0
    TAMA2
    TDEGR angle=4 dw=0
    NOP count=2
    TCOL color=34
    PSE id=0
    TAMA2
    TDEGR angle=4 dw=0
    NOP count=2
    LOOP jmp=@label_13EF count=5
    TDEGR angle=1 dw=0
.org 0x141A
@label_141A:
    TCOL color=32
    PSE id=0
    TAMA2
    TDEGR angle=-4 dw=0
    NOP count=2
    TCOL color=33
    PSE id=0
    TAMA2
    TDEGR angle=-4 dw=0
    NOP count=2
    TCOL color=34
    PSE id=0
    TAMA2
    TDEGR angle=-4 dw=0
    NOP count=2
    LOOP jmp=@label_141A count=5
    RET
.org 0x1443
@label_1443:
    TCMD cmd=1
    TCOL color=37
    TSPDA v=18 a=-5
    TTYPE type=5
    TREP rep=1
    TOPT opt=0
    TDEGS
    TXYR dx=0 dy=0
    JDIF easy=@label_1467 norm=@label_146F hard=@label_1477 luna=@label_147F
.org 0x1467
@label_1467:
    TNUMA n=48 ns=0
    JMP jmp=@label_1487
.org 0x146F
@label_146F:
    TNUMA n=64 ns=0
    JMP jmp=@label_1487
.org 0x1477
@label_1477:
    TNUMA n=80 ns=0
    JMP jmp=@label_1487
.org 0x147F
@label_147F:
    TNUMA n=96 ns=0
    JMP jmp=@label_1487
.org 0x1487
@label_1487:
    TVDEG vd=8
    TAMA2
    NOP count=60
    TVDEG vd=-8
    TAMA2
    NOP count=60
    TVDEG vd=8
    TAMA2
    NOP count=60
    TVDEG vd=-8
    TAMA2
    NOP count=60
    TVDEG vd=8
    TAMA2
    NOP count=60
    TVDEG vd=-8
    TAMA2
    NOP count=60
    TVDEG vd=8
    RET
.org 0x14AE
@label_14AE:
    TCMD cmd=5
    TCOL color=5
    TNUMA n=64 ns=10
    TSPDA v=206 a=-6
    TREP rep=20
    TTYPE type=1
    TOPT opt=0
    TXYR dx=0 dy=0
    TAMA
    TCMD cmd=0
    TCOL color=37
    TSPDA v=25 a=-3
    TVDEG vd=90
    TREP rep=50
    TTYPE type=3
    TOPT opt=0
    TDEGA angle=192 dw=16
    JDIF easy=@label_14E7 norm=@label_14EF hard=@label_14F7 luna=@label_14FF
.org 0x14E7
@label_14E7:
    TNUMA n=12 ns=0
    JMP jmp=@label_1507
.org 0x14EF
@label_14EF:
    TNUMA n=16 ns=0
    JMP jmp=@label_1507
.org 0x14F7
@label_14F7:
    TNUMA n=18 ns=0
    JMP jmp=@label_1507
.org 0x14FF
@label_14FF:
    TNUMA n=20 ns=0
    JMP jmp=@label_1507
.org 0x1507
@label_1507:
    TAMA2
    NOP count=20
    LOOP jmp=@label_1507 count=2
    RET
.org 0x1513
@label_1513:
    LNUMA n=1
    LSPDA v=64
    LCOL color=0
    LWA w=384
    LTYPE type=0
    RND reg=0
    MOD reg=0 div=65536
    CMPC reg=0 val=32768
    JL jmp=@label_1553
    TCMD cmd=8
    TCOL color=33
    TNUMA n=16 ns=0
    TSPDA v=12 a=0
    TTYPE type=0
    TOPT opt=0
    TXYR dx=0 dy=0
    TDEGA angle=0 dw=4
    TAUTO interval=32
    JMP jmp=@label_1570
.org 0x1553
@label_1553:
    TCMD cmd=92
    TCOL color=34
    TNUMA n=3 ns=3
    TSPDA v=16 a=0
    TTYPE type=0
    TOPT opt=0
    TXYR dx=0 dy=0
    TDEGA angle=0 dw=16
    TAUTO interval=30
    JMP jmp=@label_1570
.org 0x1570
@label_1570:
    LXY x=80 y=40
    LDEGA angle=254 dw=0
    LLSET
    LDEGR angle=16 dw=0
    LLSET
    LDEGR angle=16 dw=0
    LLSET
    LDEGR angle=16 dw=0
    LLSET
    LDEGR angle=16 dw=0
    LLSET
    LDEGR angle=16 dw=0
    LLSET
    LXY x=-80 y=40
    LDEGA angle=130 dw=0
    LLSET
    LDEGR angle=-16 dw=0
    LLSET
    LDEGR angle=-16 dw=0
    LLSET
    LDEGR angle=-16 dw=0
    LLSET
    LDEGR angle=-16 dw=0
    LLSET
    LDEGR angle=-16 dw=0
    LLSET
    NOP count=20
.org 0x15AD
@label_15AD:
    NOP count=8
    LLDEGR id=0 deg=1
    LLDEGR id=1 deg=1
    LLDEGR id=2 deg=1
    LLDEGR id=3 deg=1
    LLDEGR id=4 deg=1
    LLDEGR id=5 deg=1
    LLDEGR id=6 deg=-1
    LLDEGR id=7 deg=-1
    LLDEGR id=8 deg=-1
    LLDEGR id=9 deg=-1
    LLDEGR id=10 deg=-1
    LLDEGR id=11 deg=-1
    LOOP jmp=@label_15AD count=3
    NOP count=40
    LLOPEN id=255
    NOP count=40
    LLCLOSEL id=255
.org 0x15E5
@label_15E5:
    NOP count=8
    LLDEGR id=0 deg=1
    LLDEGR id=1 deg=1
    LLDEGR id=2 deg=1
    LLDEGR id=3 deg=1
    LLDEGR id=4 deg=1
    LLDEGR id=5 deg=1
    LLDEGR id=6 deg=-1
    LLDEGR id=7 deg=-1
    LLDEGR id=8 deg=-1
    LLDEGR id=9 deg=-1
    LLDEGR id=10 deg=-1
    LLDEGR id=11 deg=-1
    LOOP jmp=@label_15E5 count=3
    NOP count=40
    LLOPEN id=255
    NOP count=40
    LLCLOSEL id=255
.org 0x161D
@label_161D:
    NOP count=8
    LLDEGR id=0 deg=1
    LLDEGR id=1 deg=1
    LLDEGR id=2 deg=1
    LLDEGR id=3 deg=1
    LLDEGR id=4 deg=1
    LLDEGR id=5 deg=1
    LLDEGR id=6 deg=-1
    LLDEGR id=7 deg=-1
    LLDEGR id=8 deg=-1
    LLDEGR id=9 deg=-1
    LLDEGR id=10 deg=-1
    LLDEGR id=11 deg=-1
    LOOP jmp=@label_161D count=3
    NOP count=40
    LLOPEN id=255
    NOP count=40
    LLCLOSEL id=255
    TAUTO interval=0
.org 0x1657
@label_1657:
    NOP count=8
    LLDEGR id=0 deg=1
    LLDEGR id=1 deg=1
    LLDEGR id=2 deg=1
    LLDEGR id=3 deg=1
    LLDEGR id=4 deg=1
    LLDEGR id=5 deg=1
    LLDEGR id=6 deg=-1
    LLDEGR id=7 deg=-1
    LLDEGR id=8 deg=-1
    LLDEGR id=9 deg=-1
    LLDEGR id=10 deg=-1
    LLDEGR id=11 deg=-1
    LOOP jmp=@label_1657 count=5
    NOP count=40
    LLOPEN id=255
    NOP count=40
    LLCLOSE id=255
    RET
.org 0x1690
@label_1690:
    TCMD cmd=1
    TNUMA n=56 ns=0
    TSPDA v=18 a=0
    TTYPE type=4
    TOPT opt=0
    TXYR dx=0 dy=0
    TDEGS
    RND reg=0
    MOD reg=0 div=65536
    CMPC reg=0 val=32768
    JL jmp=@label_17AA
    TVDEG vd=-1
    TREP rep=50
    TCOL color=16
    TAMA
    NOP count=3
    TDEGR angle=0 dw=1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=1
    TREP rep=55
    TCOL color=17
    TAMA
    NOP count=3
    TDEGR angle=0 dw=1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=1
    TREP rep=60
    TCOL color=18
    TAMA
    NOP count=3
    TDEGR angle=0 dw=1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=1
    TREP rep=65
    TCOL color=19
    TAMA
    NOP count=3
    TDEGR angle=0 dw=1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=1
    RET
.org 0x17AA
@label_17AA:
    TVDEG vd=1
    TREP rep=50
    TCOL color=16
    TAMA
    NOP count=3
    TDEGR angle=0 dw=-1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=-1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=-1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=-1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=-1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=-1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=-1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=-1
    TREP rep=55
    TCOL color=17
    TAMA
    NOP count=3
    TDEGR angle=0 dw=-1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=-1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=-1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=-1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=-1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=-1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=-1
    TREP rep=60
    TCOL color=18
    TAMA
    NOP count=3
    TDEGR angle=0 dw=-1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=-1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=-1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=-1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=-1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=-1
    TREP rep=65
    TCOL color=19
    TAMA
    NOP count=3
    TDEGR angle=0 dw=-1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=-1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=-1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=-1
    TCOL color=20
    TAMA
    NOP count=3
    TDEGR angle=0 dw=-1
    RET
.org 0x189F
@label_189F:
    LCMD cmd=1
    LLA len=2560
    LDEGA angle=0 dw=18
    LNUMA n=2
    LSPDA v=384
    LCOL color=0
    LTYPE type=1
    LWA w=192
    LXY x=0 y=0
    SPDA speed=64
    JDIF easy=@label_18D4 norm=@label_18DB hard=@label_18DB luna=@label_18DB
.org 0x18D4
@label_18D4:
    LSPDA v=512
    LNUMA n=4
.org 0x18DB
@label_18DB:
    PSE id=3
    LASER
    NOP count=2
    LDEGR angle=9 dw=0
    LOOP jmp=@label_18DB count=12
    DEGXU
    MOV count=20
    LNUMR n=1
    LLR len=320
.org 0x18F6
@label_18F6:
    PSE id=3
    LASER
    NOP count=2
    LDEGR angle=-7 dw=0
    LOOP jmp=@label_18F6 count=18
    DEGXD
    MOV count=20
    LNUMR n=1
    LLR len=320
.org 0x1911
@label_1911:
    PSE id=3
    LASER
    NOP count=2
    LDEGR angle=5 dw=0
    LOOP jmp=@label_1911 count=24
    DEGXU
    MOV count=20
    LNUMR n=1
    LLR len=320
.org 0x192C
@label_192C:
    PSE id=3
    LASER
    NOP count=2
    LDEGR angle=-3 dw=0
    LOOP jmp=@label_192C count=30
    DEGXD
    MOV count=20
    LNUMR n=1
    LLR len=320
.org 0x1947
@label_1947:
    PSE id=3
    LASER
    NOP count=2
    LDEGR angle=3 dw=0
    LOOP jmp=@label_1947 count=36
    RET
.org 0x1958
@label_1958:
    LNUMA n=1
    LSPDA v=64
    LCOL color=0
    LWA w=384
    LTYPE type=0
    LXY x=0 y=0
    TCMD cmd=2
    TCOL color=37
    TSPDA v=31 a=-2
    TVDEG vd=30
    TREP rep=100
    TNUMA n=8 ns=0
    TDEGA angle=192 dw=128
    TTYPE type=3
    TOPT opt=0
    RND reg=0
    MOD reg=0 div=65536
    CMPC reg=0 val=32768
    JL jmp=@label_19A8
    MOVC dst=0 val=32
    MOVC dst=1 val=1
    DEGA angle=240
    JMP jmp=@label_19B6
.org 0x19A8
@label_19A8:
    MOVC dst=0 val=96
    MOVC dst=1 val=4294967295
    DEGA angle=144
.org 0x19B6
@label_19B6:
    SPDA speed=128
    MOVR dst=128 src=0
    LDEGR angle=100 dw=0
    LLSET
    LDEGR angle=56 dw=0
    LLSET
    LDEGR angle=-106 dw=0
    LLSET
    LDEGR angle=-100 dw=0
    LLSET
    LDEGR angle=50 dw=0
    LLSET
    MOV count=5
.org 0x19D5
@label_19D5:
    MOV count=1
    LLDEGR id=0 deg=-2
    LLDEGR id=1 deg=2
    LLDEGR id=2 deg=-1
    LLDEGR id=3 deg=1
    LOOP jmp=@label_19D5 count=45
    LLOPEN id=255
    TAUTO interval=8
    DEGR angle=-128
    CMPC reg=1 val=0
    JS jmp=@label_1A0E
.org 0x19FC
@label_19FC:
    MOV count=4
    LLDEGR id=255 deg=1
    LOOP jmp=@label_19FC count=20
    JMP jmp=@label_1A1B
.org 0x1A0E
@label_1A0E:
    MOV count=4
    LLDEGR id=255 deg=-1
    LOOP jmp=@label_1A0E count=20
.org 0x1A1B
@label_1A1B:
    LLCLOSE id=255
    TAUTO interval=0
    RET
.org 0x1A20
@label_1A20:
    TCMD cmd=1
    TSPDA v=20 a=0
    TREP rep=40
    TNUMA n=32 ns=0
    TTYPE type=4
    TOPT opt=0
    TDEGS
.org 0x1A2F
@label_1A2F:
    TCOL color=16
    TVDEG vd=-1
    TAMA
    NOP count=2
    TCOL color=17
    TVDEG vd=1
    TAMA
    NOP count=2
    TCOL color=18
    TVDEG vd=-1
    TAMA
    NOP count=2
    TCOL color=19
    TVDEG vd=1
    TAMA
    NOP count=2
    TCOL color=20
    TVDEG vd=-1
    TAMA
    NOP count=2
    TCOL color=21
    TVDEG vd=1
    TAMA
    NOP count=2
    LOOP jmp=@label_1A2F count=8
    RET
.org 0x1A67
@label_1A67:
    TCMD cmd=0
    TSPDA v=24 a=-12
    TREP rep=1
    TNUMA n=32 ns=0
    TTYPE type=2
    TOPT opt=0
    TDEGA angle=64 dw=5
    TCOL color=0
    TAMA
    NOP count=10
    TCOL color=1
    TAMA
    NOP count=10
    TCOL color=2
    TAMA
    NOP count=10
    TCOL color=3
    TAMA
    NOP count=10
    TCOL color=4
    TAMA
    NOP count=10
    TCOL color=5
    TAMA
    NOP count=10
    RET
.org 0x1A9D
@label_1A9D:
    TCMD cmd=1
    TSPDA v=20 a=0
    TNUMA n=32 ns=0
    TTYPE type=8
    TOPT opt=0
    TDEGA angle=64 dw=5
    RND reg=0
    MOD reg=0 div=65536
    MOVC dst=2 val=0
    MOVC dst=3 val=32
    CMPC reg=0 val=32768
    JL jmp=@label_1AD6
    MOVC dst=1 val=4294967277
    JMP jmp=@label_1ADC
.org 0x1AD6
@label_1AD6:
    MOVC dst=1 val=19
.org 0x1ADC
@label_1ADC:
    JDIF easy=@label_1AED norm=@label_1AF4 hard=@label_1AFB luna=@label_1B02
.org 0x1AED
@label_1AED:
    TREP rep=5
    JMP jmp=@label_1B09
.org 0x1AF4
@label_1AF4:
    TREP rep=6
    JMP jmp=@label_1B09
.org 0x1AFB
@label_1AFB:
    TREP rep=7
    JMP jmp=@label_1B09
.org 0x1B02
@label_1B02:
    TREP rep=8
    JMP jmp=@label_1B09
.org 0x1B09
@label_1B09:
    MOVR dst=142 src=0
    MOVR dst=139 src=3
    TAMA2
    NOP count=2
    ADD dst=0 src=1
    INC reg=2
    MOD reg=2 div=3
    MOVC dst=3 val=32
    ADD dst=3 src=2
    LOOP jmp=@label_1B09 count=60
    RET
.org 0x1B2F
@label_1B2F:
    TCMD cmd=82
    TSPDA v=224 a=-13
    TREP rep=40
    TNUMA n=4 ns=0
    TTYPE type=1
    TOPT opt=0
    TDEGA angle=64 dw=255
    TAUTO interval=1
.org 0x1B42
@label_1B42:
    TCOL color=0
    NOP count=4
    TCOL color=1
    NOP count=4
    TCOL color=2
    NOP count=4
    TCOL color=3
    NOP count=4
    TCOL color=4
    NOP count=4
    TCOL color=5
    NOP count=4
    LOOP jmp=@label_1B42 count=3
    TAUTO interval=0
    RET
.org 0x1B6A
@label_1B6A:
    TCMD cmd=82
    TSPDA v=224 a=-13
    TREP rep=40
    TNUMA n=2 ns=0
    TTYPE type=1
    TOPT opt=0
    MOVR dst=0 src=143
    CMPC reg=0 val=20416
    JL jmp=@label_1BC7
    TDEGA angle=224 dw=64
.org 0x1B89
@label_1B89:
    TCOL color=0
    TAMA
    TDEGR angle=2 dw=0
    NOP count=1
    TCOL color=1
    TAMA
    TDEGR angle=2 dw=0
    NOP count=1
    TCOL color=2
    TAMA
    TDEGR angle=2 dw=0
    NOP count=1
    TCOL color=3
    TAMA
    TDEGR angle=2 dw=0
    NOP count=1
    TCOL color=4
    TAMA
    TDEGR angle=2 dw=0
    NOP count=1
    TCOL color=5
    TAMA
    TDEGR angle=2 dw=0
    NOP count=1
    LOOP jmp=@label_1B89 count=10
    RET
.org 0x1BC7
@label_1BC7:
    TDEGA angle=160 dw=64
.org 0x1BCA
@label_1BCA:
    TCOL color=0
    TAMA
    TDEGR angle=-2 dw=0
    NOP count=1
    TCOL color=1
    TAMA
    TDEGR angle=-2 dw=0
    NOP count=1
    TCOL color=2
    TAMA
    TDEGR angle=-2 dw=0
    NOP count=1
    TCOL color=3
    TAMA
    TDEGR angle=-2 dw=0
    NOP count=1
    TCOL color=4
    TAMA
    TDEGR angle=-2 dw=0
    NOP count=1
    TCOL color=5
    TAMA
    TDEGR angle=-2 dw=0
    NOP count=1
    LOOP jmp=@label_1BCA count=10
    RET
    TCMD cmd=1
    TSPDA v=40 a=0
    TNUMA n=32 ns=0
    TTYPE type=8
    TOPT opt=0
    TDEGA angle=64 dw=5
    RND reg=0
    MOD reg=0 div=65536
    MOVC dst=2 val=0
    MOVC dst=3 val=32
    CMPC reg=0 val=32768
    JL jmp=@label_1C41
    MOVC dst=1 val=4294967277
    JMP jmp=@label_1C47
.org 0x1C41
@label_1C41:
    MOVC dst=1 val=19
.org 0x1C47
@label_1C47:
    JDIF easy=@label_1C58 norm=@label_1C5F hard=@label_1C66 luna=@label_1C6D
.org 0x1C58
@label_1C58:
    TREP rep=1
    JMP jmp=@label_1C74
.org 0x1C5F
@label_1C5F:
    TREP rep=2
    JMP jmp=@label_1C74
.org 0x1C66
@label_1C66:
    TREP rep=3
    JMP jmp=@label_1C74
.org 0x1C6D
@label_1C6D:
    TREP rep=4
    JMP jmp=@label_1C74
.org 0x1C74
@label_1C74:
    TDEGS
    MOVR dst=0 src=134
    MOVR dst=142 src=0
    MOVR dst=139 src=3
    TAMA2
    NOP count=5
    TAMA2
    NOP count=5
    TAMA2
    NOP count=5
    TAMA2
    NOP count=5
    TAMA2
    NOP count=5
    TAMA2
    NOP count=5
    TAMA2
    NOP count=5
    TAMA2
    NOP count=15
    ADD dst=0 src=1
    INC reg=2
    MOD reg=2 div=3
    MOVC dst=3 val=32
    ADD dst=3 src=2
    LOOP jmp=@label_1C74 count=4
    RET
.org 0x1CBA
@label_1CBA:
    TCMD cmd=1
    TSPDA v=40 a=0
    TNUMA n=32 ns=0
    TTYPE type=8
    TOPT opt=0
    TDEGA angle=64 dw=5
    RND reg=0
    MOD reg=0 div=65536
    MOVC dst=2 val=0
    MOVC dst=3 val=32
    CMPC reg=0 val=32768
    JL jmp=@label_1CF3
    MOVC dst=1 val=4294967277
    JMP jmp=@label_1CF9
.org 0x1CF3
@label_1CF3:
    MOVC dst=1 val=19
.org 0x1CF9
@label_1CF9:
    JDIF easy=@label_1D0A norm=@label_1D11 hard=@label_1D18 luna=@label_1D1F
.org 0x1D0A
@label_1D0A:
    TREP rep=1
    JMP jmp=@label_1D26
.org 0x1D11
@label_1D11:
    TREP rep=2
    JMP jmp=@label_1D26
.org 0x1D18
@label_1D18:
    TREP rep=3
    JMP jmp=@label_1D26
.org 0x1D1F
@label_1D1F:
    TREP rep=4
    JMP jmp=@label_1D26
.org 0x1D26
@label_1D26:
    TDEGS
    MOVR dst=0 src=134
    MOVR dst=142 src=0
    MOVR dst=139 src=3
    TAMA2
    NOP count=5
    ADD dst=0 src=1
    INC reg=2
    MOD reg=2 div=3
    MOVC dst=3 val=32
    ADD dst=3 src=2
    LOOP jmp=@label_1D26 count=12
    RET
.org 0x1D50
@script_18:
    SETUP hp=150 score=20000
    ANM pattern=8 speed=0
    CLIP_ON
    LTYPE type=1
    LCOL color=0
    LNUMA n=2
    LXY x=0 y=0
    LDEGA angle=192 dw=32
    TCMD cmd=9
    TDEGA angle=0 dw=5
    TNUMA n=20 ns=0
    TSPDA v=16 a=0
    TTYPE type=0
    TCOL color=18
    DEGS
    SPDA speed=320
    ACC accel=-6 count=60
    TAUTO interval=3
    NOP count=10
    TAUTO interval=0
    HLASER
    CLIP_OFF
    DEGR angle=-128
    SPDA speed=64
    ACC accel=6 count=6000
.org 0x1D98
@label_1D98:
    NOP count=1000
    JMP jmp=@label_1D98
.org 0x1DA0
@script_19:
    SETUP hp=240 score=2000
    ANM pattern=6 speed=2
    CLIP_ON
    ITEM type=2
    TCMD cmd=1
    TDEGA angle=0 dw=5
    TNUMA n=20 ns=0
    TSPDA v=12 a=0
    TTYPE type=0
    TCOL color=16
    DEGA angle=64
    SPDA speed=128
    MOV count=50
    CLIP_OFF
    RND reg=0
    MOD reg=0 div=65536
    CMPC reg=0 val=32768
    JL jmp=@label_1DEF
.org 0x1DDC
@label_1DDC:
    TAMA
    NOP count=5
    TDEGR angle=4 dw=0
    LOOP jmp=@label_1DDC count=40
    JMP jmp=@label_1DFD
.org 0x1DEF
@label_1DEF:
    TAMA
    NOP count=5
    TDEGR angle=-4 dw=0
    LOOP jmp=@label_1DEF count=50
.org 0x1DFD
@label_1DFD:
    MOV count=1000
    JMP jmp=@label_1DFD
.org 0x1E05
@script_20:
    SETUP hp=11 score=2000
    ANM pattern=6 speed=2
    CLIP_ON
    SPDA speed=64
    DEGS
    ACC accel=6 count=30
    CLIP_OFF
    ACC accel=6 count=65516
.org 0x1E21
@label_1E21:
    ACC accel=6 count=1000
    JMP jmp=@label_1E21
